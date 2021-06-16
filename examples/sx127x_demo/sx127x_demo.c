/****************************************************************************
 * apps/examples/sx127x_demo/sx127x_demo.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <debug.h>
#include <poll.h>
#include <fcntl.h>

#include <nuttx/wireless/lpwan/sx127x.h>
#include <nuttx/input/buttons.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEV_NAME      "/dev/sx127x"
#define TX_BUFFER_MAX 255

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum app_mode_e
{
  APP_MODE_RX   = 0,
  APP_MODE_TX   = 1,
  APP_MODE_RXTX = 2,
  APP_MODE_SCAN = 3
};

enum app_modulation_e
{
  APP_MODULATION_LORA = 0,
  APP_MODULATION_FSK  = 1,
  APP_MODULATION_OOK  = 2,
};

/* Application arguments */

struct args_s
{
  uint32_t frequency;
  int16_t  interval;
  int16_t  time;
  uint8_t  app_mode;
  uint8_t  modulation;
  uint8_t  datalen;
  int8_t   power;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct args_s g_args;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sx127x_help
 ****************************************************************************/

static void sx127x_help(FAR struct args_s *args)
{
  printf("Usage: sx127x [OPTIONS]\n\n");
  printf("  [-m modulation] modulation scheme (default=0)\n");
  printf("       0 - LORA\n");
  printf("       1 - FSK\n");
  printf("       2 - OOK\n");
  printf("  [-f frequency Hz] RF frequency (default=%d)\n",
         CONFIG_EXAMPLES_SX127X_RFFREQ);
  printf("  [-i interval sec] radio access time interval (default=%d)\n",
         CONFIG_EXAMPLES_SX127X_INTERVAL);
  printf("  [-l datalen] data length for TX (default=%d)\n",
         CONFIG_EXAMPLES_SX127X_TXDATA);
  printf("  [-d time sec] demo time, 0 if infinity (default=%d)\n",
         CONFIG_EXAMPLES_SX127X_TIME);
  printf("  [-r/-t/-x/-s] select app mode (default=r)\n");
  printf("       r - RX\n");
  printf("       t - TX\n");
  printf("       x - RX/TX (not supported yet)\n");
  printf("       s - SCAN\n");
  printf("  [-p power dBm] TX power (default=%d)\n",
         CONFIG_EXAMPLES_SX127X_TXPOWER);
  printf("  [-h]  print this message\n");
}

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

static int arg_decimal(FAR char **arg, FAR int *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = atoi(string);

  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct args_s *args, int argc, FAR char **argv)
{
  FAR char *ptr;
  int index;
  int nargs;
  int i_value;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          /* Interval between RX/TX operations */

          case 'i':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->interval = i_value;

              break;
            }

          /* Demo time */

          case 'd':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->time = i_value;

              break;
            }

          /* Data length for TX */

          case 'l':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->datalen = i_value;

              break;
            }

            /* Modem RF frequency */

          case 'f':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->frequency = i_value;

              break;
            }

          /* Modem modulation scheme */

          case 'm':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->modulation = i_value;

              break;
            }

          /* Modem TX power */

          case 'p':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->power = i_value;

              break;
            }

          /* RX mode */

          case 'r':
            {
              args->app_mode = APP_MODE_RX;
              index += 1;

              break;
            }

          /* TX mode */

          case 't':
            {
              args->app_mode = APP_MODE_TX;
              index += 1;

              break;
            }

          /* RX-TX mode */

          case 'x':
            {
              args->app_mode = APP_MODE_RXTX;
              index += 1;

              break;
            }

          /* Scan mode */

          case 's':
            {
              args->app_mode = APP_MODE_SCAN;
              index += 1;

              break;
            }

          /* Print help message */

          case 'h':
            {
              sx127x_help(args);
              exit(0);
            }

          /* Unsupported option */

          default:
            {
              printf("Unsupported option: %s\n", ptr);
              sx127x_help(args);
              exit(1);
            }
        }
    }
}

/****************************************************************************
 * Name: validate_args
 ****************************************************************************/

static int validate_args(FAR struct args_s *args)
{
  int ret = OK;

  /* TODO */

  return ret;
}

/****************************************************************************
 * Name: print_hex
 ****************************************************************************/

static void print_hex(uint8_t *data, int len)
{
  int i;

  if (len == 0)
    {
      printf("empty buffer!\n");
    }
  else
    {
      for (i = 0 ; i < len ; i += 1)
        {
          printf("0x%02x ", data[i]);

          if ((i + 1) % 10 == 0)
            {
              printf("\n");
            }
        }
    }

  printf("\n");
}

/****************************************************************************
 * Name: modulation_set
 ****************************************************************************/

static int modulation_set(int fd, uint8_t modulation)
{
  int ret = OK;

  switch (modulation)
    {
      case APP_MODULATION_LORA:
        {
          printf("LORA modulation\n");

          modulation = SX127X_MODULATION_LORA;
          ret = ioctl(fd, SX127XIOC_MODULATIONSET,
                      (unsigned long)&modulation);
          if (ret < 0)
            {
              printf("failed change modulation %d!\n", ret);
              goto errout;
            }
          break;
        }

      case APP_MODULATION_FSK:
        {
          printf("FSK modulation\n");

          modulation = SX127X_MODULATION_FSK;
          ret = ioctl(fd, SX127XIOC_MODULATIONSET,
                      (unsigned long)&modulation);
          if (ret < 0)
            {
              printf("failed change modulation %d!\n", ret);
              goto errout;
            }
          break;
        }

      case APP_MODULATION_OOK:
        {
          printf("OOK modulation\n");

          modulation = SX127X_MODULATION_OOK;
          ret = ioctl(fd, SX127XIOC_MODULATIONSET,
                      (unsigned long)&modulation);
          if (ret < 0)
            {
              printf("failed change modulation %d!\n", ret);
              goto errout;
            }
          break;
        }

      default:
        {
          printf("Unsupported app modulation %d!\n", modulation);
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_LPWAN_SX127X_RXSUPPORT
  struct sx127x_read_hdr_s data;
#endif
  struct sx127x_chanscan_ioc_s chanscan;
  struct args_s   *args;
  struct timespec tstart;
  struct timespec tnow;
  uint8_t buffer[TX_BUFFER_MAX];
  uint8_t opmode;
  uint8_t i;
  int ret;
  int fd;

  /* Initialize buffer with data */

  for (i = 0; i < TX_BUFFER_MAX; i += 1)
    {
      buffer[i] = i;
    }

  /* Initialize variables */

  args = &g_args;
  args->app_mode   = APP_MODE_RX;
  args->modulation = APP_MODULATION_LORA;
  args->frequency  = CONFIG_EXAMPLES_SX127X_RFFREQ;
  args->power      = CONFIG_EXAMPLES_SX127X_TXPOWER;
  args->interval   = CONFIG_EXAMPLES_SX127X_INTERVAL;
  args->time       = CONFIG_EXAMPLES_SX127X_TIME;
  args->datalen    = CONFIG_EXAMPLES_SX127X_TXDATA;

  /* Parse the command line */

  parse_args(args, argc, argv);

  /* Validate arguments */

  ret = validate_args(args);
  if (ret != OK)
    {
      printf("sx127x_main: validate arguments failed!\n");
      goto errout;
    }

  printf("Start sx127x_demo\n");

  /* Open device */

  fd = open(DEV_NAME, O_RDWR);
  if (fd < 0)
    {
      int errcode = errno;
      printf("ERROR: Failed to open device %s: %d\n", DEV_NAME, errcode);
      goto errout;
    }

  /* Set modulation */

  ret = modulation_set(fd, args->modulation);
  if (ret < 0)
    {
      printf("modulation_set failed\n");
      goto errout;
    }

  /* Set RF frequency */

  printf("Set frequency to %" PRId32 "\n", args->frequency);

  ret = ioctl(fd, WLIOC_SETRADIOFREQ, (unsigned long)&args->frequency);
  if (ret < 0)
    {
      printf("failed to change frequency %d!\n", ret);
      goto errout;
    }

  /* Set TX power */

  printf("Set power to %d\n", args->power);

  ret = ioctl(fd, WLIOC_SETTXPOWER, (unsigned long)&args->power);
  if (ret < 0)
    {
      printf("failed to change power %d!\n", ret);
      goto errout;
    }

  /* Get start time */

  clock_gettime(CLOCK_REALTIME, &tstart);

  while (1)
    {
      switch (args->app_mode)
        {
#ifdef CONFIG_LPWAN_SX127X_TXSUPPORT
          /* Transmit some data */

          case APP_MODE_TX:
            {
              printf("\nSend %d bytes\n", args->datalen);

              ret = write(fd, buffer, args->datalen);
              if (ret < 0)
                {
                  printf("write failed %d!\n", ret);
                  goto errout;
                }

              break;
            }
#endif

#ifdef CONFIG_LPWAN_SX127X_RXSUPPORT
          /* Receive data */

          case APP_MODE_RX:
            {
              /* Set radio in RX mode */

              opmode = SX127X_OPMODE_RX;

              ret = ioctl(fd, SX127XIOC_OPMODESET, (unsigned long)&opmode);
              if (ret < 0)
                {
                  printf("failed change opmode to RX %d!\n", ret);
                  goto errout;
                }

              /* TODO: add RX poll if configured */

              printf("Waiting for data\n");

              ret = read(fd, &data, sizeof(struct sx127x_read_hdr_s));
              if (ret < 0)
                {
                  printf("Read failed %d!\n", ret);
                  goto errout;
                }

              printf("\nReceived:\n");
              printf("SNR  = %d\n", data.snr);
              printf("RSSI = %d\n", data.rssi);
              printf("len  = %d\n", data.datalen);
              print_hex(data.data, data.datalen);
              printf("\n");

              break;
            }
#endif

          /* Send some data and wait for response */

          case APP_MODE_RXTX:
            {
              printf("TODO: APP RXTX\n");
              break;
            }

          /* Scan channel */

          case APP_MODE_SCAN:
            {
              /* TODO: Configure this from command line */

              chanscan.freq     = args->frequency;
              chanscan.rssi_thr = -30;
              chanscan.stime    = 2;
              chanscan.free     = false;
              chanscan.rssi_max = 0;

              ret = ioctl(fd, SX127XIOC_CHANSCAN, (unsigned long)&chanscan);
              if (ret < 0)
                {
                  printf("failed chanscan %d!\n", ret);
                  goto errout;
                }

              printf("freq = %" PRId32 " max = %d min = %d free = %d\n",
                     chanscan.freq,
                     chanscan.rssi_max, chanscan.rssi_min, chanscan.free);

              break;
            }

          default:
            {
              printf("Unsupported app mode!\n");
              goto errout;
            }
        }

      printf("wait %d sec ...\n", args->interval);
      sleep(args->interval);

      if (args->time > 0)
        {
          /* Get time now */

          clock_gettime(CLOCK_REALTIME, &tnow);

          if (tnow.tv_sec - tstart.tv_sec >= args->time)
            {
              printf("Timeout - force exit!\n");
              goto errout;
            }
        }
    }

errout:
  close(fd);
  return 0;
}
