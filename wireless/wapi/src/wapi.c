/****************************************************************************
 * apps/wireless/wapi/src/wapi.c
 *
 *   Copyright (C) 2011, 2017Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for Nuttx from WAPI:
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


/* Gets current configuration of the @a ifname using WAPI accessors and prints
 * them in a pretty fashion with their corresponding return values. If a getter
 * succeeds, we try to set that property with the same value to test the setters
 * as well.
 */

static void conf(int sock, FAR const char *ifname)
{
  int ret;
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

  /* Get ip */

  bzero(&addr, sizeof(struct in_addr));
  ret = wapi_get_ip(sock, ifname, &addr);
  printf("wapi_get_ip(): ret: %d", ret);
  if (ret >= 0)
    {
      printf(", ip: %s", inet_ntoa(addr));

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* Set ip (Make sure sin.sin_family is set to AF_INET.) */

      ret = wapi_set_ip(sock, ifname, &addr);
      printf("\nwapi_set_ip(): ret: %d", ret);
#endif
    }

  putchar('\n');

  /* Get netmask */

  bzero(&addr, sizeof(struct in_addr));
  ret = wapi_get_netmask(sock, ifname, &addr);
  printf("wapi_get_netmask(): ret: %d", ret);
  if (ret >= 0)
    {
      printf(", netmask: %s", inet_ntoa(addr));

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* set netmask (Make sure sin.sin_family is set to AF_INET.) */

      ret = wapi_set_netmask(sock, ifname, &addr);
      printf("\nwapi_set_netmask(): ret: %d", ret);
#endif
    }

  putchar('\n');

  /* Get freq */

  ret = wapi_get_freq(sock, ifname, &freq, &freq_flag);
  printf("wapi_get_freq(): ret: %d", ret);
  if (ret >= 0)
    {
      int chan;
      double tmpfreq;

      printf(", freq: %g, freq_flag: %s", freq, g_wapi_freq_flags[freq_flag]);

      ret = wapi_freq2chan(sock, ifname, freq, &chan);
      printf("\nwapi_freq2chan(): ret: %d", ret);
      if (ret >= 0)
        {
          printf(", chan: %d", chan);
        }

      ret = wapi_chan2freq(sock, ifname, chan, &tmpfreq);
      printf("\nwapi_chan2freq(): ret: %d", ret);
      if (ret >= 0)
        {
          printf(", freq: %g", tmpfreq);
        }

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* Set freq */

      ret = wapi_set_freq(sock, ifname, freq, freq_flag);
      printf("\nwapi_set_freq(): ret: %d", ret);
#endif
    }

  putchar('\n');

  /* Get essid */

  ret = wapi_get_essid(sock, ifname, essid, &essid_flag);
  printf("wapi_get_essid(): ret: %d", ret);
  if (ret >= 0)
    {
      printf(", essid: %s, essid_flag: %s",
             essid, g_wapi_essid_flags[essid_flag]);

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* Set essid */

      ret = wapi_set_essid(sock, ifname, essid, essid_flag);
      printf("\nwapi_set_essid(): ret: %d", ret);
#endif
    }

  putchar('\n');

  /* Get operating mode */

  ret = wapi_get_mode(sock, ifname, &mode);
  printf("wapi_get_mode(): ret: %d", ret);
  if (ret >= 0)
    {
      printf(", mode: %s", g_wapi_modes[mode]);

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* Set operating mode */

      ret = wapi_set_mode(sock, ifname, mode);
      printf("\nwapi_set_mode(): ret: %d", ret);
#endif
    }

  putchar('\n');

  /* Get ap */

  ret = wapi_get_ap(sock, ifname, &ap);
  printf("wapi_get_ap(): ret: %d", ret);
  if (ret >= 0)
    {
      printf(", ap: %02X:%02X:%02X:%02X:%02X:%02X",
             ap.ether_addr_octet[0], ap.ether_addr_octet[1],
             ap.ether_addr_octet[2], ap.ether_addr_octet[3],
             ap.ether_addr_octet[4], ap.ether_addr_octet[5]);

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* Set ap */

      ret = wapi_set_ap(sock, ifname, &ap);
      printf("\nwapi_set_ap(): ret: %d", ret);
#endif
    }

  putchar('\n');

  /* Get bitrate */

  ret = wapi_get_bitrate(sock, ifname, &bitrate, &bitrate_flag);
  printf("wapi_get_bitrate(): ret: %d", ret);
  if (ret >= 0)
    {
      printf(", bitrate: %d, bitrate_flag: %s", bitrate,
             g_wapi_bitrate_flags[bitrate_flag]);

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* Set bitrate */

      ret = wapi_set_bitrate(sock, ifname, bitrate, bitrate_flag);
      printf("\nwapi_set_bitrate(): ret: %d", ret);
#endif
    }

  putchar('\n');

  /* Get txpower */

  ret = wapi_get_txpower(sock, ifname, &txpower, &txpower_flag);
  printf("wapi_get_txpower(): ret: %d", ret);
  if (ret >= 0)
    {
      printf(", txpower: %d, txpower_flag: %s",
             txpower, g_wapi_txpower_flags[txpower_flag]);

#ifdef CONFIG_WIRELESS_WAPI_ENABLE_SET
      /* Set txpower */

      ret = wapi_set_txpower(sock, ifname, txpower, txpower_flag);
      printf("\nwapi_set_txpower(): ret: %d", ret);
#endif
    }

    putchar('\n');
}

/* Scans available APs in the range using given @a ifname interface. (Requires
 * root privileges to start a scan.)
 */

static void scan(int sock, FAR const char *ifname)
{
  int sleepdur = 1;
  int sleeptries = 5;
  wapi_list_t list;
  FAR wapi_scan_info_t *info;
  int ret;

  /* Start scan */

  ret = wapi_scan_init(sock, ifname);
  printf("wapi_scan_init(): ret: %d\n", ret);

  /* Wait for completion */

  do
    {
      sleep(sleepdur);
      ret = wapi_scan_stat(sock, ifname);
      printf("wapi_scan_stat(): ret: %d, sleeptries: %d\n", ret, sleeptries);
    }
  while (--sleeptries > 0 && ret > 0);

  if (ret < 0)
    {
      return;
    }

  /* Collect results */

  bzero(&list, sizeof(wapi_list_t));
  ret = wapi_scan_coll(sock, ifname, &list);
  printf("wapi_scan_coll(): ret: %d\n", ret);

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
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int wapi_main(int argc, char *argv[])
#endif
{
  FAR const char *ifname;
  wapi_list_t list;
  int ret;
  int sock;

  /* Check command line args */

  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s <IFNAME>\n", argv[0]);
      return EXIT_FAILURE;
    }

  ifname = argv[1];

  /* Make a comm. sock. */

  sock = wapi_make_socket();
  printf("wapi_make_socket(): sock: %d\n", sock);

  /* List conf */

  printf("\nconf\n");
  printf("------------\n");
  conf(sock, ifname);

  /* Scan aps */

  printf("\nscan\n");
  printf("----\n");
  scan(sock, ifname);

  /* Close comm. sock. */

  close(sock);
  return EXIT_SUCCESS;
}
