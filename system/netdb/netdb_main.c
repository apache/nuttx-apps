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

/* REVIST: Currently the availability of gethostbyaddr() depends on
 * CONFIG_NETDB_HOSTFILE.  That might not always be true, however.
 */

#undef HAVE_GETHOSTBYADDR
#ifdef CONFIG_NETDB_HOSTFILE
#  define HAVE_GETHOSTBYADDR 1
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname,
                       int exitcode) noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
#ifdef HAVE_GETHOSTBYADDR
  fprintf(stderr, "USAGE: %s --ipv4 <ipv4-addr>\n", progname);
  fprintf(stderr, "       %s --ipv6 <ipv6-addr>\n", progname);
#endif
  fprintf(stderr, "       %s --host <host-name>\n", progname);
  fprintf(stderr, "       %s --help\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct hostent *host;
  FAR const char *addrtype;
  char buffer[48];
#ifdef HAVE_GETHOSTBYADDR
  struct in_addr addr;
  int ret;
#endif

  /* Handle: netdb --help */

  if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
      show_usage(argv[0], EXIT_SUCCESS);
    }

  /* Otherwise there must be exactly two arguments following the program
   * name
   */

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

#ifdef HAVE_GETHOSTBYADDR
  /* Handle: netdb --ipv4 <ipv4-addr>  */

  else if (strcmp(argv[1], "--ipv4") == 0)
    {
      /* Convert the address to binary */

      ret = inet_pton(AF_INET, argv[2], &addr);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR -- Bad IPv4 address\n\n");
          show_usage(argv[0], EXIT_FAILURE);
        }

      /* Get the matching host name + any aliases */

      host = gethostbyaddr(&addr, sizeof(struct in_addr), AF_INET);
      if (!host)
        {
          fprintf(stderr, "ERROR -- gethostbyaddr failed.  h_errno=%d\n\n",
                  h_errno);
          return EXIT_FAILURE;
        }
    }

  /* Handle: netdb --ipv6 <ipv6-addr>  */

  else if (strcmp(argv[1], "--ipv6") == 0)
    {
      /* Convert the address to binary */

      ret = inet_pton(AF_INET6, argv[2], &addr);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR -- Bad IPv6 address\n\n");
          show_usage(argv[0], EXIT_FAILURE);
        }

      /* Get the matching host name + any aliases */

      host = gethostbyaddr(&addr, sizeof(struct in6_addr), AF_INET6);
      if (!host)
        {
          fprintf(stderr, "ERROR -- gethostbyaddr failed.  h_errno=%d\n\n",
                  h_errno);
          return EXIT_FAILURE;
        }
    }
#endif /* HAVE_GETHOSTBYADDR */

  /* Handle: netdb --host <host-name>  */

  else if (strcmp(argv[1], "--host") == 0)
    {
      /* Get the matching address + any aliases */

      host = gethostbyname(argv[2]);
      if (!host)
        {
          fprintf(stderr, "ERROR -- gethostbyname failed.  h_errno=%d\n\n",
                  h_errno);
          return EXIT_FAILURE;
        }
    }

  /* The first argument is not any argument that we recognize */

  else
    {
      fprintf(stderr, "ERROR -- Unrecognized option: %s\n\n", argv[1]);
      show_usage(argv[0], EXIT_FAILURE);
    }

  /* If we get here, then gethostbyname() or gethostbyaddr() was
   * successfully.
   */

  /* Convert the address to a string */

  /* Handle IPv4 addresses */

  if (host->h_addrtype == AF_INET)
    {
      if (inet_ntop(AF_INET, host->h_addr, buffer, 48) == NULL)
        {
          fprintf(stderr,
                  "ERROR -- gethostbyname failed.  h_errno=%d\n\n",
                  h_errno);
          return EXIT_FAILURE;
        }

      addrtype = "IPv4";
    }

  /* Handle IPv6 addresses */

  else if (host->h_addrtype == AF_INET6)
    {
      if (inet_ntop(AF_INET6, host->h_addr, buffer, 48) == NULL)
        {
          fprintf(stderr,
                  "ERROR -- gethostbyname failed.  h_errno=%d\n\n",
                  h_errno);
          return EXIT_FAILURE;
        }

      addrtype = "IPv6";
    }

  /* Huh? */

  else
    {
      fprintf(stderr,
              "ERROR -- gethostbyname address type %d not recognized.\n\n",
              host->h_addrtype);
      return EXIT_FAILURE;
    }

  /* Print the host name / address mapping */

  printf("Host: %s  %s Addr: %s\n", host->h_name, addrtype, buffer);

  /* Print any host name aliases */

  if (host->h_aliases != NULL && *host->h_aliases != NULL)
    {
      FAR char **alias;

      printf("Aliases:");
      for (alias = host->h_aliases; *alias != NULL; alias++)
        {
          printf(" %s", *alias);
        }

      putchar('\n');
    }

  return EXIT_SUCCESS;
}

#endif /* CONFIG_SYSTEM_NETDB */
