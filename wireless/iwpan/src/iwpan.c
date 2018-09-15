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
 * Name: iwpan_str2long
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

static long iwpan_str2long(FAR const char *str)
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
      return LONG_MAX;
      exit(EXIT_FAILURE);
    }

  return value;
}

/****************************************************************************
 * Name: iwpan_str2luint8
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

static uint8_t iwpan_str2luint8(FAR const char *str)
{
  long value = iwpan_str2long(str);
  if (value < 0 || value > UINT8_MAX)
    {
      fprintf(stderr, "ERROR: 8-bit value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (uint8_t)value;
}

/****************************************************************************
 * Name: iwpan_str2luint16
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

static uint16_t iwpan_str2luint16(FAR const char *str)
{
  long value = iwpan_str2long(str);
  if (value < 0 || value > UINT16_MAX)
    {
      fprintf(stderr, "ERROR: 16-bit value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (uint8_t)value;
}

/****************************************************************************
 * Name: iwpan_char2nibble
 *
 * Description:
 *   Convert an hexadecimal character to a 4-bit nibble.
 *
 ****************************************************************************/

static uint8_t iwpan_char2nibble(char ch)
{
  if (ch >= '0' && ch <= '9')
    {
      return ch - '0';
    }
  else if (ch >= 'a' && ch <= 'f')
    {
      return ch - 'a' + 10;
    }
  else if (ch >= 'A' && ch <= 'F')
    {
      return ch - 'A' + 10;
    }
  else if (ch == '\0')
    {
      fprintf(stderr, "ERROR: Unexpected end hex\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      fprintf(stderr, "ERROR: Unexpected character in hex value: %02x\n", ch);
      exit(EXIT_FAILURE);
    }
}

/****************************************************************************
 * Name: iwpan_str2eaddr
 *
 * Description:
 *   Convert a string 8-byte EADAR array.
 *
 ****************************************************************************/

static void iwpan_str2eaddr(FAR const char *str, FAR uint8_t *eaddr)
{
  FAR const char *src = str;
  uint8_t bvalue;
  char ch;
  int i;

  for (i = 0; i < 8; i++)
    {
      ch = (char)*src++;
      bvalue = iwpan_char2nibble(ch) << 4;

      ch = (char)*src++;
      bvalue |= iwpan_char2nibble(ch);

      *eaddr++ = bvalue;

      if (i < 7)
        {
          ch = (char)*src++;
          if (ch != ':')
            {
              fprintf(stderr, "ERROR: Missing colon separator: %s\n", str);
              fprintf(stderr, "       Expected xx:xx:xx:xx:xx:xx:xx:xx\n");
              exit(EXIT_FAILURE);
            }
        }
    }
}

/****************************************************************************
 * Name: iwpan_str2bool
 *
 * Description:
 *   Convert a boolean name to a boolean value.
 *
 ****************************************************************************/

static bool iwpan_str2bool(FAR const char *str)
{
  if (strcasecmp(str, "true") == 0)
    {
      return true;
    }
  else if (strcasecmp(str, "false") == 0)
    {
      return false;
    }
  else
    {
      fprintf(stderr, "ERROR: Invalid boolean name: %s\n", str);
      fprintf(stderr, "       Expected true or false\n");
      exit(EXIT_FAILURE);
    }
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
  uint8_t eaddr[IEEE802154_EADDRSIZE] =
  {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };

  union
  {
    struct ieee802154_cca_s cca;
    uint8_t b;
  } u;

  int32_t txpwr = INT32_MAX;
  uint16_t saddr = UINT16_MAX;
  uint16_t panid = UINT16_MAX;
  uint8_t chan = UINT8_MAX;
  uint8_t devmode = UINT8_MAX;
  bool promisc = false;
  bool energy = false;
  int ret;

  u.b = 0xff;

  /* Read all parameters from the radio */

  ret = sixlowpan_getchan(sock, ifname, &chan);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_getchan() failed: %d\n", ret);
    }

  ret = sixlowpan_getpanid(sock, ifname, &panid);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_getpanid() failed: %d\n", ret);
    }

  ret = sixlowpan_getsaddr(sock, ifname, &saddr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_getsaddr() failed: %d\n", ret);
    }

  ret = sixlowpan_geteaddr(sock, ifname,  eaddr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_geteaddr() failed: %d\n", ret);
    }

  ret = sixlowpan_getpromisc(sock, ifname, &promisc);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_getpromisc() failed: %d\n", ret);
    }

  ret = sixlowpan_getdevmode(sock, ifname, &devmode);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_getdevmode() failed: %d\n", ret);
    }

  ret = sixlowpan_gettxpwr(sock, ifname, &txpwr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_gettxpwr() failed: %d\n", ret);
    }

  ret = sixlowpan_getcca(sock, ifname, &u.cca);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_getcca() failed: %d\n", ret);
    }

  ret = sixlowpan_energydetect(sock, ifname, &energy);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_energydetect() failed: %d\n", ret);
    }

  /* And generate the output */

  fprintf(stderr, "\nRadio Settings (%s):\n", ifname);
  fprintf(stderr, "    chan: %u\n", chan);
  fprintf(stderr, "   saddr: %04x\n", saddr);
  fprintf(stderr, "   eaddr: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          eaddr[0], eaddr[1], eaddr[2], eaddr[3], eaddr[4], eaddr[5],
          eaddr[6], eaddr[7]);
  fprintf(stderr, "   panid: %04x\n", panid);
  fprintf(stderr, " devmode: %u\n", devmode);
  fprintf(stderr, " promisc: %s\n", promisc ? "true" : "false");
  fprintf(stderr, "     cca: %02x\n", u.b);
  fprintf(stderr, "   txpwr: %ld\n", (long)txpwr);
  fprintf(stderr, " ernergy: %s\n", energy ? "true" : "false");
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
  union
  {
    struct ieee802154_cca_s cca;
    uint8_t b;
  } u;
  int ret;

  /* Convert input strings to values */

  u.b = iwpan_str2luint8(ccastr);

  /* Set the CCA */

  ret = sixlowpan_setcca(sock, ifname, &u.cca);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_setcca() failed: %d\n", ret);
    }
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
  uint8_t chan;
  int ret;

  /* Convert input strings to values */

  chan = iwpan_str2luint8(chanstr);

  /* Set the channel */

  ret = sixlowpan_setchan(sock, ifname, chan);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_setchan() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: iwpan_devmode_cmd
 *
 * Description:
 *   Set radio device mode
 *
 ****************************************************************************/

static void iwpan_devmode_cmd(int sock, FAR const char *ifname,
                              FAR const char *modestr)
{
  uint8_t devmode;
  int ret;

  /* Convert input strings to values */

  devmode = iwpan_str2luint8(modestr);

  /* Set the devmode */

  ret = sixlowpan_setdevmode(sock, ifname, devmode);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:int sixlowpan_setdevmode() failed: %d\n", ret);
    }
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
  uint8_t eaddr[IEEE802154_EADDRSIZE];
  int ret;

  /* Convert input strings to values */

  iwpan_str2eaddr(addrstr, eaddr);

  /* Set the extended address */

  ret = sixlowpan_seteaddr(sock, ifname, eaddr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_seteaddr() failed: %d\n", ret);
    }
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
  uint16_t panid;
  int ret;

  /* Convert input strings to values */

  panid = iwpan_str2luint16(panstr);

  /* Set the PAN ID */

  ret = sixlowpan_setpanid(sock, ifname, panid);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_setpanid() failed: %d\n", ret);
    }
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
  bool promisc;
  int ret;

  /* Convert input strings to values */

  promisc = iwpan_str2bool(boolstr);

  /* Set the promisc */

  ret = sixlowpan_setpromisc(sock, ifname, promisc);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_setpromisc() failed: %d\n", ret);
    }
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
   uint16_t saddr;
  int ret;

  /* Convert input strings to values */

  saddr = iwpan_str2luint16(addrstr);

  /* Set the short address */

  ret = sixlowpan_setsaddr(sock, ifname, saddr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_setsaddr() failed: %d\n", ret);
    }
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
  long txpwr;
  int ret;

  /* Convert input strings to values */

  txpwr = iwpan_str2long(pwrstr);

  /* Set the TX power */

  ret = sixlowpan_settxpwr(sock, ifname, (int32_t)txpwr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sixlowpan_settxpwr() failed: %d\n", ret);
    }
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
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "\t<eaddr> must be entered as xx:xx:xx:xx:xx:xx:xx\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef BUILD_MODULE
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
