/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_get.c
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
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "wireless/ieee802154.h"

#include "i8sak.h"

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

union i8sak_get_u
{
  union ieee802154_attr_u attr;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_get_cmd
 *
 * Description :
 *   Get various parameters. These can be either i8sak app level options
 *   or attributes fetched via IOCTL to the network/MAC/or radio level.
 ****************************************************************************/

void i8sak_get_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  union i8sak_get_u u;
  int option;
  int fd = 0;
  int argind;

  argind = 1;
  while ((option = getopt(argc, argv, ":h")) != ERROR)
    {
      argind++;
      switch (option)
        {
          case 'h':
            fprintf(stderr, "Gets various parameters and attributes\n"
                    "Usage: %s [-h] parameter\n"
                    "    -h = this help menu\n"
                    "\n"
                    "Parameters:\n"
                    "    chan = RF channel\n"
                    "    panid = PAN Identifier\n"
                    "    saddr = this node's short address\n"
                    "    eaddr = this node's extended address\n"
                    "    addrmode = source address mode\n"
                    "    coordsaddr = associated coordinator's short addr.\n"
                    "    coordeaddr = associated coordinator's ext. addr.\n"
                    "    ep_saddr = i8sak endpoint short address\n"
                    "    ep_eaddr = i8sak endpoint extended address\n"
                    "    ep_addrmode = destination address mode"
                    "    rxonidle = Receiver on when idle\n"
                    "    txpwr = Transmit power\n"
                    "    maxretries = macMaxFrameRetries\n"
                    "    promisc = Promiscuous Mode\n"
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

  /* Make sure that there is another argument */

  if (argc <= argind)
    {
      fprintf(stderr, "ERROR: Must include an attribute to get\n");
      i8sak_cmd_error(i8sak); /* This exits for us */
    }

  /* Check for i8sak level options. Not dependent on opening file/socket */

  if (strcmp(argv[argind], "addrmode") == 0)
    {
      switch (i8sak->addrmode)
        {
          case IEEE802154_ADDRMODE_NONE:
            printf("i8sak: Addressing Mode: None\n");
            break;
          case IEEE802154_ADDRMODE_EXTENDED:
            printf("i8sak: Addressing Mode: Extended\n");
            break;
          case IEEE802154_ADDRMODE_SHORT:
            printf("i8sak: Addressing Mode: Short\n");
            break;
          default:
            printf("i8sak: Addressing Mode: Unknown\n");
            break;
        }
    }
  else if (strcmp(argv[argind], "ep_saddr") == 0)
    {
      printf("i8sak: Endpoint Short Address: "
             PRINTF_FORMAT_SADDR(i8sak->ep_addr.saddr));
    }
  else if (strcmp(argv[argind], "ep_eaddr") == 0)
    {
      printf("i8sak: Endpoint Extended Address: "
             PRINTF_FORMAT_EADDR(i8sak->ep_addr.eaddr));
    }
  else if (strcmp(argv[argind], "ep_addrmode") == 0)
    {
      switch (i8sak->ep_addr.mode)
        {
          case IEEE802154_ADDRMODE_NONE:
            printf("i8sak: Addressing Mode: None\n");
            break;
          case IEEE802154_ADDRMODE_EXTENDED:
            printf("i8sak: Addressing Mode: Extended\n");
            break;
          case IEEE802154_ADDRMODE_SHORT:
            printf("i8sak: Addressing Mode: Short\n");
            break;
          default:
            printf("i8sak: Addressing Mode: Unknown\n");
            break;
        }
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (strcmp(argv[argind], "ep_port") == 0)
    {
      printf("i8sak: Endpoint Port: %d\n",
              NTOHS(i8sak->ep_in6addr.sin6_port));
    }
  else if (strcmp(argv[argind], "snifferport") == 0)
    {
      printf("i8sak: Sniffer Port: %d\n", NTOHS(i8sak->snifferport));
    }
#endif

  /* Check for parameters that are attributes requiring file access */

  else
    {
      if (i8sak->mode == I8SAK_MODE_CHAR)
        {
          fd = open(i8sak->ifname, O_RDWR);
          if (fd < 0)
            {
              fprintf(stderr, "ERROR: cannot open %s, errno=%d\n",
                      i8sak->ifname, errno);
              i8sak_cmd_error(i8sak);
            }

          if (strcmp(argv[argind], "chan") == 0)
            {
              ieee802154_getchan(fd, &u.attr.phy.chan);
              printf("i8sak: Channel: %d\n", (int)u.attr.phy.chan);
            }
          else if (strcmp(argv[argind], "panid") == 0)
            {
              ieee802154_getpanid(fd, u.attr.mac.panid);
              printf("i8sak: PAN ID: "
                     PRINTF_FORMAT_PANID(u.attr.mac.panid));
            }
          else if (strcmp(argv[argind], "saddr") == 0)
            {
              ieee802154_getsaddr(fd, u.attr.mac.saddr);
              printf("i8sak: Short Address: "
                     PRINTF_FORMAT_SADDR(u.attr.mac.saddr));
            }
          else if (strcmp(argv[argind], "eaddr") == 0)
            {
              ieee802154_geteaddr(fd, u.attr.mac.eaddr);
              printf("i8sak: Extended Address: "
                     PRINTF_FORMAT_EADDR(u.attr.mac.eaddr));
            }

          else if (strcmp(argv[argind], "coordsaddr") == 0)
            {
              ieee802154_getcoordsaddr(fd, u.attr.mac.saddr);
              printf("i8sak: Coordinator Short Address: "
                     PRINTF_FORMAT_SADDR(u.attr.mac.coordsaddr));
            }
          else if (strcmp(argv[argind], "coordeaddr") == 0)
            {
              ieee802154_getcoordeaddr(fd, u.attr.mac.coordeaddr);
              printf("i8sak: Coordinator Extended Address: "
                     PRINTF_FORMAT_EADDR(u.attr.mac.coordeaddr));
            }
          else if (strcmp(argv[argind], "rxonidle") == 0)
            {
              ieee802154_getrxonidle(fd, &u.attr.mac.rxonidle);
              if (u.attr.mac.rxonidle)
                {
                  printf("i8sak: Receive on Idle: true\n");
                }
              else
                {
                  printf("i8sak: Receive on Idle: false\n");
                }
            }
          else if (strcmp(argv[argind], "txpwr") == 0)
            {
              ieee802154_gettxpwr(fd, &u.attr.phy.txpwr);
              printf("i8sak: Transmit Power: %d\n", (int)u.attr.phy.txpwr);
            }
          else if (strcmp(argv[argind], "maxretries") == 0)
            {
              ieee802154_getmaxretries(fd, &u.attr.mac.max_retries);
              printf("i8sak: Max Frame Retries: %d\n",
                      (int)u.attr.mac.max_retries);
            }
          else if (strcmp(argv[argind], "promisc") == 0)
            {
              ieee802154_getpromisc(fd, &u.attr.mac.promisc_mode);
              if (u.attr.mac.promisc_mode)
                {
                  printf("i8sak: Promiscuous Mode: true\n");
                }
              else
                {
                  printf("i8sak: Promiscuous Mode: false\n");
                }
            }
          else
            {
              fprintf(stderr,
                      "ERROR: unsupported parameter: %s\n", argv[argind]);
            }
        }
#ifdef CONFIG_NET_6LOWPAN
      else if (i8sak->mode == I8SAK_MODE_NETIF)
        {
          fd = socket(PF_INET6, SOCK_DGRAM, 0);
          if (fd < 0)
            {
              fprintf(stderr,
                      "ERROR: failed to open socket, errno=%d\n", errno);
              i8sak_cmd_error(i8sak);
            }

          if (strcmp(argv[argind], "chan") == 0)
            {
              sixlowpan_getchan(fd, i8sak->ifname, &u.attr.phy.chan);
              printf("i8sak: Channel: %d\n", (int)u.attr.phy.chan);
            }
          else if (strcmp(argv[argind], "panid") == 0)
            {
              sixlowpan_getpanid(fd, i8sak->ifname, u.attr.mac.panid);
              printf("i8sak: PAN ID: "
                     PRINTF_FORMAT_PANID(u.attr.mac.panid));
            }
          else if (strcmp(argv[argind], "saddr") == 0)
            {
              sixlowpan_getsaddr(fd, i8sak->ifname, u.attr.mac.saddr);
              printf("i8sak: Short Address: "
                     PRINTF_FORMAT_SADDR(u.attr.mac.saddr));
            }
          else if (strcmp(argv[argind], "eaddr") == 0)
            {
              sixlowpan_geteaddr(fd, i8sak->ifname, u.attr.mac.eaddr);
              printf("i8sak: Extended Address: "
                     PRINTF_FORMAT_EADDR(u.attr.mac.eaddr));
            }

          else if (strcmp(argv[argind], "coordsaddr") == 0)
            {
              sixlowpan_getcoordsaddr(fd, i8sak->ifname, u.attr.mac.saddr);
              printf("i8sak: Coordinator Short Address: "
                     PRINTF_FORMAT_SADDR(u.attr.mac.coordsaddr));
            }
          else if (strcmp(argv[argind], "coordeaddr") == 0)
            {
              sixlowpan_getcoordeaddr(fd, i8sak->ifname,
                                      u.attr.mac.coordeaddr);
              printf("i8sak: Coordinator Extended Address: "
                     PRINTF_FORMAT_EADDR(u.attr.mac.coordeaddr));
            }
          else if (strcmp(argv[argind], "rxonidle") == 0)
            {
              sixlowpan_getrxonidle(fd, i8sak->ifname, &u.attr.mac.rxonidle);
              if (u.attr.mac.rxonidle)
                {
                  printf("i8sak: Receive on Idle: true\n");
                }
              else
                {
                  printf("i8sak: Receive on Idle: false\n");
                }
            }
          else if (strcmp(argv[argind], "txpwr") == 0)
            {
              sixlowpan_gettxpwr(fd, i8sak->ifname, &u.attr.phy.txpwr);
              printf("i8sak: Transmit Power: %d\n", (int)u.attr.phy.txpwr);
            }
          else if (strcmp(argv[argind], "maxretries") == 0)
            {
              sixlowpan_getmaxretries(fd, i8sak->ifname,
                                      &u.attr.mac.max_retries);
              printf("i8sak: Max Retries: %d\n",
                     (int)u.attr.mac.max_retries);
            }
          else if (strcmp(argv[argind], "promisc") == 0)
            {
              sixlowpan_getpromisc(fd, i8sak->ifname,
                                   &u.attr.mac.promisc_mode);
              if (u.attr.mac.promisc_mode)
                {
                  printf("i8sak: Promiscuous Mode: true\n");
                }
              else
                {
                  printf("i8sak: Promiscuous Mode: false\n");
                }
            }
          else
            {
              fprintf(stderr, "ERROR: unsupported parameter: %s\n",
                      argv[argind]);
            }
        }
#endif
    }

  close(fd);
}
