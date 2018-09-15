/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_set.c
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

union i8sak_set_u
{
  union ieee802154_attr_u attr;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_set_cmd
 *
 * Description :
 *   Set various parameters. These can be either i8sak app level options
 *   or attributes fetched via IOCTL to the network/MAC/or radio level.
 ****************************************************************************/

void i8sak_set_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  union i8sak_set_u u;
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
                    "    chan 0-255 = RF channel (only some channels valid per radio)\n"
                    "    panid xx:xx = PAN Identifier\n"
                    "    saddr xx:xx = this node's short address\n"
                    "    eaddr xx:xx:xx:xx:xx:xx:xx:xx = this node's extended address\n"
                    "    addrmode s|e = source addressing mode\n"
                    "    ep_saddr xx:xx = i8sak endpoint short address\n"
                    "    ep_eaddr xx:xx:xx:xx:xx:xx:xx:xx = i8sak endpoint extended address\n"
                    "    ep_addrmode s|e = destination addressing mode\n"
                    "    ep_port 1-65535 = port to send to\n"
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

  if (argc < argind + 2)
    {
      fprintf(stderr, "ERROR: Too few arguments\n");
      i8sak_cmd_error(i8sak);
    }

  /* Check for i8sak level options. Not dependent on opening file/socket */

  if (strcmp(argv[argind], "addrmode") == 0)
    {
      if (*argv[argind + 1] == 's')
        {
          i8sak->addrmode = IEEE802154_ADDRMODE_SHORT;
        }
      else if (*argv[argind + 1] == 'e')
        {
          i8sak->addrmode = IEEE802154_ADDRMODE_EXTENDED;
        }
      else
        {
          fprintf(stderr, "ERROR: invalid mode. Options: s|e\n");
          i8sak_cmd_error(i8sak);
        }
    }
  else if (strcmp(argv[argind], "ep_saddr") == 0)
    {
      i8sak_str2saddr(argv[argind + 1], i8sak->ep_addr.saddr);

#ifdef CONFIG_NET_6LOWPAN
      if (i8sak->ep_addr.mode == IEEE802154_ADDRMODE_SHORT)
        {
          i8sak_update_ep_ip(i8sak);
        }
#endif
    }
  else if (strcmp(argv[argind], "ep_eaddr") == 0)
    {
      i8sak_str2eaddr(argv[argind + 1], i8sak->ep_addr.eaddr);

#ifdef CONFIG_NET_6LOWPAN
      if (i8sak->ep_addr.mode == IEEE802154_ADDRMODE_EXTENDED)
        {
          i8sak_update_ep_ip(i8sak);
        }
#endif
    }
  else if (strcmp(argv[argind], "ep_addrmode") == 0)
    {
      if (*argv[argind + 1] == 's')
        {
          i8sak->ep_addr.mode = IEEE802154_ADDRMODE_SHORT;
        }
      else if (*argv[argind + 1] == 'e')
        {
          i8sak->ep_addr.mode = IEEE802154_ADDRMODE_EXTENDED;
        }
      else
        {
          fprintf(stderr, "ERROR: invalid mode. Options: s|e\n");
          i8sak_cmd_error(i8sak);
        }

#ifdef CONFIG_NET_6LOWPAN
      i8sak_update_ep_ip(i8sak);
#endif
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (strcmp(argv[argind], "ep_port") == 0)
    {
      i8sak->ep_in6addr.sin6_port = HTONS(i8sak_str2luint16(argv[argind + 1]));
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
              u.attr.phy.chan = i8sak_str2luint8(argv[argind + 1]);
              ieee802154_setchan(fd, u.attr.phy.chan);
              i8sak->chan = u.attr.phy.chan;
            }
          else if (strcmp(argv[argind], "panid") == 0)
            {
              i8sak_str2panid(argv[argind + 1], u.attr.mac.panid);
              ieee802154_setpanid(fd, u.attr.mac.panid);
            }
          else if (strcmp(argv[argind], "saddr") == 0)
            {
              i8sak_str2saddr(argv[argind + 1], u.attr.mac.saddr);
              ieee802154_setsaddr(fd, u.attr.mac.saddr);
            }
          else if (strcmp(argv[argind], "eaddr") == 0)
            {
              i8sak_str2eaddr(argv[argind + 1], u.attr.mac.eaddr);
              ieee802154_seteaddr(fd, u.attr.mac.eaddr);
            }
          else if (strcmp(argv[argind], "rxonidle") == 0)
            {
              ieee802154_setrxonidle(fd, i8sak_str2bool(argv[argind + 1]));
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
              fprintf(stderr,  "ERROR: failed to open socket, errno=%d\n", errno);
              i8sak_cmd_error(i8sak);
            }

          if (strcmp(argv[argind], "chan") == 0)
            {
              u.attr.phy.chan = i8sak_str2luint8(argv[argind + 1]);
              sixlowpan_setchan(fd, i8sak->ifname, u.attr.phy.chan);
              i8sak->chan = u.attr.phy.chan;
            }
          else if (strcmp(argv[argind], "panid") == 0)
            {
              i8sak_str2panid(argv[argind + 1], u.attr.mac.panid);
              sixlowpan_setpanid(fd, i8sak->ifname, u.attr.mac.panid);
            }
          else if (strcmp(argv[argind], "saddr") == 0)
            {
              i8sak_str2saddr(argv[argind + 1], u.attr.mac.saddr);
              sixlowpan_setsaddr(fd, i8sak->ifname, u.attr.mac.saddr);
            }
          else if (strcmp(argv[argind], "eaddr") == 0)
            {
              i8sak_str2eaddr(argv[argind + 1], u.attr.mac.eaddr);
              sixlowpan_seteaddr(fd, i8sak->ifname, u.attr.mac.eaddr);
            }
          else if (strcmp(argv[argind], "rxonidle") == 0)
            {
              sixlowpan_setrxonidle(fd, i8sak->ifname, i8sak_str2bool(argv[argind + 1]));
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
