/****************************************************************************
 * examples/nxhello/nxhello_bkgd.c
 *
 *   Copyright (C)  Gregory Nutt. All rights reserved.
 *   Author: Alan Carvalho de Assisi <acassis@gmail.com>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/serial/pty.h>

/****************************************************************************
 * pty_test_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int pty_test_main(int argc, char *argv[])
#endif
{
  char buffer[16];
  int ret;
  int fd;

  /* Open the pseudo-terminal master multiplexor (pmtx) */

  fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open /dev/ptmx: %d\n", errno);
      return -1;
    }

  /* Grant access and unlock the slave device */

  ret = grantpt(fd);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: grantpt() failed: %d\n", errno);
      fclose(fd);
      return -1;
    }

  ret = unlockpt(fd);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: unlockpt() failed: %d\n", errno);
      fclose(fd);
      return -1;
    }

  /* Get the slave device path */

  ptsname_r(fd, buffer, 16);
  printf("Slave device: %s\n", buffer);

  /* Loop forever, echoing anything received from the slave */

  for (; ; )
    {
      char buffer;
      int in;

      in = read(fd, &buffer, 1);
      putchar(buffer);
      buffer = '\0';
    }

  return 0;
}
