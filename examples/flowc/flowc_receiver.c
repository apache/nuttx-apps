/****************************************************************************
 * apps/examples/flowc/flowc_receiver.c
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

#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <arpa/inet.h>

#include "flowc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

static char g_tty_devname[MAX_DEVNAME] =
  CONFIG_EXAMPLES_FLOWC_RECEVER_DEVNAME;
static char g_expected = 0x20;
static unsigned int g_nerrors;
static unsigned long g_count;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void check_buffer(FAR uint8_t *buf, size_t buflen)
{
  unsigned long count;
  FAR uint8_t *ptr;
  size_t i;
  int ch;

  /* Verfiy the buffer content */

  ch = g_expected;

  for (i = 0, ptr = buf, count = g_count; i < buflen; i++, ptr++, count++)
    {
      if (*ptr != ch)
        {
          printf("receiver: ERROR: "
                 "Expected %c (%02x), found %c (%02x) at count=%lu\n",
                 isprint(ch)   ? ch   : '.', ch,
                 isprint(*ptr) ? *ptr : '.', *ptr,
                 count);
          g_nerrors++;
          ch = *ptr;
        }

      if (++ch > 0x7e)
        {
          ch = 0x20;
        }
    }

  /* Update globals */

  g_expected = ch;
  if (g_expected > 0x7e)
    {
      g_expected = 0x20;
    }

  g_count = count;
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

int flowc_receiver(int argc, char **argv)
{
  uint8_t inbuf[CONFIG_EXAMPLES_FLOWC_RECEIVER_BUFSIZE];
#ifdef CONFIG_EXAMPLES_FLOWC_INPUT
  struct termios term;
#endif
  ssize_t nread;
  int readcount = 0;
  int ret;
  int fd;

  /* Parse the command line */

  ret = flowc_cmdline(argc, argv);
  if (ret != 0)
    {
      return ret;
    }

  /* Open the serial device */

  fd = open(g_tty_devname, O_RDONLY);
  if (fd < 0)
    {
      printf("sender ERROR: Failed to open %s: %d\n", g_tty_devname, errno);
      return 1;
    }

#ifdef CONFIG_EXAMPLES_FLOWC_INPUT
  /* Set input flow control */

  tcgetattr(fd, &term);
#ifdef CRTS_IFLOW
  term.c_cflag |= CRTS_IFLOW;
#else
  term.c_cflag |= CRTSCTS;
#endif
  tcsetattr(fd, TCSANOW, &term);
#endif

  /* Then receive until all data has been received correctly (possibly
   * hanging if data is lost).
   */

  while (g_count < TOTALSIZE)
    {
      nread = read(fd, inbuf, CONFIG_EXAMPLES_FLOWC_RECEIVER_BUFSIZE);
      if (nread < 0)
        {
          printf("receiver: %lu. recv failed: %d\n", g_count, errno);
          close(fd);
          return 1;
        }

      check_buffer(inbuf, nread);
      if (g_nerrors > 25)
        {
          printf("receiver: %lu. Too many errors: %u\n", g_count, g_nerrors);
          close(fd);
          return 1;
        }

      /* Increase the packet counter */

      readcount++;

#if defined(CONFIG_EXAMPLES_FLOWC_RECEIVER_DELAY) && \
    CONFIG_EXAMPLES_FLOWC_RECEIVER_DELAY > 0
      /* Delay to force flow control */

      usleep(1000 * CONFIG_EXAMPLES_FLOWC_RECEIVER_DELAY);
#endif

#ifdef CONFIG_SYSLOG_INTBUFFER
      /* Move to next line if more than 80 chars */

      if ((readcount % 80) == 0)
        {
          syslog(LOG_INFO, "\n");
        }

      /* Just to force a flush of syslog interrupt buffer.  May also provide
       * a handy indication that the test is still running.
       */

      syslog(LOG_INFO, ".");
#endif
    }

  close(fd);
  return 0;
}
