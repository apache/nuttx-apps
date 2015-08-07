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
#include <fcntl.h>
#include <errno.h>

#include <nuttx/ieee802154/ieee802154.h>

#include <apps/ieee802154/ieee802154.h>

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

struct ieee_frame_s
{
  struct ieee802154_packet_s packet;
  uint8_t                    fc1;
  uint8_t                    fc2;
  uint8_t                    seq;
  uint8_t                    *dpanid;
  uint8_t                    *daddr;
  uint8_t                    daddrlen;
  uint8_t                    *spanid;
  uint8_t                    *saddr;
  uint8_t                    saddrlen;
  uint8_t                    *payload;
};

struct ieee_coord_s
{
  int                  fd;       /* Device handle */
  uint8_t              chan;     /* PAN channel */
  uint16_t             panid;    /* PAN ID */
  uint8_t              nclients; /* Number of coordinated clients */
  struct ieee_frame_s  rxbuf;    /* General rx buffer */
  struct ieee_client_s clients[CONFIG_IEEE802154_COORD_MAXCLIENTS];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static struct ieee_coord_s g_coord;

/****************************************************************************
 * coord_initialize
 ****************************************************************************/

static void coord_initialize(FAR struct ieee_coord_s *coord, FAR char *dev, FAR char *chan, FAR char *panid)
{
  int i;
  coord->nclients = 0;
  for (i = 0; i < CONFIG_IEEE802154_COORD_MAXCLIENTS; i++)
    {
      coord->clients[i].pending.len = 0;
    }
  coord->chan  = strtol(chan , NULL, 0);
  coord->panid = strtol(panid, NULL, 0);

  coord->fd = open(dev, O_RDWR);

}

/****************************************************************************
 * coord_main
 ****************************************************************************/

static int coord_beacon(FAR struct ieee_coord_s *coord)
{
  printf("Beacon!\n");
  return 0;
}

/****************************************************************************
 * coord_main
 ****************************************************************************/

static int coord_data(FAR struct ieee_coord_s *coord)
{
  printf("Data!\n");
  return 0;
}

/****************************************************************************
 * coord_main
 ****************************************************************************/

static int coord_ack(FAR struct ieee_coord_s *coord)
{
  printf("Ack!\n");
  return 0;
}

/****************************************************************************
 * coord_main
 ****************************************************************************/

static int coord_command(FAR struct ieee_coord_s *coord)
{
  printf("Command!\n");
  return 0;
}

/****************************************************************************
 * coord_main
 ****************************************************************************/

static int coord_manage(FAR struct ieee_coord_s *coord)
{
  /* Decode frame type */
  uint8_t ftype, saddr, daddr;
  int index;

  FAR struct ieee_frame_s *rx = &coord->rxbuf;
  int i;

  for(i=0; i<rx->packet.len; i++) printf("%02X", rx->packet.data[i]);
  printf("\n");

  rx->fc1 = rx->packet.data[0];
  rx->fc2 = rx->packet.data[1];
  rx->seq = rx->packet.data[2];

  ftype = rx->fc1 & IEEE802154_FC1_FTYPE;
  daddr = rx->fc2 & IEEE802154_FC2_DADDR;
  saddr = rx->fc2 & IEEE802154_FC2_SADDR;

  index = 3;

  if(daddr == IEEE802154_DADDR_SHORT)
    {
      rx->dpanid = rx->packet.data+index;
      index += 2; /* skip dest pan id */
      rx->daddr  = rx->packet.data+index;
      index += 2; /* skip dest addr */
      rx->daddrlen = 2;
    }
  else if(daddr == IEEE802154_DADDR_EXT)
    {
      rx->dpanid = rx->packet.data+index;
      index += 2; /* skip dest pan id */
      rx->daddr  = rx->packet.data+index;
      index += 8; /* skip dest addr */
      rx->daddrlen = 8;
    }
  else
    {
      rx->dpanid = NULL;
      rx->daddr  = NULL;
      rx->daddrlen = 0;
    }

  if(saddr == IEEE802154_SADDR_SHORT)
    {
      if(rx->fc1 & IEEE802154_FC1_INTRA)
        {
          rx->spanid = rx->dpanid;
        }
      else
        {
          rx->spanid = rx->packet.data+index;
          index += 2; /*skip dest pan id*/
        }
      rx->saddr  = rx->packet.data+index;
      index += 2; /* skip dest addr */
      rx->saddrlen = 2;
    }
  else if(saddr == IEEE802154_SADDR_EXT)
    {
      if(rx->fc1 & IEEE802154_FC1_INTRA)
        {
          rx->spanid = rx->dpanid;
        }
      else
        {
          rx->spanid = rx->packet.data+index;
          index += 2; /*skip dest pan id*/
        }
      rx->saddr  = rx->packet.data+index;
      index += 8; /* skip dest addr */
      rx->saddrlen = 8;
    }
  else
    {
      rx->spanid = NULL;
      rx->saddr  = NULL;
      rx->saddrlen = 0;
    }

  printf("SADDR len %d DADDR len %d\n", rx->saddrlen, rx->daddrlen);
  switch(ftype)
    {
      case IEEE802154_FRAME_BEACON : coord_beacon (coord); break;
      case IEEE802154_FRAME_DATA   : coord_data   (coord); break;
      case IEEE802154_FRAME_ACK    : coord_ack    (coord); break;
      case IEEE802154_FRAME_COMMAND: coord_command(coord); break;
      default                      : fprintf(stderr, "unknown frame type!");
    }
  return 0;
}

/****************************************************************************
 * coord_loop
 ****************************************************************************/

static int coord_loop(FAR struct ieee_coord_s *coord)
{
  int ret = 1;
  FAR struct ieee_frame_s *rx = &coord->rxbuf;

  printf("starting on chan %d, panid %04X\n", coord->chan, coord->panid);

  ieee802154_setchan (coord->fd, coord->chan );
  ieee802154_setpanid(coord->fd, coord->panid);
  //ieee802154_setpromisc(coord->fd, TRUE);

  while(ret > 0)
    {
      ret = read(coord->fd, &rx->packet, sizeof(struct ieee802154_packet_s));
      if(ret > 0)
        {
          coord_manage(coord);
        }
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
  printf("IEEE 802.15.4 Coordinator start\n");

  if (argc<4)
    {
      printf("coord <dev> <chan> [0x]<panid>\n");
      return ERROR;
    }


  coord_initialize(&g_coord, argv[1], argv[2], argv[3]);
  if(g_coord.fd<0)
    {
      fprintf(stderr, "cannot open %s, errno=%d\n", argv[1], errno);
      exit(ERROR);
    }

  return coord_loop(&g_coord);
}
