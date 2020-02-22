/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_acceptassoc.c
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *   Author: Anthony Merlino <anthony@vergeaero.com>
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
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/fs/ioctl.h>

#include "i8sak.h"

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void acceptassoc_eventcb(FAR struct ieee802154_primitive_s *primitive,
                                FAR void *arg);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8_acceptassoc
 *
 * Description :
 *   Start accepting association requests.
 ****************************************************************************/

void i8sak_acceptassoc_cmd(FAR struct i8sak_s *i8sak, int argc,
                           FAR char *argv[])
{
  struct i8sak_eventfilter_s filter;
  bool acceptall = true; /* Start off assuming we are going to allow all
                          * connections */
  int option;
  int optcnt;
  int fd;

  optcnt = 0;
  while ((option = getopt(argc, argv, ":he:")) != ERROR)
    {
      optcnt++;
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Starts accepting association requests\n"
                    "Usage: %s [-h|e <extended address>]\n"
                    "    -h = this help menu\n"
                    "    -e = only accept requests from eaddr\n"
                    "Note: No option accepts all requests\n"
                    , argv[0]);

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;

          case 'e': /* Accept only this extended address */
            i8sak_str2eaddr(optarg, &i8sak->ep_addr.eaddr[0]);
            acceptall = false;
            break;

          case ':':
            fprintf(stderr, "ERROR: missing argument\n");

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */

          case '?':
            fprintf(stderr, "ERROR: invalid argument\n");

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */
        }
    }

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      fd = open(i8sak->ifname, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n", i8sak->ifname, errno);
          i8sak_cmd_error(i8sak);
        }

      ieee802154_setassocpermit(fd, true);
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

      sixlowpan_setassocpermit(fd, i8sak->ifname, true);
    }
#endif

  if (!optcnt)
    {
      i8sak->acceptall = acceptall;
      printf("i8sak: accepting all assoc requests\n");
    }

  i8sak_requestdaemon(i8sak);

  /* Register new callback for receiving the association notifications */

  memset(&filter, 0, sizeof(struct i8sak_eventfilter_s));
  filter.indevents.assoc = true;

  i8sak_eventlistener_addreceiver(i8sak, acceptassoc_eventcb, &filter,
                                  !i8sak->acceptall);
}

static void acceptassoc_eventcb(FAR struct ieee802154_primitive_s *primitive,
                                FAR void *arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;
  struct ieee802154_assoc_resp_s assocresp;

  printf("i8sak: a device is trying to associate\n");

  /* Send a ASSOC.resp primtive to the MAC. Copy the association
   * indication address into the association response primitive
   */

  IEEE802154_EADDRCOPY(assocresp.devaddr, primitive->u.assocind.devaddr);

  /* If the address matches our device, accept the association.
   * Otherwise, reject the association.
   */

  if (i8sak->acceptall ||
      IEEE802154_EADDRCMP(primitive->u.assocind.devaddr, i8sak->ep_addr.eaddr))
    {
      /* Assign the short address */

      IEEE802154_SADDRCOPY(assocresp.assocsaddr, i8sak->next_saddr);

      /* Select a short address for the next association */

      i8sak->next_saddr[0]++;
      if (i8sak->next_saddr[0] == 0)
        {
          /* Handle the carry */

          i8sak->next_saddr[1]++;
          DEBUGASSERT(i8sak->next_saddr[1] != 0);
        }

      assocresp.status = IEEE802154_STATUS_SUCCESS;
      printf("i8sak: accepting association request\n");
    }
  else
    {
      assocresp.status = IEEE802154_STATUS_DENIED;
      printf("i8sak: rejecting association request\n");
    }

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      ieee802154_assoc_resp(i8sak->fd, &assocresp);
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      sixlowpan_assoc_resp(i8sak->fd, i8sak->ifname, &assocresp);
    }
#endif
}
