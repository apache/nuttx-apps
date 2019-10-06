/****************************************************************************
 * examples/ustream/ustream_client.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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

#include <nuttx/config.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include "ustream.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_EXAMPLES_USTREAM_USE_POLL
  struct pollfd pfd;
#endif
  struct sockaddr_un myaddr;
  socklen_t addrlen;
  FAR char *outbuf;
  FAR char *inbuf;
  int sockfd;
  int nbytessent;
  int nbytesrecvd;
  int totalbytesrecvd;
  int ch;
  int ret;
  int i;

  /* Allocate buffers */

  outbuf = (char*)malloc(SENDSIZE);
  inbuf  = (char*)malloc(SENDSIZE);
  if (!outbuf || !inbuf)
    {
      printf("client: failed to allocate buffers\n");
      exit(1);
    }

  /* Create a new Unix domain socket */

  sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      printf("client socket failure %d\n", errno);
      goto errout_with_buffers;
    }

  /* Connect the socket to the server */

  addrlen = strlen(CONFIG_EXAMPLES_USTREAM_ADDR);
  if (addrlen > UNIX_PATH_MAX - 1)
    {
      addrlen = UNIX_PATH_MAX - 1;
    }

  myaddr.sun_family = AF_LOCAL;
  strncpy(myaddr.sun_path, CONFIG_EXAMPLES_USTREAM_ADDR, addrlen);
  myaddr.sun_path[addrlen] = '\0';

  printf("client: Connecting to %s...\n", CONFIG_EXAMPLES_USTREAM_ADDR);
  addrlen += sizeof(sa_family_t) + 1;
  ret = connect( sockfd, (struct sockaddr *)&myaddr, addrlen);
  if (ret < 0)
    {
      printf("client: connect failure: %d\n", errno);
      goto errout_with_socket;
    }

  printf("client: Connected\n");

  /* Initialize the buffer */

  ch = 0x20;
  for (i = 0; i < SENDSIZE; i++ )
    {
      outbuf[i] = ch;
      if (++ch > 0x7e)
        {
          ch = 0x20;
        }
    }

  /* Then send and receive one message */

#ifdef CONFIG_EXAMPLES_USTREAM_USE_POLL
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = sockfd;
  pfd.events = POLLOUT;

  ret = poll(&pfd, 1, -1);
  if (ret < 0)
    {
      printf("client: send-poll failed: %d\n", errno);
      goto errout_with_socket;
    }
  else if (ret == 0)
    {
      printf("client: send-poll failed: returned zero\n");
      goto errout_with_socket;
    }
  else if (!(pfd.revents & POLLOUT))
    {
      printf("client: send-poll failed: no POLLOUT\n");
      goto errout_with_socket;
    }
#endif

  printf("client: Sending %d bytes\n", SENDSIZE);
  nbytessent = send(sockfd, outbuf, SENDSIZE, 0);
  printf("client: Sent %d bytes\n", nbytessent);

  if (nbytessent < 0)
    {
      printf("client: send failed: %d\n", errno);
      goto errout_with_socket;
    }
  else if (nbytessent != SENDSIZE)
    {
      printf("client: Bad send length: %d Expected: %d\n", nbytessent, SENDSIZE);
      goto errout_with_socket;
    }

  totalbytesrecvd = 0;
  do
    {
#ifdef CONFIG_EXAMPLES_USTREAM_USE_POLL
      memset(&pfd, 0, sizeof(pfd));
      pfd.fd = sockfd;
      pfd.events = POLLIN;

      ret = poll(&pfd, 1, -1);
      if (ret < 0)
        {
          printf("client: recv-poll failed: %d\n", errno);
          goto errout_with_socket;
        }
      else if (ret == 0)
        {
          printf("client: recv-poll failed: returned zero\n");
          goto errout_with_socket;
        }
      else if (!(pfd.revents & POLLIN))
        {
          printf("client: recv-poll failed: no POLLOUT\n");
          goto errout_with_socket;
        }
#endif

      printf("client: Receiving...\n");
      nbytesrecvd = recv(sockfd, &inbuf[totalbytesrecvd], SENDSIZE - totalbytesrecvd, 0);

      if (nbytesrecvd < 0)
        {
          printf("client: recv failed: %d\n", errno);
          goto errout_with_socket;
        }
      else if (nbytesrecvd == 0)
        {
          printf("client: The server closed the connection\n");
          goto errout_with_socket;
        }

      totalbytesrecvd += nbytesrecvd;
      printf("client: Received %d of %d bytes\n", totalbytesrecvd, SENDSIZE);
    }
  while (totalbytesrecvd < SENDSIZE);

  if (totalbytesrecvd != SENDSIZE)
    {
      printf("client: Bad recv length: %d Expected: %d\n", totalbytesrecvd, SENDSIZE);
      goto errout_with_socket;
    }
  else if (memcmp(inbuf, outbuf, SENDSIZE) != 0)
    {
      printf("client: Received buffer does not match sent buffer\n");
      goto errout_with_socket;
    }

  printf("client: Terminating\n");
  close(sockfd);
  free(outbuf);
  free(inbuf);
  return 0;

errout_with_socket:
  close(sockfd);

errout_with_buffers:
  free(outbuf);
  free(inbuf);
  return 1;
}
