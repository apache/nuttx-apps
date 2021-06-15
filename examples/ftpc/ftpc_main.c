/****************************************************************************
 * apps/examples/ftpc/ftpc_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "netutils/ftpc.h"

#include "system/readline.h"

#include "ftpc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FTPC_MAX_ARGUMENTS 4

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cmdmap_s
{
  const char *cmd;        /* Name of the command */
  cmd_t       handler;    /* Function that handles the command */
  uint8_t     minargs;    /* Minimum number of arguments (including command) */
  uint8_t     maxargs;    /* Maximum number of arguments (including command) */
  const char *usage;      /* Usage instructions for 'help' command */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_delim[] = " \t\n";

static int cmd_lhelp(SESSION handle, int argc, char **argv);
static int cmd_lunrecognized(SESSION handle, int argc, char **argv);

static const struct cmdmap_s g_cmdmap[] =
{
  { "cd",       cmd_rchdir,  2, 2, "<directory>" },
  { "chmod",    cmd_rchmod,  3, 3, "<permissions> <path>" },
  { "get",      cmd_rget,    2, 4, "[-a|b] <rname> [<lname>]" },
  { "help",     cmd_lhelp,   1, 2, "" },
  { "idle",     cmd_ridle,   1, 2, "[<idletime>]" },
  { "login",    cmd_rlogin,  2, 3, "<uname> [<password>]" },
  { "ls",       cmd_rls,     1, 2, "[<dirpath>]" },
  { "quit",     cmd_rquit,   1, 1, "" },
  { "mkdir",    cmd_rmkdir,  2, 2, "<directory>" },
  { "noop",     cmd_rnoop,   1, 1, "" },
  { "put",      cmd_rput,    2, 4, "[-a|b] <lname> [<rname>]" },
  { "pwd",      cmd_rpwd,    1, 1, "" },
  { "rename",   cmd_rrename, 3, 3, "<oldname> <newname>" },
  { "rhelp",    cmd_rhelp,   1, 2, "[<command>]" },
  { "rm",       cmd_runlink, 2, 2, "" },
  { "rmdir",    cmd_rrmdir,  2, 2, "<directory>" },
  { "size",     cmd_rsize,   2, 2, "<filepath>" },
  { "time",     cmd_rtime,   2, 2, "<filepath>" },
  { "up",       cmd_rcdup,   1, 1, "" },
  { NULL,       NULL,        1, 1, NULL }
};

static char g_line[CONFIG_FTPC_LINELEN];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_lhelp
 ****************************************************************************/

static int cmd_lhelp(SESSION handle, int argc, char **argv)
{
  const struct cmdmap_s *ptr;

  printf("Local FTPC commands:\n");
  for (ptr = g_cmdmap; ptr->cmd; ptr++)
    {
      if (ptr->usage)
        {
          printf("  %s %s\n", ptr->cmd, ptr->usage);
        }
      else
        {
          printf("  %s\n", ptr->cmd);
        }
    }

  return OK;
}

/****************************************************************************
 * Name: cmd_lunrecognized
 ****************************************************************************/

static int cmd_lunrecognized(SESSION handle, int argc, char **argv)
{
  fprintf(stderr, "Command %s unrecognized\n", argv[0]);
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_argument
 ****************************************************************************/

char *ftpc_argument(char **saveptr)
{
  char *pbegin = *saveptr;
  char *pend   = NULL;
  const char *term;

  /* Find the beginning of the next token */

  for (;
       *pbegin && strchr(g_delim, *pbegin) != NULL;
       pbegin++);

  /* If we are at the end of the string with nothing
   * but delimiters found, then return NULL.
   */

  if (!*pbegin)
    {
      return NULL;
    }

  /* Does the token begin with '#' -- comment */

  else if (*pbegin == '#')
    {
      /* Return NULL meaning that we are at the end of the line */

      *saveptr = pbegin;
      pbegin   = NULL;
    }
  else
    {
      /* Otherwise, we are going to have to parse to find the end of
       * the token.  Does the token begin with '"'?
       */

      if (*pbegin == '"')
        {
          /* Yes.. then only another '"' can terminate the string */

          pbegin++;
          term = "\"";
        }
      else
        {
          /* No, then any of the usual terminators
           * will terminate the argument
           */

          term = g_delim;
        }

      /* Find the end of the string */

      for (pend = pbegin + 1;
           *pend && strchr(term, *pend) == NULL;
           pend++);

      /* pend either points to the end of the string or to
       * the first delimiter after the string.
       */

      if (*pend)
        {
          /* Turn the delimiter into a NUL terminator */

          *pend++ = '\0';
        }

      /* Save the pointer where we left off */

      *saveptr = pend;
    }

  /* Return the beginning of the token. */

  return pbegin;
}

/****************************************************************************
 * Name: ftpc_execute
 ****************************************************************************/

static int ftpc_execute(SESSION handle, int argc, char *argv[])
{
  const struct cmdmap_s *cmdmap;
  const char            *cmd;
  cmd_t                  handler = cmd_lunrecognized;
  int                    ret;

  /* The form of argv is:
   *
   * argv[0]:      The command name.  This is argv[0] when the arguments
   *               are, finally, received by the command handler
   * argv[1]:      The beginning of argument (up to FTPC_MAX_ARGUMENTS)
   * argv[argc]:   NULL terminating pointer
   */

  cmd = argv[0];

  /* See if the command is one that we understand */

  for (cmdmap = g_cmdmap; cmdmap->cmd; cmdmap++)
    {
      if (strcmp(cmdmap->cmd, cmd) == 0)
        {
          /* Check if a valid number of arguments was provided.  We
           * do this simple, imperfect checking here so that it does
           * not have to be performed in each command.
           */

          if (argc < cmdmap->minargs)
            {
              /* Fewer than the minimum number were provided */

              fprintf(stderr, "Too few arguments for '%s'\n", cmd);
              return ERROR;
            }
          else if (argc > cmdmap->maxargs)
            {
              /* More than the maximum number were provided */

              fprintf(stderr, "Too many arguments for '%s'\n", cmd);
              return ERROR;
            }
          else
            {
              /* A valid number of arguments were provided (this does
               * not mean they are right).
               */

              handler = cmdmap->handler;
              break;
            }
        }
    }

  ret = handler(handle, argc, argv);
  if (ret < 0)
    {
      fprintf(stderr, "%s failed: %d\n", cmd, errno);
    }

  return ret;
}

/****************************************************************************
 * Name: ftpc_parse
 ****************************************************************************/

int ftpc_parse(SESSION handle, char *cmdline)
{
  FAR char *argv[FTPC_MAX_ARGUMENTS + 1];
  FAR char *saveptr;
  FAR char *cmd;
  int       argc;
  int       ret;

  /* Initialize parser state */

  memset(argv, 0, FTPC_MAX_ARGUMENTS * sizeof(FAR char *));

  /* Parse out the command at the beginning of the line */

  saveptr = cmdline;
  cmd = ftpc_argument(&saveptr);

  /* Check if any command was provided -OR- if command processing is
   * currently disabled.
   */

  if (!cmd)
    {
      /* An empty line is not an error */

      return OK;
    }

  /* Parse all of the arguments following the command name. */

  argv[0] = cmd;
  for (argc = 1; argc < FTPC_MAX_ARGUMENTS; argc++)
    {
      argv[argc] = ftpc_argument(&saveptr);
      if (!argv[argc])
        {
          break;
        }
    }

  argv[argc] = NULL;

  /* Check if the maximum number of arguments was exceeded */

  if (argc > FTPC_MAX_ARGUMENTS)
    {
      fprintf(stderr, "Too many arguments\n");
      ret = -EINVAL;
    }
  else
    {
      /* Then execute the command */

      ret = ftpc_execute(handle, argc, argv);
    }

  return ret;
}

static void usage(void)
{
  fprintf(stderr,
      "Usage: ftpc [-46n] host [port]\n\
      \t-4    Use IPv4\n\
      \t-6    Use IPv6\n\
      \t-n    Allow numeric IP address only\n\
      ");
  exit(1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  union ftpc_sockaddr_u server;
  SESSION handle;
  char *host = NULL;
  char *port = NULL;
#ifdef CONFIG_LIBC_NETDB
  struct addrinfo hints;
  FAR struct addrinfo *info;
  FAR struct addrinfo *next;
#endif
  int option;
  int ret;
  int family = AF_UNSPEC;
  bool nflag = false;
  bool badarg = false;

  memset(&server, 0, sizeof(union ftpc_sockaddr_u));

  while ((option = getopt(argc, argv, "46n")) != ERROR)
    {
      switch (option)
        {
        case '4':
          family = AF_INET;
          break;
        case '6':
          family = AF_INET6;
          break;
        case 'n':
          nflag = true;
          break;
        default:
          badarg = true;
          break;
        }
    }

  if (badarg)
    {
      usage();
    }

  /* There should be one or two parameters remaining on the command line */

  if (optind >= argc)
    {
      fprintf(stderr, "%s: Missing required arguments\n", argv[0]);
      usage();
    }

  host = argv[optind];
  optind++;

  if (optind < argc)
    {
      port = argv[optind];
      optind++;
    }

  if (optind != argc)
    {
      fprintf(stderr, "%s: Too many arguments\n", argv[0]);
      usage();
    }

#ifdef CONFIG_LIBC_NETDB
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  if (nflag)
    {
      hints.ai_flags |= AI_NUMERICHOST;
    }

  /* We now get all addresses for hostname or IP address from the
   * command line.
   */

  ret = getaddrinfo(host, port, &hints, &info);
  if (ret != OK)
    {
      fprintf(stderr, "ERROR: getaddrinfo: %s\n", gai_strerror(ret));
      exit(1);
    }

  for (next = info; next != NULL; next = next->ai_next)
    {
#ifdef CONFIG_NET_IPv6
      if (next->ai_family == AF_INET6)
        {
          memcpy(&server.in6, next->ai_addr, next->ai_addrlen);
        }
#endif

#ifdef CONFIG_NET_IPv4
      if (next->ai_family == AF_INET)
        {
          memcpy(&server.in4, next->ai_addr, next->ai_addrlen);
        }
#endif

      /* Connect to the FTP server */

      handle = ftpc_connect(&server);
      if (handle)
        {
          break;
        }
    }

  freeaddrinfo(info);
  if (!handle)
    {
      fprintf(stderr, "ERROR: Failed to connect to the server: %d\n", errno);
      exit(1);
    }
#else
  /* No getaddrinfo(), use IP address only, implies nflag. */

  UNUSED(nflag);
  DEBUGASSERT(host != NULL);

  /* Try IPv6 first, then IPv4. */

#ifdef CONFIG_NET_IPv6
  if (family != AF_INET)
    {
      ret = inet_pton(AF_INET6, host, &server.in6.sin6_addr);
      if (ret == 1)
        {
          server.in6.sin6_family = AF_INET6;
          if (port != NULL)
            {
              server.in6.sin6_port = htons(atoi(port));
            }

          goto do_connect;
        }
    }
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
  if (family != AF_INET6)
    {
      ret = inet_pton(AF_INET, host, &server.in4.sin_addr);
      if (ret == 1)
        {
          server.in4.sin_family = AF_INET;
          if (port != NULL)
            {
              server.in4.sin_port = htons(atoi(port));
            }

          goto do_connect;
        }
    }
#endif /* CONFIG_NET_IPv4 */

  /* Did not get a valid address. */

  fprintf(stderr, "ERROR: Invalid IP address\n");
  exit(1);

  /* Connect to the FTP server. */

do_connect:
  handle = ftpc_connect(&server);
  if (!handle)
    {
      fprintf(stderr, "ERROR: Failed to connect to the server: %d\n", errno);
      exit(1);
    }
#endif /* CONFIG_LIBC_NETDB */

  /* Present a greeting */

  printf("NuttX FTP Client:\n");
  FFLUSH();

  /* Then enter the command line parsing loop */

  for (; ; )
    {
      /* Display the prompt string */

      fputs("nfc> ", stdout);
      FFLUSH();

      /* Get the next line of input */

#ifdef CONFIG_EXAMPLES_FTPC_FGETS
      /* fgets returns NULL on end-of-file or any I/O error */

      if (fgets(g_line, CONFIG_FTPC_LINELEN, stdin) == NULL)
        {
          fprintf(stderr, "ERROR: fgets failed: %d\n", errno);
          return 1;
        }
#else
      ret = readline(g_line, CONFIG_FTPC_LINELEN, stdin, stdout);

      /* Readline normally returns the number of characters read,
       * but will return EOF on end of file or if an error occurs.
       * Either will cause the session to terminate.
       */

      if (ret == EOF)
        {
          fprintf(stderr, "ERROR: readline failed: %d\n", errno);
          return 1;
        }
#endif
      else
        {
          /* Parse and process the command */

          ftpc_parse(handle, g_line);
          FFLUSH();
        }
    }

  return 0;
}
