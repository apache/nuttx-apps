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
 * Name: eai_to_errno
 *
 * Description:
 *   Convert EAI_* error code to errno value for unified error handling.
 *
 * Parameters:
 *   eai_error  The EAI_* error code from getaddrinfo().
 *
 * Return:
 *   Corresponding errno value.
 *
 ****************************************************************************/

#ifdef CONFIG_LIBC_NETDB
static int eai_to_errno(int eai_error)
{
  switch (eai_error)
    {
      case EAI_AGAIN:
        return EAGAIN;        /* Temporary failure, try again */

      case EAI_BADFLAGS:
        return EINVAL;        /* Invalid flags */

      case EAI_FAIL:
        return EHOSTUNREACH;  /* Non-recoverable failure */

      case EAI_FAMILY:
        return EAFNOSUPPORT;  /* Address family not supported */

      case EAI_MEMORY:
        return ENOMEM;        /* Memory allocation failure */

      case EAI_NONAME:
        return EHOSTUNREACH;  /* Host name not found */

      case EAI_SERVICE:
        return EINVAL;        /* Service not recognized */

      case EAI_SOCKTYPE:
        return EINVAL;        /* Socket type not supported */

      case EAI_SYSTEM:
        return errno;         /* System error, use errno */

      case EAI_OVERFLOW:
        return EOVERFLOW;     /* Buffer overflow */

      default:
        return EINVAL;        /* Unknown error, use invalid argument */
    }
}
#endif

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
 *   0 on success; a negative errno value on failure.
 *
 ****************************************************************************/

static int httpget_gethostip(FAR const char *hostname,
                             FAR struct in_addr *dest)
{
#ifdef CONFIG_LIBC_NETDB
  /* Netdb DNS client support is enabled */

  int ret;
  FAR struct addrinfo hint;
  FAR struct addrinfo *info;
  FAR struct sockaddr_in *addr;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;

  ret = getaddrinfo(hostname, NULL, &hint, &info);
  if (ret != OK)
    {
      return -eai_to_errno(ret);
    }

  addr = (FAR struct sockaddr_in *)info->ai_addr;
  memcpy(dest, &addr->sin_addr, sizeof(struct in_addr));

  freeaddrinfo(info);
  return OK;

#else /* CONFIG_LIBC_NETDB */
  /* No host name support */

  /* Convert strings to numeric IPv4 address */

  int ret = inet_pton(AF_INET, hostname, dest);

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv4 dotted-decimal string or -1
   * with errno set to EAFNOSUPPORT if the address family argument is
   * unsupported.
   */

  if (ret != 1)
    {
      /* ret == 0: invalid IP address string, set errno to EINVAL
       * ret == -1: system error, errno is already set to EAFNOSUPPORT
       */

      if (ret == 0)
        {
          set_errno(EINVAL);
        }

      return -errno;
    }

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
      nerr("Connection failed: %s", strerror(errno));
      goto exit_with_socket;
    }

  snprintf(buf, sizeof(buf),
           "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
           getmsg, host);

  ret = send(sock, buf, strlen(buf), 0);
  if (ret < 0)
    {
      nerr("send failed: %s", strerror(errno));
      goto exit_with_socket;
    }

  if ((ret = recv(sock, buf, sizeof(buf), 0)) < 0)
    {
      nerr("recv failed: %s", strerror(errno));
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
