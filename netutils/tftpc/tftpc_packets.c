/****************************************************************************
 * apps/netutils/tftpc/tftpc_packets.c
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <nuttx/net/netconfig.h>
#include "netutils/tftp.h"

#include "tftpc_internal.h"

#if defined(CONFIG_NET) && defined(CONFIG_NET_UDP)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tftp_mode
 ****************************************************************************/

static inline const char *tftp_mode(bool binary)
{
  return binary ? "octet" : "netascii";
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tftp_sockinit
 *
 * Description:
 *   Common initialization logic:  Create the socket and initialize the
 *   server address structure.
 *
 ****************************************************************************/

int tftp_sockinit(struct sockaddr_in *server, in_addr_t addr)
{
  struct timeval timeo;
  int sd;
  int ret;

  /* Create the UDP socket */

  sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);
      return ERROR;
    }

  /* Set the recvfrom timeout */

  timeo.tv_sec  = CONFIG_NETUTILS_TFTP_TIMEOUT / 10;
  timeo.tv_usec = (CONFIG_NETUTILS_TFTP_TIMEOUT % 10) * 100000;
  ret = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeo,
                   sizeof(struct timeval));
  if (ret < 0)
    {
      nerr("ERROR: setsockopt failed: %d\n", errno);
    }

  /* Initialize the server address structure */

  memset(server, 0, sizeof(struct sockaddr_in));
  server->sin_family      = AF_INET;
  server->sin_addr.s_addr = addr;
  server->sin_port        = HTONS(CONFIG_NETUTILS_TFTP_PORT);
  return sd;
}

/****************************************************************************
 * Name: tftp_mkreqpacket
 *
 * Description:
 *   RRQ or WRQ message format:
 *
 *     2 bytes: Opcode (network order == big-endian)
 *     N bytes: Filename
 *     1 byte:  0
 *     N bytes: mode
 *     1 byte:  0
 *
 * Return
 *  Then number of bytes in the request packet (never fails)
 *
 ****************************************************************************/

int tftp_mkreqpacket(uint8_t *buffer, size_t len, int opcode,
                     const char *path, bool binary)
{
  int ret;

  buffer[0] = opcode >> 8;
  buffer[1] = opcode & 0xff;
  ret = snprintf((char *)&buffer[2], len - 2, "%s%c%s", path, 0,
                 tftp_mode(binary)) + 3;
  return ret < len ? ret : len;
}

/****************************************************************************
 * Name: tftp_mkackpacket
 *
 * Description:
 *   ACK message format:
 *
 *     2 bytes: Opcode (network order == big-endian)
 *     2 bytes: Block number (network order == big-endian)
 *
 ****************************************************************************/

int tftp_mkackpacket(uint8_t *buffer, uint16_t blockno)
{
  buffer[0] = TFTP_ACK >> 8;
  buffer[1] = TFTP_ACK & 0xff;
  buffer[2] = blockno >> 8;
  buffer[3] = blockno & 0xff;
  return 4;
}

/****************************************************************************
 * Name: tftp_mkerrpacket
 *
 * Description:
 *   ERROR message format:
 *
 *     2 bytes: Opcode (network order == big-endian)
 *     2 bytes: Error number (network order == big-endian)
 *     N bytes: Error string
 *     1 byte:  0
 *
 ****************************************************************************/

int tftp_mkerrpacket(uint8_t *buffer, uint16_t errorcode,
                     const char *errormsg)
{
  buffer[0] = TFTP_ERR >> 8;
  buffer[1] = TFTP_ERR & 0xff;
  buffer[2] = errorcode >> 8;
  buffer[3] = errorcode & 0xff;
  strcpy((char *)&buffer[4], errormsg);
  return strlen(errormsg) + 5;
}

/****************************************************************************
 * Name: tftp_parseerrpacket
 *
 * Description:
 *   ERROR message format:
 *
 *     2 bytes: Opcode (network order == big-endian)
 *     2 bytes: Error number (network order == big-endian)
 *     N bytes: Error string
 *     1 byte:  0
 *
 ****************************************************************************/

#ifdef CONFIG_DEBUG_NET_WARN
int tftp_parseerrpacket(const uint8_t *buffer)
{
  uint16_t opcode        = (uint16_t)buffer[0] << 8 | (uint16_t)buffer[1];
  uint16_t errcode       = (uint16_t)buffer[2] << 8 | (uint16_t)buffer[3];
  FAR const char *errmsg = (const char *)&buffer[4];

  if (opcode == TFTP_ERR)
    {
      nwarn("WARNING: ERR message: %s (%d)\n", errmsg, errcode);
      return OK;
    }

  return ERROR;
}
#endif

/****************************************************************************
 * Name: tftp_recvfrom
 *
 * Description:
 *   recvfrom helper
 *
 ****************************************************************************/

ssize_t tftp_recvfrom(int sd, void *buf, size_t len,
                      struct sockaddr_in *from)
{
  socklen_t addrlen;
  ssize_t nbytes;

  /* Loop handles the case where the recvfrom is interrupted by a signal and
   * we should unconditionally try again.
   */

  for (; ; )
    {
      /* For debugging, it is helpful to start with a clean buffer */

#if defined(CONFIG_DEBUG_INFO) && defined(CONFIG_DEBUG_NET)
      memset(buf, 0, len);
#endif

      /* Receive the packet */

      addrlen = sizeof(struct sockaddr_in);
      nbytes = recvfrom(sd, buf, len, 0, (struct sockaddr *)from, &addrlen);

      /* Check for errors */

      if (nbytes < 0)
        {
          /* Check for a timeout */

          if (errno == EAGAIN)
            {
              nerr("ERROR: recvfrom timed out\n");
              return ERROR;
            }

          /* If EINTR, then loop and try again.  Other errors are fatal */

          else if (errno != EINTR)
            {
              nerr("ERROR: recvfrom failed: %d\n", errno);
              return ERROR;
            }
        }

      /* No errors?  Return the number of bytes received */

      else
        {
          return nbytes;
        }
    }
}

/****************************************************************************
 * Name: tftp_sendto
 *
 * Description:
 *   sendto helper
 *
 ****************************************************************************/

ssize_t tftp_sendto(int sd, const void *buf, size_t len,
                    struct sockaddr_in *to)
{
  ssize_t nbytes;

  /* Loop handles the case where the sendto is interrupted by a signal and
   * we should unconditionally try again.
   */

  for (; ; )
    {
      /* Send the packet */

      nbytes = sendto(sd, buf, len, 0, (struct sockaddr *)to,
                      sizeof(struct sockaddr_in));

      /* Check for errors */

      if (nbytes < 0)
        {
          /* If EINTR, then loop and try again.  Other errors are fatal */

          if (errno != EINTR)
            {
              nerr("ERROR: sendto failed: %d\n", errno);
              return ERROR;
            }
        }

      /* No errors?  Return the number of bytes received */

      else
        {
          return nbytes;
        }
    }
}

#endif /* CONFIG_NET && CONFIG_NET_UDP */
