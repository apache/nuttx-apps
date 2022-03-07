/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_poll.c
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
