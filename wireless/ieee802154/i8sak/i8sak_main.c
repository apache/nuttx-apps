/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_main.c
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2014-2015, 2017 Gregory Nutt. All rights reserved.
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

#include <nuttx/wireless/ieee802154/ieee802154_ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/


/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int tx(FAR const char *devname, FAR const char *str, int verbose);
static int start_sniffer_daemon(FAR const char *devname);

/****************************************************************************
 * Private Data
 ****************************************************************************/

uint8_t g_handle = 0;
uint8_t g_txframe[IEEE802154_MAX_MAC_PAYLOAD_SIZE];
static int sniffer_daemon(int argc, FAR char *argv[]);

bool g_sniffer_daemon_started = false;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name : start_sniff
 *
 * Description :
 *   Starts a thread to run the sniffer in the background
 ****************************************************************************/

static int start_sniffer_daemon(FAR const char *devname)
{
  int ret;
  FAR const char *sniffer_argv[2];

  printf("i8sak: Starting sniffer_daemon\n");

  if (g_sniffer_daemon_started)
    {
      printf("i8sak: sniffer_daemon already running\n");
      return EXIT_SUCCESS;
    }
  
  sniffer_argv[0] = devname;
  sniffer_argv[1] = NULL;
  
  ret = task_create("sniffer_daemon", CONFIG_IEEE802154_I8SAK_PRIORITY,
                    CONFIG_IEEE802154_I8SAK_STACKSIZE, sniffer_daemon,
                    (FAR char * const *)sniffer_argv);
  if (ret < 0)
    {
      int errcode = errno;
      printf("i8sak: ERROR: Failed to start sniffer_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  g_sniffer_daemon_started = true;
  printf("i8sak: sniffer_daemon started\n");
  
  return OK;
}
 
/****************************************************************************
 * Name : sniffer_daemon
 *
 * Description :
 *   Sniff for frames (Promiscuous mode)
 ****************************************************************************/

static int sniffer_daemon(int argc, FAR char *argv[])
{
  int ret, fd, i;
  struct mac802154dev_rxframe_s rx;

  fd = open(argv[1], O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", argv[1], errno);
      ret = errno;
      return ret;
    }

  printf("Listening...\n"); 

  /* Enable promiscuous mode */

  ret = ieee802154_setpromisc(fd, true);

  /* Make sure receiver is always on while idle */

  ret = ieee802154_setrxonidle(fd, true);

  while(1)
    {
      ret = read(fd, &rx, sizeof(struct mac802154dev_rxframe_s));
      if (ret < 0)
        {
          printf("sniffer_daemon: read failed: %d\n", errno);
          goto errout;
        }

      printf("Frame Received:\n");

      for (i = 0; i < rx.length; i++)
        {
          printf("%02X", rx.payload[i]);
        }

      printf("\n");

      fflush(stdout);
    }

errout:
  /* Turn receiver off when idle */

  ret = ieee802154_setrxonidle(fd, false);

  /* Disable promiscuous mode */

  ret = ieee802154_setpromisc(fd, false);

  printf("sniffer_daemon: closing");
  close(fd);
  g_sniffer_daemon_started = false;
  return OK;
}

/****************************************************************************
 * Name : tx
 *
 * Description :
 *   Transmit a data frame.
 ****************************************************************************/

static int tx(FAR const char *devname, FAR const char *str, int verbose)
{
  struct mac802154dev_txframe_s tx;
  int ret, str_len, fd;
  int i = 0;

  /* Open device */

  fd = open(devname, O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", devname, errno);
      ret = errno;
      return ret;
    }

  /* Set an application defined handle */

  tx.meta.msdu_handle = g_handle++;

  /* This is a normal transaction, no special handling */

  tx.meta.msdu_flags.ack_tx = 0;
  tx.meta.msdu_flags.gts_tx = 0;
  tx.meta.msdu_flags.indirect_tx = 0;

  tx.meta.ranging = IEEE802154_NON_RANGING;

  tx.meta.src_addr_mode = IEEE802154_ADDRMODE_EXTENDED;
  tx.meta.dest_addr.mode = IEEE802154_ADDRMODE_SHORT;
  tx.meta.dest_addr.saddr = 0xFADE;

  str_len = strlen(str);

  /* Each byte is represented by 2 chars */

  tx.length = str_len >> 1;

  /* Check if the number of chars is a multiple of 2 and that the number of 
   * bytes does not exceed the max MAC frame payload supported */

  if ((str_len & 1) || (tx.length > IEEE802154_MAX_MAC_PAYLOAD_SIZE))
    {
      goto error;
    }

  /* Decode hex packet */

  while (str_len > 0)
    {
      int dat;
      if (sscanf(str, "%2x", &dat) == 1)
        {
          g_txframe[i++] = dat;
          str += 2;
          str_len -= 2;
        }
      else
        {
          goto error;
        }
    }

  if (verbose)
    {
      for (i = 0; i < tx.length; i++)
        {
          printf("%02X", g_txframe[i]);
        }

      fflush(stdout);
    }

  tx.payload = &g_txframe[0];

  ret = write(fd, &tx, sizeof(struct mac802154dev_txframe_s));
  if (ret == OK)
    {
      if (verbose)
        {
          printf(" OK\n");
        }
    }
  else
    {
      printf(" write: errno=%d\n",errno);
    }

  close(fd);
  return ret;

error:
  printf("data error\n");
  close(fd);
  return ERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * usage
 ****************************************************************************/

int usage(void)
{
  printf("i8 <devname> <op> [<args>]\n"
         "  sniffer           Listen for packets\n"
         "  tx <hexpacket>    Transmit a data frame\n"
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
  FAR const char *devname;
  FAR const char *cmdname;
  int ret = OK;

  if (argc < 3)
    {
      printf("ERROR: Missing argument\n");
      return usage();
    }

  devname = argv[1];
  cmdname = argv[2];

  /* Get mode */

  if (!strcmp(cmdname, "tx"))
    {
      ret = tx(devname, argv[3], TRUE);
    }
  else if(!strcmp(argv[2], "sniffer"))
    {
      ret = start_sniffer_daemon(devname);
    }
  else
    {
      ret = usage();
    }

  return ret;
}
