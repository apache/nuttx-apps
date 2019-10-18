/****************************************************************************
 * apps/system/spi/spi_common.c
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

#include "spitool.h"

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

#if 0 /* Not used */
static int arg_hex(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 16);
  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spitool_common_args
 ****************************************************************************/

int spitool_common_args(FAR struct spitool_s *spitool, FAR char **arg)
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
      case 'b':
        ret = arg_decimal(arg, &value);
        if (value < CONFIG_SPITOOL_MINBUS || value > CONFIG_SPITOOL_MAXBUS)
          {
            goto out_of_range;
          }

        if (!spidev_exists((int)value))
          {
            goto invalid_argument;
          }

        spitool->bus = (uint8_t)value;
        return ret;

#ifdef CONFIG_SPI_CMDDATA
      case 'c':
        ret = arg_decimal(arg, &value);
        if ((value < 0) || (value > 1))
          {
            goto out_of_range;
          }

        spitool->command = value;
        return ret;
#endif

      case 'm':
        ret = arg_decimal(arg, &value);
        if ((value < 0) || (value > 3))
          {
            goto out_of_range;
          }

        spitool->mode = value;
        return ret;

      case 'n':
        ret = arg_decimal(arg, &value);
        if ((value < 0) || (value > 0xffff))
          {
            goto out_of_range;
          }

        spitool->csn = value;
        return ret;

      case 't':
        ret = arg_decimal(arg, &value);
        if ((value < 0) || (value > SPIDEVTYPE_USER))
          {
            goto out_of_range;
          }

        spitool->devtype = value;
        return ret;

      case 'f':
        ret = arg_decimal(arg, &value);
        if (value == 0)
          {
            goto out_of_range;
          }

        spitool->freq = value;
        return ret;

      case 'r':
        ret = arg_decimal(arg, &value);
        if (value < 0)
          {
            goto out_of_range;
          }

        spitool->count = (uint32_t)value;
        return ret;

      case 'u':
        ret = arg_decimal(arg, &value);
        if ((value < 0) || (value > 65535))
          {
            goto out_of_range;
          }

        spitool->udelay = value;
        return ret;

      case 'w':
        ret = arg_decimal(arg, &value);
        if (value != 8 && value != 16)
          {
            goto out_of_range;
          }

        spitool->width = (uint8_t)value;
        return ret;

      case 'x':
        ret = arg_decimal(arg, &value);
        if ((value < 0) || (value > MAX_XDATA))
          {
            goto out_of_range;
          }

        spitool->count = value;
        return ret;

      default:
        goto invalid_argument;
    }

invalid_argument:
  spitool_printf(spitool, g_spiarginvalid, ptr);
  return ERROR;

out_of_range:
  spitool_printf(spitool, g_spiargrange, ptr);
  return ERROR;
}
