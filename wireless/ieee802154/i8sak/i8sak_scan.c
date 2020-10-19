/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_scan.c
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2014-2015, 2017 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *
 *   Author: Anthony Merlino <anthony@vergeaero.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/fs/ioctl.h>

#include "i8sak.h"

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void scan_eventcb(FAR struct ieee802154_primitive_s *primitive,
                         FAR void *arg);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_scan_cmd
 *
 * Description :
 *   Request association with the Coordinator
 ****************************************************************************/

void i8sak_scan_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  struct ieee802154_scan_req_s scan;
  struct i8sak_eventfilter_s filter;
  int fd = 0;
  int option;
  int argind;
  int i;
  int minchannel;
  int maxchannel;

  scan.type = IEEE802154_SCANTYPE_PASSIVE;

  argind = 1;
  while ((option = getopt(argc, argv, ":hpae")) != ERROR)
    {
      argind++;
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Requests association with endpoint\n"
                    "Usage: %s [-h|p|a|e] minCh-maxCh\n"
                    "    -h = this help menu\n"
                    "    -p = passive scan (default)\n"
                    "    -a = active scan\n"
                    "    -e = energy scan\n"
                    , argv[0]);

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;

          case 'p':
            scan.type = IEEE802154_SCANTYPE_PASSIVE;
            break;

          case 'a':
            scan.type = IEEE802154_SCANTYPE_ACTIVE;
            break;

          case 'e':
            scan.type = IEEE802154_SCANTYPE_ED;
            break;

          case ':':
            fprintf(stderr, "ERROR: missing argument\n");

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */

          case '?':
            fprintf(stderr, "ERROR: unknown argument\n");

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */
        }
    }

  /* There should always be one argument after the list option argument */

  if (argc != argind + 1)
    {
      fprintf(stderr, "ERROR: invalid channel list\n");
      i8sak_cmd_error(i8sak);
    }

  scan.duration = 7;
  scan.chpage = i8sak->chpage;

  /* Parse channel list */

  sscanf(argv[argind], "%d-%d", &minchannel, &maxchannel);

  scan.numchan = maxchannel - minchannel + 1;

  if (scan.numchan > 15)
    {
      fprintf(stderr, "ERROR: too many channels\n");
      i8sak_cmd_error(i8sak);
    }

  for (i = 0; i < scan.numchan; i++)
    {
      scan.channels[i] = minchannel + i;
    }

  i8sak_requestdaemon(i8sak);

  /* Register new callback for receiving the scan confirmation notification */

  memset(&filter, 0, sizeof(struct i8sak_eventfilter_s));
  filter.confevents.scan = true;

  i8sak_eventlistener_addreceiver(i8sak, scan_eventcb, &filter, true);

  printf("i8sak: starting scan\n");

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      fd = open(i8sak->ifname, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n",
                  i8sak->ifname, errno);
          i8sak_cmd_error(i8sak);
        }

      ieee802154_scan_req(fd, &scan);
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      fd = socket(PF_INET6, SOCK_DGRAM, 0);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: failed to open socket, errno=%d\n", errno);
          i8sak_cmd_error(i8sak);
        }

      sixlowpan_scan_req(fd, i8sak->ifname, &scan);
    }
#endif

  close(fd);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void scan_eventcb(FAR struct ieee802154_primitive_s *primitive,
                         FAR void *arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;
  FAR struct ieee802154_scan_conf_s *scan = &primitive->u.scanconf;
  int i;

  printf("\n\ni8sak: Scan complete: %s\n",
         IEEE802154_STATUS_STRING[scan->status]);

  printf("Scan type: ");

  switch (scan->type)
    {
      case IEEE802154_SCANTYPE_ACTIVE:
        printf("Active\n");
        break;
      case IEEE802154_SCANTYPE_PASSIVE:
        printf("Passive\n");
        break;
      case IEEE802154_SCANTYPE_ED:
        printf("Energy\n");
        break;
      default:
        printf("Unknown\n");
        break;
    }

  printf("Scan results: \n");

  if (scan->type == IEEE802154_SCANTYPE_ACTIVE ||
      scan->type == IEEE802154_SCANTYPE_PASSIVE)
    {
      /* Copy the results from the notification */

      i8sak->npandesc = scan->numresults;
      memcpy(i8sak->pandescs, scan->pandescs,
             sizeof(struct ieee802154_pandesc_s) * i8sak->npandesc);

      /* Print the results out */

      for (i = 0; i < scan->numresults; i++)
        {
          printf("Result %d\n", i);
          printf("    Channel: %u\n", scan->pandescs[i].chan);
          printf("    PAN ID: "
                 PRINTF_FORMAT_PANID(scan->pandescs[i].coordaddr.panid));

          if (scan->pandescs[i].coordaddr.mode == IEEE802154_ADDRMODE_SHORT)
            {
              printf("    Coordinator Short Address: "
                     PRINTF_FORMAT_SADDR(scan->pandescs[i].coordaddr.saddr));
            }
          else
            {
              printf("    Coordinator Extended Address: "
                     PRINTF_FORMAT_EADDR(scan->pandescs[i].coordaddr.eaddr));
            }
        }
    }

  if (scan->type == IEEE802154_SCANTYPE_ED)
    {
      /* Print the results out */

      for (i = 0; i < scan->numresults; i++)
        {
          printf("Result %d\n", i);
          printf("    Channel: %u Energy: %d\n",
                 scan->chlist[i], scan->edlist[i]);
        }
    }

  i8sak_requestdaemon(i8sak);
}
