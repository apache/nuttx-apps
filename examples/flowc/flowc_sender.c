/****************************************************************************
 * apps/examples/flowc/flowc_sender.c
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

#include "config.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

#include "flowc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

static char g_tty_devname[MAX_DEVNAME] =
  CONFIG_EXAMPLES_FLOWC_SENDER_DEVNAME;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void fill_buffer(FAR uint8_t *buf)
{
  FAR uint8_t *ptr;
  int ch;

  for (ch = 0x20; ch < 0x7f; ch++)
    {
      *ptr++ = ch;
    }
}

/****************************************************************************
 * flowc_cmdline
 ****************************************************************************/

static int flowc_cmdline(int argc, char **argv)
{
  /* Currently only a single command line option is supported:  The receiver
   * IP address.
   */

  if (argc == 2)
    {
      strncpy(g_tty_devname, argv[1], MAX_DEVNAME);
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Too many arguments\n");
      fprintf(stderr, "USAGE: %s [<tty-devname>]\n", argv[0]);
      return 1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int flowc_sender(int argc, char **argv)
{
  uint8_t outbuf[SENDSIZE];
#ifdef CONFIG_EXAMPLES_FLOWC_OUTPUT
  struct termios term;
#endif
  ssize_t nsent;
  int ret;
  int fd;
  int i;

  /* Parse the command line */

  ret = flowc_cmdline(argc, argv);
  if (ret != 0)
    {
      return ret;
    }

  /* Open the serial device */

  fd = open(g_tty_devname, O_WRONLY);
  if (fd < 0)
    {
      printf("sender ERROR: Failed to open %s: %d\n", g_tty_devname, errno);
      return 1;
    }

#ifdef CONFIG_EXAMPLES_FLOWC_OUTPUT
  /* Set output flow control */

  tcgetattr(fd, &term);
#ifdef CCTS_OFLOW
  term.c_cflag |= CCTS_OFLOW;
#else
  term.c_cflag |= CRTSCTS;
#endif
  tcsetattr(fd, TCSANOW, &term);
#endif

  /* Set up the output buffer */

  fill_buffer(outbuf);

  /* Then send the buffer multiple times as fast as possible */

  for (i = 0; i < NSENDS; i++)
    {
      /* Send the message */

      nsent = write(fd, outbuf, SENDSIZE);
      if (nsent < 0)
        {
          printf("sender: %lu. write failed: %d\n",
                 (unsigned long)i*SENDSIZE, errno);
          close(fd);
          return 1;
        }
      else if (nsent != SENDSIZE)
        {
          printf("sender: %lu. Bad send length: %ld Expected: %d\n",
                 (unsigned long)i*SENDSIZE, (long)nsent, SENDSIZE);
          close(fd);
          return 1;
        }
    }

  close(fd);
  return 0;
}
