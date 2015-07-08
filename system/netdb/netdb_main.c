/****************************************************************************
 * apps/system/netdb/netdb.c
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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
#include <nuttx/compiler.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef CONFIG_SYSTEM_NETDB

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_SYSTEM_NETDB_STACKSIZE
#  define CONFIG_SYSTEM_NETDB_STACKSIZE 2048
#endif

#ifndef CONFIG_SYSTEM_NETDB_PRIORITY
#  define CONFIG_SYSTEM_NETDB_PRIORITY 50
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode) noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "USAGE: %s --ipv4 <ipv4-addr>\n", progname);
  fprintf(stderr, "       %s --ipv6 <ipv6-addr>\n", progname);
  fprintf(stderr, "       %s --host <host-name>\n", progname);
  fprintf(stderr, "       %s --help\n", progname);
  exit(exitcode);
}

 /****************************************************************************
 * Public Functions
 ****************************************************************************/

int netdb_main(int argc, char **argv)
{
  FAR struct hostent *host;
  char buffer[48];
  int ret;

  if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
      show_usage(argv[0], EXIT_SUCCESS);
    }
  else if (argc < 3)
    {
      fprintf(stderr, "ERROR -- Missing arguments\n\n");
      show_usage(argv[0], EXIT_FAILURE);
    }
  else if (argc > 3)
    {
      fprintf(stderr, "ERROR -- Too many arguments\n\n");
      show_usage(argv[0], EXIT_FAILURE);
    }
  else if (strcmp(argv[1], "--ipv4") == 0)
    {
      struct in_addr addr;

      ret = inet_pton(AF_INET, argv[2], &addr);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR -- Bad IPv4 address\n\n");
          show_usage(argv[0], EXIT_FAILURE);
        }

      host = gethostbyaddr(&addr, sizeof(struct in_addr), AF_INET);
      if (!host)
        {
          fprintf(stderr, "ERROR -- gethostbyaddr failed.  h_errno=%d\n\n",
                  h_errno);
          show_usage(argv[0], EXIT_FAILURE);
        }

      printf("Addr: %s  Host: %s\n", argv[2], host->h_name);
      return EXIT_SUCCESS;
    }
  else if (strcmp(argv[1], "--ipv6") == 0)
    {
      struct in_addr addr;

      ret = inet_pton(AF_INET6, argv[2], &addr);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR -- Bad IPv6 address\n\n");
          show_usage(argv[0], EXIT_FAILURE);
        }

      host = gethostbyaddr(&addr, sizeof(struct in6_addr), AF_INET6);
      if (!host)
        {
          fprintf(stderr, "ERROR -- gethostbyaddr failed.  h_errno=%d\n\n",
                  h_errno);
          show_usage(argv[0], EXIT_FAILURE);
        }

      printf("Addr: %s  Host: %s\n", argv[2], host->h_name);
      return EXIT_SUCCESS;
    }
  else if (strcmp(argv[1], "--host") == 0)
    {
      FAR const char *addrtype;

      host = gethostbyname(argv[2]);
      if (!host)
        {
          fprintf(stderr, "ERROR -- gethostbyname failed.  h_errno=%d\n\n",
                  h_errno);
          show_usage(argv[0], EXIT_FAILURE);
        }

      if (host->h_addrtype == AF_INET)
        {
          if (inet_ntop(AF_INET, host->h_addr, buffer,
                        sizeof(struct in_addr)) == NULL)
            {
              fprintf(stderr,
                      "ERROR -- gethostbyname failed.  h_errno=%d\n\n",
                      h_errno);
              show_usage(argv[0], EXIT_FAILURE);
            }

          addrtype = "IPv4";
        }
      else if (host->h_addrtype == AF_INET6)
        {
          if (inet_ntop(AF_INET6, host->h_addr, buffer,
                        sizeof(struct in6_addr)) == NULL)
            {
              fprintf(stderr,
                      "ERROR -- gethostbyname failed.  h_errno=%d\n\n",
                      h_errno);
              show_usage(argv[0], EXIT_FAILURE);
            }

          addrtype = "IPv6";
        }
      else
        {
           fprintf(stderr, "ERROR -- gethostbyname address type&d  not recognized.\n\n",
                   host->h_addrtype);
           show_usage(argv[0], EXIT_FAILURE);
        }

      printf("Host: %s  %s Addr: %s\n", argv[2], addrtype, buffer);
      return EXIT_SUCCESS;
    }
  else
    {
      fprintf(stderr, "ERROR -- Unrecognized option: %s\n\n", argv[1]);
      show_usage(argv[0], EXIT_FAILURE);
    }
}

#endif /* CONFIG_SYSTEM_NETDB */
