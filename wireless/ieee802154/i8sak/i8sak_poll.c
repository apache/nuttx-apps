/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_poll.c
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
 * Private Function Prototypes
 ****************************************************************************/

static void poll_eventcb(FAR struct ieee802154_primitive_s *primitive,
                         FAR void *arg);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_poll_cmd
 *
 * Description :
 *   Try and extract data from the coordinator
 ****************************************************************************/

void i8sak_poll_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  struct ieee802154_poll_req_s pollreq;
  struct i8sak_eventfilter_s eventfilter;
  int option;
  int fd = 0;
  int ret;

  while ((option = getopt(argc, argv, ":h")) != ERROR)
    {
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Polls coordinator for data\n"
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

  i8sak_requestdaemon(i8sak);

  /* Register new callback for receiving the association notifications */

  memset(&eventfilter, 0, sizeof(struct i8sak_eventfilter_s));
  eventfilter.confevents.poll = true;

  i8sak_eventlistener_addreceiver(i8sak, poll_eventcb, &eventfilter, true);

  memcpy(&pollreq.coordaddr, &i8sak->ep_addr,
         sizeof(struct ieee802154_addr_s));

  if (pollreq.coordaddr.mode == IEEE802154_ADDRMODE_SHORT)
    {
      printf("i8sak: Polling " PRINTF_FORMAT_SADDR(pollreq.coordaddr.saddr));
    }
  else
    {
      printf("i8sak: Polling " PRINTF_FORMAT_EADDR(pollreq.coordaddr.eaddr));
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

      ieee802154_poll_req(fd, &pollreq);
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

      sixlowpan_poll_req(fd, i8sak->ifname, &pollreq);
    }
#endif

  close(fd);

  /* Wait here, the event listener will notify us if the correct event
   * occurs
   */

  ret = sem_wait(&i8sak->sigsem);
  if (ret != OK)
    {
      printf("i8sak: poll cancelled\n");
      i8sak_cmd_error(i8sak);
    }

  i8sak_releasedaemon(i8sak);
}

/****************************************************************************
 * Private Function
 ****************************************************************************/

static void poll_eventcb(FAR struct ieee802154_primitive_s *primitive,
                         FAR void *arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;

  if (primitive->u.pollconf.status == IEEE802154_STATUS_SUCCESS)
    {
      printf("i8sak: POLL.request succeeded\n");
    }
  else
    {
      printf("i8sak: POLL.request failed: %s\n",
             IEEE802154_STATUS_STRING[primitive->u.pollconf.status]);
    }

  sem_post(&i8sak->sigsem);
}
