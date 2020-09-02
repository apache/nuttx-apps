/****************************************************************************
 * apps/wireless/wapi/src/wapi.c
 *
 *   Copyright (C) 2011, 2017, 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Largely and original work, but highly influenced by sampled code provided
 * with WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of  source code must  retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "netutils/netlib.h"

#include "wireless/wapi.h"
#include "util.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Generic form of a command handler */

typedef CODE int (*wapi_cmd_t)(int sock, int argc, FAR char **argv);

/* Describes one command */

struct wapi_command_s
{
  FAR const char *name;
  uint8_t minargs;
  uint8_t maxargs;
  wapi_cmd_t handler;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int wapi_str2int(FAR const char *str);
static double wapi_str2double(FAR const char *str);
static unsigned int wapi_str2ndx(FAR const char *name,
                                 FAR const char **list);
static void wapi_showusage(FAR const char *progname, int exitcode);

static int wapi_show_cmd         (int sock, int argc, FAR char **argv);
static int wapi_ip_cmd           (int sock, int argc, FAR char **argv);
static int wapi_mask_cmd         (int sock, int argc, FAR char **argv);
static int wapi_freq_cmd         (int sock, int argc, FAR char **argv);
static int wapi_essid_cmd        (int sock, int argc, FAR char **argv);
static int wapi_psk_cmd          (int sock, int argc, FAR char **argv);
static int wapi_disconnect_cmd   (int sock, int argc, FAR char **argv);
static int wapi_mode_cmd         (int sock, int argc, FAR char **argv);
static int wapi_ap_cmd           (int sock, int argc, FAR char **argv);
static int wapi_bitrate_cmd      (int sock, int argc, FAR char **argv);
static int wapi_txpower_cmd      (int sock, int argc, FAR char **argv);
static int wapi_scan_results_cmd (int sock, int argc, FAR char **argv);
static int wapi_scan_cmd         (int sock, int argc, FAR char **argv);
#ifdef CONFIG_WIRELESS_WAPI_INITCONF
static int wapi_reconnect_cmd    (int sock, int argc, FAR char **argv);
static int wapi_save_config_cmd  (int sock, int argc, FAR char **argv);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct wapi_command_s g_wapi_commands[] =
{
  {"help",         0, 0, NULL},
  {"show",         1, 1, wapi_show_cmd},
  {"scan",         1, 2, wapi_scan_cmd},
  {"scan_results", 1, 1, wapi_scan_results_cmd},
  {"ip",           2, 2, wapi_ip_cmd},
  {"mask",         2, 2, wapi_mask_cmd},
  {"freq",         3, 3, wapi_freq_cmd},
  {"essid",        3, 3, wapi_essid_cmd},
  {"psk",          3, 3, wapi_psk_cmd},
  {"disconnect",   1, 1, wapi_disconnect_cmd},
  {"mode",         2, 2, wapi_mode_cmd},
  {"ap",           2, 2, wapi_ap_cmd},
  {"bitrate",      3, 3, wapi_bitrate_cmd},
  {"txpower",      3, 3, wapi_txpower_cmd},
#ifdef CONFIG_WIRELESS_WAPI_INITCONF
  {"reconnect",    1, 1, wapi_reconnect_cmd},
  {"save_config",  1, 1, wapi_save_config_cmd},
#endif
};

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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

static unsigned int wapi_str2ndx(FAR const char *name, FAR const char **list)
{
  unsigned int ndx;

  /* Check the first character is enough, all prefix with WAPI_* */

  if (isdigit(name[0]))
    {
      return atoi(name);
    }

  for (ndx = 0; list[ndx]; ndx++)
    {
      if (strcmp(name, list[ndx]) == 0)
        {
          return ndx;
        }
    }

  WAPI_ERROR("ERROR: Invalid option string: %s\n", name);
  WAPI_ERROR("       Valid options include:\n");
  for (ndx = 0; list[ndx]; ndx++)
    {
      WAPI_ERROR("       - [%d] %s\n", ndx, list[ndx]);
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

static int wapi_show_cmd(int sock, int argc, FAR char **argv)
{
  enum wapi_bitrate_flag_e bitrate_flag;
  enum wapi_txpower_flag_e txpower_flag;
  char essid[WAPI_ESSID_MAX_SIZE + 1];
  enum wapi_essid_flag_e essid_flag;
  FAR const char *ifname = argv[0];
  enum wapi_freq_flag_e freq_flag;
  enum wapi_mode_e mode;
  struct ether_addr ap;
  struct in_addr addr;
  double tmpfreq;
  int bitrate;
  int txpower;
  double freq;
  int chan;
  int ret;

  printf("%s Configuration:\n", ifname);

  /* Get ip */

  bzero(&addr, sizeof(struct in_addr));
  ret = wapi_get_ip(sock, ifname, &addr);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_get_ip() failed: %d\n", ret);
      return ret;
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

  bzero(essid, sizeof(essid));
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
      printf("     Mode: %s\n", g_wapi_modes[mode]);
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

  return 0;
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

static int wapi_ip_cmd(int sock, int argc, FAR char **argv)
{
  struct in_addr addr;

  /* Format the request */

  addr.s_addr = inet_addr(argv[1]);

  /* Set the IP address */

  return wapi_set_ip(sock, argv[0], &addr);
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

static int wapi_mask_cmd(int sock, int argc, FAR char **argv)
{
  struct in_addr addr;

  /* Format the request */

  addr.s_addr = inet_addr(argv[1]);

  /* Set the network mask */

  return wapi_set_netmask(sock, argv[0], &addr);
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

static int wapi_freq_cmd(int sock, int argc, FAR char **argv)
{
  double frequency;
  enum wapi_freq_flag_e freq_flag;

  /* Convert input strings to values */

  frequency = wapi_str2double(argv[1]);
  freq_flag = (enum wapi_freq_flag_e)wapi_str2ndx(argv[2],
                                                  g_wapi_freq_flags);

  /* Set the frequency */

  return wapi_set_freq(sock, argv[0], frequency, freq_flag);
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

static int wapi_essid_cmd(int sock, int argc, FAR char **argv)
{
  enum wapi_essid_flag_e essid_flag;

  /* Convert input strings to values */

  essid_flag = (enum wapi_essid_flag_e)
    wapi_str2ndx(argv[2], g_wapi_essid_flags);

  /* Set the ESSID */

  return wapi_set_essid(sock, argv[0], argv[1], essid_flag);
}

/****************************************************************************
 * Name: wapi_psk_cmd
 *
 * Description:
 *  Set the Passphrase
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int wapi_psk_cmd(int sock, int argc, FAR char **argv)
{
  enum wpa_alg_e alg_flag;

  /* Convert input strings to values */

  alg_flag = (enum wpa_alg_e)wapi_str2ndx(argv[2], g_wapi_alg_flags);

  /* Set the Passphrase */

  return wpa_driver_wext_set_key_ext(sock, argv[0], alg_flag,
                                     argv[1], strlen(argv[1]));
}

/****************************************************************************
 * Name: wapi_disconnect_cmd
 *
 * Description:
 *   Disconnect the AP in the range using given ifname interface.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int wapi_disconnect_cmd(int sock, int argc, FAR char **argv)
{
  wpa_driver_wext_disconnect(sock, argv[0]);

  return 0;
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

static int wapi_mode_cmd(int sock, int argc, FAR char **argv)
{
  enum wapi_mode_e mode;

  /* Convert input strings to values */

  mode = (enum wapi_mode_e)wapi_str2ndx(argv[1], g_wapi_modes);

  /* Set operating mode */

  return wapi_set_mode(sock, argv[0], mode);
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

static int wapi_ap_cmd(int sock, int argc, FAR char **argv)
{
  struct ether_addr ap;

  /* Convert input strings to values */

  sscanf(argv[1], "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
         &ap.ether_addr_octet[0], &ap.ether_addr_octet[1],
         &ap.ether_addr_octet[2], &ap.ether_addr_octet[3],
         &ap.ether_addr_octet[4], &ap.ether_addr_octet[5]);

  /* Set ap */

  return wapi_set_ap(sock, argv[0], &ap);
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

static int wapi_bitrate_cmd(int sock, int argc, FAR char **argv)
{
  enum wapi_bitrate_flag_e bitrate_flag;
  int bitrate;

  /* Convert input strings to values */

  bitrate      = wapi_str2int(argv[1]);
  bitrate_flag = (enum wapi_bitrate_flag_e)
    wapi_str2ndx(argv[2], g_wapi_bitrate_flags);

  /* Set bitrate */

  return wapi_set_bitrate(sock, argv[0], bitrate, bitrate_flag);
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

static int wapi_txpower_cmd(int sock, int argc, FAR char **argv)
{
  enum wapi_txpower_flag_e txpower_flag;
  int txpower;

  /* Convert input strings to values */

  txpower      = wapi_str2int(argv[1]);
  txpower_flag = (enum wapi_txpower_flag_e)
    wapi_str2ndx(argv[2], g_wapi_txpower_flags);

  /* Set txpower */

  return wapi_set_txpower(sock, argv[0], txpower, txpower_flag);
}

/****************************************************************************
 * Name: wapi_scan_results_cmd
 *
 * Description:
 *   Print the scan results.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int wapi_scan_results_cmd(int sock, int argc, FAR char **argv)
{
  int sleepdur = 200 * 1000;
  int sleeptries = 25;
  struct wapi_list_s list;
  FAR struct wapi_scan_info_s *info;
  int ret;

  /* Wait for completion */

  do
    {
      ret = wapi_scan_stat(sock, argv[0]);
      if (ret < 0)
        {
          WAPI_ERROR("ERROR: wapi_scan_stat() failed: %d, sleeptries: %d\n",
                      ret, sleeptries);
        }
      else if (ret == 1)
        {
          usleep(sleepdur);
        }
    }
  while (--sleeptries > 0 && ret > 0);

  if (ret < 0)
    {
      return ret;
    }

  /* Collect results */

  bzero(&list, sizeof(struct wapi_list_s));
  ret = wapi_scan_coll(sock, argv[0], &list);
  if (ret < 0)
    {
      WAPI_ERROR("ERROR: wapi_scan_coll() failed: %d\n", ret);
      return ret;
    }

  /* Print found aps */

  printf("bssid / frequency / signal level / ssid\n");
  for (info = list.head.scan; info; info = info->next)
    {
      printf("%02x:%02x:%02x:%02x:%02x:%02x\t%g\t%d\t%s\n",
             info->ap.ether_addr_octet[0], info->ap.ether_addr_octet[1],
             info->ap.ether_addr_octet[2], info->ap.ether_addr_octet[3],
             info->ap.ether_addr_octet[4],  info->ap.ether_addr_octet[5],
             info->freq, info->rssi, info->essid);
    }

  /* Free ap list */

  wapi_scan_coll_free(&list);
  return 0;
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

static int wapi_scan_cmd(int sock, int argc, FAR char **argv)
{
  FAR const char *essid;
  int ret;

  essid = argc > 1 ? argv[1] : NULL;

  /* Start scan */

  ret = wapi_scan_init(sock, argv[0], essid);
  if (ret < 0)
    {
      return ret;
    }

  return wapi_scan_results_cmd(sock, 1, argv);
}

#ifdef CONFIG_WIRELESS_WAPI_INITCONF

/****************************************************************************
 * Name: wapi_reconnect_cmd
 *
 * Description:
 *   Reconnect the AP in the range using given ifname interface.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int wapi_reconnect_cmd(int sock, int argc, FAR char **argv)
{
  struct wpa_wconfig_s conf;
  FAR void *load;
  int ret;

  load = wapi_load_config(argv[0], NULL, &conf);
  if (load == NULL)
    {
      return -1;
    }

  ret = wpa_driver_wext_associate(&conf);

  wapi_unload_config(load);

  return ret;
}

/****************************************************************************
 * Name: wapi_save_config_cmd
 *
 * Description:
 *   Scans available APs in the range using given ifname interface.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static int wapi_save_config_cmd(int sock, int argc, FAR char **argv)
{
  char essid[WAPI_ESSID_MAX_SIZE + 1];
  enum wapi_essid_flag_e essid_flag;
  struct wpa_wconfig_s conf;
  struct ether_addr ap;
  uint8_t if_flags;
  uint32_t value;
  size_t psk_len;
  char psk[32];
  int ret;

  ret = netlib_getifstatus(argv[0], &if_flags);
  if (ret < 0)
    {
      return ret;
    }

  if (!IFF_IS_RUNNING(if_flags))
    {
      return -1;
    }

  psk_len = sizeof(psk);

  memset(&conf, 0, sizeof(struct wpa_wconfig_s));
  ret = wapi_get_mode(sock, argv[0], &conf.sta_mode);
  if (ret < 0)
    {
      return ret;
    }

  memset(essid, 0, sizeof(essid));
  ret = wapi_get_essid(sock, argv[0], essid, &essid_flag);
  if (ret < 0)
    {
      return ret;
    }

  conf.ssid = essid;
  conf.ssidlen = strnlen(essid, sizeof(essid));

  ret = wapi_get_ap(sock, argv[0], &ap);
  if (ret < 0)
    {
      return ret;
    }

  conf.bssid = (FAR const char *)ap.ether_addr_octet;

  memset(psk, 0, sizeof(psk));
  ret = wpa_driver_wext_get_key_ext(sock,
                                    argv[0],
                                    &conf.alg,
                                    psk,
                                    &psk_len);
  if (ret == 0)
    {
      conf.passphrase = psk;
      conf.phraselen = psk_len;
    }

  ret = wpa_driver_wext_get_auth_param(sock,
                                       argv[0],
                                       IW_AUTH_WPA_VERSION,
                                       &value);
  if (ret < 0)
    {
      conf.auth_wpa = IW_AUTH_WPA_VERSION_WPA2;
    }
  else
    {
      conf.auth_wpa = value;
    }

  ret = wpa_driver_wext_get_auth_param(sock,
                                       argv[0],
                                       IW_AUTH_CIPHER_PAIRWISE,
                                       &value);
  if (ret < 0)
    {
      if (conf.phraselen > 0)
        conf.cipher_mode = IW_AUTH_CIPHER_CCMP;
      else
        conf.cipher_mode = IW_AUTH_CIPHER_NONE;
    }
  else
    {
      conf.cipher_mode = value;
    }

  return wapi_save_config(argv[0], NULL, &conf);
}
#endif

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

  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "\t%s show         <ifname>\n", progname);
  fprintf(stderr, "\t%s scan         <ifname>\n", progname);
  fprintf(stderr, "\t%s scan_results <ifname>\n", progname);
  fprintf(stderr, "\t%s ip           <ifname> <IP address>\n", progname);
  fprintf(stderr, "\t%s mask         <ifname> <mask>\n", progname);
  fprintf(stderr, "\t%s freq         <ifname> <frequency>  <index/flag>\n",
                   progname);
  fprintf(stderr, "\t%s essid        <ifname> <essid>      <index/flag>\n",
                   progname);
  fprintf(stderr, "\t%s psk          <ifname> <passphrase> <index/flag>\n",
                   progname);
  fprintf(stderr, "\t%s disconnect   <ifname>\n", progname);
  fprintf(stderr, "\t%s mode         <ifname> <ifname>     <index/mode>\n",
                   progname);
  fprintf(stderr, "\t%s ap           <ifname> <ifname>     <MAC address>\n",
                   progname);
  fprintf(stderr, "\t%s bitrate      <ifname> <bitrate>    <index/flag>\n",
                   progname);
  fprintf(stderr, "\t%s txpower      <ifname> <txpower>    <index/flag>\n",
                   progname);
#ifdef CONFIG_WIRELESS_WAPI_INITCONF
  fprintf(stderr, "\t%s reconnect    <ifname>\n", progname);
  fprintf(stderr, "\t%s save_config  <ifname>\n", progname);
#endif
  fprintf(stderr, "\t%s help\n", progname);

  fprintf(stderr, "\nFrequency Flags:\n");
  for (i = 0; g_wapi_freq_flags[i]; i++)
    {
      fprintf(stderr, "\t[%d] %s\n", i, g_wapi_freq_flags[i]);
    }

  fprintf(stderr, "\nESSID Flags:\n");
  for (i = 0; g_wapi_essid_flags[i]; i++)
    {
      fprintf(stderr, "\t[%d] %s\n", i, g_wapi_essid_flags[i]);
    }

  fprintf(stderr, "\nPassphrase algorithm Flags:\n");
  for (i = 0; g_wapi_alg_flags[i]; i++)
    {
      fprintf(stderr, "\t[%d] %s\n", i, g_wapi_alg_flags[i]);
    }

  fprintf(stderr, "\nOperating Modes:\n");
  for (i = 0; g_wapi_modes[i]; i++)
    {
      fprintf(stderr, "\t[%d] %s\n", i, g_wapi_modes[i]);
    }

  fprintf(stderr, "\nBitrate Flags:\n");
  for (i = 0; g_wapi_bitrate_flags[i]; i++)
    {
      fprintf(stderr, "\t[%d] %s\n", i, g_wapi_bitrate_flags[i]);
    }

  fprintf(stderr, "\nTX power Flags:\n");
  for (i = 0; g_wapi_txpower_flags[i]; i++)
    {
      fprintf(stderr, "\t[%d] %s\n", i, g_wapi_txpower_flags[i]);
    }

  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
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

  if (wapicmd->maxargs + 2 < argc)
    {
      WAPI_ERROR("ERROR: Garbage at end of command ignored\n");
    }
  else if (wapicmd->minargs + 2 > argc)
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

  if (wapicmd->handler(sock, argc - 2, argc == 2 ? NULL : &argv[2]) < 0)
    {
      WAPI_ERROR("ERROR: Process command (%s) failed.\n", cmdname);
    }

  /* Close communication socket */

  close(sock);
  return EXIT_SUCCESS;
}
