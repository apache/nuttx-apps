/****************************************************************************
 * apps/examples/serialblaster/serialblaster_main.c
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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <nuttx/clock.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <nuttx/fs/fs.h>
#include <nuttx/arch.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUFFERED_IO

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char s[] = "abcdefghijklmnopqrstuvwxyz";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * serialblaster_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  int fd;
  FAR char *devpath;
  const int slength = sizeof(s)-1;
  int size = slength;
  int rem;

  if (argc == 1)
    {
      devpath = CONFIG_EXAMPLES_SERIALBLASTER_DEVPATH;
    }
  else if (argc == 2)
    {
      devpath = argv[1];
    }
  else if (argc == 3)
    {
      devpath = argv[1];
      size =  strtol(argv[2], NULL, 10);
    }
  else
    {
      fprintf(stderr, "Usage: %s [devpath]\n", argv[0]);
      goto errout;
    }

  fd = open(devpath, O_RDWR);
  if (fd < 0)
    {
      printf("%s: ERROR Failed to open %s\n", argv[0], devpath);
      return -1;
    }

  rem = size;
  printf("Sending %d bytes of data to %s (fd=%d)\n", size, devpath, fd);
  while (rem > 0)
    {
      if (rem > slength)
        {
          ret = write(fd, s, slength);
          rem = rem - slength;
        }
      else
        {
          ret = write(fd, s, rem);
          rem = 0;
        }

      UNUSED(ret);
    }

  up_udelay(5000000);
  close(fd);
  return 0;

errout:
  fflush(stderr);
  return EXIT_FAILURE;
}
