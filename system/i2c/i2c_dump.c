/****************************************************************************
 * apps/system/i2c/i2c_dump.c
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
 * Name: i2ctool_dump
 ****************************************************************************/

static int i2ctool_dump(FAR struct i2ctool_s *i2ctool, int fd,
                        uint8_t regaddr, FAR uint8_t *buf, int nbytes)
{
  struct i2c_msg_s msg[2];
  int ret;

  /* Set up data structures */

  if (i2ctool->hasregindx)
    {
      msg[0].frequency = i2ctool->freq;
      msg[0].addr      = i2ctool->addr;
      msg[0].flags     = I2C_M_NOSTOP;
      msg[0].buffer    = &regaddr;
      msg[0].length    = 1;

      msg[1].frequency = i2ctool->freq;
      msg[1].addr      = i2ctool->addr;
      msg[1].flags     = I2C_M_READ;

      msg[1].buffer = buf;
      msg[1].length = nbytes;

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
    }
  else
    {
      /* no register index "-r" has been specified so
       * we do a pure read (no write of index)
       */

      msg[0].frequency = i2ctool->freq;
      msg[0].addr      = i2ctool->addr;
      msg[0].flags     = I2C_M_READ;

      msg[0].buffer = buf;
      msg[0].length = nbytes;

      ret = i2cdev_transfer(fd, msg, 1);
    }

  return ret;
}

/****************************************************************************
 * Name: i2ccmd_dump
 ****************************************************************************/

int i2ccmd_dump(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv)
{
  FAR char *ptr;
  FAR uint8_t *buf;
  uint8_t regaddr;
  int dumpcnt;
  int nargs;
  int argndx;
  int ret;
  int fd;

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

  /* There may be one more thing on the command line:  The repetition
   * count.
   */

  dumpcnt = 1;
  if (argndx < argc)
    {
      dumpcnt = atoi(argv[argndx]);
      if (dumpcnt < 1 || dumpcnt > MAX_DUMP_CNT)
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

  regaddr = i2ctool->regaddr;
  ret = OK;

  /* allocate read buffer */

  if (NULL == (buf = calloc(1, MAX_DUMP_CNT)))
    {
      fprintf(stderr, g_i2ccmdfailed, "dump", errno);
      goto errout_with_fildes;
    }

  /* Read from the I2C bus */

  ret = i2ctool_dump(i2ctool, fd, regaddr, buf, dumpcnt);

  /* Display the result */

  if (ret == OK)
    {
      i2ctool_printf(i2ctool, "DUMP Bus: %d Addr: %02x Subaddr: ",
                     i2ctool->bus, i2ctool->addr);

      /* if the index register was set, print it */

      if (i2ctool->hasregindx)
        {
          i2ctool_printf(i2ctool, "%02x\n", regaddr);
        }
      else
        {
          i2ctool_printf(i2ctool, "--\n");
        }

      /* dump the data in hex and ASCII format */

      i2ctool_hexdump(OUTSTREAM(i2ctool), buf, dumpcnt);
    }
  else
    {
      i2ctool_printf(i2ctool, g_i2cxfrerror, argv[0], -ret);
    }

  /* free read buffer */

  free(buf);

errout_with_fildes:
  close(fd);
  return ret;
}
