/****************************************************************************
 * apps/ieee802154/coord/coord_main.c
 *
 *   Copyright (C) 2014-2015 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2014-2015 Sebastien Lorquet. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
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

/* Application description
 *
 * The coordinator is a central node in any IEEE 802.15.4 wireless network.
 * It listens for clients and dispatches short addresses. It stores data from
 * source clients and makes it available to destination clients.
 * Also, in beacon enabled networks, it broadcasts beacons frames and 
 * manages guaranteed time slots.
 * On non-beacon enabled networks, it sends a beacon only when a beacon
 * request is received.
 *
 * This coordinator is generic. It does not interpret the contents of data
 * frames. It only interprets command frames to manage client associations
 * and data dispatch.
 *
 * There is no support for mesh networking (coord/coord traffic and packet
 * forwarding).
 *
 * There is no support either for data security (yet).
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>

#include <nuttx/ieee802154/ieee802154.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/
#ifndef CONFIG_IEEE802154_COORD_MAXCLIENTS
#define CONFIG_IEEE802154_COORD_MAXCLIENTS 8
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ieee_client_s
{
  uint8_t                    eaddr[8]; /* client extended address */
  uint8_t                    saddr[2]; /* client short address */
  struct ieee802154_packet_s pending;  /* pending packet for device */
};

struct ieee_coord_s
{
  int                        fd;       /* Device handle */
  uint8_t                    chan;     /* PAN channel */
  uint8_t                    panid[2]; /* PAN ID */
  uint8_t                    nclients; /* Number of coordinated clients */
  struct ieee802154_packet_s rxbuf;    /* General rx buffer */
  struct ieee_client_s       clients[CONFIG_IEEE802154_COORD_MAXCLIENTS];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static struct ieee_coord_s g_coord;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void coord_initialize(struct ieee_coord_s *coord)
{
  int i;
  coord->nclients = 0;
  for (i = 0; i < CONFIG_IEEE802154_COORD_MAXCLIENTS; i++)
    {
      coord->clients[i].pending.len = 0;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static int coord_loop(FAR struct ieee_coord_s *coord, FAR char *dev, int chan, int panid)
{
  uint8_t pan[2];
  int ret = OK;

  pan[0] = panid >> 8;
  pan[1] = panid &  0xFF;

  printf("starting %s on chan %d, panid %02X%02X", dev, chan, pan[0], pan[1]);

  while(ret == OK)
    {
      ret = read(coord->fd, &coord->rxbuf, sizeof(struct ieee802154_packet_s));

    }
  return OK;
}

/****************************************************************************
 * coord_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int coord_main(int argc, FAR char *argv[])
#endif
{
  int chan;
  int panid;

  coord_initialize(&g_coord);
  printf("IEEE 802.15.4 Coordinator start\n");

  if (argc<4)
    {
      printf("coord <dev> <chan> <panid>");
      return ERROR;
    }

  chan  = strtol(argv[2], NULL, 0);
  panid = strtol(argv[3], NULL, 0);

  return coord_loop(&g_coord, argv[1], chan, panid);
}
