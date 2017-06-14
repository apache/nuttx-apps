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
 *   Author: Gregory Nuttx <gnutt@nuttx.org>
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

#include <nuttx/wireless/ieee802154/ieee802154_ioctl.h>
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

void i8sak_startpan_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  struct ieee802154_reset_req_s resetreq;
  struct ieee802154_start_req_s startreq;
  int fd, i, option;

  while ((option = getopt(argc, argv, ":h")) != ERROR)
    {
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Starts PAN as PAN Coordinator\n"
                    "Usage: %s [-h]\n"
                    "    -h = this help menu\n"
                    , argv[0]);
            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;
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

  fd = open(i8sak->devname, O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", i8sak->devname, errno);
      i8sak_cmd_error(i8sak);
    }

  /* Reset the MAC layer */

  printf("\ni8sak: resetting MAC layer\n");
  resetreq.rst_pibattr = true;
  ieee802154_reset_req(fd, &resetreq);

  /* Make sure receiver is always on */

  ieee802154_setrxonidle(fd, true);

  /* If the addresses has never been automatically or manually set before, set
   * it assuming that we are the default PAN coordinator address and the
   * endpoint is the default device address.
   */
   
  if (!i8sak->addrset)
    {
      for (i = 0; i < IEEE802154_EADDR_LEN; i++)
      {
        i8sak->addr.eaddr[i] = 
          (uint8_t)((CONFIG_IEEE802154_I8SAK_PANCOORD_EADDR >> (i*8)) & 0xFF);
      }

      i8sak->addr.mode = IEEE802154_ADDRMODE_SHORT;
      i8sak->addr.saddr = CONFIG_IEEE802154_I8SAK_PANCOORD_SADDR;
      i8sak->addr.panid = CONFIG_IEEE802154_I8SAK_PANID;

      for (i = 0; i < IEEE802154_EADDR_LEN; i++)
      {
        i8sak->ep.eaddr[i] = 
          (uint8_t)((CONFIG_IEEE802154_I8SAK_DEV_EADDR >> (i*8)) & 0xFF);
      }
  
      i8sak->ep.mode = IEEE802154_ADDRMODE_SHORT;
      i8sak->ep.saddr = CONFIG_IEEE802154_I8SAK_DEV_SADDR;
      i8sak->ep.panid = CONFIG_IEEE802154_I8SAK_PANID;
    }

  /* Set EADDR and SADDR */

  ieee802154_seteaddr(fd, &i8sak->addr.eaddr[0]);
  ieee802154_setsaddr(fd, i8sak->addr.saddr);

  /* Tell the MAC to start acting as a coordinator */

  printf("i8sak: starting PAN\n");

  startreq.panid = i8sak->addr.panid;
  startreq.chnum = i8sak->chnum;
  startreq.chpage = i8sak->chpage; 
  startreq.beaconorder = 15;
  startreq.pancoord = true;
  startreq.coordrealign = false;

  ieee802154_start_req(fd, &startreq);

  close(fd);
}