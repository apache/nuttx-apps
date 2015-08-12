/****************************************************************************
 * ieee802154/i8sak/i8sak_main.c
 * IEEE 802.15.4 Swiss Army Knife
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

/****************************************************************************
 * This app can be used to control an IEEE 802.15.4 device from command line.
 * Example valid ieee802154 packets:
 *
 * Beacon request, seq number 01:
 *   xx xx 01
 *
 * Simple data packet 
 * from long address xx to long address yy, pan id pp for src and dest
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
#include <nuttx/ieee802154/ieee802154.h>
#include <nuttx/ieee802154/ieee802154_dev.h>
#include <apps/ieee802154/ieee802154.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ieee802154_packet_s gRxPacket;
static struct ieee802154_packet_s gTxPacket;
static uint8_t gChan;

uint8_t levels[16];
uint8_t disp[16];

int scan(int fd)
{
  int ret = OK;
  uint8_t chan, energy;

  printf("\n");

  /* do scan */
  memset(disp,0,16);
 
  while(1)
    {
      for (chan=0; chan<16; chan++)
        {
          ret = ieee802154_setchan(fd, chan+11);
          if (ret<0)
            {
              printf("Device is not an IEEE 802.15.4 interface!\n");
              return ret;
            }
          ret = ioctl(fd, MAC854IOCGED, (unsigned long)&energy);
          if (ret<0)
            {
              printf("Device is not an IEEE 802.15.4 interface!\n");
              return ret;
            }

          levels[chan] = energy;
        }

      /* compute max with decay */

      for (chan=0; chan<16; chan++)
        {
          if(levels[chan] > disp[chan])
            {
               disp[chan] = levels[chan];
            }
         else
            {
              if(disp[chan] > 0) disp[chan] -= 1;
            }
        }

      for (chan=0;chan < 16; chan++)
        {
          printf("%2d : [%3d] ",chan+11, disp[chan]);
          energy = disp[chan] >> 3;
          while(energy-- > 0) printf("#");
          printf("                                \n");
        }

      /* move cursor 17 lines up : http://www.tldp.org/HOWTO/Bash-Prompt-HOWTO/x361.html */
      printf("\x1B[16A");
    }

  return ret;
}

/****************************************************************************
 * Name : status
 *
 * Description :
 *   Show device status
 ****************************************************************************/

static int status(int fd)
{
  int ret,i;
  uint8_t chan, eaddr[8];
  uint16_t panid, saddr;
  bool promisc;
  struct ieee802154_cca_s cca;

  /* Get information */

  ret = ioctl(fd, MAC854IOCGPANID, (unsigned long)&panid);
  if (ret)
    {
      printf("MAC854IOCGPANID failed\n");
      return ret;
    }
  ret = ioctl(fd, MAC854IOCGCHAN, (unsigned long)&chan);
  if (ret)
    {
      printf("MAC854IOCGCHAN failed\n");
      return ret;
    }

  ret = ioctl(fd, MAC854IOCGSADDR, (unsigned long)&saddr);
  if (ret)
    {
      printf("MAC854IOCGSADDR failed\n");
      return ret;
    }
  ret = ioctl(fd, MAC854IOCGEADDR, (unsigned long)&eaddr[0]);
  if (ret)
    {
      printf("MAC854IOCGEADR failed\n");
      return ret;
    }
  ret = ioctl(fd, MAC854IOCGPROMISC, (unsigned long)&promisc);
  if (ret)
    {
      printf("MAC854IOCGPROMISC failed\n");
      return ret;
    }
  ret = ieee802154_getcca(fd, &cca);
  if (ret)
    {
      return ret;
    }
#if 0
  ret = ioctl(fd, MAC854IOCGORDER, (unsigned long)&order);
  if (ret)
    {
      printf("MAC854IOCGORDER failed\n");
      return ret;
    }
#endif
  /* Display */

  printf("PANID %02X%02X CHAN %2d (%d MHz)\nSADDR %02X%02X EADDR ", 
          panid>>8, panid&0xFF, chan, 2350+(5*chan), saddr>>8, saddr&0xFF);
  for (i=0; i<8; i++)
    {
      printf("%02X", eaddr[i]);
    }
  printf("\nCCA: ");
  if (cca.use_ed)
    {
      printf("ED (%d) ", cca.edth);
    }
  if (cca.use_cs)
    {
      printf("CS (%d)", cca.csth);
    }
  printf("\nPromisc: %s\n", promisc?"Yes":"No");
  return 0;
}

/****************************************************************************
 * Name : display
 *
 * Description :
 *   Display a single packet
 ****************************************************************************/

static int display(FAR struct ieee802154_packet_s *pack, bool verbose)
{
  int i;
  int     hlen=0, dhlen=0;
  char    buf[IEEE802154_ADDRSTRLEN+1];
  struct ieee802154_addr_s dest,src;

  hlen = ieee802154_addrparse(pack, &dest, &src);

  dhlen = hlen;
  if(hlen>pack->len) dhlen = 0;

  printf("chan=%2d rssi=%3u lqi=%3u len=%3u ", gChan, pack->rssi, pack->lqi, pack->len - dhlen);

  if(hlen<0)
    {
      printf("[invalid header] ");
      dhlen = 0;
    }
  else
    {
      ieee802154_addrtostr(buf,sizeof(buf),&src);
      printf("[%s -> ", buf);
      ieee802154_addrtostr(buf,sizeof(buf),&dest);
      printf("%s] ", buf);
    }

  for (i = 0; i < pack->len - dhlen; i++)
    {
      printf("%02X", pack->data[i+dhlen]);
    }
  printf("\n");
  return 0;
}

/****************************************************************************
 * Name : sniff
 *
 * Description :
 *   Listen for all packets with a valid CRC on a given channel.
 ****************************************************************************/
struct sniffargs
{
  int fd;
  int verbose;
};

static void* sniff(void *arg)
{
  int ret;
  struct sniffargs *sa = (struct sniffargs*)arg;
  int fd = sa->fd;

  printf("Listening...\n");
  while (1)
    {
      ret = read(fd, &gRxPacket, sizeof(struct ieee802154_packet_s));
      if(ret < 0)
        {
          if (errno == EAGAIN)
            {
            continue;
            }
          if (errno == EINTR)
            {
            printf("read: interrupted\n");
            break;
            }
          else
            {
              printf("read: errno=%d\n",errno);
              break;
            }
        } 

      /* Display packet */
      display(&gRxPacket, sa->verbose);
    }
  

  return (void*)ret;
}

/****************************************************************************
 * Name : tx
 *
 * Description :
 *   Transmit a frame.
 ****************************************************************************/

static int tx(int fd, FAR struct ieee802154_packet_s *pack, int verbose)
{
  int i,ret;

  if(verbose)
    {
      for (i = 0; i < pack->len; i++)
        {
          printf("%02X", pack->data[i]);
        }
      fflush(stdout);
    }
  ret = write(fd, pack, sizeof(struct ieee802154_packet_s));
  if(ret==OK)
    {
      if(verbose) printf(" OK\n");
    }
  else
    {
      printf(" write: errno=%d\n",errno);
    }
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * usage
 ****************************************************************************/

int usage(void)
{
  printf("i8 <device> <op> <arg>\n"
         "  scan\n"
         "  dump\n"
         "  chan <ch>\n"
         "  snif\n"
         "  stat\n"
         "  edth <off|rssi>\n"
         "  csth <off|corr>\n"
         "  tx <hexpacket>\n"
         );
  return ERROR;
}

/****************************************************************************
 * i8_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int i8_main(int argc, char *argv[])
#endif
{
  int fd;
  int ret = OK;
  unsigned long arg=0;
  struct ieee802154_cca_s cca;

  printf("IEEE packet sniffer/dumper argc=%d\n",argc);
  if (argc<3)
    {
      return usage();
    }

  if (argc>=4)
    {
    arg = atol(argv[3]);
    }
  /* open device */

  fd = open(argv[1], O_RDWR);
  if (fd<0)
    {
      printf("cannot open %s, errno=%d\n", argv[1], errno);
      ret = errno;
      goto exit;
    }

  /* get mode */
  if (!strcmp(argv[2], "scan"))
    {
    ret = scan(fd);
    }
  else if (!strcmp(argv[2], "dump"))
    {
    ret = ioctl(fd, 1000, 0);
    }
  else if (!strcmp(argv[2], "stat"))
    {
    ret = status(fd);
    }
  else if (!strcmp(argv[2], "chan"))
    {
      if(argc != 4)
        {
        ret = usage();
        }
      ret = ieee802154_setchan(fd, arg);
      gChan = arg;
    }
  else if (!strcmp(argv[2], "edth"))
    {
      if(argc != 4)
        {
        goto usage;
        }
      ret = ieee802154_getcca(fd, &cca);
      if(!strcmp("off",argv[3]))
        {
          cca.use_ed = 0;
        }
      else
        {
          cca.use_ed = 1;
          cca.edth   = arg;
        }
      ret = ieee802154_setcca(fd, &cca);
    }
  else if (!strcmp(argv[2], "csth"))
    {
      if(argc != 4)
        {
        goto usage;
        }
      ret = ieee802154_getcca(fd, &cca);
      if(!strcmp("off",argv[3]))
        {
          cca.use_cs = 0;
        }
      else
        {
          cca.use_cs = 1;
          cca.csth   = arg;
        }
      ret = ieee802154_setcca(fd, &cca);
    }
  else if (!strcmp(argv[2], "snif"))
    {
      struct sniffargs args;

      ret = ieee802154_setpromisc(fd, TRUE);

      args.fd = fd;
      args.verbose = FALSE;
      ret = (int)sniff(&args);

      ret = ieee802154_setpromisc(fd, FALSE);

    }
  else if (!strcmp(argv[2], "tx"))
    {
    int id=0;
    int len = strlen(argv[3]);
    FAR char *ptr = argv[3];

    if (len & 1)
      {
        goto data_error;
      }

    /* decode hex packet */

    while (id<125 && len>0)
      {
        int dat;
        if (sscanf(ptr, "%2x", &dat)==1)
          {
            gTxPacket.data[id++] = dat;
            ptr += 2;
            len -= 2;
          }
        else
          {
data_error:
            printf("data error\n");
            ret = ERROR;
            goto error;
          }
      }
    gTxPacket.len = id;

    ret = tx(fd, &gTxPacket, TRUE);
    }
  else if (!strcmp(argv[2], "beacons"))
    {
      struct sniffargs args;
      struct ieee802154_addr_s dest;
      pthread_t pth;
      int i;
    
      args.fd = fd;
      args.verbose = FALSE;

      pthread_create(&pth, NULL, sniff, &args);
  
      //beacon request
      gTxPacket.len = 0;
      gTxPacket.data[gTxPacket.len++] = 0x03; //mac command, no ack, no panid compression
      gTxPacket.data[gTxPacket.len++] = 0x00; //short destination address, no source address
      gTxPacket.data[gTxPacket.len++] = 0;    //seq
      dest.ia_len = 2;
      dest.ia_panid = 0xFFFF;
      dest.ia_saddr = 0xFFFF;
      ieee802154_addrstore(&gTxPacket, &dest, NULL);
      gTxPacket.data[gTxPacket.len++] = 0xFF; //panid
      gTxPacket.data[gTxPacket.len++] = 0xFF;
      gTxPacket.data[gTxPacket.len++] = 0xFF; //saddr
      gTxPacket.data[gTxPacket.len++] = 0xFF;
      gTxPacket.data[gTxPacket.len++] = IEEE802154_CMD_BEACON_REQ;
//    for(i=1;i<3;i++)
      while(1)
        {
          int ch;
          for(ch=11; ch<27; ch++)
            {
              printf("chan=%2d...\r", ch); fflush(stdout);
              ieee802154_setchan(fd, ch);
              gChan = ch;
              gTxPacket.data[2] = i; //seq
              tx(fd, &gTxPacket, FALSE);
              sleep(1);
            }
            i++; if(i==256) i=0;
        }
      pthread_kill(pth, SIGUSR1);
    }
  else
    {
usage:
    ret = usage();
    }
 
error:
  close(fd);
exit:
  return ret;
}

