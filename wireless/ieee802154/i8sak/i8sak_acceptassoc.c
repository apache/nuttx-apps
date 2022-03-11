/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_acceptassoc.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n",
                  i8sak->ifname, errno);
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
      IEEE802154_EADDRCMP(primitive->u.assocind.devaddr,
                          i8sak->ep_addr.eaddr))
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
