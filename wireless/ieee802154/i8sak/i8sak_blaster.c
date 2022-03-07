/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_blaster.c
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
#include <arpa/inet.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include <nuttx/wireless/ieee802154/ieee802154_device.h>

#include "wireless/ieee802154.h"

#include "i8sak.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static inline void i8sak_blaster_start(FAR struct i8sak_s *i8sak);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void i8sak_blaster_start(FAR struct i8sak_s *i8sak)
{
  if (!i8sak->blasterenabled)
    {
      i8sak->startblaster = true;

      /* Signal the daemon to start running */

      printf("i8sak: starting blaster\n");
      sem_post(&i8sak->updatesem);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_blaster
 *
 * Description :
 *   Continuously transmit a packet
 ****************************************************************************/

void i8sak_blaster_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  int option;

  i8sak_requestdaemon(i8sak);

  if (argc < 2)
    {
      i8sak_blaster_start(i8sak);
    }

  while ((option = getopt(argc, argv, "hqp:f:")) != ERROR)
    {
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Blasts frames\n"
                    "Usage: %s [-h|q|f <hex-payload>|p <period_ms>]\n"
                    "    -h = this help menu\n"
                    "    -q = quit blasting\n"
                    "    -f = set frame (and starts blaster)\n"
                    "    -p = set period (and start blaster)\n"
                    "Note: No option starts blaster with defaults\n"
                    , argv[0]);

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;
          case 'q': /* Quit blaster */
            i8sak->blasterenabled = false;
            break;

          case 'p': /* Inline change blaster period */
            i8sak->blasterperiod = atoi(optarg);
            i8sak_blaster_start(i8sak);
            break;

          case 'f': /* Inline change blaster frame */
            i8sak->payload_len = i8sak_str2payload(optarg,
                                                   &i8sak->payload[0]);
            i8sak_blaster_start(i8sak);
            break;

          case ':':
            fprintf(stderr, "ERROR: missing argument\n");

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */
            break;

          case '?':
            fprintf(stderr, "ERROR: unknown argument\n");

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */
        }
    }
}

/****************************************************************************
 * Name : i8sak_blaster_thread
 *
 * Description :
 *   Send frames periodically
 ****************************************************************************/

pthread_addr_t i8sak_blaster_thread(pthread_addr_t arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;
  struct mac802154dev_txframe_s tx;

#ifdef CONFIG_NET_6LOWPAN
  if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      if (bind(i8sak->fd, (struct sockaddr *)&i8sak->ep_in6addr,
               sizeof(struct sockaddr_in6)) < 0)
        {
          fprintf(stderr, "ERROR: failure to bind sock: %d\n", errno);
          exit(1);
        }
    }
#endif

  while (i8sak->blasterenabled)
    {
      usleep(i8sak->blasterperiod * 1000);

      if (i8sak->mode == I8SAK_MODE_CHAR)
        {
          /* Set an application defined handle */

          tx.meta.handle = i8sak->msdu_handle++;

          /* This is a normal transaction, no special handling */

          tx.meta.flags.ackreq = 1;
          tx.meta.flags.usegts = 0;
          tx.meta.ranging = IEEE802154_NON_RANGING;

          tx.meta.srcmode = i8sak->addrmode;
          memcpy(&tx.meta.destaddr, &i8sak->ep_addr,
                 sizeof(struct ieee802154_addr_s));

          /* Each byte is represented by 2 chars */

          tx.length = i8sak->payload_len;
          tx.payload = &i8sak->payload[0];

          write(i8sak->fd, &tx, sizeof(struct mac802154dev_txframe_s));
        }
#ifdef CONFIG_NET_6LOWPAN
      else if (i8sak->mode == I8SAK_MODE_NETIF)
        {
          sendto(i8sak->fd, i8sak->payload, i8sak->payload_len, 0,
                 (struct sockaddr *)&i8sak->ep_in6addr,
                 sizeof(struct sockaddr_in6));
        }
#endif
    }

  return NULL;
}
