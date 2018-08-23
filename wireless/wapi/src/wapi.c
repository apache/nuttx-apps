/****************************************************************************
 * apps/wireless/wapi/src/wapi.c
 *
 *   Copyright (C) 2011, 2017Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Largely and original work, but highly influenced by sampled code provided
 * with WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "wireless/wapi.h"
#include "util.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Describes one command */

struct wapi_command_s
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

static int wapi_str2int(FAR const char *str);
static double wapi_str2double(FAR const char *str);
static unsigned int wapi_str2ndx(FAR const char *name, FAR const char **list,
                                 unsigned int listlen);

static void wapi_show_cmd(int sock, FAR const char *ifname);
static void wapi_ip_cmd(int sock, FAR const char *ifname,
                        FAR const char *addrstr);
static void wapi_mask_cmd(int sock, FAR const char *ifname,
                          FAR const char *maskstr);
static void wapi_freq_cmd(int sock, FAR const char *ifname,
                          FAR const char *freqstr, FAR const char *flagstr);
static void wapi_essid_cmd(int sock, FAR const char *ifname,
                           FAR const char *essid, FAR const char *flagstr);
static void wapi_mode_cmd(int sock, FAR const char *ifname,
                          FAR const char *modestr);
static void wapi_ap_cmd(int sock, FAR const char *ifname,
                        FAR const char *macstr);
static void wapi_bitrate_cmd(int sock, FAR const char *ifname,
                             FAR const char *ratestr, FAR const char *flagstr);
static void wapi_txpower_cmd(int sock, FAR const char *ifname,
                             FAR const char *pwrstr, FAR const char *flagstr);
static void wapi_scan_cmd(int sock, FAR const char *ifname);

static void wapi_showusage(FAR const char *progname, int exitcode);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct wapi_command_s g_wapi_commands[] =
{
  {"help",    0, (CODE void *)NULL},
  {"show",    1, (CODE void *)wapi_show_cmd},
  {"scan",    1, (CODE void *)wapi_scan_cmd},
  {"ip",      2, (CODE void *)wapi_ip_cmd},
  {"mask",    2, (CODE void *)wapi_mask_cmd},
  {"freq",    3, (CODE void *)wapi_freq_cmd},
  {"essid",   3, (CODE void *)wapi_essid_cmd},
  {"mode",    2, (CODE void *)wapi_mode_cmd},
  {"ap",      2, (CODE void *)wapi_ap_cmd},
  {"bitrate", 3, (CODE void *)wapi_bitrate_cmd},
  {"txpower", 3, (CODE void *)wapi_txpower_cmd},
};

#define NCOMMANDS (sizeof(g_wapi_commands) / sizeof(struct wapi_command_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_str2int
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

static int wapi_str2int(FAR const char *str)
{
  FAR char *endptr;
  long value;

  value = strtol(str, &endptr, 0);
  if (*endptr != '\0')
    {
      WAPI_ERROR("ERROR: Garbage after numeric argument\n");
      exit(EXIT_FAILURE);
    }

  if (value > INT_MAX || value < INT_MIN)
    {
      WAPI_ERROR("ERROR: Integer value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (int)value;
}

/****************************************************************************
 * Name: wapi_str2double
 *
 * Description:
 *   Convert a string to a double value
 *
 ****************************************************************************/

static double wapi_str2double(FAR const char *str)
{
  FAR char *endptr;
  double value;

  value = strtod(str, &endptr);
  if (*endptr != '\0')
    {
      WAPI_ERROR("ERROR: Garbage after numeric argument\n");
      exit(EXIT_FAILURE);
    }

  return value;
}

/****************************************************************************
 * Name: wapi_str2ndx
 *
 * Description:
 *   Return the index of a string in a list of strings
 *
 ****************************************************************************/

static unsigned int wapi_str2ndx(FAR const char *name, FAR const char **list,
                                 unsigned int listlen)
{
  unsigned int ndx;

  for (ndx = 0; ndx < listlen; ndx++)
    {
      if (strcmp(name, list[ndx]) == 0)
        {
          return ndx;
        }
    }

  WAPI_ERROR("ERROR: Invalid option string: %s\n", name);
  WAPI_ERROR("       Valid options include:\n");
  for (ndx = 0; ndx < listlen; ndx++)
    {
      WAPI_ERROR("       - %s\n", list[ndx]);
    }

  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Name: wapi_show_cmd
 *
 * Description:
 *   Gets current configuration of the ifname using WAPI accessors and prints
 *   them in a pretty fashion with their corresponding return values. If a
 *   getter succeeds, we try to set that property with the same value to test
 *   the setters as well.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_show_cmd(int sock, FAR const char *ifname)
{
  struct in_addr addr;

  double freq;
  enum wapi_freq_flag_e freq_flag;

  char essid[WAPI_ESSID_MAX_SIZE + 1];
  enum wapi_essid_flag_e essid_flag;

  enum wapi_mode_e mode;

  struct ether_addr ap;

  int bitrate;
  enum wapi_bitrate_flag_e bitrate_flag;

  int txpower;
  enum wapi_txpower_flag_e txpower_flag;

  int ret;

  printf("%s Configuration:\n", ifname);

  /* Get ip */

  bzero(&addr, sizeof(struct in_addr));
  ret = wapi_get_ip(sock, ifname, &addr);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_ip() failed: %d\n", ret);
    }
  else
    {
      printf("       IP: %s\n", inet_ntoa(addr));
    }

  /* Get netmask */

  bzero(&addr, sizeof(struct in_addr));
  ret = wapi_get_netmask(sock, ifname, &addr);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_netmask() failed: %d\n", ret);
    }
  else
    {
      printf("  NetMask: %s\n", inet_ntoa(addr));
    }

  /* Get frequency */

  ret = wapi_get_freq(sock, ifname, &freq, &freq_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_freq() failed: %d\n", ret);
    }
  else
    {
      double tmpfreq;
      int chan;

      printf("Frequency: %g\n", freq);
      printf("     Flag: %s\n", g_wapi_freq_flags[freq_flag]);

      ret = wapi_freq2chan(sock, ifname, freq, &chan);
      if (ret < 0)
        {
          WAPI_ERROR("ERROR: wapi_freq2chan() failed: %d\n", ret);
        }
      else
        {
          printf("  Channel: %d\n", chan);
        }

      ret = wapi_chan2freq(sock, ifname, chan, &tmpfreq);
      if (ret < 0)
        {
          WAPI_ERROR("ERROR: wapi_chan2freq() failed: %d\n", ret);
        }
      else
        {
          printf("Frequency: %g\n", tmpfreq);
        }
    }

  /* Get the ESSID */

  ret = wapi_get_essid(sock, ifname, essid, &essid_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_essid() failed: %d\n", ret);
    }
  else
    {
      printf("    ESSID: %s\n", essid);
      printf("     Flag: %s\n", g_wapi_essid_flags[essid_flag]);
    }

  /* Get operating mode */

  ret = wapi_get_mode(sock, ifname, &mode);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_mode() failed: %d\n", ret);
    }
  else
    {
      printf("     Mode: %s", g_wapi_modes[mode]);
    }

  /* Get AP */

  ret = wapi_get_ap(sock, ifname, &ap);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_ap() failed: %d\n", ret);
    }
  else
    {
      printf("       AP: %02x:%02x:%02x:%02x:%02x:%02x\n",
             ap.ether_addr_octet[0], ap.ether_addr_octet[1],
             ap.ether_addr_octet[2], ap.ether_addr_octet[3],
             ap.ether_addr_octet[4], ap.ether_addr_octet[5]);
    }

  /* Get bitrate */

  ret = wapi_get_bitrate(sock, ifname, &bitrate, &bitrate_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_bitrate() failed: %d\n", ret);
    }
  else
    {
      printf("  BitRate: %d\n", bitrate);
      printf("     Flag: %s\n", g_wapi_bitrate_flags[bitrate_flag]);
    }

  /* Get txpower */

  ret = wapi_get_txpower(sock, ifname, &txpower, &txpower_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_txpower() failed: %d\n", ret);
    }
  else
    {
      printf("  TxPower: %d\n", txpower);
      printf("     Flag: %s\n", g_wapi_txpower_flags[txpower_flag]);
    }
}

/****************************************************************************
 * Name: wapi_ip_cmd
 *
 * Description:
 *   Set the IP address.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_ip_cmd(int sock, FAR const char *ifname,
                        FAR const char *addrstr)
{
  struct in_addr addr;
  int ret;

  /* Format the request */

  addr.s_addr = inet_addr(addrstr);

  /* Set the IP address */

  ret = wapi_set_ip(sock, ifname, &addr);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_set_ip() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_mask_cmd
 *
 * Description:
 *  Set the network mask
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_mask_cmd(int sock, FAR const char *ifname,
                          FAR const char *maskstr)
{
  struct in_addr addr;
  int ret;

  /* Format the request */

  addr.s_addr = inet_addr(maskstr);

  /* Set the network mask */

  ret = wapi_set_netmask(sock, ifname, &addr);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_set_netmask() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_freq_cmd
 *
 * Description:
 *  Set the frequency
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_freq_cmd(int sock, FAR const char *ifname,
                          FAR const char *freqstr, FAR const char *flagstr)
{
  double frequency;
  enum wapi_freq_flag_e freq_flag;
  int ret;

  /* Convert input strings to values */

  frequency = wapi_str2double(freqstr);
  freq_flag = (enum wapi_freq_flag_e)wapi_str2ndx(flagstr, g_wapi_freq_flags,
                                             IW_FREQ_NFLAGS);

  /* Set the frequency */

  ret = wapi_set_freq(sock, ifname, frequency, freq_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: \nwapi_set_freq() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_essid_cmd
 *
 * Description:
 *  Set the ESSID
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_essid_cmd(int sock, FAR const char *ifname,
                           FAR const char *essid, FAR const char *flagstr)
{
  enum wapi_essid_flag_e essid_flag;
  int ret;

  /* Convert input strings to values */

  essid_flag = (enum wapi_essid_flag_e)wapi_str2ndx(flagstr, g_wapi_essid_flags, 2);

  /* Set the ESSID */

  ret = wapi_set_essid(sock, ifname, essid, essid_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_set_essid() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_mode_cmd
 *
 * Description:
 *  Set the operating mode
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_mode_cmd(int sock, FAR const char *ifname,
                          FAR const char *modestr)
{
  enum wapi_mode_e mode;
  int ret;

  /* Convert input strings to values */

  mode = (enum wapi_mode_e)wapi_str2ndx(modestr, g_wapi_modes, IW_MODE_NFLAGS);

  /* Set operating mode */

  ret = wapi_set_mode(sock, ifname, mode);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: \nwapi_set_mode() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_ap_cmd
 *
 * Description:
 *  Set the AP
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_ap_cmd(int sock, FAR const char *ifname,
                        FAR const char *macstr)
{
  struct ether_addr ap;
  int ret;

  /* Convert input strings to values */

  sscanf(macstr, "%02x:%02x:%02x:%02x:%02x:%02x",
         &ap.ether_addr_octet[0], &ap.ether_addr_octet[1],
         &ap.ether_addr_octet[2], &ap.ether_addr_octet[3],
         &ap.ether_addr_octet[4], &ap.ether_addr_octet[5]);

  /* Set ap */

  ret = wapi_set_ap(sock, ifname, &ap);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: \nwapi_set_ap() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_bitrate_cmd
 *
 * Description:
 *  Set the bit rate
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_bitrate_cmd(int sock, FAR const char *ifname,
                             FAR const char *ratestr, FAR const char *flagstr)

{
  enum wapi_bitrate_flag_e bitrate_flag;
  int bitrate;
  int ret;

  /* Convert input strings to values */

  bitrate      = wapi_str2int(ratestr);
  bitrate_flag = (enum wapi_bitrate_flag_e)
    wapi_str2ndx(flagstr, g_wapi_bitrate_flags, 2);

  /* Set bitrate */

  ret = wapi_set_bitrate(sock, ifname, bitrate, bitrate_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: \nwapi_set_bitrate() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_txpower_cmd
 *
 * Description:
 *  Set the TX power
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_txpower_cmd(int sock, FAR const char *ifname,
                             FAR const char *pwrstr, FAR const char *flagstr)
{
  enum wapi_txpower_flag_e txpower_flag;
  int txpower;
  int ret;

  /* Convert input strings to values */

  txpower      = wapi_str2int(pwrstr);
  txpower_flag = (enum wapi_txpower_flag_e)
    wapi_str2ndx(flagstr, g_wapi_txpower_flags, IW_TXPOW_NFLAGS);

  /* Set txpower */

  ret = wapi_set_txpower(sock, ifname, txpower, txpower_flag);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: \nwapi_set_txpower() failed: %d\n", ret);
    }
}

/****************************************************************************
 * Name: wapi_scan_cmd
 *
 * Description:
 *   Scans available APs in the range using given ifname interface.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_scan_cmd(int sock, FAR const char *ifname)
{
  int sleepdur = 1;
  int sleeptries = 5;
  struct wapi_list_s list;
  FAR struct wapi_scan_info_s *info;
  int ret;

  /* Start scan */

  ret = wapi_scan_init(sock, ifname);
  WAPI_ERROR("ERROR: wapi_scan_init() failed: %d\n", ret);

  /* Wait for completion */

  do
    {
      sleep(sleepdur);
      ret = wapi_scan_stat(sock, ifname);
      WAPI_ERROR("ERROR: wapi_scan_stat() failed: %d, sleeptries: %d\n",
                 ret, sleeptries);
    }
  while (--sleeptries > 0 && ret > 0);

  if (ret < 0)
    {
      return;
    }

  /* Collect results */

  bzero(&list, sizeof(struct wapi_list_s));
  ret = wapi_scan_coll(sock, ifname, &list);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_scan_coll() failed: %d\n", ret);
    }

  /* Print found aps */

  for (info = list.head.scan; info; info = info->next)
    {
      printf("    %02x:%02x:%02x:%02x:%02x:%02x %s\n",
             info->ap.ether_addr_octet[0], info->ap.ether_addr_octet[1],
             info->ap.ether_addr_octet[2], info->ap.ether_addr_octet[3],
             info->ap.ether_addr_octet[4],  info->ap.ether_addr_octet[5],
             (info->has_essid ? info->essid : ""));
    }

  /* Free ap list */

  info = list.head.scan;
  while (info)
    {
      FAR struct wapi_scan_info_s *temp;

      temp = info->next;
      free(info);
      info = temp;
    }
}

/****************************************************************************
 * Name: wapi_showusage
 *
 * Description:
 *   Show program usage.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_showusage(FAR const char *progname, int exitcode)
{
  int i;

  fprintf(stderr, "Usage: %s show <ifname>\n", progname);
  fprintf(stderr, "       %s scan <ifname>\n", progname);
  fprintf(stderr, "       %s ip <ifname> <IP address>\n", progname);
  fprintf(stderr, "       %s mask <ifname> <mask>\n", progname);
  fprintf(stderr, "       %s freq <ifname> <frequency> <flag>\n", progname);
  fprintf(stderr, "       %s essid <ifname> <essid> <flag>\n", progname);
  fprintf(stderr, "       %s mode <ifname> <ifname> <mode>\n", progname);
  fprintf(stderr, "       %s ap <ifname> <ifname> <MAC address>\n", progname);
  fprintf(stderr, "       %s bitrate <ifname> <bitrate> <flag>\n", progname);
  fprintf(stderr, "       %s txpower <ifname> <txpower> <flag>\n", progname);
  fprintf(stderr, "       %s help\n", progname);

  fprintf(stderr, "\nFrequency Flags:\n");
  for (i = 0; i < IW_FREQ_NFLAGS; i++)
    {
       fprintf(stderr, "       %s\n", g_wapi_freq_flags[i]);
    }

  fprintf(stderr, "\nESSID Flags:\n");
  for (i = 0; i < 2; i++)
    {
      fprintf(stderr, "       %s\n", g_wapi_essid_flags[i]);
    }

  fprintf(stderr, "\nOperating Modes:\n");
  for (i = 0; i < IW_MODE_NFLAGS; i++)
    {
      fprintf(stderr, "       %s\n", g_wapi_modes[i]);
    }

  fprintf(stderr, "\nBitrate Flags:\n");
  for (i = 0; i < 2; i++)
    {
      fprintf(stderr, "       %s\n", g_wapi_bitrate_flags[i]);
    }

  fprintf(stderr, "\nTX power Flags:\n");
  for (i = 0; i < IW_TXPOW_NFLAGS; i++)
    {
      fprintf(stderr, "       %s\n", g_wapi_txpower_flags[i]);
    }

  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_LOADABLE
int main(int argc, FAR char *argv[])
#else
int wapi_main(int argc, char *argv[])
#endif
{
  FAR const char *cmdname;
  FAR const struct wapi_command_s *wapicmd;
  int sock;
  int i;

  /* Get the command */

  if (argc < 2)
    {
      WAPI_ERROR("ERROR: Missing command\n");
      wapi_showusage(argv[0], EXIT_FAILURE);
    }

  cmdname = argv[1];

  /* Find the command in the g_wapi_command[] list */

  wapicmd = NULL;
  for (i = 0; i < NCOMMANDS; i++)
    {
      FAR const struct wapi_command_s *cmd = &g_wapi_commands[i];
      if (strcmp(cmdname, cmd->name) == 0)
        {
          wapicmd = cmd;
          break;
        }
    }

  if (wapicmd == NULL)
    {
      WAPI_ERROR("ERROR: Unsupported command: %s\n", cmdname);
      wapi_showusage(argv[0], EXIT_FAILURE);
    }

  if (wapicmd->noptions + 2 < argc)
    {
      WAPI_ERROR("ERROR: Garbage at end of command ignored\n");
    }
  else if (wapicmd->noptions + 2 > argc)
    {
      WAPI_ERROR("ERROR: Missing required command options: %s\n",
                 cmdname);
      wapi_showusage(argv[0], EXIT_FAILURE);
    }

  /* Special case the help command which has no arguments, no handler,
   * and does not need a socket.
   */

  if (wapicmd->handler == NULL)
    {
      wapi_showusage(argv[0], EXIT_SUCCESS);
    }

  /* Create a communication socket */

  sock = wapi_make_socket();
  if (sock < 0)
    {
      WAPI_ERROR("ERROR: wapi_make_socket() failed: %d\n", sock);
      return EXIT_FAILURE;
    }

  /* Dispatch the command handling */

  switch (wapicmd->noptions)
    {
      default:
      case 0:
        WAPI_ERROR("ERROR: Internal craziness\n");
        wapi_showusage(argv[0], EXIT_FAILURE);

      case 1:
        ((cmd1_t)wapicmd->handler)(sock, argv[2]);
        break;

      case 2:
        ((cmd2_t)wapicmd->handler)(sock, argv[2], argv[3]);
        break;

      case 3:
        ((cmd3_t)wapicmd->handler)(sock, argv[2], argv[3], argv[4]);
        break;
    }

  /* Close communication socket */

  close(sock);
  return EXIT_SUCCESS;
}
