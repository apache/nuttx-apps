/****************************************************************************
 * apps/examples/random/random_main.c
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
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#ifndef CONFIG_EXAMPLES_MAXSAMPLES
#  define CONFIG_EXAMPLES_MAXSAMPLES 64
#endif

#ifndef CONFIG_EXAMPLES_NSAMPLES
#  define CONFIG_EXAMPLES_NSAMPLES 8
#endif

#if CONFIG_EXAMPLES_NSAMPLES > CONFIG_EXAMPLES_MAXSAMPLES
#  warning CONFIG_EXAMPLES_NSAMPLES > CONFIG_EXAMPLES_MAXSAMPLES
#  undef CONFIG_EXAMPLES_NSAMPLES
#  define CONFIG_EXAMPLES_NSAMPLES CONFIG_EXAMPLES_MAXSAMPLES
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * rand_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uint32_t buffer[CONFIG_EXAMPLES_MAXSAMPLES];
  int nsamples = CONFIG_EXAMPLES_NSAMPLES;
  ssize_t nread;
  int fd;

  /* One argument is possible... the number of samples to collect */

  if (argc > 1)
    {
      nsamples = atoi(argv[1]);
    }

  /* Clip the number of samples to the configured buffer size */

  if (nsamples < 0)
    {
      nsamples = 0;
    }
  else if (nsamples > CONFIG_EXAMPLES_MAXSAMPLES)
    {
      nsamples = CONFIG_EXAMPLES_MAXSAMPLES;
    }

  /* fill buffer to make it super-clear as to what has and has not been written */

  memset(buffer, 0xcc, sizeof(buffer));

  /* Open /dev/random */

  fd = open("/dev/random", O_RDONLY);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open /dev/random: %d\n", errcode);
      exit(EXIT_FAILURE);
    }

  /* Read the requested number of samples */

  printf("Reading %d random numbers\n", nsamples);
  fflush(stdout);
  nread = read(fd, buffer, nsamples * sizeof(uint32_t));
  if (nread < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Read from /dev/random failed: %d\n", errcode);
      close(fd);
      exit(EXIT_FAILURE);
    }

  if (nread != nsamples * sizeof(uint32_t))
    {
      fprintf(stderr, "ERROR: Read from /dev/random only produced %d bytes\n",
              (int)nread);
      close(fd);
      exit(EXIT_FAILURE);
    }

  /* Dump the sample buffer */

  lib_dumpbuffer("Random values", (FAR const uint8_t*)buffer, nread);
  close(fd);
  return EXIT_SUCCESS;
}
