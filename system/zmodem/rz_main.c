/****************************************************************************
 * system/zmodem/rz_main.c
 *
 *   Copyright (C) 2013, 2018 Gregory Nutt. All rights reserved.
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
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>

#include "system/zmodem.h"
#include "zm.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname, int errcode)
{
  fprintf(stderr, "USAGE: %s [OPTIONS]\n",
                  progname);
  fprintf(stderr, "\nWhere OPTIONS include the following:\n");
  fprintf(stderr, "\t-d <device>: Communication device to use.  Default: %s\n",
                  CONFIG_SYSTEM_ZMODEM_DEVNAME);
  fprintf(stderr, "\t-p <path>: Folder to hold the received file.  Default: %s\n",
                  CONFIG_SYSTEM_ZMODEM_MOUNTPOINT);
  fprintf(stderr, "\t-h: Show this text and exit\n");
  exit(errcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int rz_main(int argc, FAR char **argv)
#endif
{
  ZMRHANDLE handle;
  FAR const char *devname = CONFIG_SYSTEM_ZMODEM_DEVNAME;
  FAR const char *pathname = CONFIG_SYSTEM_ZMODEM_MOUNTPOINT;
  int exitcode = EXIT_FAILURE;
  int option;
  int ret;
  int fd;

  /* Parse input parameters */

  while ((option = getopt(argc, argv, ":d:hp:")) != ERROR)
    {
      switch (option)
        {
          case 'd':
            devname = optarg;
            break;

          case 'h':
            show_usage(argv[0], EXIT_SUCCESS);
            break;

          case 'p':
            pathname = optarg;
            break;

          case ':':
            fprintf(stderr, "ERROR: Missing required argument\n");
            show_usage(argv[0], EXIT_FAILURE);
            break;

          default:
          case '?':
            fprintf(stderr, "ERROR: Unrecognized option\n");
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  /* Nothing else is expected on the command line */

  if (optind < argc)
    {
      printf("ERROR: Too many command line arguments\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  /* Open the device for read/write access */

  fd = open(devname, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s\n", devname);
      goto errout;
    }

#ifdef CONFIG_SYSTEM_ZMODEM_FLOWC
  /* Enable hardware Rx/Tx flow control */

  zm_flowc(fd);
#endif

  /* Get the Zmodem handle */

  handle = zmr_initialize(fd);
  if (!handle)
    {
      fprintf(stderr, "ERROR: Failed to get Zmodem handle\n");
      goto errout_with_device;
    }

  /* And begin reception of files */

  ret = zmr_receive(handle, pathname);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: File reception failed: %d\n", ret);
      goto errout_with_zmodem;
    }

  exitcode = EXIT_SUCCESS;

errout_with_zmodem:
  (void)zmr_release(handle);

errout_with_device:
#ifdef CONFIG_SYSTEM_ZMODEM_FLOWC
  /* Flush the serial output to assure do not hang trying to drain it */

  tcflush(fd, TCIOFLUSH);
#endif

  (void)close(fd);

errout:
  return exitcode;
}
