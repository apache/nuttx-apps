/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_startpan.c
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2014-2015, 2017 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2014-2015 Sebastien Lorquet. All rights reserved.
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_startpan_cmd
 *
 * Description :
 *   Start PAN and accept association requests
 ****************************************************************************/

void i8sak_startpan_cmd(FAR struct i8sak_s *i8sak,
                        int argc, FAR char *argv[])
{
  struct ieee802154_start_req_s startreq;
  bool beaconenabled = false;
  int option;
  int argind;
  int fd = 0;

  argind = 1;
  while ((option = getopt(argc, argv, ":hb")) != ERROR)
    {
      argind++;
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Starts PAN as PAN Coordinator\n"
                    "Usage: %s [-h|b|s] xx:xx\n"
                    "    -h = this help menu\n"
                    "    -b = start beacon-enabled PAN\n"
                    , argv[0]);

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;

          case 'b':
            beaconenabled = true;
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

  if (argc < argind + 1)
    {
      fprintf(stderr, "ERROR: missing PAN ID\n");
      i8sak_cmd_error(i8sak);
    }

  /*  Prepare the START.request primitive */

  i8sak_str2panid(argv[argind], startreq.panid);
  startreq.chan  = i8sak->chan;
  startreq.chpage = i8sak->chpage;

  if (beaconenabled)
    {
      startreq.beaconorder = 8;
      startreq.superframeorder = 5;
    }
  else
    {
      startreq.beaconorder = 15;
    }

  startreq.pancoord     = true;
  startreq.coordrealign = false;
  startreq.battlifeext  = false;

  /* Perform the operations using the appropriate libmac hooks */

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      fd = open(i8sak->ifname, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n",
                  i8sak->ifname, errno);
          i8sak_cmd_error(i8sak);
        }

      /* Reset the MAC layer */

      printf("i8sak: resetting MAC layer\n");
      ieee802154_reset_req(fd, true);

      /* Make sure receiver is always on */

      ieee802154_setrxonidle(fd, true);

      /* Tell the MAC to start acting as a coordinator */

      printf("i8sak: starting PAN\n");

      ieee802154_start_req(fd, &startreq);
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

      /* Reset the MAC layer */

      printf("i8sak: resetting MAC layer\n");
      sixlowpan_reset_req(fd, i8sak->ifname, true);

      /* Make sure receiver is always on */

      sixlowpan_setrxonidle(fd, i8sak->ifname, true);

      /* Tell the MAC to start acting as a coordinator */

      printf("i8sak: starting PAN\n");

      sixlowpan_start_req(fd, i8sak->ifname, &startreq);
    }
#endif

  close(fd);
}
