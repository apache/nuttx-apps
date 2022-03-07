/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_assoc.c
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "wireless/ieee802154.h"

#include "i8sak.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void assoc_eventcb(FAR struct ieee802154_primitive_s *primitive,
                          FAR void *arg);

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
  struct i8sak_eventfilter_s filter;
  FAR struct ieee802154_pandesc_s *pandesc;
  struct ieee802154_set_req_s setreq;
  bool retry     = false;
  int maxretries = 0;
  int retrycnt;
  int fd = 0;
  int option;
  int optcnt;
  int ret;
  uint8_t resindex;

  setreq.attr = IEEE802154_ATTR_MAC_RESPONSE_WAIT_TIME;
  setreq.attrval.mac.resp_waittime = 32;

  optcnt = 0;
  while ((option = getopt(argc, argv, ":hp:e:s:w:r:t:")) != ERROR)
    {
      optcnt++;
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Requests association with endpoint\n"
                    "Usage: %s [-h] [-w <count>\n"
                    "    -h = this help menu\n"
                    "    -p = coordinator PAN ID\n"
                    "    -e = coordinator ext address\n"
                    "    -s = coordinator short address\n"
                    "    -w = wait and retry on failure\n"
                    "    -r = use scan result index\n"
                    "    -t = response wait time\n"
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
            memcpy(&i8sak->ep_addr, &pandesc->coordaddr,
                   sizeof(struct ieee802154_addr_s));
            break;

          case 'p':

            /* Parse short address and put it into the i8sak instance */

            i8sak_str2panid(optarg, i8sak->ep_addr.panid);
            break;

          case 's':

            /* Parse short address and put it into the i8sak instance */

            i8sak_str2saddr(optarg, i8sak->ep_addr.saddr);
            i8sak->ep_addr.mode = IEEE802154_ADDRMODE_SHORT;
            break;

          case 'e':

            /* Parse extended address and put it into the i8sak instance */

            i8sak_str2eaddr(optarg, i8sak->ep_addr.eaddr);
            i8sak->ep_addr.mode = IEEE802154_ADDRMODE_EXTENDED;
            break;

          case 't':

            /* Parse wait time and set the parameter in the request */

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

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      fd = open(i8sak->ifname, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n",
                  i8sak->ifname, errno);
          i8sak_cmd_error(i8sak);
        }

      ieee802154_set_req(fd, &setreq);
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

      sixlowpan_set_req(fd, i8sak->ifname, &setreq);
    }
#endif

  i8sak_requestdaemon(i8sak);

  /* Register new callback for receiving the association notifications. */

  memset(&filter, 0, sizeof(struct i8sak_eventfilter_s));
  filter.confevents.assoc = true;

  i8sak_eventlistener_addreceiver(i8sak, assoc_eventcb, &filter, false);

  /* Loop for the specified retry count if the association fails.
   */

  retrycnt = 0;
  for (; ; )
    {
      printf("i8sak: issuing ASSOC. request %d\n", retrycnt + 1);

      /* Issue association request */

      assocreq.chan = i8sak->chan;
      assocreq.chpage = i8sak->chpage;

      memcpy(&assocreq.coordaddr, &i8sak->ep_addr,
             sizeof(struct ieee802154_addr_s));

      assocreq.capabilities.devtype = 0;
      assocreq.capabilities.powersource = 1;
      assocreq.capabilities.rxonidle = 1;
      assocreq.capabilities.security = 0;
      assocreq.capabilities.allocaddr = 1;

      if (i8sak->mode == I8SAK_MODE_CHAR)
        {
          ieee802154_assoc_req(fd, &assocreq);
        }
#ifdef CONFIG_NET_6LOWPAN
      else if (i8sak->mode == I8SAK_MODE_NETIF)
        {
          sixlowpan_assoc_req(fd, i8sak->ifname, &assocreq);
        }
#endif

      /* Wait for the assocconf event */

      i8sak->assoc  = true;
      i8sak->result = -EBUSY;

      ret = sem_wait(&i8sak->sigsem);
      if (ret != OK)
        {
          i8sak->assoc = false;
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

  i8sak_eventlistener_removereceiver(i8sak, assoc_eventcb);
  i8sak_releasedaemon(i8sak);
  close(fd);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void assoc_eventcb(FAR struct ieee802154_primitive_s *primitive,
                          FAR void *arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;

  if (primitive->u.assocconf.status == IEEE802154_STATUS_SUCCESS)
    {
      printf("i8sak: ASSOC.request succeeded\n");
      i8sak->result = OK;
    }
  else
    {
      printf("i8sak: ASSOC.request failed: %s\n",
             IEEE802154_STATUS_STRING[primitive->u.assocconf.status]);
      i8sak->result = -EAGAIN;
    }

  if (i8sak->assoc)
    {
      i8sak->assoc = false;
      sem_post(&i8sak->sigsem);
    }
}
