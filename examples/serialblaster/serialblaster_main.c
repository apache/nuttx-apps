/****************************************************************************
 * examples/serialblaster/serialblaster_main.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *            Bob Doiron
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
  int size = 0;
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
      printf("dev_ttyS2: ERROR Failed to open /dev/ttyS2\n");
      return -1;
    }

  rem = size;
  while (size > sizeof(s))
    {
      if (rem > 26)
        {
          ret = write(fd, s, (sizeof(s)-1));
        }

      rem = rem - 26;
      if (rem < 26)
        {
          ret = write(fd, s, rem);
        }

      size = size - 26;
      UNUSED(ret);
    }

  up_udelay(5000000);
  close(fd);
  return 0;

errout:
  fflush(stderr);
  return EXIT_FAILURE;
}
