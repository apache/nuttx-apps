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
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "wireless/wapi.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_show_command
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

static void wapi_show_command(int sock, FAR const char *ifname)
{
  struct in_addr addr;

  double freq;
  wapi_freq_flag_t freq_flag;

  char essid[WAPI_ESSID_MAX_SIZE + 1];
  wapi_essid_flag_t essid_flag;

  wapi_mode_t mode;

  struct ether_addr ap;

  int bitrate;
  wapi_bitrate_flag_t bitrate_flag;

  int txpower;
  wapi_txpower_flag_t txpower_flag;

  int ret;

  printf("%s Configuration:\n", ifname);

  /* Get ip */

  bzero(&addr, sizeof(struct in_addr));
  ret = wapi_get_ip(sock, ifname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_get_ip() failed: %d", ret);
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
      fprintf(stderr, "ERROR: wapi_get_netmask() failed: %d", ret);
    }
  else
    {
      printf("  NetMask: %s", inet_ntoa(addr));
    }

  /* Get frequency */

  ret = wapi_get_freq(sock, ifname, &freq, &freq_flag);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_get_freq() failed: %d", ret);
    }
  else
    {
      double tmpfreq;
      int chan;

      printf("Frequency: %g\n", freq;
      printf("     Flag: %s\n", g_wapi_freq_flags[freq_flag]);

      ret = wapi_freq2chan(sock, ifname, freq, &chan);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: wapi_freq2chan() failed: %d", ret);
        }
      else
        {
          printf("  Channel: %d\n", chan);
        }

      ret = wapi_chan2freq(sock, ifname, chan, &tmpfreq);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: wapi_chan2freq() failed: %d", ret);
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
      fprintf(stderr, "ERROR: wapi_get_essid() failed: %d", ret);
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
      fprintf(stderr, "ERROR: wapi_get_mode() failed: %d", ret);
    }
  else
    {
      printf("     Mode: %s", g_wapi_modes[mode]);
    }

  /* Get AP */

  ret = wapi_get_ap(sock, ifname, &ap);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_get_ap() failed: %d", ret);
    }
  else
    {
      printf("       AP: %02x:%02x:%02x:%02x:%02x:%02x",
             ap.ether_addr_octet[0], ap.ether_addr_octet[1],
             ap.ether_addr_octet[2], ap.ether_addr_octet[3],
             ap.ether_addr_octet[4], ap.ether_addr_octet[5]);
    }

  /* Get bitrate */

  ret = wapi_get_bitrate(sock, ifname, &bitrate, &bitrate_flag);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_get_bitrate() failed: %d", ret);
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
      fprintf(stderr, "ERROR: wapi_get_txpower() failed: %d", ret);
    }
  else
    {
      printf("  TxPower: %d\n", txpower);
      printf("     Flag: %s\n", g_wapi_txpower_flags[txpower_flag]);
    }
}

/****************************************************************************
 * Name: wapi_setip_cmd
 *
 * Description:
 *   Set the IP address.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_setip_cmd(int sock, FAR const char *addrstr)
{
  struct in_addr addr;
  int ret;

  /* Format the request */
#warning Missing logic

  /* Set the IP address */

  ret = wapi_set_ip(sock, ifname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_set_ip() failed: %d", ret);
    }
}

/****************************************************************************
 * Name: wapi_setmask_cmd
 *
 * Description:
 *  Set the network mask
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_setmask_cmd(int sock, FAR const char *maskstr)
{
  struct in_addr addr;
  int ret;

  /* Format the request */
#warning Missing logic

  /* Set the network mask */

  ret = wapi_set_netmask(sock, ifname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_set_netmask() failed: %d", ret);
    }
}

/****************************************************************************
 * Name: wapi_setfreq_cmd
 *
 * Description:
 *  Set the frequency
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_setfreq_cmd(int sock, double frequency,
                             wapi_freq_flag_t freq_flag)
{
  int ret;

  /* Set the network mask */

  ret = wapi_set_freq(sock, ifname, freq, freq_flag);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: \nwapi_set_freq() failed: %d", ret);
    }
}

/****************************************************************************
 * Name: wapi_setessid_cmd
 *
 * Description:
 *  Set the frequency
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_setessid_cmd(int sock, FAR char *essid,
                              wapi_essid_flag_t essid_flag)
{
  int ret;

  /* Set essid */

  ret = wapi_set_essid(sock, ifname, essid, essid_flag);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_set_essid() failed: %d", ret);
    }
}

/****************************************************************************
 * Name: wapi_setmode_cmd
 *
 * Description:
 *  Set the operating mode
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_setmode_cmd(int sock, FAR char *ifname, wapi_mode_t mode)
{
  int ret;

  /* Set operating mode */

  ret = wapi_set_mode(sock, ifname, mode);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: \nwapi_set_mode() failed: %d", ret);
    }
}

/****************************************************************************
 * Name: wapi_setap_cmd
 *
 * Description:
 *  Set the AP
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_setap_cmd(int sock, FAR char *ifname,
                           FAR struct ether_addr *ap)
{
  int ret;

  /* Set ap */

  ret = wapi_set_ap(sock, ifname, ap);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: \nwapi_set_ap() failed: %d", ret);
    }
}

/****************************************************************************
 * Name: wapi_setbitrate_cmd
 *
 * Description:
 *  Set the bit rate
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_setbitrate_cmd(int sock, int bitrate,
                                wapi_bitrate_flag_t bitrate_flag)
{
  int ret;

  /* Set bitrate */

  ret = wapi_set_bitrate(sock, ifname, bitrate, bitrate_flag);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: \nwapi_set_bitrate() failed: %d", ret);
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

static void wapi_txpower_cmd(int sock, int txpower,
                             wapi_txpower_flag_t txpower_flag)
{

  int ret;

  /* Set txpower */

  ret = wapi_set_txpower(sock, ifname, txpower, txpower_flag);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: \nwapi_set_txpower() failed: %d", ret);
    }
}

/****************************************************************************
 * Name: wapi_scan
 *
 * Description:
 *   Scans available APs in the range using given ifname interface.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void wapi_scan(int sock, FAR const char *ifname)
{
  int sleepdur = 1;
  int sleeptries = 5;
  wapi_list_t list;
  FAR wapi_scan_info_t *info;
  int ret;

  /* Start scan */

  ret = wapi_scan_init(sock, ifname);
  fprintf(stderr, "ERROR: wapi_scan_init() failed: %d\n", ret);

  /* Wait for completion */

  do
    {
      sleep(sleepdur);
      ret = wapi_scan_stat(sock, ifname);
      fprintf(stderr, "ERROR: wapi_scan_stat() failed: %d, sleeptries: %d\n", ret, sleeptries);
    }
  while (--sleeptries > 0 && ret > 0);

  if (ret < 0)
    {
      return;
    }

  /* Collect results */

  bzero(&list, sizeof(wapi_list_t));
  ret = wapi_scan_coll(sock, ifname, &list);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: wapi_scan_coll() failed: %d\n", ret);
    }

  /* Print found aps */

  for (info = list.head.scan; info; info = info->next)
    {
      printf(">> %02x:%02x:%02x:%02x:%02x:%02x %s\n",
             info->ap.ether_addr_octet[0], info->ap.ether_addr_octet[1],
             info->ap.ether_addr_octet[2], info->ap.ether_addr_octet[3],
             info->ap.ether_addr_octet[4],  info->ap.ether_addr_octet[5],
             (info->has_essid ? info->essid : ""));
    }

  /* Free ap list */

  info = list.head.scan;
  while (info)
    {
      FAR wapi_scan_info_t *temp;

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
  fprintf(stderr, "Usage: %s show <ifname>\n", progname);
  fprintf(stderr, "       %s setip <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s setmask <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s setfreq <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s setessid <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s setmode <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s setap <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s setbitrate <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s txpower <ifname> OPTIONS\n", progname);
  fprintf(stderr, "       %s scan <ifname>\n", progname);
  fprintf(stderr, "       %s help\n", progname);

  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int wapi_main(int argc, char *argv[])
#endif
{
  FAR const char *cmd;
  FAR const char *ifname;
  wapi_list_t list;
  int ret;
  int sock;

  /* Check command line args */

  if (argc == 1 && strcmp(argv[1], "help") == 0)
    {
      wapi_showusage(argv[0], EXIT_SUCCESS);
    }
  else if (argc < 3)
    {
      fprintf(stderr, "ERROR: Too few command line arguments\n");
      wapi_showusage(argv[0], EXIT_FAILURE);
    }

  cmd    = argv[1];
  ifname = argv[2];

  /* Create a communication sock. */

  sock = wapi_make_socket();
  if (sock < 0)
    {
      fprintf(stderr, "ERROR: wapi_make_socket() failed: %d\n", sock);
    }

  /* Execute command */

  if (strcmp(cmd, "show")
    {
      wapi_show_command(sock, ifname);
    }
  else if (strcmp(cmd, "setip")
    {
      /* Parse command specific options */
#warning Missing logic *

      /* Execute the command */
      //wapi_setip_cmd(sock, addrstr);
    }
  else if (strcmp(cmd, "setmask")
    {
      /* Parse command specific options */
#warning Missing logic *

      /* Execute the command */
      //wapi_setmask_cmd(sock, maskstr);
    }
  else if (strcmp(cmd, "setfreq")
    {
      /* Parse command specific options */
#warning Missing logic *

      /* Execute the command */
      //wapi_setfreq_cmd(sock, frequency, freq_flag);
    }
  else if (strcmp(cmd, "setessid")
    {
      /* Parse command specific options */
#warning Missing logic *

      /* Execute the command */
      //wapi_setessid_cmd(sock, essid, essid_flag);
    }
  else if (strcmp(cmd, "setmode")
    {
      /* Parse command specific options */
#warning Missing logic *

      /* Execute the command */
      //wapi_setmode_cmd(sock, ifname, mode);
    }
  else if (strcmp(cmd, "setbitrate")
    {
      /* Parse command specific options */
#warning Missing logic *

      /* Execute the command */
      //wapi_setap_cmd(sock, ifname, ap);
    }
  else if (strcmp(cmd, "txpower")
    {
      /* Parse command specific options */
#warning Missing logic *

      /* Execute the command */
      //wapi_setbitrate_cmd(sock, bitrate, bitrate_flag);
    }
  else if (strcmp(cmd, "scan")
    {
      wapi_scan(sock, ifname);
    }
  else if (strcmp(cmd, "help")
    {
      fprintf(stderr, "WARNING: Garbage after help common ignored.\n");
      wapi_showusage(argv[0], EXIT_SUCCESS);
    }
  else
    {
      fprintf(stderr, "WARNING: Unrecognized command: %s\n", cmd);
      wapi_showusage(argv[0], EXIT_FAILURE);
    }

  /* Close communication socket */

  close(sock);
  return EXIT_SUCCESS;
}
