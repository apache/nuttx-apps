/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_get.c
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
      switch (option)
        {
          argind++;
          case 'h':
            fprintf(stderr, "Gets various parameters and attributes\n"
                    "Usage: %s [-h] parameter\n"
                    "    -h = this help menu\n"
                    " \n"
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
      printf("i8sak: Endpoint Port: %d\n", NTOHS(i8sak->ep_in6addr.sin6_port));
    }
  else if (strcmp(argv[argind], "snifferport") == 0)
    {
      printf("i8sak: Sniffer Port: %d\n", NTOHS(i8sak->snifferport));
    }
#endif

  /* Check for paramters that are attributes requiring file access */

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
          else
            {
              fprintf(stderr, "ERROR: unsupported parameter: %s\n", argv[argind]);
            }
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
              sixlowpan_getcoordeaddr(fd, i8sak->ifname, u.attr.mac.coordeaddr);
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
          else
            {
              fprintf(stderr, "ERROR: unsupported parameter: %s\n", argv[argind]);
            }
        }
#endif
    }

  close(fd);
}
