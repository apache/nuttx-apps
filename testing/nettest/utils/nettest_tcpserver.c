/****************************************************************************
 * apps/testing/nettest/utils/nettest_tcpserver.c
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

#include <netinet/in.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_BUFFER_SIZE        80
#define EXIT_MSG                "exit"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nettest_listener_s
{
  fd_set master;
  fd_set working;
  char   buffer[TEST_BUFFER_SIZE];
  int    listensdv4;
  int    listensdv6;
  int    mxsd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: net_closeclient
 ****************************************************************************/

static void nettest_closeclient(FAR struct nettest_listener_s *nls, int sd)
{
  close(sd);
  FD_CLR(sd, &nls->master);

  /* If we just closed the max SD, then search downward for the next
   * biggest SD.
   */

  while (FD_ISSET(nls->mxsd, &nls->master) == false)
    {
      nls->mxsd -= 1;
    }
}

/****************************************************************************
 * Name: is_exit_message
 ****************************************************************************/

static inline bool is_exit_message(FAR char *buffer)
{
  return strncmp(buffer, EXIT_MSG, strlen(EXIT_MSG)) == 0;
}

/****************************************************************************
 * Name: net_incomingdata
 ****************************************************************************/

static inline bool nettest_incomingdata(FAR struct nettest_listener_s *nls,
                                        int sd)
{
  FAR char *ptr;
  int nbytes;
  int ret;

  /* Read data from the socket */

  for (; ; )
    {
      ret = recv(sd, nls->buffer, TEST_BUFFER_SIZE, 0);
      if (ret <= 0)
        {
          if (ret < 0 && errno == EAGAIN)
            {
              return false;
            }

          nettest_closeclient(nls, sd);
          return false;
        }

      nls->buffer[ret] = '\0';
      if (is_exit_message(nls->buffer))
        {
          return true;
        }

      /* Echo the data back to the client */

      nbytes = ret;
      for (ptr = nls->buffer; nbytes > 0; )
        {
          ret = send(sd, ptr, nbytes, 0);
          if (ret >= 0)
            {
              nbytes -= ret;
              ptr    += ret;
            }
          else if (errno == EAGAIN)
            {
              usleep(1000);
            }
          else
            {
              nettest_closeclient(nls, sd);
              return false;
            }
        }
    }
}

/****************************************************************************
 * Name: net_connection
 ****************************************************************************/

static inline void nettest_connection(FAR struct nettest_listener_s *nls,
                                      int lsd)
{
  /* Loop until all connections have been processed */

  for (; ; )
    {
      int sd = accept4(lsd, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);
      if (sd >= 0)
        {
#ifdef CONFIG_FDCHECK
          sd = fdcheck_restore(sd);
#endif

          /* Add the new connection to the master set */

          FD_SET(sd, &nls->master);
          if (sd > nls->mxsd)
            {
              nls->mxsd = sd;
            }
        }
      else
        {
          return;
        }
    }
}

/****************************************************************************
 * Name: net_cfgsocket
 ****************************************************************************/

static int nettest_tcpsocket(FAR struct sockaddr *addr, int addrlen)
{
  int listensd;
  int value = 1;
  int ret;

  listensd = socket(addr->sa_family, SOCK_STREAM |
                    SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
  if (listensd < 0)
    {
      return listensd;
    }

#ifdef CONFIG_FDCHECK
  listensd = fdcheck_restore(listensd);
#endif

  ret = setsockopt(listensd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));
  if (ret < 0)
    {
      goto errout;
    }

  ret = bind(listensd, addr, addrlen);
  if (ret < 0)
    {
      goto errout;
    }

  ret = listen(listensd, 32);
  if (ret < 0)
    {
      goto errout;
    }

  return listensd;

errout:
  close(listensd);
  return ret;
}

/****************************************************************************
 * Name: nettest_lo_addr
 ****************************************************************************/

socklen_t nettest_lo_addr(FAR struct sockaddr *addr, int family)
{
  switch (family)
  {
    case AF_INET:
      FAR struct sockaddr_in *addr4 = (FAR struct sockaddr_in *)addr;
      addr4->sin_family      = AF_INET;
      addr4->sin_port        = htons(CONFIG_TESTING_NET_TCP_PORT);
      addr4->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      return sizeof(struct sockaddr_in);
    case AF_INET6:
      FAR struct sockaddr_in6 *addr6 = (FAR struct sockaddr_in6 *)addr;
      addr6->sin6_family = AF_INET6;
      addr6->sin6_port   = htons(CONFIG_TESTING_NET_TCP_PORT);
      addr6->sin6_addr   = (struct in6_addr)IN6ADDR_LOOPBACK_INIT;
      return sizeof(struct sockaddr_in6);
    default:
      return 0;
  }
}

/****************************************************************************
 * Name: net_mksocket
 ****************************************************************************/

static inline bool nettest_mksockets(FAR struct nettest_listener_s *nls)
{
  struct sockaddr_storage addr;
  socklen_t addrlen;

  FD_ZERO(&nls->master);

#ifdef CONFIG_NET_IPv4
  addrlen         = nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET);
  nls->listensdv4 = nettest_tcpsocket((FAR struct sockaddr *)&addr, addrlen);
  if (nls->listensdv4 < 0)
    {
      return false;
    }

  FD_SET(nls->listensdv4, &nls->master);
#endif

#ifdef CONFIG_NET_IPv6
  addrlen         = nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET6);
  nls->listensdv6 = nettest_tcpsocket((FAR struct sockaddr *)&addr, addrlen);
  if (nls->listensdv6 < 0)
    {
      return false;
    }

  FD_SET(nls->listensdv6, &nls->master);
#endif

  return true;
}

/****************************************************************************
 * Name: nettest_lo_listener
 ****************************************************************************/

FAR static void *nettest_lo_listener(pthread_addr_t pvarg)
{
  FAR sem_t *sem = (FAR sem_t *)pvarg;
  struct nettest_listener_s nls;
  struct timeval timeout;
  bool exiting = false;
  int nsds;
  int ret;
  int i;

  /* Set up a listening socket */

  memset(&nls, 0, sizeof(struct nettest_listener_s));
  nls.listensdv4 = -1;
  nls.listensdv6 = -1;

  if (!nettest_mksockets(&nls))
    {
      sem_post(sem);
      return NULL;
    }

  nls.mxsd = MAX(nls.listensdv4, nls.listensdv6);

  /* Set up indefinitely timeout */

  timeout.tv_sec  = -1;
  timeout.tv_usec = 0;

  /* Loop waiting for incoming connections or for incoming data
   * on any of the connect sockets.
   */

  sem_post(sem);

  while (!exiting)
    {
      /* Wait on select */

      memcpy(&nls.working, &nls.master, sizeof(fd_set));
      ret = select(nls.mxsd + 1, &nls.working, NULL, NULL, &timeout);
      if (ret < 0)
        {
          break;
        }

      /* Check for timeout */

      if (ret == 0)
        {
          continue;
        }

      /* Find which descriptors caused the wakeup */

      nsds = ret;
      for (i = 0; i <= nls.mxsd && nsds > 0; i++)
        {
          /* Is this descriptor ready? */

          if (FD_ISSET(i, &nls.working))
            {
              /* Yes, is it our listener? */

              nsds--;
              if (i == nls.listensdv4 || i == nls.listensdv6)
                {
                  nettest_connection(&nls, i);
                }
              else
                {
                  exiting = nettest_incomingdata(&nls, i);
                }
            }
        }
    }

  /* Close the listening socket */

#ifdef CONFIG_NET_IPv4
  close(nls.listensdv4);
#endif
#ifdef CONFIG_NET_IPv6
  close(nls.listensdv6);
#endif

  /* Keeps some compilers from complaining */

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nettest_destroy_tcp_lo_server
 ****************************************************************************/

int nettest_destroy_tcp_lo_server(pthread_t server_tid)
{
  int sockfd;
  int ret;
  struct sockaddr_storage myaddr;
#ifdef CONFIG_NET_IPv4
  socklen_t addrlen = nettest_lo_addr((FAR struct sockaddr *)&myaddr,
                                      AF_INET);
#else
  socklen_t addrlen = nettest_lo_addr((FAR struct sockaddr *)&myaddr,
                                      AF_INET6);
#endif

  if (server_tid > 0)
    {
      sockfd = socket(myaddr.ss_family, SOCK_STREAM, 0);
      if (sockfd < 0)
        {
          return -errno;
        }

      ret = connect(sockfd, (FAR struct sockaddr *)&myaddr, addrlen);
      if (ret < 0)
        {
          goto err_destory;
        }

      ret = send(sockfd, EXIT_MSG, strlen(EXIT_MSG), 0);
      if (ret < 0)
        {
          goto err_destory;
        }

      pthread_join(server_tid, NULL);
      close(sockfd);
    }

  return 0;

err_destory:
  close(sockfd);
  return -errno;
}

/****************************************************************************
 * Name: nettest_create_tcp_lo_server
 ****************************************************************************/

pthread_t nettest_create_tcp_lo_server(void)
{
  pthread_t server_tid;
  sem_t sem;
  int ret;

  sem_init(&sem, 0, 0);

  ret = pthread_create(&server_tid, NULL, nettest_lo_listener, &sem);
  if (ret != 0)
    {
      return -ret;
    }

  /* Wait for the server to start */

  sem_wait(&sem);

  return server_tid;
}
