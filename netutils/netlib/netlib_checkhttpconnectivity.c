/****************************************************************************
 * apps/netutils/netlib/netlib_checkhttpconnectivity.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#ifdef CONFIG_LIBC_NETDB
#  include <netdb.h>
#endif

#include <arpa/inet.h>
#include <netinet/in.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: httpget_gethostip
 *
 * Description:
 *   Check the http status code
 *
 * Parameters:
 *   hostname  The host name to use in the nslookup.
 *   dest      The location to return the IPv4 address.
 *
 * Return:
 *   0 on success; a nagtive on failure.
 *
 ****************************************************************************/

static int httpget_gethostip(FAR const char *hostname,
                             FAR struct in_addr *dest)
{
#ifdef CONFIG_LIBC_NETDB
  /* Netdb DNS client support is enabled */

  FAR struct addrinfo hint;
  FAR struct addrinfo *info;
  FAR struct sockaddr_in *addr;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;

  if (getaddrinfo(hostname, NULL, &hint, &info) != OK)
    {
      return -errno;
    }

  addr = (FAR struct sockaddr_in *)info->ai_addr;
  memcpy(dest, &addr->sin_addr, sizeof(struct in_addr));

  freeaddrinfo(info);
  return OK;

#else /* CONFIG_LIBC_NETDB */
  /* No host name support */

  /* Convert strings to numeric IPv4 address */

  int ret = inet_pton(AF_INET, hostname, dest);

  if (ret < 0)
    {
      return -errno;
    }

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv4 dotted-decimal string or -1
   * with errno set to EAFNOSUPPORT if the address family argument is
   * unsupported.
   */

  return OK;

#endif /* CONFIG_LIBC_NETDB */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_check_httpconnectivity
 *
 * Description:
 *   Check the http status code
 *
 * Parameters:
 *   host         The remote host to check
 *   getmsg       The http GET message
 *   port         The port of remote host
 *   expect_code  The except http code of http code
 *
 * Return:
 *   0 on success; a nagtive on failure.
 *
 ****************************************************************************/

int netlib_check_httpconnectivity(FAR const char *host,
                                  FAR const char *getmsg,
                                  int port, int expect_code)
{
  int sock;
  int ret;
  int status_code = -1;
  struct sockaddr_in server_addr;
  char buf[256];

  ret = httpget_gethostip(host, &server_addr.sin_addr);
  if (ret < 0)
    {
      nerr("Failed to get host ip");
      return ret;
    }

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      nerr("Failed to create socket");
      return -errno;
    }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (ret < 0)
    {
      perror("Connection failed");
      goto exit_with_socket;
    }

  snprintf(buf, sizeof(buf),
           "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
           getmsg, host);

  ret = send(sock, buf, strlen(buf), 0);
  if (ret < 0)
    {
      perror("send failed");
      goto exit_with_socket;
    }

  if ((ret = recv(sock, buf, sizeof(buf), 0)) < 0)
    {
      perror("recv failed");
      goto exit_with_socket;
    }

  /* get the http code, and check with except code */

  if (sscanf(buf, "HTTP/1.1 %d", &status_code) == 1 &&
             status_code == expect_code)
    {
      ninfo("HTTP connectivity is ok");
      ret = OK;
    }
  else
    {
      nerr("HTTP connectivity is not ok status code: %d\n", status_code);
      ret = -status_code;
    }

exit_with_socket:
  close(sock);
  return ret;
}
