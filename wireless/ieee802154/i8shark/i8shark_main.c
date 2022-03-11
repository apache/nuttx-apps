/****************************************************************************
 * apps/wireless/ieee802154/i8shark/i8shark_main.c
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

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include <nuttx/wireless/ieee802154/ieee802154_device.h>

#include "netutils/netlib.h"
#include "wireless/ieee802154.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifndef CONFIG_IEEE802154_I8SHARK_DAEMON_PRIORITY
#  define CONFIG_IEEE802154_I8SHARK_DAEMON_PRIORITY SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_IEEE802154_I8SHARK_DAEMON_STACKSIZE
#  define CONFIG_IEEE802154_I8SHARK_DAEMON_STACKSIZE 2048
#endif

#ifndef CONFIG_IEEE802154_I8SHARK_CHANNEL
#  define CONFIG_IEEE802154_I8SHARK_CHANNEL 11
#endif

#ifndef CONFIG_IEEE802154_I8SHARK_HOST_IPADDR
#  error "You must define CONFIG_IEEE802154_I8SHARK_HOST_IPADDR"
#endif

#ifndef CONFIG_IEEE802154_I8SHARK_FORWARDING_IFNAME
#  define CONFIG_IEEE802154_I8SHARK_FORWARDING_IFNAME "eth0"
#endif

#define I8SHARK_MAX_DEVPATH 15

#define ZEP_MAX_HDRSIZE 32
#define I8SHARK_MAX_ZEPFRAME IEEE802154_MAX_PHY_PACKET_SIZE + ZEP_MAX_HDRSIZE

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct i8shark_state_s
{
  bool initialized      : 1;
  bool daemon_started   : 1;
  bool daemon_shutdown  : 1;

  pid_t daemon_pid;

  /* User exposed settings */

  FAR char devpath[I8SHARK_MAX_DEVPATH];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int i8shark_init(FAR struct i8shark_state_s *i8shark);
static int i8shark_daemon(int argc, FAR char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct i8shark_state_s g_i8shark;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i8shark_init
 ****************************************************************************/

static int i8shark_init(FAR struct i8shark_state_s *i8shark)
{
  if (i8shark->initialized)
    {
      return OK;
    }

  /* Set the default settings using config options */

  strcpy(i8shark->devpath, CONFIG_IEEE802154_I8SHARK_DEVPATH);

  /* Flags for synchronzing with daemon state */

  i8shark->daemon_started = false;
  i8shark->daemon_shutdown = false;
  i8shark->initialized = true;

  return OK;
}

/****************************************************************************
 * Name : i8shark_daemon
 *
 * Description :
 *   This daemon reads all incoming IEEE 802.15.4 frames from a MAC802154
 *   character driver, packages the frames into a Wireshark Zigbee
 *   Encapsulate Protocol (ZEP) packet and sends it over Ethernet to the
 *   specified host machine running Wireshark.
 *
 ****************************************************************************/

static int i8shark_daemon(int argc, FAR char *argv[])
{
  int ret;
  int fd;
  struct sockaddr_in addr;
  struct sockaddr_in raddr;
  socklen_t addrlen;
  int sockfd;

  fprintf(stderr, "i8shark: daemon started\n");
  g_i8shark.daemon_started = true;

  fd = open(g_i8shark.devpath, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr,
              "ERROR: cannot open %s, errno=%d\n",
              g_i8shark.devpath, errno);
      g_i8shark.daemon_started = false;
      ret = errno;
      return ret;
    }

  /* Place the MAC into promiscuous mode */

  ieee802154_setpromisc(fd, true);

  /* Always listen */

  ieee802154_setrxonidle(fd, true);

  /* Create a UDP socket to send the data to Wireshark */

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, "ERROR: socket failure %d\n", errno);
      g_i8shark.daemon_started = false;
      return -1;
    }

  /* We bind to the IP address of the outbound interface so that
   * the OS knows which interface to use to send the packet.
   */

  netlib_get_ipv4addr(CONFIG_IEEE802154_I8SHARK_FORWARDING_IFNAME,
                      &addr.sin_addr);
  addr.sin_port   = 0;
  addr.sin_family = AF_INET;
  addrlen = sizeof(struct sockaddr_in);

  if (bind(sockfd, (FAR struct sockaddr *)&addr, addrlen) < 0)
    {
      fprintf(stderr, "ERROR: Bind failure: %d\n", errno);
      return -1;
    }

  /* Setup our remote address.
   * Wireshark expects ZEP packets over UDP on port 17754
   */

  raddr.sin_family      = AF_INET;
  raddr.sin_port        = HTONS(17754);
  raddr.sin_addr.s_addr = HTONL(CONFIG_IEEE802154_I8SHARK_HOST_IPADDR);

  /* Loop until the daemon is shutdown reading incoming IEEE 802.15.4 frames,
   * packing them into Wireshark "Zigbee Encapsulation Packets" (ZEP)
   * and sending them over UDP to Wireshark.
   */

  while (!g_i8shark.daemon_shutdown)
    {
      struct mac802154dev_rxframe_s frame;
      enum ieee802154_frametype_e ftype;
      uint8_t zepframe[I8SHARK_MAX_ZEPFRAME];
      clock_t systime;
      int i = 0;
      int nbytes;

      /* Get an incoming frame from the MAC character driver */

      ret = read(fd, &frame, sizeof(struct mac802154dev_rxframe_s));
      if (ret < 0)
        {
          continue;
        }

      /* First 2 bytes of packet represent preamble. For ZEP, "EX" */

      zepframe[i++] = 'E';
      zepframe[i++] = 'X';

      /* The next byte is the version. We are using V2 */

      zepframe[i++] = 2;

      /* Next byte is type. ZEP only differentiates between ACK and Data. My
       * assumption is that Data also includes MAC command frames and beacon
       * frames. So we really only need to check if it's an ACK or not.
       */

      ftype = ((*(uint16_t *)frame.payload) & IEEE802154_FRAMECTRL_FTYPE) >>
              IEEE802154_FRAMECTRL_SHIFT_FTYPE;

      if (ftype == IEEE802154_FRAME_ACK)
        {
          zepframe[i++] = 2;

          /* Not sure why, but the ZEP header allows for a 4-byte sequence
           * no. despite 802.15.4 sequence number only being 1-byte
           */

          zepframe[i] = frame.meta.dsn;
          i += 4;
        }
      else
        {
          zepframe[i++] = 1;

          /* Next bytes is the Channel ID */

          ieee802154_getchan(fd, &zepframe[i++]);

          /* For now, just hard code the device ID to an arbitrary value */

          zepframe[i++] = 0xfa;
          zepframe[i++] = 0xde;

          /* Not completely sure what LQI mode is. My best guess as of now
           * based on a few comments in the Wireshark code is that it
           * determines whether the last 2 bytes of the frame portion of the
           * packet is the CRC or the LQI.
           * I believe it is CRC = 1, LQI = 0. We will assume the CRC is the
           * last few bytes as that is what the MAC layer expects.
           * However, this may be a bad assumption for certain radios.
           */

          zepframe[i++] = 1;

          /* Next byte is the LQI value */

          zepframe[i++] = frame.meta.lqi;

          /* Need to use NTP to get time, but for now,
           * include the system time
           */

          systime = clock();
          memcpy(&zepframe[i], &systime, 8);
          i += 8;

          /* Not sure why, but the ZEP header allows for a 4-byte sequence
           * no. despite 802.15.4 sequence number only being 1-byte
           */

          zepframe[i]   = frame.meta.dsn;
          zepframe[i + 1] = 0;
          zepframe[i + 2] = 0;
          zepframe[i + 3] = 0;
          i += 4;

          /* Skip 10-bytes for reserved fields */

          i += 10;

          /* Last byte is the length */

#ifdef CONFIG_IEEE802154_I8SHARK_XBEE_APPHDR
          zepframe[i++] = frame.length - 2;
#else
          zepframe[i++] = frame.length;
#endif
        }

      /* The ZEP header is filled, now copy the frame in */

#ifdef CONFIG_IEEE802154_I8SHARK_XBEE_APPHDR
      memcpy(&zepframe[i], frame.payload, frame.offset);
      i += frame.offset;

      /* XBee radios use a 2 byte "application header" to support duplicate
       * packet detection.  Wireshark doesn't know how to handle this data,
       * so we provide a configuration option that drops the first 2 bytes
       * of the payload portion of the frame for all sniffed frames
       *
       * NOTE:
       * Since we remove data from the frame, the FCS is no longer valid
       * and Wireshark will fail to disect the frame.  Wireshark ignores a
       * case where the FCS is not included in the actual frame.  Therefore,
       * we subtract 4 rather than 2 to remove the FCS field so that the
       * disector will not fail.
       */

      memcpy(&zepframe[i], (frame.payload + frame.offset + 2),
             (frame.length - frame.offset - 2));
      i += frame.length - frame.offset - 4;
#else
      /* If FCS suppression is enabled, subtract the FCS length to reduce the
       * piece of the frame copied.
       */

#ifdef CONFIG_IEEE802154_I8SHARK_SUPPRESS_FCS
    {
        uint8_t fcslen;
        ieee802154_getfcslen(fd, &fcslen);
        frame.length -= fcslen;
    }
#endif

      memcpy(&zepframe[i], frame.payload, frame.length);
      i += frame.length;
#endif

      /* Send the encapsulated frame to Wireshark over UDP */

      nbytes = sendto(sockfd, zepframe, i, 0,
                      (struct sockaddr *)&raddr, addrlen);
      if (nbytes < i)
        {
          fprintf(stderr,
                  "ERROR: sendto() did not send all bytes. %d\n", errno);
        }
    }

  g_i8shark.daemon_started = false;
  close(fd);
  printf("i8shark: daemon closing\n");
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i8shark_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int argind = 1;

  if (!g_i8shark.initialized)
    {
      i8shark_init(&g_i8shark);
    }

  if (argc > 1)
    {
      /* If the first argument is an interface,
       * update our character device path
       */

      if (strncmp(argv[argind], "/dev/", 5) == 0)
        {
          /* Check if the name is the same as the current one */

          if (!strcmp(g_i8shark.devpath, argv[argind]))
            {
              /* Adapter daemon can't be running when we change
               * device path
               */

              if (g_i8shark.daemon_started)
                {
                  printf("Can't change devpath when daemon is running.\n");
                  exit(1);
                }

              /* Copy the path into our state structure */

              strcpy(g_i8shark.devpath, argv[1]);
            }

          argind++;
        }
    }

  /* If the daemon is not running, start it. */

  g_i8shark.daemon_pid = task_create("i8shark",
                                 CONFIG_IEEE802154_I8SHARK_DAEMON_PRIORITY,
                                 CONFIG_IEEE802154_I8SHARK_DAEMON_STACKSIZE,
                                 i8shark_daemon, NULL);
  if (g_i8shark.daemon_pid < 0)
    {
      fprintf(stderr, "failed to start daemon\n");
      return ERROR;
    }

  return OK;
}
