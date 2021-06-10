/****************************************************************************
 * apps/system/zmodem/sz_main.c
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
  fprintf(stderr, "USAGE: %s [OPTIONS] <lname> [<lname> [<lname> ...]]\n",
                  progname);
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "\t<lname> is the local file name\n");
  fprintf(stderr, "\nand OPTIONS include the following:\n");
  fprintf(stderr,
          "\t-d <device>: Communication device to use.  Default: %s\n",
          CONFIG_SYSTEM_ZMODEM_DEVNAME);
  fprintf(stderr, "\t-r <rname>: Remote file name.  Default <lname>\n");
  fprintf(stderr, "\t-x <mode>: Transfer type\n");
  fprintf(stderr, "\t\t0: Normal file (default)\n");
  fprintf(stderr, "\t\t1: Binary file\n");
  fprintf(stderr, "\t\t2: Convert \\n to local EOF convention\n");
  fprintf(stderr, "\t\t3: Resume or append to existing file\n");
  fprintf(stderr, "\t-o <option>: Transfer option\n");
  fprintf(stderr, "\t\t0: Implementation dependent\n");
  fprintf(stderr, "\t\t1: Transfer if source newer or longer\n");
  fprintf(stderr, "\t\t2: Transfer if different CRC or length\n");
  fprintf(stderr, "\t\t3: Append to existing file, if any\n");
  fprintf(stderr, "\t\t4: Replace existing file (default)\n");
  fprintf(stderr, "\t\t5: Transfer if source is newer\n");
  fprintf(stderr, "\t\t6: Transfer if dates or lengths different\n");
  fprintf(stderr, "\t\t7: Protect: transfer only if dest doesn't exist\n");
  fprintf(stderr, "\t\t8: Change filename if destination exists\n");
  fprintf(stderr, "\t-s: Skip if file not present at receiving end\n");
  fprintf(stderr, "\t-h: Show this text and exit\n");
  exit(errcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  enum zm_xfertype_e xfrtype = XM_XFERTYPE_NORMAL;
  enum zm_option_e xfroption = XM_OPTION_REPLACE;
  ZMSHANDLE handle;
  FAR const char *rname = NULL;
  FAR const char *devname = CONFIG_SYSTEM_ZMODEM_DEVNAME;
  FAR char *endptr;
  bool skip = false;
  long tmp;
  int exitcode = EXIT_FAILURE;
#ifdef CONFIG_SERIAL_TERMIOS
  struct termios saveterm;
#endif
  int option;
  int ret;
  int fd;

  /* Parse input parameters */

  while ((option = getopt(argc, argv, ":d:ho:r:sx:")) != ERROR)
    {
      switch (option)
        {
          case 'd':
            devname = optarg;
            break;

          case 'h':
            show_usage(argv[0], EXIT_SUCCESS);
            break;

          case 'o':
            tmp = strtol(optarg, &endptr, 10);
            if (tmp < 0 || tmp > 8)
              {
                fprintf(stderr,
                        "ERROR: Transfer option out of range: %ld\n",
                        tmp);
                show_usage(argv[0], EXIT_FAILURE);
              }
            else
              {
                xfroption = (enum zm_option_e)tmp;
              }
            break;

          case 'r':
            rname = optarg;
            break;

          case 's':
             skip = true;
             break;

          case 'x':
            tmp = strtol(optarg, &endptr, 10);
            if (tmp < 0 || tmp > 3)
              {
                fprintf(stderr,
                        "ERROR: Transfer type out of range: %ld\n",
                        tmp);
                show_usage(argv[0], EXIT_FAILURE);
              }
            else
              {
                xfrtype = (enum zm_xfertype_e)tmp;
              }
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

  /* There should be one final parameters remaining on the command line */

  if (optind >= argc)
    {
      printf("ERROR: Missing required 'lname' argument\n");
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

  handle = zms_initialize(fd);
  if (!handle)
    {
      fprintf(stderr, "ERROR: Failed to get Zmodem handle\n");
      goto errout_with_device;
    }

  /* And perform the transfer(s) */

  for (; optind < argc; optind++)
    {
      /* By the default, the remote file name is the same as the local file
       * name.  This will, of course, fail miserably if rname is specified
       * and there more than one lnames on the command line.  Don't do that.
       */

      FAR const char *nextlname = argv[optind];
      FAR const char *nextrname;
      FAR char *ralloc;

      /* Get the next remote file name */

      nextrname = rname;
      ralloc    = NULL;

      if (!nextrname)
        {
          /* No remote filename, use the basename of the local filename.
           * NOTE: that we have to duplicate the local filename to do this
           * because basename() modifies the original string.
           */

          ralloc = strdup(nextlname);
          if (!ralloc)
           {
             fprintf(stderr, "ERROR: Out-of-memory\n");
             goto errout_with_device;
           }

          nextrname = basename(ralloc);
        }

      /* Transfer the file */

      ret = zms_send(handle, nextlname, nextrname, xfrtype, xfroption, skip);

      /* Free any allocations made for the remote file name */

      if (ralloc)
        {
          free(ralloc);
        }

      /* Check if the transfer was successful */

      if (ret < 0)
        {
          fprintf(stderr, "ERROR: Transfer of %s failed: %d\n",
                  nextlname, errno);
          goto errout_with_zmodem;
       }
    }

  exitcode = EXIT_SUCCESS;

errout_with_zmodem:
  zms_release(handle);

errout_with_device:
#ifdef CONFIG_SERIAL_TERMIOS
# ifdef CONFIG_SYSTEM_ZMODEM_FLOWC
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
