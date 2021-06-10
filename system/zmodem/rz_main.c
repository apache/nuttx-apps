/****************************************************************************
 * apps/system/zmodem/rz_main.c
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
  fprintf(stderr,
          "USAGE: %s [OPTIONS]\n",
          progname);
  fprintf(stderr,
          "\nWhere OPTIONS include the following:\n");
  fprintf(stderr,
          "\t-d <device>: Communication device to use.  Default: %s\n",
          CONFIG_SYSTEM_ZMODEM_DEVNAME);
  fprintf(stderr,
          "\t-p <path>: Folder to hold the received file.  Default: %s\n",
          CONFIG_SYSTEM_ZMODEM_MOUNTPOINT);
  fprintf(stderr, "\t-h: Show this text and exit\n");
  exit(errcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  ZMRHANDLE handle;
  FAR const char *devname = CONFIG_SYSTEM_ZMODEM_DEVNAME;
  FAR const char *pathname = CONFIG_SYSTEM_ZMODEM_MOUNTPOINT;
  int exitcode = EXIT_FAILURE;
#ifdef CONFIG_SERIAL_TERMIOS
  struct termios saveterm;
#endif
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

#ifdef CONFIG_SERIAL_TERMIOS
  /* Save the current terminal setting */

  tcgetattr(fd, &saveterm);

  /* Enable the raw mode */

  zm_rawmode(fd);

#ifdef CONFIG_SYSTEM_ZMODEM_FLOWC
  /* Enable hardware Rx/Tx flow control */

  zm_flowc(fd);
#endif
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
  zmr_release(handle);

errout_with_device:
#ifdef CONFIG_SERIAL_TERMIOS
#ifdef CONFIG_SYSTEM_ZMODEM_FLOWC
  /* Flush the serial output to assure do not hang trying to drain it */

  tcflush(fd, TCIOFLUSH);
#endif

  /* Restore the saved terminal setting */

  tcsetattr(fd, TCSANOW, &saveterm);
#endif

  close(fd);

errout:
  return exitcode;
}
