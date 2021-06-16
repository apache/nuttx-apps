/****************************************************************************
 * apps/examples/ustream/ustream_server.c
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
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
  FAR char *buffer;
  int listensd;
  int acceptsd;
  int nbytesread;
  int totalbytesread;
  int nbytessent;
  int ch;
  int ret;
  int i;

  /* Allocate a BIG buffer */

  buffer = (char*)malloc(2*SENDSIZE);
  if (!buffer)
    {
      printf("server: failed to allocate buffer\n");
      exit(1);
    }

  /* Create a new Unix domain socket */

  listensd = socket(PF_LOCAL, SOCK_STREAM, 0);
  if (listensd < 0)
    {
      printf("server: socket failure: %d\n", errno);
      goto errout_with_buffer;
    }

  /* Bind the socket to a local address */

  addrlen = strlen(CONFIG_EXAMPLES_USTREAM_ADDR);
  if (addrlen > UNIX_PATH_MAX - 1)
    {
      addrlen = UNIX_PATH_MAX - 1;
    }

  myaddr.sun_family = AF_LOCAL;
  strncpy(myaddr.sun_path, CONFIG_EXAMPLES_USTREAM_ADDR, addrlen);
  myaddr.sun_path[addrlen] = '\0';

  addrlen += sizeof(sa_family_t) + 1;
  ret = bind(listensd, (struct sockaddr*)&myaddr, addrlen);
  if (ret < 0)
    {
      printf("server: bind failure: %d\n", errno);
      goto errout_with_listensd;
    }

  /* Listen for connections on the bound socket */

  printf("server: Accepting connections on %s ...\n", CONFIG_EXAMPLES_USTREAM_ADDR);

  if (listen(listensd, 5) < 0)
    {
      printf("server: listen failure %d\n", errno);
      goto errout_with_listensd;
    }

  /* Accept only one connection */

#ifdef CONFIG_EXAMPLES_USTREAM_USE_POLL
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = listensd;
  pfd.events = POLLIN;

  ret = poll(&pfd, 1, -1);
  if (ret < 0)
    {
      printf("server: poll failure: %d\n", errno);
      goto errout_with_listensd;
    }
  else if (ret == 0)
    {
      printf("server: poll failure: returned zero\n");
      goto errout_with_listensd;
    }
  else if (!(pfd.revents & POLLIN))
    {
      printf("server: poll failure: no POLLIN\n");
      goto errout_with_listensd;
    }
#endif

  acceptsd = accept(listensd, (struct sockaddr*)&myaddr, &addrlen);
  if (acceptsd < 0)
    {
      printf("server: accept failure: %d\n", errno);
      goto errout_with_listensd;
    }

  printf("server: Connection accepted -- receiving\n");

  /* Receive canned message */

  totalbytesread = 0;
  while (totalbytesread < SENDSIZE)
    {
#ifdef CONFIG_EXAMPLES_USTREAM_USE_POLL
      memset(&pfd, 0, sizeof(pfd));
      pfd.fd = acceptsd;
      pfd.events = POLLIN;

      ret = poll(&pfd, 1, -1);
      if (ret < 0)
        {
          printf("server: recv-poll failed: %d\n", errno);
          goto errout_with_acceptsd;
        }
      else if (ret == 0)
        {
          printf("server: recv-poll failed: returned zero\n");
          goto errout_with_acceptsd;
        }
      else if (!(pfd.revents & POLLIN))
        {
          printf("server: recv-poll failed: no POLLIN\n");
          goto errout_with_acceptsd;
        }
#endif

      printf("server: Reading...\n");
      nbytesread = recv(acceptsd, &buffer[totalbytesread], 2*SENDSIZE - totalbytesread, 0);
      if (nbytesread < 0)
        {
          printf("server: recv failed: %d\n", errno);
          goto errout_with_acceptsd;
        }
      else if (nbytesread == 0)
        {
          printf("server: The client broke the connection\n");
          goto errout_with_acceptsd;
        }

      totalbytesread += nbytesread;
      printf("server: Received %d of %d bytes\n", totalbytesread, SENDSIZE);
    }

  /* Verify the message */

  if (totalbytesread != SENDSIZE)
    {
      printf("server: Received %d / Expected %d bytes\n", totalbytesread, SENDSIZE);
      goto errout_with_acceptsd;
    }

  ch = 0x20;
  for (i = 0; i < SENDSIZE; i++ )
    {
      if (buffer[i] != ch)
        {
          printf("server: Byte %d is %02x / Expected %02x\n", i, buffer[i], ch);
          goto errout_with_acceptsd;
        }

      if (++ch > 0x7e)
        {
          ch = 0x20;
        }
    }

  /* Then send the same data back to the client */

#ifdef CONFIG_EXAMPLES_USTREAM_USE_POLL
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = acceptsd;
  pfd.events = POLLOUT;

  ret = poll(&pfd, 1, -1);
  if (ret < 0)
    {
      printf("server: send-poll failed: %d\n", errno);
      goto errout_with_acceptsd;
    }
  else if (ret == 0)
    {
      printf("server: send-poll failed: returned zero\n");
      goto errout_with_acceptsd;
    }
  else if (!(pfd.revents & POLLOUT))
    {
      printf("server: send-poll failed: no POLLOUT\n");
      goto errout_with_acceptsd;
    }
#endif

  printf("server: Sending %d bytes\n", totalbytesread);
  nbytessent = send(acceptsd, buffer, totalbytesread, 0);
  if (nbytessent <= 0)
    {
      printf("server: send failed: %d\n", errno);
      goto errout_with_acceptsd;
    }

  printf("server: Sent %d bytes\n", nbytessent);
  printf("server: Terminating\n");
  close(listensd);
  close(acceptsd);
  free(buffer);
  return 0;

errout_with_acceptsd:
  close(acceptsd);

errout_with_listensd:
  close(listensd);

errout_with_buffer:
  free(buffer);
  return 1;
}
