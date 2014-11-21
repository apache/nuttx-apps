/****************************************************************************
 * examples/bridge/host_main.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_sndmessage[] = MESSAGE;
static uint8_t g_rdbuffer[EXAMPLES_BRIDGE_SEND_IOBUFIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * bridge_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct sockaddr_in recvaddr;
  struct sockaddr_in listenaddr;
  struct sockaddr_in sendaddr;
  socklen_t addrlen;
  in_addr_t tmpaddr;
  ssize_t nrecvd;
  ssize_t nsent;
  int optval;
  int recvsd;
  int sndsd;
  int i;
  int j;

  /* Create a UDP send socket */

  printf(LABEL "Create send socket\n");

  sndsd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sndsd < 0)
    {
      fprintf(stderr, LABEL "ERROR: Failed to create send socket: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(sndsd, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(int)) < 0)
    {
      fprintf(stderr, LABEL "ERROR: setsockopt SO_REUSEADDR failure: %d\n", errno);
      goto errout_with_sendsd;
    }

  /* Bind the socket to a local address */

  sendaddr.sin_family      = AF_INET;
  sendaddr.sin_port        = htons(EXAMPLES_BRIDGE_SEND_SNDPORT);
  sendaddr.sin_addr.s_addr = htonl(EXAMPLES_BRIDGE_SEND_IPHOST);

  if (bind(sndsd, (struct sockaddr*)&sendaddr, sizeof(struct sockaddr_in)) < 0)
    {
      printf(LABEL "bind failure: %d\n", errno);
      goto errout_with_sendsd;
    }

  /* Create a UDP receive socket */

  printf(LABEL "Create receive socket\n");

  recvsd = socket(PF_INET, SOCK_DGRAM, 0);
  if (recvsd < 0)
    {
      fprintf(stderr, LABEL "ERROR: Failed to create receive socket: %d\n", errno);
      goto errout_with_sendsd;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(recvsd, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(int)) < 0)
    {
      fprintf(stderr, LABEL "ERROR: setsockopt SO_REUSEADDR failure: %d\n", errno);
      goto errout_with_recvsd;
    }

  /* Bind the socket to a local address */

  listenaddr.sin_family      = AF_INET;
  listenaddr.sin_port        = htons(EXAMPLES_BRIDGE_RECV_RECVPORT);
  listenaddr.sin_addr.s_addr = htonl(EXAMPLES_BRIDGE_RECV_IPHOST);

  if (bind(recvsd, (struct sockaddr*)&listenaddr, sizeof(struct sockaddr_in)) < 0)
    {
      fprintf(stderr, LABEL "ERROR: bind failure: %d\n", errno);
      goto errout_with_recvsd;
    }

   /* Send a packet */

   printf(LABEL "Sending %lu bytes\n", sizeof(g_sndmessage));
   nsent = sendto(sndsd, g_sndmessage,  sizeof(g_sndmessage), 0,
                 (struct sockaddr*)&sendaddr, sizeof(struct sockaddr_in));

  /* Check for send errors */

  if (nsent < 0)
    {
      fprintf(stderr, LABEL "ERROR: sendto failed: %d\n", errno);
      goto errout_with_recvsd;
    }
  else if (nsent != sizeof(g_sndmessage))
    {
      fprintf(stderr, LABEL "ERROR: Bad send length: %ld Expected: %d\n",
              (long)nsent, sizeof(g_sndmessage));
      goto errout_with_recvsd;
    }

  printf(LABEL "Message sent successfully\n");

  /* Read a packet */

  printf(LABEL "Receiving up to %d bytes\n",  EXAMPLES_BRIDGE_SEND_IOBUFIZE);

  addrlen = sizeof(struct sockaddr_in);
  nrecvd = recvfrom(recvsd, g_rdbuffer, EXAMPLES_BRIDGE_SEND_IOBUFIZE, 0,
                    (struct sockaddr*)&recvaddr, &addrlen);

  tmpaddr = ntohl(recvaddr.sin_addr.s_addr);
  printf(LABEL "Received %ld bytes from %d.%d.%d.%d:%d\n",
         (long)nrecvd,
         tmpaddr >> 24, (tmpaddr >> 16) & 0xff,
         (tmpaddr >> 8) & 0xff, tmpaddr & 0xff,
         ntohs(recvaddr.sin_port));

  /* Check for a receive error or zero bytes received.  The negative
   * return value indicates a receive error; Zero would mean that the
   * other side of the "connection" performed an "orderly" shutdown.
   * This should not occur with a UDP socket and so must also be an
   * error of some kind.
   */

  if (nrecvd <= 0)
    {
      if (nrecvd < 0)
        {
          fprintf(stderr, LABEL "ERROR: recvfrom failed: %d\n", errno);
        }
      else
        {
          fprintf(stderr, LABEL "ERROR: recvfrom returned zero\n");
        }

      goto errout_with_recvsd;
    }

  if (nrecvd != nrecvd)
    {
      fprintf(stderr, LABEL "ERROR: Number of bytes received differs from number sent\n");
    }

  /* Dump the received packet */

  for (i = 0, j = 0; i < nrecvd; i++)
    {
      if ( g_rdbuffer[i] == ' ' && j >= 64)
        {
          putchar('\n');
          j = 0;
        }
      else if (isprint(g_rdbuffer[i]))
        {
          putchar(g_rdbuffer[i]);
          j++;
        }
      else if (g_rdbuffer[i] == '\n')
        {
          putchar('\n');
          j = 0;
        }
      else if (g_rdbuffer[i] != '\r')
        {
          printf("\%03o", g_rdbuffer[i]);
          j += 4;
        }
    }

  close(recvsd);
  close(recvsd);
  return EXIT_SUCCESS;

errout_with_recvsd:
   close(recvsd);
errout_with_sendsd:
   close(sndsd);
   return EXIT_FAILURE;
}

