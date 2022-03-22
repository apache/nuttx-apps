/****************************************************************************
 * apps/examples/rpmsgsocket/rpsock_server.c
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
#include <netpacket/rpmsg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>
#include <errno.h>

/****************************************************************************
 * Private types
 ****************************************************************************/

struct rpsock_arg_s
{
  int  fd;
  bool nonblock;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void *rpsock_thread(pthread_addr_t pvarg)
{
  struct rpsock_arg_s *args = pvarg;
  struct pollfd pfd;
  char buf[255];
  ssize_t ret;

  while (1)
    {
      char *tmp;
      int snd;

      if (args->nonblock)
        {
          memset(&pfd, 0, sizeof(struct pollfd));
          pfd.fd = args->fd;
          pfd.events = POLLIN;

          ret = poll(&pfd, 1, -1);
          if (ret < 0)
            {
              printf("server poll failed errno %d\n", errno);
              break;
            }
        }

      ret = recv(args->fd, buf, sizeof(buf), 0);
      if (ret == 0 || (ret < 0 && errno == ECONNRESET))
        {
          printf("server recv data normal exit\n");
          break;
        }
      else if (ret < 0 && errno == EAGAIN)
        {
          usleep(10);
          continue;
        }
      else if (ret < 0 && errno == EINPROGRESS)
        {
          memset(&pfd, 0, sizeof(struct pollfd));
          pfd.fd = args->fd;
          pfd.events = POLLOUT;
          ret = poll(&pfd, 1, -1);
          if (ret < 0)
            {
              printf("server: poll failure: %d\n", errno);
              break;
            }

          continue;
        }
      else if (ret < 0)
        {
          printf("server recv data failed ret %d, errno %d\n", ret, errno);
          break;
        }

      snd = ret;
      tmp = buf;
      while (snd > 0)
        {
          if (args->nonblock)
            {
              memset(&pfd, 0, sizeof(struct pollfd));
              pfd.fd = args->fd;
              pfd.events = POLLOUT;

              ret = poll(&pfd, 1, -1);
              if (ret < 0)
                {
                  printf("server: poll failure: %d\n", errno);
                  break;
                }
            }

          ret = send(args->fd, tmp, snd, 0);
          if (ret > 0)
            {
              tmp += ret;
              snd -= ret;
            }
          else if (ret == 0)
            {
              printf("server send data normal exit\n");
              break;
            }
          else if (errno == EAGAIN)
            {
              usleep(10);
            }
          else
            {
              printf("server send data failed errno %d\n", errno);
              break;
            }
        }
    }

  printf("server Complete ret %d, errno %d\n", ret, errno);
  free(args);
  return NULL;
}

static int rpsock_stream_server(int argc, char *argv[])
{
  struct sockaddr_rpmsg myaddr;
  bool nonblock = false;
  socklen_t addrlen;
  int listensd;
  int ret;

  /* Create a new rpmsg domain socket */

  if (strcmp(argv[2], "nonblock") == 0)
    {
      nonblock = true;
    }

  printf("server: create socket SOCK_STREAM nonblock %d\n", nonblock);

  if (nonblock)
    {
      listensd = socket(PF_RPMSG, SOCK_STREAM | SOCK_NONBLOCK, 0);
    }
  else
    {
      listensd = socket(PF_RPMSG, SOCK_STREAM, 0);
    }

  if (listensd < 0)
    {
      printf("server: socket failure: %d\n", errno);
      return -1;
    }

  /* Bind the socket to a local address */

  myaddr.rp_family = AF_RPMSG;
  strncpy(myaddr.rp_name, argv[3], RPMSG_SOCKET_NAME_SIZE);
  if (argc == 5)
    {
      strncpy(myaddr.rp_cpu, argv[4], RPMSG_SOCKET_CPU_SIZE);
    }
  else
    {
      myaddr.rp_cpu[0] = '\0';
    }

  printf("server: bind cpu %s, name %s ...\n",
          myaddr.rp_cpu, myaddr.rp_name);
  ret = bind(listensd, (struct sockaddr *)&myaddr, sizeof(myaddr));
  if (ret < 0)
    {
      printf("server: bind failure: %d\n", errno);
      goto errout_with_listensd;
    }

  /* Listen for connections on the bound socket */

  printf("server: listen ...\n");
  ret = listen(listensd, 16);
  if (ret < 0)
    {
      printf("server: listen failure %d\n", errno);
      goto errout_with_listensd;
    }

  while (1)
    {
      struct rpsock_arg_s *args;
      pthread_t thread;
      struct pollfd pfd;
      int new;

      if (nonblock)
        {
          memset(&pfd, 0, sizeof(struct pollfd));
          pfd.fd = listensd;
          pfd.events = POLLIN;

          ret = poll(&pfd, 1, -1);
          if (ret < 0)
            {
              printf("server: poll failure: %d\n", errno);
              goto errout_with_listensd;
            }
        }

      printf("server: try accept ...\n");
      new = accept(listensd, (struct sockaddr *)&myaddr, &addrlen);
      if (new < 0)
          break;

      printf("server: Connection accepted -- %d\n", new);

      args = malloc(sizeof(struct rpsock_arg_s));
      assert(args);

      args->fd       = new;
      args->nonblock = nonblock;

      pthread_create(&thread, NULL, rpsock_thread,
                     (pthread_addr_t)args);
      pthread_detach(thread);
    }

  printf("server: Terminating\n");
  close(listensd);
  return 0;

errout_with_listensd:
  close(listensd);

  return -errno;
}

static int rpsock_dgram_server(int argc, char *argv[])
{
  struct sockaddr_rpmsg myaddr;
  struct rpsock_arg_s *args;
  bool nonblock = false;
  int fd;
  int ret;

  /* Create a new rpmsg domain socket */

  if (strcmp(argv[2], "nonblock") == 0)
    {
      nonblock = true;
    }

  printf("server: create socket SOCK_STREAM nonblock %d\n", nonblock);

  if (nonblock)
    {
      fd = socket(PF_RPMSG, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    }
  else
    {
      fd = socket(PF_RPMSG, SOCK_DGRAM, 0);
    }

  if (fd < 0)
    {
      printf("server: socket failure: %d\n", errno);
      return -errno;
    }

  /* Bind the socket to a local address */

  myaddr.rp_family = AF_RPMSG;
  strncpy(myaddr.rp_name, argv[3], RPMSG_SOCKET_NAME_SIZE);
  if (argc == 5)
    {
      strncpy(myaddr.rp_cpu, argv[4], RPMSG_SOCKET_CPU_SIZE);
    }
  else
    {
      myaddr.rp_cpu[0] = '\0';
    }

  printf("server: bind cpu %s, name %s ...\n",
          myaddr.rp_cpu, myaddr.rp_name);
  ret = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
  ret = connect(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
  if (ret < 0 && errno == EINPROGRESS)
    {
      struct pollfd pfd;
      memset(&pfd, 0, sizeof(struct pollfd));
      pfd.fd = fd;
      pfd.events = POLLOUT;

      ret = poll(&pfd, 1, -1);
      if (ret < 0)
        {
          printf("server: poll failure: %d\n", errno);
          close(fd);
          return ret;
        }
    }
  else if (ret < 0)
    {
      printf("server: bind failure: %d\n", errno);
      close(fd);
      return ret;
    }

  args = malloc(sizeof(struct rpsock_arg_s));
  assert(args);

  args->fd       = fd;
  args->nonblock = nonblock;

  rpsock_thread(args);

  close(fd);
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 4 && argc != 5)
    {
      printf("Usage: rpsock_server stream/dgram"
             " block/nonblock rp_name [rp_cpu]\n");
      return -EINVAL;
    }

  if (!strcmp(argv[1], "stream"))
    {
      return rpsock_stream_server(argc, argv);
    }
  else if (!strcmp(argv[1], "dgram"))
    {
      return rpsock_dgram_server(argc, argv);
    }

  return -EINVAL;
}
