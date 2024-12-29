/****************************************************************************
 * apps/examples/rpmsgsocket/rpsock_client.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <errno.h>
#include <inttypes.h>
#include <netpacket/rpmsg.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SYNCSIZE CONFIG_NET_RPMSG_RXBUF_SIZE
#define BUFSIZE  SYNCSIZE * 2
#define BUFHEAD  64
#ifndef ALIGN_UP
#  define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#endif

/****************************************************************************
 * Private types
 ****************************************************************************/

struct rpsock_arg_s
{
  FAR char *inbuf;
  FAR char *outbuf;
  bool nonblock;
  int  bufsize;
  int  check;
  int  fd;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static FAR void *rpsock_send_thread(FAR void *pvarg)
{
  FAR struct rpsock_arg_s *args = pvarg;
  FAR char *buf = args->outbuf;
  int bufsize = args->bufsize;
  int fd = args->fd;
  int total = 0;
  ssize_t ret;
  int cnt = 0;

  while (cnt < 2000)
    {
      FAR volatile uint32_t *intp;
      struct pollfd pfd;
      FAR char *tmp;
      int snd;
      int i;

      intp = (FAR uint32_t *)buf;
      for (i = 0; i < bufsize / sizeof(uint32_t); i++)
        {
          intp[i] = cnt * bufsize / sizeof(uint32_t) + i;
        }

      tmp = buf;
      snd = bufsize;
      while (snd > 0)
        {
          if (args->nonblock)
            {
              memset(&pfd, 0, sizeof(struct pollfd));
              pfd.fd = fd;
              pfd.events = POLLOUT;

              ret = poll(&pfd, 1, -1);
              if (ret < 0)
                {
                  printf("client: poll failure: %d\n", errno);
                  break;
                }
            }

          ret = send(fd, tmp, snd, 0);
          if (ret > 0)
            {
              printf("client send data, cnt %d, total %d\n",
                      cnt, cnt * bufsize);
              total += ret;
              tmp += ret;
              snd -= ret;
            }
          else if (ret < 0 && errno != EAGAIN)
            {
              printf("client send data failed errno %d\n", errno);
            }
          else if (args->nonblock)
            {
              usleep(10);
            }
        }

      if (cnt && cnt % 1024 == 0)
        {
          sleep(3);
        }

      cnt++;
    }

  sleep(5);

  snprintf(buf, 64, "endflags, send total %d", total);
  send(fd, buf, 64, 0);

  return NULL;
}

static int rpsock_unsync_test(FAR struct rpsock_arg_s *args)
{
  pthread_attr_t attr;
  pthread_t thread;
  int total = 0;
  int cnt = 0;
  int ret;

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 10 * 1024);
  ret = pthread_create(&thread, &attr, rpsock_send_thread, args);
  if (ret < 0)
    {
      return ret;
    }

  pthread_detach(thread);

  while (1)
    {
      struct pollfd pfd;

      if (args->nonblock)
        {
          memset(&pfd, 0, sizeof(struct pollfd));
          pfd.fd = args->fd;
          pfd.events = POLLIN;

          ret = poll(&pfd, 1, -1);
          if (ret < 0)
            {
              printf("client: poll failure: %d\n", errno);
              break;
            }
        }

      ret = recv(args->fd, args->inbuf, args->bufsize, 0);
      if (ret > 0)
        {
          FAR uint32_t *intp;
          int checks;
          int i;

          if (strncmp(args->inbuf, "endflags,", 9) == 0)
            {
              printf("client recv done, total %d, %s\n", total, args->inbuf);
              break;
            }

          printf("client recv data, act len %d, total %d\n", ret, total);

          if (args->check && ret > 4)
            {
              checks = ret - (ALIGN_UP(total, 4) - total);
              intp   = (FAR uint32_t *)(args->inbuf +
                                        (ALIGN_UP(total, 4) - total));

              for (i = 0; i < checks / sizeof(uint32_t); i++)
                {
                  if (intp[i] != ALIGN_UP(total, 4) / sizeof(uint32_t) + i)
                    {
                      printf("client check fail total %d, \
                              i %d, %08" PRIx32 ", %08zx\n",
                              ALIGN_UP(total, 4), i, intp[i],
                              ALIGN_UP(total, 4) / sizeof(uint32_t) + i);
                    }
                }
            }

          total += ret;
        }
      else
        {
          printf("client recv data failed errno %d\n", errno);
        }

      if (cnt++ > 1000)
        {
          sleep(2);
          cnt = 0;
        }
    }

  return 0;
}

static int rpsock_stream_client(FAR char *argv[])
{
  struct sockaddr_rpmsg myaddr;
  struct rpsock_arg_s args;
  bool nonblock = false;
  int cnt = 0;
  FAR char *outbuf;
  FAR char *inbuf;
  int sockfd;
  int ret;

  /* Allocate buffers */

  outbuf = malloc(BUFSIZE);
  inbuf  = malloc(BUFSIZE);
  if (!outbuf || !inbuf)
    {
      printf("client: failed to allocate buffers\n");
      ret = -ENOMEM;
      goto errout_with_buffers;
    }

  /* Create a new rpmsg domain socket */

  if (strcmp(argv[2], "nonblock") == 0)
    {
      nonblock = true;
    }

  printf("client: create socket SOCK_STREAM nonblock %d\n", nonblock);

  if (nonblock)
    {
      sockfd = socket(PF_RPMSG, SOCK_STREAM | SOCK_NONBLOCK, 0);
    }
  else
    {
      sockfd = socket(PF_RPMSG, SOCK_STREAM, 0);
    }

  if (sockfd < 0)
    {
      printf("client socket failure %d\n", errno);
      goto errout_with_buffers;
    }

  /* Connect the socket to the server */

  myaddr.rp_family = AF_RPMSG;
  strlcpy(myaddr.rp_name, argv[3], RPMSG_SOCKET_NAME_SIZE);
  strlcpy(myaddr.rp_cpu, argv[4], RPMSG_SOCKET_CPU_SIZE);

  printf("client: Connecting to %s,%s...\n", myaddr.rp_cpu, myaddr.rp_name);
  ret = connect(sockfd, (FAR struct sockaddr *)&myaddr, sizeof(myaddr));
  if (ret < 0 && errno == EINPROGRESS)
    {
      struct pollfd pfd;
      memset(&pfd, 0, sizeof(struct pollfd));
      pfd.fd = sockfd;
      pfd.events = POLLOUT;

      ret = poll(&pfd, 1, -1);
      if (ret < 0)
        {
          printf("client: poll failure: %d\n", errno);
          goto errout_with_socket;
        }
    }
  else if (ret < 0)
    {
      printf("client: connect failure: %d\n", errno);
      goto errout_with_socket;
    }

  printf("client: Connected\n");

  while (1)
    {
      size_t sendsize = BUFHEAD + cnt * (random() % 64);
      size_t recvsize = 0;
      ssize_t act;
      FAR char *tmp;
      FAR int *ptr;
      int snd;
      int i;

      if (sendsize > SYNCSIZE)
        {
          break;
        }

      snprintf(outbuf, BUFHEAD, "process%04d, msg%04d, name:%s",
               getpid(), cnt, argv[3]);

      ptr = (FAR int *)(outbuf + BUFHEAD);
      for (i = 0; i < (sendsize - BUFHEAD) / 4; i++)
        {
          ptr[i] = cnt * 100 + i;
        }

      printf("client send data, cnt %d, total len %zu, BUFHEAD %s\n",
                        cnt, sendsize, outbuf);

      tmp = outbuf;
      snd = sendsize;
      while (snd > 0)
        {
          ret = send(sockfd, tmp, snd, 0);
          if (ret < 0)
            {
              continue;
            }

          tmp += ret;
          snd -= ret;
        }

      tmp = inbuf;
      while (1)
        {
          struct pollfd pfd;

          if (nonblock)
            {
              memset(&pfd, 0, sizeof(struct pollfd));
              pfd.fd = sockfd;
              pfd.events = POLLIN;

              ret = poll(&pfd, 1, -1);
              if (ret < 0)
                {
                  printf("client: poll failure: %d\n", errno);
                  break;
                }
            }

          act = recv(sockfd, tmp, SYNCSIZE, 0);
          if (act == -EAGAIN)
            {
              continue;
            }
          else if (act < 0)
            {
              printf("client recv data failed %zd\n", act);
              break;
            }

          if (recvsize == 0)
            {
              printf("client recv data %s\n", tmp);
            }

          recvsize += act;
          tmp      += act;

          if (recvsize >= sendsize)
            {
              printf("client recv total %zu, send total %zu\n",
                     recvsize, sendsize);
              break;
            }
        }

      ptr = (FAR int *)(inbuf + BUFHEAD);
      for (i = 0; i < (recvsize - BUFHEAD) / 4; i++)
        {
          if (ptr[i] != cnt * 100 + i)
            {
              printf("client check fail i %d, %d, %d\n", i, ptr[i], cnt + i);
            }
        }

      cnt++;
    }

  args.fd  = sockfd;
  args.inbuf = inbuf;
  args.outbuf = outbuf;
  args.nonblock = nonblock;
  args.bufsize = BUFSIZE;
  args.check = true;

  rpsock_unsync_test(&args);

  printf("client: Terminating\n");

errout_with_socket:
  close(sockfd);

errout_with_buffers:
  free(outbuf);
  free(inbuf);
  return -errno;
}

static int rpsock_dgram_client(FAR char *argv[])
{
  struct sockaddr_rpmsg myaddr;
  struct rpsock_arg_s args;
  bool nonblock = false;
  FAR char *outbuf;
  FAR char *inbuf;
  int sockfd;
  int ret;

  /* Allocate buffers */

  outbuf = malloc(BUFSIZE);
  inbuf  = malloc(BUFSIZE);
  if (!outbuf || !inbuf)
    {
      printf("client: failed to allocate buffers\n");
      ret = -ENOMEM;
      goto errout_with_buffers;
    }

  /* Create a new rpmsg domain socket */

  if (strcmp(argv[2], "nonblock") == 0)
    {
      nonblock = true;
    }

  printf("client: create socket SOCK_DGRAM nonblock %d\n", nonblock);

  if (nonblock)
    {
      sockfd = socket(PF_RPMSG, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    }
  else
    {
      sockfd = socket(PF_RPMSG, SOCK_DGRAM, 0);
    }

  if (sockfd < 0)
    {
      printf("client socket failure %d\n", errno);
      goto errout_with_buffers;
    }

  /* Connect the socket to the server */

  myaddr.rp_family = AF_RPMSG;
  strlcpy(myaddr.rp_name, argv[3], RPMSG_SOCKET_NAME_SIZE);
  strlcpy(myaddr.rp_cpu, argv[4], RPMSG_SOCKET_CPU_SIZE);

  printf("client: Connecting to %s,%s...\n", myaddr.rp_cpu, myaddr.rp_name);
  ret = connect(sockfd, (FAR struct sockaddr *)&myaddr, sizeof(myaddr));
  if (ret < 0 && errno == EINPROGRESS)
    {
      struct pollfd pfd;
      memset(&pfd, 0, sizeof(struct pollfd));
      pfd.fd = sockfd;
      pfd.events = POLLOUT;

      ret = poll(&pfd, 1, -1);
      if (ret < 0)
        {
          printf("[client] poll failure: %d\n", errno);
          goto errout_with_socket;
        }
    }
  else if (ret < 0)
    {
      printf("client: connect failure: %d\n", errno);
      goto errout_with_socket;
    }

  printf("client: Connected\n");

  args.fd  = sockfd;
  args.inbuf = inbuf;
  args.outbuf = outbuf;
  args.nonblock = nonblock;
  args.bufsize = SYNCSIZE - 32;
  args.check = false;

  rpsock_unsync_test(&args);

  printf("client: Terminating\n");

errout_with_socket:
  close(sockfd);

errout_with_buffers:
  free(outbuf);
  free(inbuf);
  return -errno;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rpsock_client_main
 *
 * Description:
 *   Main entry point for the rpsock_client example.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 4)
    {
      printf("Usage: rpsock_client stream/dgram"
             " block/nonblock rp_name rp_cpu\n");
      return -EINVAL;
    }

  if (!strcmp(argv[1], "stream"))
    {
      return rpsock_stream_client(argv);
    }
  else if (!strcmp(argv[1], "dgram"))
    {
      return rpsock_dgram_client(argv);
    }

  return -EINVAL;
}
