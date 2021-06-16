/****************************************************************************
 * apps/examples/bridge/host_main.c
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
  struct sockaddr_in receiver;
  struct sockaddr_in sender;
  struct sockaddr_in fromaddr;
  struct sockaddr_in toaddr;
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

  printf(LABEL "Create send socket: IPHOST=%08x PORT=INPORT_ANY\n",
         EXAMPLES_BRIDGE_SEND_IPHOST);

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

  sender.sin_family      = AF_INET;
  sender.sin_port        = 0;
  sender.sin_addr.s_addr = htonl(EXAMPLES_BRIDGE_SEND_IPHOST);

  if (bind(sndsd, (struct sockaddr*)&sender, sizeof(struct sockaddr_in)) < 0)
    {
      printf(LABEL "bind failure: %d\n", errno);
      goto errout_with_sendsd;
    }

  /* Create a UDP receive socket */

  printf(LABEL "Create receive socket: IPHOST=%08x RECVPORT=%d\n",
         EXAMPLES_BRIDGE_RECV_IPHOST, EXAMPLES_BRIDGE_SEND_HOSTPORT);

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

  receiver.sin_family      = AF_INET;
  receiver.sin_port        = htons(EXAMPLES_BRIDGE_SEND_HOSTPORT);
  receiver.sin_addr.s_addr = htonl(EXAMPLES_BRIDGE_RECV_IPHOST);

  if (bind(recvsd, (struct sockaddr*)&receiver, sizeof(struct sockaddr_in)) < 0)
    {
      fprintf(stderr, LABEL "ERROR: bind failure: %d\n", errno);
      goto errout_with_recvsd;
    }

  /* Send a packet */

  printf(LABEL "Sending %lu bytes: IPTARGET=%08x PORT=%d\n",
         sizeof(g_sndmessage),
         EXAMPLES_BRIDGE_RECV_IPADDR, EXAMPLES_BRIDGE_RECV_RECVPORT);

  toaddr.sin_family      = AF_INET;
  toaddr.sin_port        = htons(EXAMPLES_BRIDGE_RECV_RECVPORT);
  toaddr.sin_addr.s_addr = htonl(EXAMPLES_BRIDGE_RECV_IPADDR);

   nsent = sendto(sndsd, g_sndmessage,  sizeof(g_sndmessage), 0,
                 (struct sockaddr*)&toaddr, sizeof(struct sockaddr_in));

  /* Check for send errors */

  if (nsent < 0)
    {
      fprintf(stderr, LABEL "ERROR: sendto failed: %d\n", errno);
      goto errout_with_recvsd;
    }
  else if (nsent != sizeof(g_sndmessage))
    {
      fprintf(stderr, LABEL "ERROR: Bad send length: %ld Expected: %lu\n",
              (long)nsent, sizeof(g_sndmessage));
      goto errout_with_recvsd;
    }

  printf(LABEL "Message sent successfully\n");

  /* Read a packet */

  printf(LABEL "Receiving up to %d bytes\n",  EXAMPLES_BRIDGE_SEND_IOBUFIZE);

  addrlen = sizeof(struct sockaddr_in);
  nrecvd = recvfrom(recvsd, g_rdbuffer, EXAMPLES_BRIDGE_SEND_IOBUFIZE, 0,
                    (struct sockaddr*)&fromaddr, &addrlen);

  tmpaddr = ntohl(fromaddr.sin_addr.s_addr);
  printf(LABEL "Received %ld bytes from %d.%d.%d.%d:%d\n",
         (long)nrecvd,
         tmpaddr >> 24, (tmpaddr >> 16) & 0xff,
         (tmpaddr >> 8) & 0xff, tmpaddr & 0xff,
         ntohs(fromaddr.sin_port));

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
