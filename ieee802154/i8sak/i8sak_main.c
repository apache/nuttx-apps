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

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ieee802154_packet_s gRxPacket;
static struct ieee802154_packet_s gTxPacket;

int setchan(int fd, uint8_t chan)
{
  int ret = ioctl(fd, MAC854IOCSCHAN, (unsigned long)chan );
  if (ret<0)
    {
      printf("MAC854IOCSCHAN failed\n");
    }
  return ret;
}

int setcca(int fd, FAR struct ieee802154_cca_s *cca)
{
  int ret = ioctl(fd, MAC854IOCSCCA, (unsigned long)cca );
  if (ret<0)
    {
      printf("MAC854IOCSCCA failed\n");
    }
  return ret;
}

int getcca(int fd, FAR struct ieee802154_cca_s *cca)
{
  int ret = ioctl(fd, MAC854IOCGCCA, (unsigned long)cca );
  if (ret<0)
    {
      printf("MAC854IOCGCCA failed\n");
    }
  return ret;
}

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
          ret = setchan(fd, chan+11);
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
  ret = ioctl(fd, MAC854IOCGCCA, (unsigned long)&cca);
  if (ret)
    {
      printf("MAC854IOCGCCA failed\n");
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

static int display(FAR struct ieee802154_packet_s *pack)
{
  int i;
  printf("len=%3u rssi=%3u lqi=%3u [", pack->len, pack->rssi, pack->lqi);
  for (i = 0; i < pack->len; i++)
    {
      printf("%02X", pack->data[i]);
    }
  printf("]\n");
  return 0;
}

/****************************************************************************
 * Name : sniff
 *
 * Description :
 *   Listen for all packets with a valid CRC on a given channel.
 ****************************************************************************/

static int sniff(int fd, int chan)
{
  int ret;
  ret = setchan(fd, chan);
  if (ret<0)
    {
      return ret;
    }

  ret = ioctl(fd, MAC854IOCSPROMISC, TRUE);
  if (ret<0)
    {
      printf("Device is not an IEEE 802.15.4 interface!\n");
      return ret;
    }

  printf("Listening on channel %d in promisc mode.\n",chan);
  while (1)
    {
      ret = read(fd, &gRxPacket, sizeof(struct ieee802154_packet_s));
      if(ret < 0)
        {
          if (errno == EAGAIN)
            {
            continue;
            }
          else
            {
              printf("read: errno=%d\n",errno);
              break;
            }
        } 

      /* Display packet */
      display(&gRxPacket);
    }
  
  return ret;
}

/****************************************************************************
 * Name : tx
 *
 * Description :
 *   Transmit a frame.
 ****************************************************************************/

static int tx(int fd, int chan, FAR struct ieee802154_packet_s *pack)
{
  int i,ret;

  ret = setchan(fd, chan);
  if (ret<0)
    {
      return ret;
    }

  for (i = 0; i < pack->len; i++)
    {
      printf("%02X", pack->data[i]);
    }
  fflush(stdout);
  ret = write(fd, pack, sizeof(struct ieee802154_packet_s));
  if(ret==OK)
    {
      printf(" OK\n");
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
         "  snif <ch>\n"
         "  stat\n"
         "  chan <ch>\n"
         "  edth <off|rssi>\n"
         "  csth <off|corr>\n"
         "  tx <ch> <hexpacket>\n"
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
      ret = setchan(fd, arg);
    }
  else if (!strcmp(argv[2], "edth"))
    {
      if(argc != 4)
        {
        goto usage;
        }
      ret = getcca(fd, &cca);
      if(!strcmp("off",argv[3]))
        {
          cca.use_ed = 0;
        }
      else
        {
          cca.use_ed = 1;
          cca.edth   = arg;
        }
      ret = setcca(fd, &cca);
    }
  else if (!strcmp(argv[2], "csth"))
    {
      if(argc != 4)
        {
        goto usage;
        }
      ret = getcca(fd, &cca);
      if(!strcmp("off",argv[3]))
        {
          cca.use_cs = 0;
        }
      else
        {
          cca.use_cs = 1;
          cca.csth   = arg;
        }
      ret = setcca(fd, &cca);
    }
  else if (!strcmp(argv[2], "snif"))
    {
    ret = sniff(fd,arg);
    }
  else if (!strcmp(argv[2], "tx"))
    {
    int id=0;
    unsigned long ch = arg;
    int len = strlen(argv[4]);
    FAR char *ptr = argv[4];

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

    ret = tx(fd, ch, &gTxPacket);
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

