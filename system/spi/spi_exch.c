/****************************************************************************
 * apps/system/spi/spi_exch.c
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
              /* Uneven number of characters or illegal character error */

              spitool_printf(spitool, g_spiincompleteparam, argv[0]);
              return ERROR;
            }

          *txdatap++ = (HTOI(*a) << 4) | HTOI(*(a + 1));
          a += 2;
        }

      argndx += 1;
    }

  spitool_printf(spitool, "Sending:\t");
  for (d = 0; d < spitool->count; d++)
    {
      if (spitool->width <= 8)
        {
          spitool_printf(spitool, "%02X ", txdata[d]);
        }
      else
        {
          spitool_printf(spitool, "%04X ", ((uint16_t *)txdata)[d]);
        }
    }

  spitool_printf(spitool, "\n");

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

  spitool_printf(spitool, "Received:\t");
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
