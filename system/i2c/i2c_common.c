/****************************************************************************
 * apps/system/i2c/i2c_common.c
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

#include "i2ctool.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

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

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: arg_hex
 ****************************************************************************/

static int arg_hex(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 16);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2ctool_common_args
 ****************************************************************************/

int i2ctool_common_args(FAR struct i2ctool_s *i2ctool, FAR char **arg)
{
  FAR char *ptr = *arg;
  long value;
  int ret;

  if (ptr[0] != '-')
    {
      goto invalid_argument;
    }

  switch (ptr[1])
    {
      case 'a':
        ret = arg_hex(arg, &value);
        if (value < CONFIG_I2CTOOL_MINADDR || value > CONFIG_I2CTOOL_MAXADDR)
          {
            goto out_of_range;
          }

        i2ctool->addr = (uint8_t)value;
        return ret;

      case 'b':
        ret = arg_decimal(arg, &value);
        if (value < CONFIG_I2CTOOL_MINBUS || value > CONFIG_I2CTOOL_MAXBUS)
          {
            goto out_of_range;
          }

        if (!i2cdev_exists((int)value))
          {
            goto invalid_argument;
          }

        i2ctool->bus = (uint8_t)value;
        return ret;

      case 'f':
        ret = arg_decimal(arg, &value);
        if (value == 0)
          {
            goto out_of_range;
          }

        i2ctool->freq = value;
        return ret;

      case 'i':
        i2ctool->autoincr = true;
        return 1;

      case 'j':
        i2ctool->autoincr = false;
        return 1;

      case 'n':
        i2ctool->start = false;
        return 1;

      case 'r':
        ret = arg_hex(arg, &value);
        if (value < 0 || value > CONFIG_I2CTOOL_MAXREGADDR)
          {
            goto out_of_range;
          }

        i2ctool->regaddr = (uint8_t)value;
        i2ctool->hasregindx = true;
        return ret;

      case 's':
        i2ctool->start = true;
        return 1;

      case 'w':
        ret = arg_decimal(arg, &value);
        if (value != 8 && value != 16)
          {
            goto out_of_range;
          }

        i2ctool->width = (uint8_t)value;
        return ret;

      default:
        goto invalid_argument;
    }

invalid_argument:
  i2ctool_printf(i2ctool, g_i2carginvalid, ptr);
  return ERROR;

out_of_range:
  i2ctool_printf(i2ctool, g_i2cargrange, ptr);
  return ERROR;
}
