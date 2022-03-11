/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_sniffer.c
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
#include <nuttx/net/sixlowpan.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include <nuttx/wireless/ieee802154/ieee802154_device.h>

#include "wireless/ieee802154.h"

#include "i8sak.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_sniffer_cmd
 *
 * Description :
 *   Sniff for frames (Promiscuous mode)
 ****************************************************************************/

void i8sak_sniffer_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  int option;
  int fd = 0;
  int ret;

  ret = OK;
  while ((option = getopt(argc, argv, ":hd:")) != ERROR)
    {
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Starts sniffer\n"
                    "Usage: %s [-h|d]\n"
                    "    -h = this help menu\n"
                    , argv[0]);

            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;

          case 'd':
#ifdef CONFIG_NET_6LOWPAN
            i8sak->snifferport = HTONS(atoi(optarg));
#endif
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

  if (ret != OK)
    {
      i8sak_cmd_error(i8sak);
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

      /* Enable promiscuous mode */

      if (i8sak->verbose)
        {
          printf("i8sak: turning on promiscuous mode.\n");
        }

      ieee802154_setpromisc(fd, true);

      /* Make sure receiver is always on while idle */

      if (i8sak->verbose)
        {
          printf("i8sak: setting receiveonidle.\n");
        }

      ieee802154_setrxonidle(fd, true);
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

      /* Make sure receiver is always on while idle */

      if (i8sak->verbose)
        {
          printf("i8sak: setting receiveonidle.\n");
        }

      sixlowpan_setrxonidle(fd, i8sak->ifname, true);
    }
#endif

  close(fd);

  i8sak_requestdaemon(i8sak);

  /* Start the sniffer thread. We use the existing daemon thread so that this
   * thread belongs to the daemon task group.
   */

  if (!i8sak->snifferenabled)
    {
      i8sak->startsniffer = true;

      /* Signal the daemon to start running the sniffer thread */

      printf("i8sak: starting sniffer\n");
      sem_post(&i8sak->updatesem);
    }
}

/****************************************************************************
 * Name : i8sak_sniffer_thread
 *
 * Description :
 *   Listen for frames.
 ****************************************************************************/

pthread_addr_t i8sak_sniffer_thread(pthread_addr_t arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;
  struct mac802154dev_rxframe_s frame;
#ifdef CONFIG_NET_6LOWPAN
  uint8_t buf[IEEE802154_MAX_MAC_PAYLOAD_SIZE];
#endif
  int ret;
  int i;

#ifdef CONFIG_NET_6LOWPAN
  if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      struct sockaddr_in6 addr;
      socklen_t addrlen;

      /* Bind the socket to a local address */

      addr.sin6_family  = AF_INET6;
      addr.sin6_port    = i8sak->snifferport;
      memset(&addr.sin6_addr, 0, sizeof(struct in6_addr));
      addrlen           = sizeof(struct sockaddr_in6);

      if (bind(i8sak->fd, (struct sockaddr *)&addr, addrlen) < 0)
        {
          fprintf(stderr, "ERROR: failure to bind sock: %d\n", errno);
          exit(1);
        }
    }
#endif

  while (i8sak->snifferenabled)
    {
      if (i8sak->mode == I8SAK_MODE_CHAR)
        {
          ret = read(i8sak->fd, &frame,
                     sizeof(struct mac802154dev_rxframe_s));
          if (ret < 0)
            {
              continue;
            }

          printf("i8sak: sniffer: Data Received:\n");
          for (i = 0; i < frame.length; i++)
            {
              printf("%02X", frame.payload[i]);
            }

          printf("\n");
          fflush(stdout);
        }
#ifdef CONFIG_NET_6LOWPAN
      else if (i8sak->mode == I8SAK_MODE_NETIF)
        {
          ret = recv(i8sak->fd, buf, sizeof(buf), 0);
          if (ret < 0)
            {
              continue;
            }

          printf("i8sak: sniffer: Data Received:\n");
          for (i = 0; i < ret; i++)
            {
              printf("%02X", buf[i]);
            }

          printf("\n");
          fflush(stdout);
        }
#endif
    }

  return NULL;
}
