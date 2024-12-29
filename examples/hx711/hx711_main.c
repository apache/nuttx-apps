/****************************************************************************
 * apps/examples/hx711/hx711_main.c
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
#include <nuttx/analog/hx711.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct hx711_config
{
  char dev[32];
  char dump;
  char channel;
  unsigned char gain;
  unsigned average;
  unsigned loops;
  float precision;
  int val_per_unit;
  int sign;
} g_hx711_config;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hx711_print_help
 *
 * Description:
 *   Prints help onto stdout.
 *
 ****************************************************************************/

static void hx711_print_help(const char *progname)
{
  printf("usage: %s [-d <device>] [options]\n"
         "\n"
         "\t-h       print this help message\n"
         "\t-d<path> path to hx711 device, default: /dev/hx711_0\n"
         "\t-t<prec> tares the scale with specified precision, might"
         "take few seconds to complete.\n"
         "\t         If you set value per unit, precision is in units, "
         "otherwise it's raw values.\n"
         "\t         If units are used, float can be passed like 0.1\n"
         "\t-v<val>  value read that coresponds to one unit. This value "
         "has to be\n"
         "\t         calibrated first before it's known\n"
         "\t-s       reverse sign, if values decreses when mass increases, "
         "pass this\n"
         "\t-D       dumps current device settings (like, average, channel, "
         "gain etc.)\n"
         "\t-a<avg>  set how many samples should be averaged before "
         "returning value,\n"
         "\t         values [1..%d] are valid\n"
         "\t-c<chan> set channel to read (either 'a' or 'b' is valid)\n"
         "\t-g<gain> set adc gain, for channel 'a' 64 and 128 are valid,\n"
         "\t         for channel 'b', only 64 is valid\n"
         "\t-r<num>  read this number of samples before exiting, samples "
         "will be printed\n"
         "\t         on stdout as string, one sample per line\n"
         "\n"
         "Check documentation for full description\n",
         progname, HX711_MAX_AVG_SAMPLES);
}

/****************************************************************************
 * Name: hx711_parse_args
 *
 * Description:
 *   Parses command line arguments
 *
 ****************************************************************************/

static int hx711_parse_args(int argc, FAR char *argv[])
{
  int opt;

  memset(&g_hx711_config, 0x00, sizeof(g_hx711_config));

  while ((opt = getopt(argc, argv, "hd:Dt:a:c:g:r:v:s")) != -1)
    {
      switch (opt)
        {
        case 'h':
          hx711_print_help(argv[0]);
          return 1;

        case 'd':
          strcpy(g_hx711_config.dev, optarg);
          break;

        case 't':
          g_hx711_config.precision = atof(optarg);
          break;

        case 'D':
          g_hx711_config.dump = 1;
          break;

        case 'a':
          g_hx711_config.average = atoi(optarg);
          break;

        case 'c':
          g_hx711_config.channel = optarg[0];
          break;

        case 'g':
          g_hx711_config.gain = atoi(optarg);
          break;

        case 'r':
          g_hx711_config.loops = atoi(optarg);
          break;

        case 's':
          g_hx711_config.sign = -1;
          break;

        case 'v':
          g_hx711_config.val_per_unit = atoi(optarg);
          break;

        default:
          fprintf(stderr, "Invalid option, run with -h for help\n");
          return 1;
        }
    }

  if (g_hx711_config.dev[0] == '\0')
    {
      strcpy(g_hx711_config.dev, "/dev/hx711_0");
    }

  return 0;
}

/****************************************************************************
 * Name: hx711_set_options
 *
 * Description:
 *   Set hx711 options if they were specified on command line
 *
 * Input Parameters:
 *   fd - opened hx711 instance
 *
 ****************************************************************************/

int hx711_set_options(int fd)
{
  int ret;

  /* Set channel to read */

  if (g_hx711_config.channel > 0)
    {
      if ((ret = ioctl(fd, HX711_SET_CHANNEL, g_hx711_config.channel)))
        {
          fprintf(stderr, "Failed to set channel to %c, error: %s\n",
                  g_hx711_config.channel, strerror(errno));
          return -1;
        }
    }

  /* Set channel ADC gain */

  if (g_hx711_config.gain > 0)
    {
      if ((ret = ioctl(fd, HX711_SET_GAIN, g_hx711_config.gain)))
        {
          fprintf(stderr, "Failed to set gain to %d, error: %s\n",
                  g_hx711_config.gain, strerror(errno));
          return -1;
        }
    }

  /* Set how many samples to average before printing value */

  if (g_hx711_config.average > 0)
    {
      if ((ret = ioctl(fd, HX711_SET_AVERAGE, g_hx711_config.average)))
        {
          fprintf(stderr, "Failed to set average to %d, error: %s\n",
                  g_hx711_config.average, strerror(errno));
          return -1;
        }
    }

  /* Set if sign should be reversed or not */

  if (g_hx711_config.sign)
    {
      if ((ret = ioctl(fd, HX711_SET_SIGN, &g_hx711_config.sign)))
        {
          fprintf(stderr, "Failed to set sign to %d, error: %s\n",
                  g_hx711_config.sign, strerror(errno));
          return -1;
        }
    }

  /* Set what value coresponds to 1 unit */

  if (g_hx711_config.val_per_unit > 0)
    {
      if ((ret = ioctl(fd, HX711_SET_VAL_PER_UNIT,
                       g_hx711_config.val_per_unit)))
        {
          fprintf(stderr, "Failed to set val per unit to %d, error: %s\n",
                  g_hx711_config.val_per_unit, strerror(errno));
          return -1;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: hx711_dump_options
 *
 * Description:
 *   Reads current hx711 settings from kernel driver and dumps them on stdout
 *
 ****************************************************************************/

static void hx711_dump_options(int fd)
{
  unsigned average;
  unsigned char gain;
  char channel;
  unsigned val_per_unit;

  ioctl(fd, HX711_GET_GAIN, &gain);
  ioctl(fd, HX711_GET_CHANNEL, &channel);
  ioctl(fd, HX711_GET_AVERAGE, &average);
  ioctl(fd, HX711_GET_VAL_PER_UNIT, &val_per_unit);

  printf("Current settings for: %s\n", g_hx711_config.dev);
  printf("average.............: %u\n", average);
  printf("channel.............: %c\n", channel);
  printf("gain................: %d\n", gain);
  printf("value per unit......: %u\n", val_per_unit);
}

/****************************************************************************
 * Name: hx711_read_and_dump
 *
 * Description:
 *   Reads configured number of samples, and dumps them to terminal.
 *
 ****************************************************************************/

static int hx711_read_and_dump(int fd)
{
  int32_t sample;

  while (g_hx711_config.loops--)
    {
      if (read(fd, &sample, sizeof(sample)) != sizeof(sample))
        {
          fprintf(stderr, "Error reading from %s, error %s\n",
                  g_hx711_config.dev, strerror(errno));
          return -1;
        }

      printf("%"PRId32"\n", sample);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hx711
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;

  if (hx711_parse_args(argc, argv))
    {
      return 1;
    }

  fd = open(g_hx711_config.dev, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "Failed to open %s, error: %s\n", g_hx711_config.dev,
              strerror(errno));
      return 1;
    }

  if (hx711_set_options(fd))
    {
      close(fd);
      return 1;
    }

  if (g_hx711_config.precision)
    {
      ioctl(fd, HX711_GET_VAL_PER_UNIT, &g_hx711_config.val_per_unit);

      if (g_hx711_config.val_per_unit)
        {
          printf("Taring with %fg precision\n", g_hx711_config.precision);
        }

      if (ioctl(fd, HX711_TARE, &g_hx711_config.precision))
        {
          perror("Failed to tare the scale");
          close(fd);
          return 1;
        }
    }

  if (g_hx711_config.dump)
    {
      hx711_dump_options(fd);
    }

  if (g_hx711_config.loops)
    {
      hx711_read_and_dump(fd);
    }

  close(fd);
  return 0;
}
