/****************************************************************************
 * apps/netutils/rexecd/rexecd.c
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

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <netpacket/rpmsg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define REXECD_PORT    512
#define REXECD_BUFSIZE 512

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *doit(pthread_addr_t pvarg);
static int getstr(int fd, FAR char *buf);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int getstr(int fd, FAR char *buf)
{
  FAR char *end = buf;
  int ret;

  do
    {
      ret = read(fd, end, 1);
      if (ret <= 0)
        {
          return ret;
        }

      end += ret;
    }
  while (*(end - 1));

  return ret;
}

static FAR void *doit(pthread_addr_t pvarg)
{
  char buf[REXECD_BUFSIZE];
  FAR FILE *fp;
  int sock = (int)pvarg;
  int ret;
  int len;

  /* we need to read err_sock, user and passwd, but ignore them */

  getstr(sock, buf);
  getstr(sock, buf);
  getstr(sock, buf);

  /* we need to read command */

  getstr(sock, buf);
  fp = popen(buf, "r");
  if (!fp)
    {
      goto errout;
    }

  while (1)
    {
      ret = fread(buf, 1, REXECD_BUFSIZE, fp);
      if (ret <= 0)
        {
          break;
        }

      do
        {
          len = write(sock, buf, ret);
          if (len <= 0)
            {
              break;
            }

          ret -= len;
        }
      while (ret > 0);
    }

  pclose(fp);
errout:
  close(sock);
  return NULL;
}

static void usage(FAR const char *progname)
{
  fprintf(stderr, "Usage: %s [-4|-6|-r]\n", progname);
  fprintf(stderr, "Remote Execution Daemon:\n"
                  " -4, Specify address family to AF_INET(default)\n"
                  " -6, Specify address family to AF_INET6\n"
                  " -r, Specify address family to AF_RPMSG\n");
  exit(EXIT_FAILURE);
}

int main(int argc, FAR char **argv)
{
  struct sockaddr_storage addr;
  pthread_attr_t attr;
  pthread_t tid;
  int family;
  int option;
  int serv;
  int sock;
  int ret;

  family = AF_INET;
  while ((option = getopt(argc, argv, "46r")) != ERROR)
    {
      switch (option)
        {
          case '4':
            family = AF_INET;
            break;
          case '6':
            family = AF_INET6;
            break;
          case 'r':
            family = AF_RPMSG;
            break;
          default:
            usage(argv[0]);
        }
    }

  serv = socket(family, SOCK_STREAM, 0);
  if (serv < 0)
    {
      return serv;
    }

  memset(&addr, 0, sizeof(addr));
  switch (family)
    {
      default:
      case AF_INET:
        ((FAR struct sockaddr_in *)&addr)->sin_family = AF_INET;
        ((FAR struct sockaddr_in *)&addr)->sin_port = REXECD_PORT;
        ret = sizeof(struct sockaddr_in);
        break;
      case AF_INET6:
        ((FAR struct sockaddr_in6 *)&addr)->sin6_family = AF_INET6;
        ((FAR struct sockaddr_in6 *)&addr)->sin6_port = REXECD_PORT;
        ret = sizeof(struct sockaddr_in6);
        break;
      case AF_RPMSG:
        ((FAR struct sockaddr_rpmsg *)&addr)->rp_family = AF_RPMSG;
        snprintf(((FAR struct sockaddr_rpmsg *)&addr)->rp_name,
                 RPMSG_SOCKET_NAME_SIZE, "%d", REXECD_PORT);
        ret = sizeof(struct sockaddr_rpmsg);
    }

  ret = bind(serv, (FAR struct sockaddr *)&addr, ret);
  if (ret < 0)
    {
      goto err_out;
    }

  ret = listen(serv, 5);
  if (ret < 0)
    {
      goto err_out;
    }

  ret = pthread_attr_init(&attr);
  if (ret != 0)
    {
      goto err_out;
    }

  ret = pthread_attr_setstacksize(&attr, CONFIG_NETUTILS_REXECD_STACKSIZE);
  if (ret != 0)
    {
      goto attr_out;
    }

  ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (ret != 0)
    {
      goto attr_out;
    }

  while (1)
    {
      sock = accept(serv, NULL, 0);
      if (sock < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }
          else
            {
              ret = sock;
              goto attr_out;
            }
        }

      ret = pthread_create(&tid, &attr, doit, (pthread_addr_t)sock);
      if (ret < 0)
        {
          close(sock);
        }
    }

attr_out:
  pthread_attr_destroy(&attr);
err_out:
  close(serv);
  return ret;
}
