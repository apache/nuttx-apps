/****************************************************************************
 * apps/netutils/tftpc/tftpc_get.c
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
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>

#include <nuttx/net/netconfig.h>
#include "netutils/tftp.h"

#include "tftpc_internal.h"

#if defined(CONFIG_NET) && defined(CONFIG_NET_UDP)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TFTP_RETRIES 3

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tftp_parsedatapacket
 ****************************************************************************/

static inline int tftp_parsedatapacket(FAR const uint8_t *packet,
                                       FAR uint16_t *opcode,
                                       FAR uint16_t *blockno)
{
  *opcode = (uint16_t)packet[0] << 8 | (uint16_t)packet[1];
  if (*opcode == TFTP_DATA)
    {
      *blockno = (uint16_t)packet[2] << 8 | (uint16_t)packet[3];
      return OK;
    }
#ifdef CONFIG_DEBUG_NET_WARN
  else if (*opcode == TFTP_ERR)
    {
      tftp_parseerrpacket(packet);
    }
#endif

  return ERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tftpget_cb
 *
 * Input Parameters:
 *   remote - The name of the file on the TFTP server.
 *   addr   - The IP address of the server in network order
 *   binary - TRUE:  Perform binary ('octet') transfer
 *            FALSE: Perform text ('netascii') transfer
 *   cb     - callback that will be called with data packets
 *   ctx    - pointer passed to the previous callback
 *
 ****************************************************************************/

int tftpget_cb(FAR const char *remote, in_addr_t addr, bool binary,
               tftp_callback_t tftp_cb, FAR void *ctx)
{
  struct sockaddr_in server;  /* The address of the TFTP server */
  struct sockaddr_in from;    /* The address the last UDP message recv'd from */
  FAR uint8_t *packet;        /* Allocated memory to hold one packet */
  uint16_t blockno = 0;       /* The current transfer block number */
  uint16_t opcode;            /* Received opcode */
  uint16_t rblockno;          /* Received block number */
  int len;                    /* Generic length */
  int sd;                     /* Socket descriptor for socket I/O */
  int retry;                  /* Retry counter */
  int nbytesrecvd = 0;        /* The number of bytes received in the packet */
  int ndatabytes;             /* The number of data bytes received */
  int result = ERROR;         /* Assume failure */
  int ret;                    /* Generic return status */

  /* Allocate the buffer to used for socket/disk I/O */

  packet = (FAR uint8_t *)zalloc(TFTP_IOBUFSIZE);
  if (!packet)
    {
      nerr("ERROR: packet memory allocation failure\n");
      errno = ENOMEM;
      return result;
    }

  /* Initialize a UDP socket and setup the server address */

  sd = tftp_sockinit(&server, addr);
  if (sd < 0)
    {
      goto errout;
    }

  /* Then enter the transfer loop.  Loop until the entire file has
   * been received or until an error occurs.
   */

  do
    {
      /* Increment the TFTP block number for the next transfer */

      blockno++;

      /* Send the next block if the file within a loop.  We will
       * retry up to TFTP_RETRIES times before giving up on the
       * transfer.
       */

      for (retry = 0; retry < TFTP_RETRIES; retry++)
        {
          /* Send the read request using the well-known port number before
           * receiving the first block.  Each retry of the first block will
           * re-send the request.
           */

          if (blockno == 1)
            {
              len             = tftp_mkreqpacket(packet, TFTP_RRQ, remote,
                                                 binary);
              server.sin_port = HTONS(CONFIG_NETUTILS_TFTP_PORT);
              ret             = tftp_sendto(sd, packet, len, &server);
              if (ret != len)
                {
                  goto errout_with_sd;
                }

              /* Subsequent sendto will use the port number selected by the
               * TFTP server in the DATA packet.  Setting the server port to
               * zero here indicates that we have not yet received the server
               * port number.
               */

              server.sin_port = 0;
            }

          /* Get the next packet from the server */

          nbytesrecvd = tftp_recvfrom(sd, packet, TFTP_IOBUFSIZE, &from);

          /* Check if anything valid was received */

          if (nbytesrecvd > 0)
            {
              /* Verify the sender address and port number */

              if (server.sin_addr.s_addr != from.sin_addr.s_addr)
                {
                  ninfo("Invalid address in DATA\n");
                  retry--;
                  continue;
                }

              if (server.sin_port && server.sin_port != from.sin_port)
                {
                  ninfo("Invalid port in DATA\n");
                  len = tftp_mkerrpacket(packet, TFTP_ERR_UNKID,
                                         TFTP_ERRST_UNKID);
                  ret = tftp_sendto(sd, packet, len, &from);
                  retry--;
                  continue;
                }

              /* Parse the incoming DATA packet */

              if (nbytesrecvd < TFTP_DATAHEADERSIZE)
                {
                  /* Packet is not big enough to be parsed */

                  ninfo("Tiny data packet ignored\n");
                  continue;
                }

              if (tftp_parsedatapacket(packet, &opcode, &rblockno) != OK ||
                  blockno != rblockno)
                {
                  /* Opcode is not TFTP_DATA or the block number is
                   * unexpected.
                   */

                  ninfo("Parse failure\n");
                  if (opcode > TFTP_MAXRFC1350)
                    {
                      len = tftp_mkerrpacket(packet, TFTP_ERR_ILLEGALOP,
                                             TFTP_ERRST_ILLEGALOP);
                      ret = tftp_sendto(sd, packet, len, &from);
                    }

                  continue;
                }

              /* Replace the server port to the one in the good response */

              if (!server.sin_port)
                {
                  server.sin_port = from.sin_port;
                }

              /* Then break out of the loop */

              break;
            }
        }

      /* Did we exhaust all of the retries? */

      if (retry == TFTP_RETRIES)
        {
          ninfo("Retry limit exceeded\n");
          goto errout_with_sd;
        }

      /* Write the received data chunk to the file */

      ndatabytes = nbytesrecvd - TFTP_DATAHEADERSIZE;
      tftp_dumpbuffer("Recvd DATA",
                      packet + TFTP_DATAHEADERSIZE, ndatabytes);
      if (tftp_cb(ctx, 0, packet + TFTP_DATAHEADERSIZE, ndatabytes) < 0)
        {
          goto errout_with_sd;
        }

      /* Send the acknowledgment */

      len = tftp_mkackpacket(packet, blockno);
      ret = tftp_sendto(sd, packet, len, &server);
      if (ret != len)
        {
          goto errout_with_sd;
        }

      ninfo("ACK blockno %d\n", blockno);
    }
  while (ndatabytes >= TFTP_DATASIZE);

  /* Return success */

  result = OK;

errout_with_sd:
  close(sd);

errout:
  free(packet);

  return result;
}

/****************************************************************************
 * Name: tftp_write
 ****************************************************************************/

static ssize_t tftp_write(FAR void *ctx, uint32_t offset, FAR uint8_t *buf,
                          size_t len)
{
  int fd = (intptr_t)ctx;
  size_t left = len;
  ssize_t nbyteswritten;

  while (left > 0)
    {
      /* Write the data... repeating the write in the event that it was
       * interrupted by a signal.
       */

      do
        {
          nbyteswritten = write(fd, buf, left);
        }
      while (nbyteswritten < 0 && errno == EINTR);

      /* Check for non-EINTR errors */

      if (nbyteswritten < 0)
        {
          nerr("ERROR: write failed: %d\n", errno);
          return ERROR;
        }

      /* Handle partial writes */

      ninfo("Wrote %zd bytes to file\n", nbyteswritten);
      left -= nbyteswritten;
      buf  += nbyteswritten;
    }

  return len;
}

/****************************************************************************
 * Name: tftpget
 *
 * Input Parameters:
 *   remote - The name of the file on the TFTP server.
 *   local  - Path to the location on a mounted filesystem where the file
 *            will be stored.
 *   addr   - The IP address of the server in network order
 *   binary - TRUE:  Perform binary ('octect') transfer
 *            FALSE: Perform text ('netascii') transfer
 *
 ****************************************************************************/

int tftpget(FAR const char *remote, FAR const char *local, in_addr_t addr,
            bool binary)
{
  int fd;                     /* File descriptor for file I/O */
  int result = ERROR;         /* Generic return status */

  /* Open the file for writing */

  fd = open(local, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      nerr("ERROR: open failed: %d\n", errno);
      goto errout;
    }

  result = tftpget_cb(remote, addr, binary, tftp_write,
                      (FAR void *)(intptr_t)fd);

  close(fd);

errout:
  return result;
}

#endif /* CONFIG_NET && CONFIG_NET_UDP */
