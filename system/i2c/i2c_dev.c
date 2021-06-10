/****************************************************************************
 * apps/system/i2c/i2c_dev.c
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
  struct i2c_msg_s msg;
  FAR char *ptr;
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

  /* There should be exactly two more things on the command line:
   * The first and last addresses to be probed.
   */

  if (argndx + 1 < argc)
    {
      first = strtol(argv[argndx], NULL, 16);
      last  = strtol(argv[argndx + 1], NULL, 16);
      if (first < 0 || first > 0x7f || last < 0 ||
          last > 0x7f || first > last)
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
      i2ctool_printf(i2ctool, "Failed to get bus %d\n",
                      i2ctool->bus);
      goto errout;
    }

  /* Probe each address */

  i2ctool_printf(i2ctool,
                 "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
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

          regaddr       = i2ctool->regaddr;

          msg.frequency = i2ctool->freq;
          msg.addr      = addr;
          msg.flags     = I2C_M_READ;
          msg.buffer    = &regaddr;
          msg.length    = 1;

          ret = i2cdev_transfer(fd, &msg, 1);

          if (ret == OK)
            {
              i2ctool_printf(i2ctool, "%02x ", addr);
            }
          else
            {
              i2ctool_printf(i2ctool, "-- ");
            }

          i2ctool_flush(i2ctool);
        }

      i2ctool_printf(i2ctool, "\n");
      i2ctool_flush(i2ctool);
    }

  close(fd);

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
