/****************************************************************************
 * apps/netutils/rexec/rexec.c
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
  int sock;
  int ret;

  sock = rexec_af(&arg->host, arg->port, arg->user,
                  arg->password, arg->command,
                  NULL, arg->af);
  if (sock < 0)
    {
      return sock;
    }

  while (1)
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

  free(arg->host);
  close(sock);
  return ret;
}

int main(int argc, FAR char **argv)
{
  char cmd[CONFIG_NSH_LINELEN];
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
      strcat(cmd, argv[i]);
      strcat(cmd, " ");
    }

  arg.command = cmd;
  return do_rexec(&arg);
}
