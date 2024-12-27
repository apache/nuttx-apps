/****************************************************************************
 * apps/netutils/rexec/rexec.c
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

#include <sys/poll.h>
#include <sys/types.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define REXEC_BUFSIZE     512
#define REXEC_SERVER_PORT 512

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct rexec_arg_s
{
  FAR const char *password;
  FAR const char *command;
  FAR const char *user;
  FAR char *host;
  sa_family_t af;
  int port;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int do_rexec(FAR struct rexec_arg_s *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(FAR const char *progname)
{
  fprintf(stderr, "Usage: %s [-u user] [-H host] [-p password] "
                  "[-P port] [-4|-6|-r] command\n", progname);
  fprintf(stderr, "Remote Execution Client:\n"
                  "  -u, Specify the user, default is \\0\n"
                  "  -H, Specify the hostname\n"
                  "  -p, Specify the password, default is \\0\n"
                  "  -P, Specify the port to connect to, default is 512\n"
                  "  -4, Specify address family AF_INET(default)\n"
                  "  -6, Specify address family AF_INET6\n"
                  "  -r, Specify address family AF_RPMSG\n");
  exit(EXIT_FAILURE);
}

static int do_rexec(FAR struct rexec_arg_s *arg)
{
  char buffer[REXEC_BUFSIZE];
  struct pollfd fds[2];
  int sock;
  int ret;

  sock = rexec_af(&arg->host, htons(arg->port), arg->user,
                  arg->password, arg->command,
                  NULL, arg->af);
  if (sock < 0)
    {
      return sock;
    }

  memset(fds, 0, sizeof(fds));
  fds[0].fd = sock;
  fds[0].events = POLLIN;
  fds[1].fd = STDIN_FILENO;
  fds[1].events = POLLIN;

  while (1)
    {
      ret = poll(fds, 2, -1);
      if (ret <= 0)
        {
          continue;
        }

      if (fds[0].revents & POLLIN)
        {
          ret = read(sock, buffer, REXEC_BUFSIZE);
          if (ret <= 0)
            {
              break;
            }

          ret = write(STDOUT_FILENO, buffer, ret);
          if (ret < 0)
            {
              break;
            }
        }

      if (fds[1].revents & POLLIN)
        {
          ret = read(STDIN_FILENO, buffer, REXEC_BUFSIZE);
          if (ret <= 0)
            {
              break;
            }

          ret = write(sock, buffer, ret);
          if (ret < 0)
            {
              break;
            }
        }

      if (((fds[0].revents | fds[1].revents) & POLLHUP) &&
          ((fds[0].revents | fds[1].revents) & POLLIN) == 0)
        {
          break;
        }
    }

  free(arg->host);
  close(sock);
  return ret;
}

int main(int argc, FAR char **argv)
{
  char cmd[LINE_MAX];
  struct rexec_arg_s arg;
  int option;
  int i;

  memset(&arg, 0, sizeof(arg));

  /* The default port of rexec to connect rexecd */

  arg.port = REXEC_SERVER_PORT;
  arg.af = AF_INET;

  while ((option = getopt(argc, argv, "u:H:p:P:46r")) != ERROR)
    {
      switch (option)
        {
          case 'u':
            arg.user = optarg;
            break;
          case 'H':
            arg.host = optarg;
            break;
          case 'p':
            arg.password = optarg;
            break;
          case 'P':
            arg.port = atoi(optarg);
            break;
          case '4':
            arg.af = AF_INET;
            break;
          case '6':
            arg.af = AF_INET6;
            break;
          case 'r':
            arg.af = AF_RPMSG;
            break;
          default:
            usage(argv[0]);
        }
    }

  if (optind == argc || !arg.host)
    {
      usage(argv[0]);
    }

  cmd[0] = '\0';
  for (i = optind; i < argc; i++)
    {
      strlcat(cmd, argv[i], sizeof(cmd));
      strlcat(cmd, " ", sizeof(cmd));
    }

  arg.command = cmd;
  return do_rexec(&arg);
}
