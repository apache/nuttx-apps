/****************************************************************************
 * apps/system/gdbstub/gdbstub.c
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

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <nuttx/gdbstub.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int gdb_monitor(FAR struct gdb_state_s *state, FAR const char *cmd)
{
#ifdef CONFIG_SYSTEM_POPEN
  FAR FILE *file;

  file = popen(cmd, "r");
  if (file != NULL)
    {
      char buf[128];

      while (1)
        {
          size_t len = fread(buf, 1, sizeof(buf) - 1, file);
          if (len == 0 && (feof(file) || ferror(file)))
            {
              break;
            }

          buf[len] = '\0';
          gdb_console_message(state, buf);
        }

      pclose(file);
      return 0;
    }
  else
    {
      return -errno;
    }
#else
  return -EPROTONOSUPPORT;
#endif
}

static ssize_t gdb_send(FAR void *priv, FAR const char *buf, size_t len)
{
  int fd = *(FAR int *)priv;
  size_t i = 0;

  while (i < len)
    {
      ssize_t ret = write(fd, buf + i, len - i);
      if (ret < 0)
        {
          return -errno;
        }

      i += ret;
    }

  return len;
}

static ssize_t gdb_recv(FAR void *priv, FAR void *buf,
                        size_t len)
{
  int fd = *(FAR int *)priv;
  size_t i = 0;

  while (i < len)
    {
      ssize_t ret = read(fd, (FAR char *)buf + i, len - i);
      if (ret < 0)
        {
          return -errno;
        }

      i += ret;
    }

  return len;
}

static void usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s [tty Options | Net Options] \n", progname);
  fprintf(stderr, "tty Options:\n");
  fprintf(stderr, "   -d [tty device path]  etc:/dev/ttyS0\n");
  fprintf(stderr, "Net Options:\n");
  fprintf(stderr, "   -p [Port]  etc:1234\n");
  fprintf(stderr, "   -h: show help message and exit\n");
  fprintf(stderr, "Choose one of the two modes of tty and net\n");

  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_NET
  FAR char *port = NULL;
  int sock = 0;
#endif
  FAR struct gdb_state_s *state;
  FAR char *dev = NULL;
  int ret;
  int fd;

#ifdef CONFIG_NET
  while ((ret = getopt_long(argc, argv, "d:p:h", NULL, NULL)) != ERROR)
#else
  while ((ret = getopt_long(argc, argv, "d:h", NULL, NULL)) != ERROR)
#endif
    {
      switch (ret)
      {
        case 'd':
          dev = optarg;
          break;
#ifdef CONFIG_NET
        case 'p':
          port = optarg;
          break;
#endif
        case 'h':
        case '?':
        default:
          usage(argv[0]);
          break;
      }
    }

#ifdef CONFIG_NET
  if (port)
    {
      struct sockaddr_in addr;

      if (((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0))
        {
          fprintf(stderr, "ERROR: Failed to open socket: %d\n", errno);
          return -errno;
        }

      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = htons(atoi(port));
      if ((bind(sock, (FAR struct sockaddr *)&addr, sizeof(addr)) < 0))
        {
          fprintf(stderr, "ERROR: Failed to bind socket: %d\n", errno);
          return -errno;
        }

      if (listen(sock, 1) < 0)
        {
          fprintf(stderr, "ERROR: Failed to listen socket: %d\n", errno);
          return -errno;
        }

reconnect:
      if (((fd = accept4(sock, NULL, NULL, SOCK_CLOEXEC)) < 0))
        {
          fprintf(stderr, "ERROR: Failed to accept socket: %d\n", errno);
          return -errno;
        }
    }
  else
#endif
  if (dev)
    {
      fd = open(dev, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: Failed to open %s: %d\n", dev, errno);
          return -errno;
        }
    }
  else
    {
      usage(argv[0]);
    }

  state = gdb_state_init(gdb_send, gdb_recv, gdb_monitor, &fd);
  if (state == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

  do
    {
      ret = gdb_process(state, 0, NULL);
      if (ret == -ECONNRESET)
        {
#ifdef CONFIG_NET
          if (port)
            {
              close(fd);
              goto reconnect;
            }

#endif
          continue;
        }
    }
  while (ret >= 0);

  gdb_state_uninit(state);

errout:
  close(fd);
#ifdef CONFIG_NET
  if (port && sock)
    {
      close(sock);
    }
#endif

  return ret;
}
