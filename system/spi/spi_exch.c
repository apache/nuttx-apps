/****************************************************************************
 * apps/system/spi/spi_exch.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Dave Marples <dave@marples.net>
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

#include <stdlib.h>

#include <ctype.h>

#include <nuttx/spi/spi_transfer.h>

#include "spitool.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spicmd_exch
 ****************************************************************************/

#define ISHEX(x) ((((x)>='0') && ((x)<='9')) || ((toupper(x)>='A') && (toupper(x)<='F')))
#define HTOI(x) ( (((x)>='0') && ((x)<='9')) ? (x)-'0':toupper(x)-'A'+10 )

int spicmd_exch(FAR struct spitool_s *spitool, int argc, FAR char **argv)
{
  FAR char *ptr;
  int nargs;
  int argndx;
  int ret;
  int fd;

  uint8_t txdata[MAX_XDATA] =
  {
    0
  };
  uint8_t rxdata[MAX_XDATA] =
  {
    0
  };
  uint8_t *txdatap = txdata;
  struct spi_trans_s trans;
  struct spi_sequence_s seq;
  uint32_t d;

  /* Parse any command line arguments */

  for (argndx = 1; argndx < argc; )
    {
      /* Break out of the loop when the last option has been parsed */

      ptr = argv[argndx];
      if (*ptr != '-')
        {
          break;
        }

      /* Otherwise, check for common options */

      nargs = spitool_common_args(spitool, &argv[argndx]);
      if (nargs < 0)
        {
          return ERROR;
        }

      argndx += nargs;
    }

  /* There may be transmit data on the command line */

  if (argc - argndx > spitool->count)
    {
      spitool_printf(spitool, g_spitoomanyargs, argv[0]);
      return ERROR;
    }


  while (argndx < argc)
    {
      FAR uint8_t *a = (uint8_t *)argv[argndx];
      while (*a)
        {
          if ((*(a + 1) == 0) || !ISHEX(*a) || !ISHEX(*(a + 1)))
            {
              /* Uneven number of characters or illegal char .... that's an error */

              spitool_printf(spitool, g_spiincompleteparam, argv[0]);
              return ERROR;
            }

          *txdatap++ = (HTOI(*a) << 4) | HTOI(*(a + 1));
          a += 2;
        }

      argndx += 1;
    }

  /* Get a handle to the SPI bus */

  fd = spidev_open(spitool->bus);
  if (fd < 0)
    {
      spitool_printf(spitool, "Failed to get bus %d\n", spitool->bus);
      return ERROR;
    }

  /* Set up the transfer profile */

  seq.dev = SPIDEV_ID(spitool->devtype, spitool->csn);
  seq.mode = spitool->mode;
  seq.nbits = spitool->width;
  seq.frequency = spitool->freq;
  seq.ntrans = 1;
  seq.trans = &trans;

  trans.deselect = true;
#ifdef CONFIG_SPI_CMDDATA
  trans.cmd = spitool->command;
#endif
  trans.delay = spitool->udelay;
  trans.nwords = spitool->count;
  trans.txbuffer = txdata;
  trans.rxbuffer = rxdata;

  ret = spidev_transfer(fd, &seq);

  close(fd);

  if (ret)
    {
      return ret;
    }

  spitool_printf(spitool, "Received: ");
  for (d = 0; d < spitool->count; d++)
    {
      if (spitool->width <= 8)
        {
          spitool_printf(spitool, "%02X ", rxdata[d]);
        }
      else
        {
          spitool_printf(spitool, "%04X ", ((uint16_t *)rxdata)[d]);
        }
    }

  spitool_printf(spitool, "\n");

  return ret;
}
