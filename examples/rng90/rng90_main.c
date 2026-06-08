/****************************************************************************
 * apps/examples/rng90/rng90_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nuttx/crypto/rng90.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_RNG90_DEVPATH
#  define CONFIG_EXAMPLES_RNG90_DEVPATH "/dev/rng0"
#endif

#define RNG90_MAX_BYTES 32

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: print_hex
 *
 * Description:
 *   Dump a buffer as a hex string, 16 bytes per line.
 *
 ****************************************************************************/

static void print_hex(FAR const uint8_t *buf, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++)
    {
      printf("%02x", buf[i]);
      if ((i & 0x0f) == 0x0f)
        {
          printf("\n");
        }
      else
        {
          printf(" ");
        }
    }

  if ((len & 0x0f) != 0)
    {
      printf("\n");
    }
}

/****************************************************************************
 * Name: usage
 ****************************************************************************/

static void usage(FAR const char *progname)
{
  printf("Usage: %s [<devpath>] [<count>]\n", progname);
  printf("  <devpath>  RNG90 device (default %s)\n",
         CONFIG_EXAMPLES_RNG90_DEVPATH);
  printf("  <count>    number of random bytes to read, 1..%d (default %d)\n",
         RNG90_MAX_BYTES, RNG90_MAX_BYTES);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * rng90_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *devpath = CONFIG_EXAMPLES_RNG90_DEVPATH;
  int count = RNG90_MAX_BYTES;
  uint8_t buf[RNG90_MAX_BYTES];
  int fd;
  int ret;

  /* Parse the optional command line arguments */

  if (argc > 1)
    {
      if (argv[1][0] == '-')
        {
          usage(argv[0]);
          return EXIT_FAILURE;
        }

      devpath = argv[1];
    }

  if (argc > 2)
    {
      count = atoi(argv[2]);
      if (count < 1 || count > RNG90_MAX_BYTES)
        {
          printf("Invalid count %d, clamping to %d\n", count,
                 RNG90_MAX_BYTES);
          count = RNG90_MAX_BYTES;
        }
    }

  /* Opening the device wakes the RNG90 up; closing puts it to sleep. */

  fd = open(devpath, O_RDONLY);
  if (fd < 0)
    {
      printf("ERROR: Failed to open %s: %d\n", devpath, errno);
      return EXIT_FAILURE;
    }

  printf("RNG90 example: device %s\n", devpath);

  /* Read random bytes from the device */

  ret = read(fd, buf, (size_t)count);
  if (ret < 0)
    {
      printf("ERROR: read failed: %d\n", errno);
      close(fd);
      return EXIT_FAILURE;
    }

  printf("Read %d random byte(s):\n", ret);
  print_hex(buf, (size_t)ret);

  close(fd);
  return EXIT_SUCCESS;
}
