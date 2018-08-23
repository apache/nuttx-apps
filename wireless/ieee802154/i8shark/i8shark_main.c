/****************************************************************************
 * wireless/ieee802154/i8shark/i8shark_main.c
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

  uint8_t chan;
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
 * Name: i8shark_help
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static void i8shark_help(void)
{
  printf("Usage: i8shark ![interface] [OPTIONS]\n");
  printf("\nArguments are \"sticky\".\n");
  printf("\nInterface only needs to be specified the first time\n");
  printf("OPTIONS include:\n");
  printf("  [-h] shows this message and exits\n");
}
#endif

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}
#endif

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}
#endif

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static void parse_args(FAR struct i8shark_state_s *i8shark, int argc, FAR char **argv)
{
  FAR char *ptr;
  int index;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          case 'h':
            i8shark_help();
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            i8shark_help();
            exit(1);
        }
    }
}
#endif

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

  i8shark->chan = CONFIG_IEEE802154_I8SHARK_CHANNEL;
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
 *   This daemon reads all incoming IEEE 802.15.4 frames from a MAC802154 character
 *   driver, packages the frames into a Wireshark Zigbee Encapsulate Protocol (ZEP)
 *   packet and sends it over Ethernet to the specified host machine running
 *   Wireshark.
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
      fprintf(stderr, "ERROR: cannot open %s, errno=%d\n", g_i8shark.devpath, errno);
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

  /* We bind to the IP address of the outbound interface so that the OS knows
   * which interface to use to send the packet.
   */

  netlib_get_ipv4addr(CONFIG_IEEE802154_I8SHARK_FORWARDING_IFNAME, &addr.sin_addr);
  addr.sin_port   = 0;
  addr.sin_family = AF_INET;
  addrlen = sizeof(struct sockaddr_in);

  if (bind(sockfd, (FAR struct sockaddr *)&addr, addrlen) < 0)
    {
      fprintf(stderr, "ERROR: Bind failure: %d\n", errno);
      return -1;
    }

  /* Setup our remote address. Wireshark expects ZEP packets over UDP on port 17754 */

  raddr.sin_family      = AF_INET;
  raddr.sin_port        = HTONS(17754);
  raddr.sin_addr.s_addr = HTONL(CONFIG_IEEE802154_I8SHARK_HOST_IPADDR);

  /* Loop until the daemon is shutdown reading incoming IEEE 802.15.4 frames,
   * packing them into Wireshark "Zigbee Encapsulation Packets" (ZEP) and sending
   * them over UDP to Wireshark.
   */

  while (!g_i8shark.daemon_shutdown)
    {
      struct mac802154dev_rxframe_s frame;
      enum ieee802154_frametype_e ftype;
      uint8_t zepframe[I8SHARK_MAX_ZEPFRAME];
      clock_t systime;
      int ind = 0;
      int nbytes;

      /* Get an incoming frame from the MAC character driver */

      ret = read(fd, &frame, sizeof(struct mac802154dev_rxframe_s));
      if (ret < 0)
        {
          continue;
        }

      /* First 2 bytes of packet represent preamble. For ZEP, "EX" */

      zepframe[ind++] = 'E';
      zepframe[ind++] = 'X';

      /* The next byte is the version. We are using V2 */

      zepframe[ind++] = 2;

      /* Next byte is type. ZEP only differentiates between ACK and Data. My
       * assumption is that Data also includes MAC command frames and beacon
       * frames. So we really only need to check if it's an ACK or not.
       */

      ftype = ((*(uint16_t *)frame.payload) & IEEE802154_FRAMECTRL_FTYPE) >>
              IEEE802154_FRAMECTRL_SHIFT_FTYPE;

      if (ftype == IEEE802154_FRAME_ACK)
        {
          zepframe[ind++] = 2;

          /* Not sure why, but the ZEP header allows for a 4-byte sequence no.
           * despite 802.15.4 sequence number only being 1-byte
           */

          zepframe[ind] = frame.meta.dsn;
          ind += 4;
        }
      else
        {
          zepframe[ind++] = 1;

          /* Next bytes is the Channel ID */

          ieee802154_getchan(fd, &zepframe[ind++]);

          /* For now, just hard code the device ID to an arbitrary value */

          zepframe[ind++] = 0xFA;
          zepframe[ind++] = 0xDE;

          /* Not completely sure what LQI mode is. My best guess as of now based
           * on a few comments in the Wireshark code is that it determines whether
           * the last 2 bytes of the frame portion of the packet is the CRC or the
           * LQI.  I believe it is CRC = 1, LQI = 0. We will assume the CRC is the
           * last few bytes as that is what the MAC layer expects. However, this
           * may be a bad assumption for certain radios.
           */

          zepframe[ind++] = 1;

          /* Next byte is the LQI value */

          zepframe[ind++] = frame.meta.lqi;

          /* Need to use NTP to get time, but for now, include the system time */

          systime = clock();
          memcpy(&zepframe[ind], &systime, 8);
          ind += 8;

          /* Not sure why, but the ZEP header allows for a 4-byte sequence no.
           * despite 802.15.4 sequence number only being 1-byte
           */

          zepframe[ind]   = frame.meta.dsn;
          zepframe[ind+1] = 0;
          zepframe[ind+2] = 0;
          zepframe[ind+3] = 0;
          ind += 4;

          /* Skip 10-bytes for reserved fields */

          ind += 10;

          /* Last byte is the length */

#ifdef CONFIG_IEEE802154_I8SHARK_XBEE_APPHDR
          zepframe[ind++] = frame.length - 2;
#else
          zepframe[ind++] = frame.length;
#endif
        }

      /* The ZEP header is filled, now copy the frame in */

#ifdef CONFIG_IEEE802154_I8SHARK_XBEE_APPHDR
      memcpy(&zepframe[ind], frame.payload, frame.offset);
      ind += frame.offset;

      /* XBee radios use a 2 byte "application header" to support duplicate packet
       * detection.  Wireshark doesn't know how to handle this data, so we provide
       * a configuration option that drops the first 2 bytes of the payload portion
       * of the frame for all sniffed frames
       *
       * NOTE: Since we remove data from the frame, the FCS is no longer valid
       * and Wireshark will fail to disect the frame.  Wireshark ignores a case
       * where the FCS is not included in the actual frame.  Therefore, we
       * subtract 4 rather than 2 to remove the FCS field so that the disector
       * will not fail.
       */

      memcpy(&zepframe[ind], (frame.payload + frame.offset + 2),
             (frame.length - frame.offset - 2));
      ind += frame.length - frame.offset - 4;
#else
      memcpy(&zepframe[ind], frame.payload, frame.length);
      ind += frame.length;
#endif

      /* Send the encapsulated frame to Wireshark over UDP */

      nbytes = sendto(sockfd, zepframe, ind, 0, (struct sockaddr*)&raddr, addrlen);
      if (nbytes < ind)
        {
          fprintf(stderr, "ERROR: sendto() did not send all bytes. %d\n", errno);
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

#ifdef CONFIG_BUILD_LOADABLE
int main(int argc, FAR char *argv[])
#else
int i8shark_main(int argc, char *argv[])
#endif
{
  int argind = 1;

  if (!g_i8shark.initialized)
    {
      i8shark_init(&g_i8shark);
    }

  if (argc > 1)
    {
      /* If the first argument is an interface, update our character device path */

      if (strncmp(argv[argind], "/dev/", 5) == 0)
        {
          /* Check if the name is the same as the current one */

          if (!strcmp(g_i8shark.devpath, argv[argind]))
            {
              /* Adapter daemon can't be running when we change device path */

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
