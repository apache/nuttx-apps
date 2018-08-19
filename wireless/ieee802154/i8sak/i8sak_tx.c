/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_tx.c
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
#include <nuttx/wireless/ieee802154/ieee802154_device.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "wireless/ieee802154.h"

#include "i8sak.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void tx_eventcb(FAR struct ieee802154_primitive_s *primitive, FAR void *arg);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8_tx_cmd
 *
 * Description :
 *   Transmit a data frame.
 ****************************************************************************/

void i8sak_tx_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  enum ieee802154_devmode_e devmode;
  struct i8sak_eventfilter_s eventfilter;
  struct mac802154dev_txframe_s tx;
  bool sendasdev = false;
  bool sendmax = false;
  int option;
  int fd = 0;
  int argind = 1;
  int ret = OK;

  while ((option = getopt(argc, argv, ":hdm")) != ERROR)
    {
      argind++;
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Transmits packet to endpoint\n"
                    "Usage: %s [-h|d|m] [<hex-payload>]\n"
                    "    -h = this help menu\n"
                    "    -d = send as device instead of coord\n"
                    "    -m = send the largest frame possible\n"
                    , argv[0]);

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;

          case 'd':
            sendasdev = true;
            break;

          case 'm':
            sendmax = true;
            break;

          case ':':
            fprintf(stderr, "ERROR: missing argument\n");

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */

          case '?':
            fprintf(stderr, "ERROR: unknown argument\n");
            ret = ERROR;
            optind = -1;
            i8sak_cmd_error(i8sak);
        }
    }

  if (argc == argind + 1)
    {
      i8sak->payload_len = i8sak_str2payload(argv[1], i8sak->payload);
    }

  if (sendmax)
    {
      i8sak->payload_len = IEEE802154_MAX_SAFE_MAC_PAYLOAD_SIZE;
    }

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      fd = open(i8sak->ifname, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n",
                  i8sak->ifname, errno);
          i8sak_cmd_error(i8sak);
        }

      /* Check if transaction should be indirect or not */

      ieee802154_getdevmode(fd, &devmode);
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

      /* Check if transaction should be indirect or not */

      sixlowpan_getdevmode(fd, i8sak->ifname, &devmode);
    }
#endif

  if (!sendasdev)
    {
      /* If we are acting as an endpoint, send as normal CSMA (non-indirect)
       * If we are a coordinator or PAN coordinator, send as indirect.
       */

      if (devmode == IEEE802154_DEVMODE_ENDPOINT)
        {
          tx.meta.flags.indirect = false;
        }
      else
        {
          tx.meta.flags.indirect = true;
        }
    }
  else
    {
      /* We cannot send a frame as direct if we are the PAN coordinator. Maybe
       * this should be the hook for sending payload in beacon? But for now,
       * let's just thow an error.
       */

      if (devmode == IEEE802154_DEVMODE_PANCOORD)
        {
          fprintf(stderr, "ERROR: invalid option\n");
          close(fd);
          i8sak_cmd_error(i8sak);
        }

      tx.meta.flags.indirect = false;
    }

  i8sak_requestdaemon(i8sak);

  /* Register new oneshot callback for receiving the association notifications */

  memset(&eventfilter, 0, sizeof(struct i8sak_eventfilter_s));
  eventfilter.confevents.data = true;

  i8sak_eventlistener_addreceiver(i8sak, tx_eventcb, &eventfilter, true);

  /* Send the frame */

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      /* Set an application defined handle */

      tx.meta.handle = i8sak->msdu_handle++;

      /* This is a normal transaction, no special handling */

      tx.meta.flags.ackreq = 1;
      tx.meta.flags.usegts = 0;

      if (tx.meta.flags.indirect)
        {
          if (i8sak->verbose)
            {
              printf("i8sak: queuing indirect transaction\n");
              fflush(stdout);
            }
        }
      else
        {
          if (i8sak->verbose)
            {
              printf("i8sak: queuing CSMA transaction\n");
              fflush(stdout);
            }
        }

      tx.meta.ranging = IEEE802154_NON_RANGING;

      tx.meta.srcmode = i8sak->addrmode;
      memcpy(&tx.meta.destaddr, &i8sak->ep_addr, sizeof(struct ieee802154_addr_s));

      /* Each byte is represented by 2 chars */

      tx.length = i8sak->payload_len;
      tx.payload = i8sak->payload;

      ret = write(fd, &tx, sizeof(struct mac802154dev_txframe_s));
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      struct sockaddr_in6 addr;

      addr.sin6_family     = AF_INET6;
      addr.sin6_port       = HTONS(0);
      memset(addr.sin6_addr.s6_addr, 0, sizeof(struct in6_addr));

      if (bind(i8sak->fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in6)) < 0)
        {
          fprintf(stderr, "ERROR: failure to bind sock: %d\n", errno);
          close(fd);
          i8sak_cmd_error(i8sak);
        }

      ret = sendto(fd, i8sak->payload, i8sak->payload_len, 0,
                   (struct sockaddr*)&i8sak->ep_in6addr, sizeof(struct sockaddr_in6));
    }
#endif

  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to transmit packet: errno=%d\n", errno);
      close(fd);
      i8sak_cmd_error(i8sak);
    }

  close(fd);
}

static void tx_eventcb(FAR struct ieee802154_primitive_s *primitive, FAR void *arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;

  if (primitive->u.dataconf.status == IEEE802154_STATUS_SUCCESS)
    {
      printf("i8sak: frame tx success\n");
    }
  else
    {
      printf("i8sak: frame failed to send: %s\n",
             IEEE802154_STATUS_STRING[primitive->u.dataconf.status]);
    }

  sem_post(&i8sak->sigsem);

  i8sak_requestdaemon(i8sak);
}