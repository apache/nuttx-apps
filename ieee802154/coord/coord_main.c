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
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

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
  struct ieee802154_addr_s   dest;
  struct ieee802154_addr_s   src;
  uint8_t                    seq;
  uint8_t                    plen;
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
 * coord_beacon
 ****************************************************************************/

static int coord_beacon(FAR struct ieee_coord_s *coord)
{
  printf("Beacon!\n");
  return 0;
}

/****************************************************************************
 * coord_data
 ****************************************************************************/

static int coord_data(FAR struct ieee_coord_s *coord)
{
  printf("Data!\n");
  return 0;
}

/****************************************************************************
 * coord_ack
 ****************************************************************************/

static int coord_ack(FAR struct ieee_coord_s *coord)
{
  printf("Ack!\n");
  return 0;
}

/****************************************************************************
 * coord_command_beacon_req
 ****************************************************************************/

static int coord_command_beacon_req(FAR struct ieee_coord_s *coord)
{
  FAR struct ieee_frame_s *rx = &coord->rxbuf;
  
  //check the request looks good
  
  printf("Beacon request\n");

  //compose a basic response
  
  return 0;
}

/****************************************************************************
 * coord_command
 ****************************************************************************/

static int coord_command(FAR struct ieee_coord_s *coord)
{
  FAR struct ieee_frame_s *rx = &coord->rxbuf;
  uint8_t cmd = rx->payload[0];

  printf("Command %02X!\n",cmd);

  switch(cmd)
    {
      case IEEE802154_CMD_ASSOC_REQ      : break;
      case IEEE802154_CMD_ASSOC_RSP      : break;
      case IEEE802154_CMD_DIS_NOT        : break;
      case IEEE802154_CMD_DATA_REQ       : break;
      case IEEE802154_CMD_PANID_CONF_NOT : break;
      case IEEE802154_CMD_ORPHAN_NOT     : break;
      case IEEE802154_CMD_BEACON_REQ     : return coord_command_beacon_req(coord); break;
      case IEEE802154_CMD_COORD_REALIGN  : break;
      case IEEE802154_CMD_GTS_REQ        : break;
    }
  return 0;
}

/****************************************************************************
 * coord_manage
 ****************************************************************************/

static int coord_manage(FAR struct ieee_coord_s *coord)
{
  /* Decode frame type */
  uint8_t fc1, ftype;
  int     hlen;
  char    buf[IEEE802154_ADDRSTRLEN+1];

  FAR struct ieee_frame_s *rx = &coord->rxbuf;
  int i;


  fc1 = rx->packet.data[0];
  rx->seq = rx->packet.data[2];

  hlen = ieee802154_addrparse(&rx->packet, &rx->dest, &rx->src);
  if(hlen<0)
    {
      printf("invalid packet\n");
      return 1;
    }

  rx->payload = rx->packet.data + hlen;
  rx->plen    = rx->packet.len  - hlen;

  ftype = fc1 & IEEE802154_FC1_FTYPE;

  ieee802154_addrtostr(buf,sizeof(buf),&rx->src);
  printf("[%s -> ", buf);
  ieee802154_addrtostr(buf,sizeof(buf),&rx->dest);
  printf("%s] ", buf);
  
  for(i=0; i<rx->plen; i++) printf("%02X", rx->payload[i]);
  printf("\n");

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

static volatile int gRun;
static pthread_t gDaemonPid;

void task_signal(int sig)
{
  gRun = 0;
}

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

#define ACTION_STOP 1
#define ACTION_PANID 2

struct message
{
  int action;
  unsigned long param;
};

struct message g_message;

/****************************************************************************
 * coord_task
 ****************************************************************************/
int coord_task(int s_argc, char **s_argv)
{
  FAR struct ieee_frame_s *rx = &g_coord.rxbuf;
  int ret;
  
  coord_initialize(&g_coord, s_argv[3], s_argv[4], s_argv[5]);

  printf("IEEE 802.15.4 Coordinator started, chan %d, panid %04X, argc %d\n", g_coord.chan, g_coord.panid, s_argc);

  ieee802154_setchan (g_coord.fd, g_coord.chan );
  ieee802154_setpanid(g_coord.fd, g_coord.panid);

  if(g_coord.fd<0)
    {
      fprintf(stderr, "cannot open %s, errno=%d\n", s_argv[3], errno);
      exit(ERROR);
    }
  gRun = 1;

  while(gRun)
    {
      ret = read(g_coord.fd, &rx->packet, sizeof(struct ieee802154_packet_s));
      if(ret > 0)
        {
          coord_manage(&g_coord);
        }
      if(ret < 0)
        {
          if(errno==4) //EINTR, signal received
            {
              switch(g_message.action)
                {
                  case ACTION_STOP:
                    gRun = 0; 
                    break;
                    
                  case ACTION_PANID:
                    g_coord.panid = (uint16_t)g_message.param;
                    ieee802154_setpanid(g_coord.fd, g_coord.panid);
                    break;
                    
                  default:
                    printf("received unknown message\n");
                }
            }
        }
    }
  
  printf("IEEE 802.15.4 Coordinator stopped\n");  
  return 0;
}

/****************************************************************************
 * coord_main
11 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int coord_main(int argc, FAR char *argv[])
#endif
{
  
  if(argc < 2)
    {
      printf("coord start | stop | panid\n");
      exit(1);
    }
  
  if(!strcmp(argv[1], "start"))
    {
      if(argc != 5)
        {
          printf("coord start <dev> <chan> <panid>\n");
          exit(EXIT_FAILURE);
        }
      if(gRun)
        {
          printf("Already started.\n");
          exit(EXIT_FAILURE);
        }
      printf("IEEE 802.15.4 Coordinator starting...\n");  
      gDaemonPid = task_create("coord", SCHED_PRIORITY_DEFAULT,
                                   1024,
                                   coord_task, argv);
    }
  else if(!strcmp(argv[1], "stop"))
    {
      if(!gRun)
        {
norun:
          printf("Not started.\n");
          exit(EXIT_FAILURE);
        }
      printf("IEEE 802.15.4 Coordinator stopping...\n");  
      g_message.action = ACTION_STOP;
      kill(gDaemonPid, SIGUSR1);
    }
  else if(!strcmp(argv[1], "status"))
    {
      printf("IEEE 802.15.4 Coordinator %s.\n", gRun?"started":"stopped");  
    }
  else if(!strcmp(argv[1], "panid"))
    {
      if(argc != 3)
        {
          printf("coord panid <panid>\n");
          exit(EXIT_FAILURE);
        }
      if(!gRun)
        {
          goto norun;
        }
      g_message.action = ACTION_PANID;
      g_message.param = strtol(argv[2], NULL, 0);
      kill(gDaemonPid, SIGUSR1);
    }
  return EXIT_SUCCESS;
}
