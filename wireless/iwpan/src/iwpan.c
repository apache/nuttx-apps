/****************************************************************************
 * apps/wireless/iwpan/src/iwpan.c
 *
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Anthony Merlino <anthony@vergeaero.com>
 *           Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of  source code must  retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "wireless/ieee802154.h"
#include "wireless/iwpan.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The address family that we used to create the socket really does not
 * matter.  It should, however, be valid in the current configuration.
 */

#if defined(CONFIG_NET_IPv4)
#  define PF_INETX PF_INET
#elif defined(CONFIG_NET_IPv6)
#  define PF_INETX PF_INET6
#endif

/* SOCK_DGRAM is the preferred socket type to use when we just want a
 * socket for performing driver ioctls.  However, we can't use SOCK_DRAM
 * if UDP is disabled.
 */

#ifdef CONFIG_NET_UDP
# define SOCK_IWPAN SOCK_DGRAM
#else
# define SOCK_IWPAN SOCK_STREAM
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Describes one command */

struct iwpan_command_s
{
  FAR const char *name;
  uint8_t noptions;
  CODE void *handler;
};

/* Generic form of a command handler */

typedef void (*cmd1_t)(int sock, FAR const char *arg1);
typedef void (*cmd2_t)(int sock, FAR const char *arg1,
                       FAR const char *arg2);
typedef void (*cmd3_t)(int sock, FAR const char *arg1,
                       FAR const char *arg2, FAR const char *arg3);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void iwpan_show_cmd(int sock, FAR const char *ifname);
static void iwpan_cca_cmd(int sock, FAR const char *ifname,
              FAR const char *ccastr);
static void iwpan_chan_cmd(int sock, FAR const char *ifname,
              FAR const char *chanstr);
static void iwpan_devmode_cmd(int sock, FAR const char *ifname,
              FAR const char *modstr);
static void iwpan_eaddr_cmd(int sock, FAR const char *ifname,
              FAR const char *addrstr);
static void iwpan_panid_cmd(int sock, FAR const char *ifname,
              FAR const char *panstr);
static void iwpan_promisc_cmd(int sock, FAR const char *ifname,
              FAR const char *boolstr);
static void iwpan_saddr_cmd(int sock, FAR const char *ifname,
              FAR const char *addrstr);
static void iwpan_txpwr_cmd(int sock, FAR const char *ifname,
              FAR const char *pwrstr);

static void iwpan_showusage(FAR const char *progname, int exitcode);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct iwpan_command_s g_iwpan_commands[] =
{
  {"help",    0, (CODE void *)NULL},
  {"show",    1, (CODE void *)iwpan_show_cmd},
  {"cca",     2, (CODE void *)iwpan_cca_cmd},
  {"chan",    2, (CODE void *)iwpan_chan_cmd},
  {"devmode", 2, (CODE void *)iwpan_devmode_cmd},
  {"eaddr",   2, (CODE void *)iwpan_eaddr_cmd},
  {"panid",   2, (CODE void *)iwpan_panid_cmd},
  {"promisc", 2, (CODE void *)iwpan_promisc_cmd},
  {"saddr",   2, (CODE void *)iwpan_saddr_cmd},
  {"txpwr",   2, (CODE void *)iwpan_txpwr_cmd},
};

#define NCOMMANDS (sizeof(g_iwpan_commands) / sizeof(struct iwpan_command_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iwpan_str2int
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

static int iwpan_str2int(FAR const char *str)
{
  FAR char *endptr;
  long value;

  value = strtol(str, &endptr, 0);
  if (*endptr != '\0')
    {
      fprintf(stderr, "ERROR: Garbage after numeric argument\n");
      exit(EXIT_FAILURE);
    }

  if (value > INT_MAX || value < INT_MIN)
    {
      fprintf(stderr, "ERROR: Integer value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (int)value;
}

/****************************************************************************
 * Name: iwpan_str2double
 *
 * Description:
 *   Convert a string to a double value
 *
 ****************************************************************************/

static double iwpan_str2double(FAR const char *str)
{
  FAR char *endptr;
  double value;

  value = strtod(str, &endptr);
  if (*endptr != '\0')
    {
      fprintf(stderr, "ERROR: Garbage after numeric argument\n");
      exit(EXIT_FAILURE);
    }

  return value;
}

/****************************************************************************
 * Name: iwpan_show_cmd
 *
 * Description:
 *   Show all radio device settings
 *
 ****************************************************************************/

static void iwpan_show_cmd(int sock, FAR const char *ifname)
{

}

/****************************************************************************
 * Name: iwpan_cca_cmd
 *
 * Description:
 *   Set radio device Clear Channel Assessment (CCA) thresholds.
 *
 ****************************************************************************/

static void iwpan_cca_cmd(int sock, FAR const char *ifname,
                          FAR const char *ccastr)
{

}

/****************************************************************************
 * Name: iwpan_chan_cmd
 *
 * Description:
 *   Set radio device channel
 *
 ****************************************************************************/

static void iwpan_chan_cmd(int sock, FAR const char *ifname,
                           FAR const char *chanstr)
{

}

/****************************************************************************
 * Name: iwpan_devmode_cmd
 *
 * Description:
 *   Set radio device mode
 *
 ****************************************************************************/

static void iwpan_devmode_cmd(int sock, FAR const char *ifname,
                              FAR const char *modstr)
{

}

/****************************************************************************
 * Name: iwpan_eaddr_cmd
 *
 * Description:
 *   Set radio device extended address
 *
 *
 ****************************************************************************/

static void iwpan_eaddr_cmd(int sock, FAR const char *ifname,
                            FAR const char *addrstr)
{

}

/****************************************************************************
 * Name: iwpan_panid_cmd
 *
 * Description:
 *   Set radio device PAN ID.
 *
 *
 ****************************************************************************/

static void iwpan_panid_cmd(int sock, FAR const char *ifname,
                            FAR const char *panstr)
{

}

/****************************************************************************
 * Name: iwpan_promisc_cmd
 *
 * Description:
 *   Set radio device PAN ID.
 *
 *
 ****************************************************************************/

static void iwpan_promisc_cmd(int sock, FAR const char *ifname,
                              FAR const char *boolstr)
{

}

/****************************************************************************
 * Name: iwpan_saddr_cmd
 *
 * Description:
 *   Set radio device short address
 *
 ****************************************************************************/

static void iwpan_saddr_cmd(int sock, FAR const char *ifname,
                            FAR const char *addrstr)
{

}

/****************************************************************************
 * Name: iwpan_txpwr_cmd
 *
 * Description:
 *   Set radio device TX power
 *
 ****************************************************************************/

static void iwpan_txpwr_cmd(int sock, FAR const char *ifname,
                            FAR const char *pwrstr)
{

}

/****************************************************************************
 * Name: iwpan_showusage
 *
 * Description:
 *   Show program usage.
 *
 ****************************************************************************/

static void iwpan_showusage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage:\n", progname);
  fprintf(stderr, "\t%s <command> <ifname> [OPTIONS]\n", progname);
  fprintf(stderr, "\nWhere supported commands an [OPTIONS] appear below\n");
  fprintf(stderr, "\nRadio Query:\n");
  fprintf(stderr, "\t%s show <ifname>\n", progname);
  fprintf(stderr, "\nRadio Settings:\n");
  fprintf(stderr, "\t%s cca <ifname> <hex cca>\n", progname);
  fprintf(stderr, "\t%s chan <ifname> <channel>\n", progname);
  fprintf(stderr, "\t%s devmode <ifname> <devmode>\n", progname);
  fprintf(stderr, "\t%s eaddr <ifname> <eaddr>\n", progname);
  fprintf(stderr, "\t%s panid <ifname> <panid>\n", progname);
  fprintf(stderr, "\t%s promisc <ifname> <true|false>\n", progname);
  fprintf(stderr, "\t%s saddr <ifname> <saddr>\n", progname);
  fprintf(stderr, "\t%s txpwr <ifname> <txpwr>\n", progname);
  fprintf(stderr, "\nMAC Commands:\n");
  fprintf(stderr, "\t%s assoc <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s disassoc <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s get <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s gts <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s poll <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s rxenab <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s scan <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s set <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s start <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s sync <ifname> TBD\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int iwpan_main(int argc, char *argv[])
#endif
{
  FAR const char *cmdname;
  FAR const struct iwpan_command_s *iwpancmd;
  int sock;
  int i;

  /* Get the command */

  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing command\n");
      iwpan_showusage(argv[0], EXIT_FAILURE);
    }

  cmdname = argv[1];

  /* Find the command in the g_iwpan_command[] list */

  iwpancmd = NULL;
  for (i = 0; i < NCOMMANDS; i++)
    {
      FAR const struct iwpan_command_s *cmd = &g_iwpan_commands[i];
      if (strcmp(cmdname, cmd->name) == 0)
        {
          iwpancmd = cmd;
          break;
        }
    }

  if (iwpancmd == NULL)
    {
      fprintf(stderr, "ERROR: Unsupported command: %s\n", cmdname);
      iwpan_showusage(argv[0], EXIT_FAILURE);
    }

  if (iwpancmd->noptions + 2 < argc)
    {
      fprintf(stderr, "ERROR: Garbage at end of command ignored\n");
    }
  else if (iwpancmd->noptions + 2 > argc)
    {
      fprintf(stderr, "ERROR: Missing required command options: %s\n",
                 cmdname);
      iwpan_showusage(argv[0], EXIT_FAILURE);
    }

  /* Special case the help command which has no arguments, no handler,
   * and does not need a socket.
   */

  if (iwpancmd->handler == NULL)
    {
      iwpan_showusage(argv[0], EXIT_SUCCESS);
    }

  /* Create a communication socket */

  sock = socket(PF_INETX, SOCK_IWPAN, 0);
  if (sock < 0)
    {
      fprintf(stderr, "ERROR: iwpan_make_socket() failed: %d\n", sock);
      return EXIT_FAILURE;
    }

  /* Dispatch the command handling */

  switch (iwpancmd->noptions)
    {
      default:
      case 0:
        fprintf(stderr, "ERROR: Internal craziness\n");
        iwpan_showusage(argv[0], EXIT_FAILURE);

      case 1:
        ((cmd1_t)iwpancmd->handler)(sock, argv[2]);
        break;

      case 2:
        ((cmd2_t)iwpancmd->handler)(sock, argv[2], argv[3]);
        break;

      case 3:
        ((cmd3_t)iwpancmd->handler)(sock, argv[2], argv[3], argv[4]);
        break;
    }

  /* Close communication socket */

  close(sock);
  return EXIT_SUCCESS;
}
