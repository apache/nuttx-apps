/****************************************************************************
 * apps/system/i2c/i2c_dev.c
 *
 *   Copyright (C) 2011, 2016, 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <nuttx/i2c/i2c_master.h>

#include "i2ctool.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2ccmd_dev
 ****************************************************************************/

int i2ccmd_dev(FAR struct i2ctool_s *i2ctool, int argc, char **argv)
{
  struct i2c_msg_s msg[2];
  FAR char *ptr;
  union
  {
    uint16_t data16;
    uint8_t  data8;
  } u;

  uint8_t regaddr;
  uint8_t saveaddr;
  long first;
  long last;
  int addr;
  int nargs;
  int argndx;
  int ret;
  int fd;
  int i;
  int j;

  /* A register address other than zero may be provided on the command line.
   * However, for backward compatibility, the address zero will be used
   * unless the address is specifically included on the command line for
   * this command.
   */

  saveaddr         = i2ctool->regaddr;
  i2ctool->regaddr = 0;

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

      nargs = i2ctool_common_args(i2ctool, &argv[argndx]);
      if (nargs < 0)
        {
          goto errout;
        }

      argndx += nargs;
    }

  /* There should be exactly two more things on the command line:  The first and
   * last addresses to be probed.
   */

  if (argndx + 1 < argc)
    {
      first = strtol(argv[argndx], NULL, 16);
      last  = strtol(argv[argndx + 1], NULL, 16);
      if (first < 0 || first > 0x7f || last < 0 || last > 0x7f || first > last)
        {
          i2ctool_printf(i2ctool, g_i2cargrange, argv[0]);
          goto errout;
        }

      argndx += 2;
    }
  else
    {
      i2ctool_printf(i2ctool, g_i2cargrequired, argv[0]);
      goto errout;
    }

  if (argndx != argc)
    {
      i2ctool_printf(i2ctool, g_i2ctoomanyargs, argv[0]);
      goto errout;
    }

  /* Get a handle to the I2C bus */

  fd = i2cdev_open(i2ctool->bus);
  if (fd < 0)
    {
       i2ctool_printf(i2ctool, "Failed to get bus %d\n", i2ctool->bus);
       goto errout;
    }

  /* Probe each address */

  i2ctool_printf(i2ctool, "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
  for (i = 0; i < 128; i += 16)
    {
      i2ctool_printf(i2ctool, "%02x: ", i);
      for (j = 0; j < 16; j++)
        {
          /* Skip addresses that are out of the selected range */

          addr = i + j;
          if (addr < first || addr > last)
            {
              i2ctool_printf(i2ctool, "   ");
              continue;
            }

          /* Set up data structures */

          regaddr          = i2ctool->regaddr;

          msg[0].frequency = i2ctool->freq;
          msg[0].addr      = addr;
          msg[0].flags     = I2C_M_NOSTOP;
          msg[0].buffer    = &regaddr;
          msg[0].length    = 1;

          msg[1].frequency = i2ctool->freq;
          msg[1].addr      = addr;
          msg[1].flags     = I2C_M_READ;

          if (i2ctool->width == 8)
            {
              msg[1].buffer = &u.data8;
              msg[1].length = 1;
            }
          else
            {
              msg[1].buffer = (uint8_t*)&u.data16;
              msg[1].length = 2;
            }

          if (i2ctool->start)
            {
              ret = i2cdev_transfer(fd, &msg[0], 1);
              if (ret == OK)
                {
                  ret = i2cdev_transfer(fd, &msg[1], 1);
                }
            }
          else
            {
              ret = i2cdev_transfer(fd, msg, 2);
            }

          if (ret == OK)
            {
              i2ctool_printf(i2ctool, "%02x ", addr);
            }
          else
            {
              i2ctool_printf(i2ctool, "-- ");
            }
        }

      i2ctool_printf(i2ctool, "\n");
      i2ctool_flush(i2ctool);
    }

  (void)close(fd);

errout:

  /* Restore the previous "sticky" register address unless a new register
   * address was provided on the command line.  In that case the new
   * register address is retained.
   */

  if (i2ctool->regaddr == 0)
    {
      i2ctool->regaddr = saveaddr;
    }

  return OK;
}
