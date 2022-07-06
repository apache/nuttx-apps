/****************************************************************************
 * apps/examples/ws2812/ws2812_main.c
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

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/leds/ws2812.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct neo_config_s
{
  FAR char  *path;
  int        loops;
  int        leds;
  int        delay;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct neo_config_s config =
{
  NULL,                    /* path  - default set in main */
  4,                       /* loop  - all colors 4 times  */
  CONFIG_WS2812_LED_COUNT, /* leds  - based on config     */
  20000                    /* delay - (in µs)  ~50Hz      */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: help
 ****************************************************************************/

static void help(FAR struct neo_config_s *conf)
{
  printf("Usage: ws2812 [OPTIONS]\n");

  printf("\nArguments are \"sticky\".  "
         "For example, once the device path is\n");
  printf("specified, that path will be re-used until it is changed.\n");

  printf("  [-p path] selects the ws2812 device.  "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_WS2812_DEFAULT_DEV, conf->path ? conf->path
                                                        : "NONE");

  printf("  [-l leds] selects number of ws2812s in the chain.  "
         "Default: %d Current: %d\n",
         CONFIG_WS2812_LED_COUNT, conf->leds);

  printf("  [-r repeat] selects the number change cycles.  "
         "Default: %d Current: %d\n",
         4, conf->loops);

  printf("  [-d delay] selects delay between updates.  "
         "Default: %d us Current: %d us\n",
         20000, conf->delay);
}

/****************************************************************************
 * Name: set_devpath
 ****************************************************************************/

static void set_devpath(FAR struct neo_config_s *conf,
                        FAR const char          *devpath)
{
  /* Get rid of any old device path */

  if (conf->path != NULL)
    {
      free(conf->path);
    }

  /* Then set-up the new device path by copying the string */

  conf->path = strdup(devpath);
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

static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int       ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct neo_config_s *conf,
                       int argc,
                       FAR char **argv)
{
  FAR char *ptr;
  FAR char *str;
  long      value;
  int       index;
  int       nargs;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          fprintf(stderr, "Invalid options format: %s\n", ptr);
          exit(1);
        }

      switch (ptr[1])
        {
          case 'l':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1 || value > INT_MAX)
              {
                fprintf(stderr, "Led count out of range: %ld\n", value);
                exit(1);
              }

            conf->leds = (uint32_t)value;
            index += nargs;
            break;

          case 'r':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1 || value > INT_MAX)
              {
                fprintf(stderr, "Repeat out of range: %ld\n", value);
                exit(1);
              }

            conf->loops = (uint32_t)value;
            index += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[index], &str);
            set_devpath(conf, str);
            index += nargs;
            break;

          case 'd':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 1 || value > INT_MAX)
              {
                fprintf(stderr, "Delay out of range: %ld\n", value);
                exit(1);
              }

            conf->delay = (int)value;
            index += nargs;
            break;

          case 'h':
            help(conf);
            exit(0);

          default:
            fprintf(stderr, "Unsupported option: %s\n", ptr);
            help(conf);
            exit(1);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ws2812_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR uint32_t *buffer;
  FAR uint32_t *bp;
  ssize_t       result;

  if (config.path == NULL)
    {
      config.path = strdup(CONFIG_EXAMPLES_WS2812_DEFAULT_DEV);
    }

  /* Parse the command line */

  parse_args(&config, argc, argv);

  /* Run the display loop */

  int fd = open(config.path, O_WRONLY);
  if (fd < 0)
    {
      fprintf(stderr,
              "ws2812_main: open %s failed: %d\n",
              config.path,
              errno);
      goto errout;
    }

  buffer = calloc(4, config.leds);

  if (buffer == NULL)
    {
      fprintf(stderr, "ws2812_main: buffer allocation failed: %d\n", errno);
      goto errout;
    }

  for (int i = 0; i < config.loops; ++i)
    {
      for (int j = 0; j < 256; ++j)
        {
          bp  = buffer;

          for (int k = 0; k < config.leds; ++k)
            {
              *bp++ = ws2812_gamma_correct(
                          ws2812_hsv_to_rgb((j + k) & 0xff,
                                            0xff,
                                            0xff));
            }

          lseek(fd, 0, SEEK_SET);

          result = write(fd, buffer, 4 * config.leds);
          if (result != 4 * config.leds)
            {
              fprintf(stderr,
                      "ws2812_main: write failed: %d  %d\n",
                      result,
                      errno);

              goto errout_with_dev;
            }

          usleep(config.delay);
        }
    }

  free(buffer);
  close(fd);
  fflush(stdout);
  return OK;

errout_with_dev:
  close(fd);

errout:
  fflush(stdout);
  return ERROR;
}
