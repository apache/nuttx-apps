/****************************************************************************
 * examples/random/random_main.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int rand_main(int argc, char *argv[])
#endif
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
      (void)close(fd);
      exit(EXIT_FAILURE);
    }

  if (nread != nsamples * sizeof(uint32_t))
    {
      fprintf(stderr, "ERROR: Read from /dev/random only produced %d bytes\n",
              (int)nread);
      (void)close(fd);
      exit(EXIT_FAILURE);
    }

  /* Dump the sample buffer */

  lib_dumpbuffer("Random values", (FAR const uint8_t*)buffer, nread);
  (void)close(fd);
  return EXIT_SUCCESS;
}

