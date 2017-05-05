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

static int tx(int fd, FAR const char *str, int verbose);

/****************************************************************************
 * Private Data
 ****************************************************************************/

uint8_t g_handle = 0;
uint8_t g_txframe[IEEE802154_MAX_MAC_PAYLOAD_SIZE];

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name : tx
 *
 * Description :
 *   Transmit a data frame.
 ****************************************************************************/

static int tx(int fd, FAR const char *str, int verbose)
{
  struct mac802154dev_txframe_s tx;
  int ret, str_len;
  int i = 0;

  /* Set an application defined handle */

  tx.meta.msdu_handle = g_handle++;

  /* This is a normal transaction, no special handling */

  tx.meta.msdu_flags.ack_tx = 0;
  tx.meta.msdu_flags.gts_tx = 0;
  tx.meta.msdu_flags.indirect_tx = 0;

  tx.meta.ranging = IEEE802154_NON_RANGING;

  tx.meta.src_addr_mode = IEEE802154_ADDRMODE_EXTENDED;

  str_len = strlen(str);

  /* Each byte is represented by 2 chars */

  tx.length = str_len >> 1;

  /* Check if the number of chars is a multiple of 2 and that the number of 
   * bytes does not exceed the max MAC frame payload supported */

  if ((str_len & 1) || (tx.length > IEEE802154_MAX_MAC_PAYLOAD_SIZE))
    {
      goto data_error;
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
          goto data_error;
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

return ret;

data_error:
          printf("data error\n");
          return ERROR;;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * usage
 ****************************************************************************/

int usage(void)
{
  printf("i8 <device> <op> [<args>]\n"
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
  unsigned long arg = 0;
  int fd;
  int ret = OK;

  if (argc < 3)
    {
      return usage();
    }

  if (argc >= 4)
    {
      arg = atol(argv[3]);
    }

  /* Open device */

  fd = open(argv[1], O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", argv[1], errno);
      ret = errno;
      goto exit;
    }

  /* Get mode */

  if (!strcmp(argv[2], "tx"))
    {
      ret = tx(fd, argv[3], TRUE);
      if (ret < 0)
      {
        goto error;
      }
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
