/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_assoc.c
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <nuttx/fs/ioctl.h>

#include "i8sak.h"

#include <nuttx/wireless/ieee802154/ieee802154_ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void assoc_eventcb(FAR struct ieee802154_notif_s *notif, FAR void *arg);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_assoc
 *
 * Description :
 *   Request association with the Coordinator
 ****************************************************************************/

void i8sak_assoc_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  struct ieee802154_assoc_req_s assocreq;
  struct wpanlistener_eventfilter_s filter;
  FAR struct ieee802154_pandesc_s *pandesc;
  struct ieee802154_set_req_s setreq;
  bool retry     = false;
  int maxretries = 0;
  int retrycnt;
  int fd;
  int option;
  int optcnt;
  int ret;
  uint8_t resindex;

  setreq.attr = IEEE802154_ATTR_MAC_RESPONSE_WAIT_TIME;
  setreq.attrval.mac.resp_waittime = 32;

  /* If the addresses has never been automatically or manually set before, set
   * it assuming that we are the default device address and the endpoint is the
   * default PAN Coordinator address.  This is actually the way the i8sak settings
   * are configured, so just set the flag if it's not already set.
   */

  if (!i8sak->addrset)
    {
      i8sak->addrset = true;
    }

  optcnt = 0;
  while ((option = getopt(argc, argv, ":hr:s:e:w:t:")) != ERROR)
    {
      optcnt++;
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Requests association with endpoint\n"
                    "Usage: %s [-h] [-w <count>\n"
                    "    -h = this help menu\n"
                    "    -w = wait and retry on failure\n"
                    "    -r = use scan result index\n"
                    "    -s = coordinator short address"
                    "    -e = coordinator ext address"
                    "    -t = response wait time"
                    , argv[0]);

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;

          case 'r':
            resindex = i8sak_str2luint8(optarg);

            if (resindex >= i8sak->npandesc)
              {
                fprintf(stderr, "ERROR: missing argument\n");

                /* Must manually reset optind if we are going to exit early */

                optind = -1;
                i8sak_cmd_error(i8sak); /* This exits for us */
              }

            pandesc = &i8sak->pandescs[resindex];

            i8sak->chan   = pandesc->chan;
            i8sak->chpage = pandesc->chpage;
            memcpy(&i8sak->ep, &pandesc->coordaddr, sizeof(struct ieee802154_addr_s));
            break;

          case 's':
            /* Parse short address and put it into the i8sak instance */

            i8sak_str2saddr(optarg, i8sak->ep.saddr);
            i8sak->ep.mode= IEEE802154_ADDRMODE_SHORT;
            break;

          case 'e':
            /* Parse extended address and put it into the i8sak instance */

            i8sak_str2eaddr(optarg, i8sak->ep.eaddr);
            i8sak->ep.mode = IEEE802154_ADDRMODE_EXTENDED;
            break;
          
          case 't':
            /* Parse wait time and set the paremeter in the request */

            setreq.attrval.mac.resp_waittime = i8sak_str2luint8(optarg);
            break;

          case 'w':
            /* Wait and retry if we fail to associate */

            retry      = true;
            maxretries = atoi(optarg);
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

  /* If none of the option flags were used, and there is an argument included,
   * assume it is the PAN ID
   */

  if (optcnt && argc == 2)
    {
      i8sak_str2panid(argv[1], i8sak->ep.panid);
    }

  fd = open(i8sak->devname, O_RDWR);
  if (fd < 0)
    {
      printf("i8sak: cannot open %s, errno=%d\n", i8sak->devname, errno);
      i8sak_cmd_error(i8sak);
    }

  ieee802154_set_req(fd, &setreq);

  /* Register new callback for receiving the association notifications. */

  memset(&filter, 0, sizeof(struct wpanlistener_eventfilter_s));
  filter.confevents.assoc = true;

  wpanlistener_add_eventreceiver(&i8sak->wpanlistener, assoc_eventcb,
                                 &filter, (FAR void *)i8sak, false);

  /* Loop for the specified retry count if the association fails.
   */

  retrycnt = 0;
  for (; ; )
    {
      printf("i8sak: issuing ASSOC. request %d\n", retrycnt + 1);

      /* Issue association request */

      assocreq.chan = i8sak->chan;
      assocreq.chpage = i8sak->chpage;

      memcpy(&assocreq.coordaddr, &i8sak->ep,
             sizeof(struct ieee802154_addr_s));

      assocreq.capabilities.devtype = 0;
      assocreq.capabilities.powersource = 1;
      assocreq.capabilities.rxonidle = 1;
      assocreq.capabilities.security = 0;
      assocreq.capabilities.allocaddr = 1;

      ieee802154_assoc_req(fd, &assocreq);

      /* Wait for the assocconf event */

      i8sak->assoc  = true;
      i8sak->result = -EBUSY;

      ret = sem_wait(&i8sak->sigsem);
      sem_post(&i8sak->exclsem);
      if (ret != OK)
        {
          i8sak->assoc = false;
          printf("i8sak: test cancelled\n");
          i8sak_cmd_error(i8sak);
          break;
        }

      /* Check if the association was successful */

      if (i8sak->result == OK)
        {
          /* Break out and return if the association was successful. */

          break;
        }

      /* Not successful .. were we asked to retry in the event of an
       * association failure?
       */

      if (!retry)
        {
          /* No retries... break out now with the failed association */

          break;
        }

      /* A retry count of zero means to retry forever.  Otherwise,
       * abort as soon as the maximum number of retries have been
       * performed.
       */

      if (maxretries > 0 && retrycnt >= maxretries)
        {
          /* Max retry count exceeded -- break out now with a failed
           * association.
           */

          break;
        }

      /* Otherwise, wait a bit and try again */

      sleep(2);
      retrycnt++;
    }

  /* Clean up and return */

  (void)wpanlistener_remove_eventreceiver(&i8sak->wpanlistener,
                                          assoc_eventcb);
  close(fd);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void assoc_eventcb(FAR struct ieee802154_notif_s *notif, FAR void *arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;

  if (notif->u.assocconf.status == IEEE802154_STATUS_SUCCESS)
    {
      printf("i8sak: ASSOC.request succeeded\n");
      i8sak->result = OK;
    }
  else
    {
      printf("i8sak: ASSOC.request failed: %s\n",
             IEEE802154_STATUS_STRING[notif->u.assocconf.status]);
      i8sak->result = -EAGAIN;
    }

  if (i8sak->assoc)
    {
      i8sak->assoc = false;
      sem_post(&i8sak->sigsem);
    }
}
