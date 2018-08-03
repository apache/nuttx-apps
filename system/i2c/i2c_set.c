/****************************************************************************
 * apps/system/i2c/i2c_set.c
 *
 *   Copyright (C) 2011, 2016 Gregory Nutt. All rights reserved.
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
 * Name: i2ccmd_set
 ****************************************************************************/

int i2ccmd_set(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv)
{
  FAR char *ptr;
  uint8_t regaddr;
  long value;
  long repititions;
  int nargs;
  int argndx;
  int ret;
  int fd;
  int i;

  /* Parse any command line arguments */

  for (argndx = 1; argndx < argc; )
    {
      /* Break out of the look when the last option has been parsed */

      ptr = argv[argndx];
      if (*ptr != '-')
        {
          break;
        }

      /* Otherwise, check for common options */

      nargs = i2ctool_common_args(i2ctool, &argv[argndx]);
      if (nargs < 0)
        {
          return ERROR;
        }
      argndx += nargs;
    }

  /* There must be at least one more thing on the command line:  The value
   * to be written.
   */

  if (argndx < argc)
    {
      value = strtol(argv[argndx], NULL, 16);
      if (i2ctool->width == 8)
        {
          if (value < 0 || value > 255)
            {
              i2ctool_printf(i2ctool, g_i2cargrange, argv[0]);
              return ERROR;
            }
        }
      else if (value < 0 || value > 65535)
        {
          i2ctool_printf(i2ctool, g_i2cargrange, argv[0]);
          return ERROR;
        }

      argndx++;
    }
  else
    {
      i2ctool_printf(i2ctool, g_i2cargrequired, argv[0]);
      return ERROR;
    }

  /* There may be one more thing on the command line:  The repitition
   * count.
   */

  repititions = 1;
  if (argndx < argc)
    {
      repititions = strtol(argv[argndx], NULL, 16);
      if (repititions < 1)
        {
          i2ctool_printf(i2ctool, g_i2cargrange, argv[0]);
          return ERROR;
        }

      argndx++;
    }

  if (argndx != argc)
    {
      i2ctool_printf(i2ctool, g_i2ctoomanyargs, argv[0]);
      return ERROR;
    }

  /* Get a handle to the I2C bus */

  fd = i2cdev_open(i2ctool->bus);
  if (fd < 0)
    {
       i2ctool_printf(i2ctool, "Failed to get bus %d\n", i2ctool->bus);
       return ERROR;
    }

  /* Loop for the requested number of repititions */

  regaddr = i2ctool->regaddr;
  ret = OK;

  for (i = 0; i < repititions; i++)
    {
      /* Write to the I2C bus */

      ret = i2ctool_set(i2ctool, fd, regaddr, (uint16_t)value);

      /* Display the result */

      if (ret == OK)
        {
          i2ctool_printf(i2ctool, "WROTE Bus: %d Addr: %02x Subaddr: %02x Value: ",
                         i2ctool->bus, i2ctool->addr, i2ctool->regaddr);
          if (i2ctool->width == 8)
            {
              i2ctool_printf(i2ctool, "%02x\n", (int)value);
            }
          else
            {
              i2ctool_printf(i2ctool, "%04x\n", (int)value);
            }
        }
      else
        {
          i2ctool_printf(i2ctool, g_i2cxfrerror, argv[0], -ret);
          break;
        }

      /* Auto-increment the address if so configured */

      if (i2ctool->autoincr)
        {
          regaddr++;
        }
    }

  (void)close(fd);
  return ret;
}

/****************************************************************************
 * Name: i2ctool_set
 ****************************************************************************/

int i2ctool_set(FAR struct i2ctool_s *i2ctool, int fd, uint8_t regaddr,
                uint16_t value)
{
  struct i2c_msg_s msg[2];
  union
  {
    uint16_t data16;
    uint8_t  data8;
  } u;
  int ret;

  /* Set up data structures */

  msg[0].frequency = i2ctool->freq;
  msg[0].addr      = i2ctool->addr;
  msg[0].flags     = I2C_M_NOSTOP;
  msg[0].buffer    = &regaddr;
  msg[0].length    = 1;

  msg[1].frequency = i2ctool->freq;
  msg[1].addr      = i2ctool->addr;
  msg[1].flags     = 0;

  if (i2ctool->width == 8)
    {
      u.data8       = (uint8_t)value;
      msg[1].buffer = &u.data8;
      msg[1].length = 1;
    }
  else
    {
      u.data16      = value;
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
      msg[1].flags |= I2C_M_NOSTART;
      ret = i2cdev_transfer(fd, msg, 2);
    }

  return ret;
}
