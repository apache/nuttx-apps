/****************************************************************************
 * apps/examples/npzero/npzero_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/power/npzero.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Peripherals of the nPZero G1S Development Kit */

#define ACC_PERIPH     3    /* LIS2DW12 accelerometer, SPI, PMOD 3 */
#define TEMP_PERIPH    4    /* AS6212 temperature sensor, I2C, PMOD 4 */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* LIS2DW12: CTRL1 (0x20) = 0x10 (12.5 Hz, low-power), then read OUT_X_L
 * (0x28 | read bit).  Wake up when |X| >= 1000 (about 61 mg).
 */

static const uint8_t g_acc_init[] =
{
  0x20, 0x10
};

static const uint8_t g_acc_read[] =
{
  0xa8
};

static const struct npzero_periph_s g_acc =
{
  .pwmod    = NPZERO_PWMOD_PERIODIC,
  .tmod     = NPZERO_TMOD_READ,
  .pswmod   = NPZERO_PSWMOD_LOGIC,
  .intmod   = NPZERO_INTMOD_IN_HIGH,
  .spi      = true,
  .spimode  = 0,
  .dtype    = NPZERO_DTYPE_INT16,
  .period   = 50,                 /* 5 s with the 10 Hz clock */
  .throver  = 1000,
  .thrunder = (uint16_t)-1000,
  .twtp     = 10,
  .tinit    = NPZERO_WAIT_256,
  .twait    = NPZERO_WAIT_256,
  .ninit    = sizeof(g_acc_init),
  .init     = g_acc_init,
  .nread    = sizeof(g_acc_read),
  .read     = g_acc_read,
};

/* AS6212 at 0x49: CONFIG (0x01) = 0x82, TLOW (0x02) = 0xa0, then read the
 * temperature (0x00, big endian).  Wake up when T >= 25 C or T <= 10 C.
 */

static const uint8_t g_temp_init[] =
{
  0x01, 0x82, 0x02, 0xa0
};

static const struct npzero_periph_s g_temp =
{
  .pwmod    = NPZERO_PWMOD_PERIODIC,
  .tmod     = NPZERO_TMOD_READ,
  .pswmod   = NPZERO_PSWMOD_LOGIC,
  .intmod   = NPZERO_INTMOD_IN_HIGH,
  .dtype    = NPZERO_DTYPE_INT16,
  .seqrw    = true,
  .wunak    = true,
  .swap     = true,
  .addr     = 0x49,
  .rreg     = 0x00,
  .retries  = 3,
  .period   = 300,                /* 30 s with the 10 Hz clock */
  .throver  = 3200,               /* 25 C in units of 1/128 C */
  .thrunder = 1280,               /* 10 C */
  .twtp     = 0x31,               /* 49 x 640 us = 31.36 ms */
  .tinit    = NPZERO_WAIT_256,
  .twait    = NPZERO_WAIT_256,
  .ninit    = sizeof(g_temp_init) / 2,
  .init     = g_temp_init,
};

static const struct npzero_config_s g_config =
{
  .hostmode = NPZERO_HOSTMODE_LOGIC,
  .pswint   = (1 << (ACC_PERIPH - 1)) | (1 << (TEMP_PERIPH - 1)),
  .wakeup   = NPZERO_SYSCFG1_WUP_P(ACC_PERIPH) |
              NPZERO_SYSCFG1_WUP_P(TEMP_PERIPH),
  .sclksel  = NPZERO_SCLK_LPOSC,
  .sclkdiv  = NPZERO_SCLKDIV_1,
  .adcclk   = NPZERO_ADCCLK_SCLK,
  .i2cpull  = NPZERO_I2CPULL_AUTO,
  .clkout   = NPZERO_CLKOUT_OFF,
  .timeout  = 3000,               /* 300 s with the 10 Hz clock */
  .intpull  =
  {
    NPZERO_INTPULL_50K, NPZERO_INTPULL_50K,
    NPZERO_INTPULL_50K, NPZERO_INTPULL_50K
  },
  .periph   =
  {
    NULL, NULL, &g_acc, &g_temp
  },
};

/* Lowest input voltage (mV) of the ADC codes (datasheet, Table 50) */

static const uint8_t g_adc_ext_code[] =
{
  0x00, 0x08, 0x0f, 0x15, 0x19, 0x1c, 0x1f, 0x22, 0x23, 0x24, 0x26, 0x28,
  0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34
};

static const uint16_t g_adc_ext_mv[] =
{
  600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700,
  1800, 1900, 2000, 2100, 2200, 2300, 2400, 2600, 2700, 2900, 3100, 3400
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: npzero_adc_mv
 *
 * Description:
 *   Convert an ADC code to the lowest matching input voltage.  The codes of
 *   the VBAT channel are the ones of the ADC_IN channel minus 0x20.
 *
 ****************************************************************************/

static int npzero_adc_mv(uint8_t code)
{
  int i;

  for (i = sizeof(g_adc_ext_code) - 1; i > 0; i--)
    {
      if (code >= g_adc_ext_code[i])
        {
          break;
        }
    }

  return g_adc_ext_mv[i];
}

/****************************************************************************
 * Name: npzero_show_value
 ****************************************************************************/

static void npzero_show_value(int fd, int periph)
{
  struct npzero_value_s value;
  int32_t v;
  int32_t a;

  value.periph = periph;
  if (ioctl(fd, NPZEROIOC_GETVALUE, (unsigned long)&value) < 0)
    {
      printf("ERROR: GETVALUE failed: %d\n", errno);
      return;
    }

  v = (int16_t)value.value;
  if (periph == ACC_PERIPH)
    {
      /* LIS2DW12, +/-2 g full scale: 1 LSB = 2000 / 32768 mg */

      printf("  Acceleration X: %" PRId32 " mg\n", v * 125 / 2048);
    }
  else if (periph == TEMP_PERIPH)
    {
      /* AS6212: 1 LSB = 1/128 C */

      v = v * 1000 / 128;
      a = v < 0 ? -v : v;
      printf("  Temperature: %s%" PRId32 ".%03" PRId32 " C\n",
             v < 0 ? "-" : "", a / 1000, a % 1000);
    }
  else
    {
      printf("  Value: 0x%04x\n", value.value);
    }
}

/****************************************************************************
 * Name: npzero_show_status
 *
 * Description:
 *   Show the reason of the last wake-up of the host and the last values of
 *   the peripherals that triggered it.
 *
 ****************************************************************************/

static int npzero_show_status(int fd)
{
  struct npzero_status_s status;
  struct npzero_adc_s adc;
  int i;

  if (ioctl(fd, NPZEROIOC_GETSTATUS, (unsigned long)&status) < 0)
    {
      printf("ERROR: GETSTATUS failed: %d\n", errno);
      return -errno;
    }

  printf("STA1: 0x%02x STA2: 0x%02x\n", status.sta1, status.sta2);

  /* Only the soft reset bit of RST_SRC is reliable (errata #5) */

  if (status.sta1 & NPZERO_STA1_RSTSRC_SOFT)
    {
      printf("Soft reset\n");
    }

  if (status.sta1 & NPZERO_STA1_FTOUT)
    {
      printf("Global time-out\n");
    }

  if (status.sta1 & (NPZERO_STA1_FA1 | NPZERO_STA1_FA2))
    {
      if (ioctl(fd, NPZEROIOC_GETADC, (unsigned long)&adc) >= 0)
        {
          if (status.sta1 & NPZERO_STA1_FA1)
            {
              printf("VBAT trigger: 0x%02x (>= %d mV)\n", adc.core,
                     npzero_adc_mv(adc.core + 0x20));
            }

          if (status.sta1 & NPZERO_STA1_FA2)
            {
              printf("ADC_IN trigger: 0x%02x (>= %d mV)\n", adc.ext,
                     npzero_adc_mv(adc.ext));
            }
        }
    }

  for (i = 1; i <= NPZERO_NPERIPH; i++)
    {
      if (status.sta2 & NPZERO_STA2_FNAKP(i))
        {
          printf("Peripheral %d: NAK\n", i);
        }

      if (status.sta2 & NPZERO_STA2_FP(i))
        {
          printf("Peripheral %d: trigger\n", i);
          npzero_show_value(fd, i);
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * npzero_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  bool statusonly = false;
  uint8_t id;
  int ret;
  int fd;

  if (argc > 1)
    {
      if (strcmp(argv[1], "-s") != 0)
        {
          printf("Usage: %s [-s]\n"
                 "  -s: show the status only, do not enter Idle state\n",
                 argv[0]);
          return EXIT_FAILURE;
        }

      statusonly = true;
    }

  fd = open(CONFIG_EXAMPLES_NPZERO_DEVPATH, O_RDWR);
  if (fd < 0)
    {
      printf("ERROR: open %s failed: %d\n",
             CONFIG_EXAMPLES_NPZERO_DEVPATH, errno);
      return EXIT_FAILURE;
    }

  ret = ioctl(fd, NPZEROIOC_GETID, (unsigned long)&id);
  if (ret < 0)
    {
      printf("ERROR: GETID failed: %d\n", errno);
      goto errout;
    }

  printf("nPZero G1S ID: 0x%02x\n", id);

  ret = npzero_show_status(fd);
  if (ret < 0 || statusonly)
    {
      goto errout;
    }

  ret = ioctl(fd, NPZEROIOC_CONFIGURE, (unsigned long)&g_config);
  if (ret < 0)
    {
      printf("ERROR: CONFIGURE failed: %d\n", errno);
      goto errout;
    }

  printf("Configured, Idle state in %d ms\n",
         CONFIG_EXAMPLES_NPZERO_IDLE_DELAY);
  usleep(CONFIG_EXAMPLES_NPZERO_IDLE_DELAY * 1000);

  /* The nPZero switches the host off now, until the next wake-up */

  ret = ioctl(fd, NPZEROIOC_IDLE, 0);
  if (ret < 0)
    {
      printf("ERROR: IDLE failed: %d\n", errno);
    }

errout:
  close(fd);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
