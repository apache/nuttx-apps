/****************************************************************************
 * apps/system/cu/cu_main.c
 *
 *   Copyright (C) 2014 sysmocom - s.f.m.c. GmbH. All rights reserved.
 *   Author: Harald Welte <hwelte@sysmocom.de>
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

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <debug.h>

#include "system/readline.h"

#include "cu.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SIG_INT
#  define SIGINT 10
#else
#  define SIGINT CONFIG_SIG_INT
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum parity_mode
{
  PARITY_NONE,
  PARITY_EVEN,
  PARITY_ODD,
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct cu_globals_s g_cu;
#ifdef CONFIG_SERIAL_TERMIOS
static int fd_std_tty;
static struct termios g_tio_std;
static struct termios g_tio_dev;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cu_listener
 *
 * Description:
 *   Entry point for the listener thread.
 *
 ****************************************************************************/

static FAR void *cu_listener(FAR void *parameter)
{
  for (; ; )
    {
      int rc;
      char ch;

      rc = read(g_cu.infd, &ch, 1);
      if (rc <= 0)
        {
          break;
        }

      fputc(ch, stdout);
      fflush(stdout);
    }

  /* Won't get here */

  return NULL;
}

static void sigint(int sig)
{
  pthread_cancel(g_cu.listener);
  tcflush(g_cu.outfd, TCIOFLUSH);
  close(g_cu.outfd);
  close(g_cu.infd);
  exit(0);
}

#ifdef CONFIG_SERIAL_TERMIOS
static int set_termios(int fd, int rate, enum parity_mode parity,
                       int rtscts, int nocrlf)
{
  int rc = 0;
  int ret;
  struct termios tio;

  tio = g_tio_dev;

  tio.c_cflag &= ~(PARENB | PARODD | CRTSCTS);

  switch (parity)
    {
      case PARITY_EVEN:
        tio.c_cflag |= PARENB;
        break;

      case PARITY_ODD:
        tio.c_cflag |= PARENB | PARODD;
        break;

      case PARITY_NONE:
        break;
    }

  /* set baudrate */

  if (rate != 0)
    {
      cfsetspeed(&tio, rate);
    }

  if (rtscts)
    {
      tio.c_cflag |= CRTS_IFLOW | CCTS_OFLOW;
    }

  tio.c_oflag = OPOST;

  /* enable or disable \n -> \r\n conversion during write */

  if (nocrlf == 0)
    {
      tio.c_oflag |= ONLCR;
    }

  ret = tcsetattr(fd, TCSANOW, &tio);
  if (ret)
    {
      fprintf(stderr, "set_termios: ERROR during tcsetattr(): %d\n", errno);
      rc = -1;
      goto errout;
    }

  /* for tty stdout force enable or disable \n -> \r\n conversion */

  if (fd_std_tty >= 0)
  {
    tio = g_tio_std;
    if (nocrlf == 0)
      {
        tio.c_oflag |= ONLCR;
      }
    else
      {
        tio.c_oflag &= ~ONLCR;
      }

    ret = tcsetattr(fd_std_tty, TCSANOW, &tio);
    if (ret)
      {
        fprintf(stderr, "set_termios: ERROR during tcsetattr(): %d\n",
                errno);
        rc = -1;
      }
  }

errout:
  return rc;
}

static int retrieve_termios(int fd)
{
  tcsetattr(fd, TCSANOW, &g_tio_dev);
  if (fd_std_tty >= 0)
    {
      tcsetattr(fd_std_tty, TCSANOW, &g_tio_std);
    }

  return 0;
}
#endif

static void print_help(void)
{
  printf("Usage: cu [options]\n"
         " -l: Use named device (default %s)\n"
#ifdef CONFIG_SERIAL_TERMIOS
         " -e: Set even parity\n"
         " -o: Set odd parity\n"
         " -s: Use given speed (default %d)\n"
         " -r: Disable RTS/CTS flow control (default: on)\n"
         " -c: Disable lf -> crlf conversion (default: off)\n"
#endif
         " -f: Enable endless mode without escape sequence (default: off)\n"
         " -?: This help\n",
#ifdef CONFIG_SERIAL_TERMIOS
         CONFIG_SYSTEM_CUTERM_DEFAULT_DEVICE,
         CONFIG_SYSTEM_CUTERM_DEFAULT_BAUD
#else
         CONFIG_SYSTEM_CUTERM_DEFAULT_DEVICE
#endif
        );
}

static void print_escape_help(void)
{
  printf("[Escape sequences]\n"
         "[~. hangup]\n"
         );
}

static int cu_cmd(char bcmd)
{
  switch (bcmd)
    {
    case '?':
      print_escape_help();
      break;

    case '.':
      return 1;

    /* FIXME: implement other commands such as send/receive file */
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cu_main
 *
 * Description:
 *   Main entry point for the serial terminal example.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  pthread_attr_t attr;
  struct sigaction sa;
  FAR char *devname = CONFIG_SYSTEM_CUTERM_DEFAULT_DEVICE;
#ifdef CONFIG_SERIAL_TERMIOS
  int baudrate = CONFIG_SYSTEM_CUTERM_DEFAULT_BAUD;
  enum parity_mode parity = PARITY_NONE;
  int rtscts = 1;
  int nocrlf = 0;
#endif
  int nobreak = 0;
  int option;
  int ret;
  int bcmd;
  int start_of_line = 1;
  int exitval = EXIT_FAILURE;
  bool badarg = false;

  /* Initialize global data */

  memset(&g_cu, 0, sizeof(struct cu_globals_s));

  /* Install signal handlers */

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sigint;
  sigaction(SIGINT, &sa, NULL);

  optind = 0;   /* global that needs to be reset in FLAT mode */
  while ((option = getopt(argc, argv, "l:s:cefhor?")) != ERROR)
    {
      switch (option)
        {
          case 'l':
            devname = optarg;
            break;

#ifdef CONFIG_SERIAL_TERMIOS
          case 's':
            baudrate = atoi(optarg);
            break;

          case 'e':
            parity = PARITY_EVEN;
            break;

          case 'o':
            parity = PARITY_ODD;
            break;

          case 'r':
            rtscts = 0;
            break;

          case 'c':
            nocrlf = 1;
            break;
#endif

          case 'f':
            nobreak = 1;
            break;

          case 'h':
          case '?':
            print_help();
            badarg = true;
            exitval = EXIT_SUCCESS;
            break;

          default:
            badarg = true;
            exitval = EXIT_FAILURE;
            break;
        }
    }

  if (badarg)
    {
      return exitval;
    }

  /* Open the serial device for writing */

  g_cu.outfd = open(devname, O_WRONLY);
  if (g_cu.outfd < 0)
    {
      fprintf(stderr, "cu_main: ERROR: Failed to open %s for writing: %d\n",
              devname, errno);
      goto errout_with_devinit;
    }

#ifdef CONFIG_SERIAL_TERMIOS
  /* remember serial device termios attributes */

  ret = tcgetattr(g_cu.outfd, &g_tio_dev);
  if (ret)
    {
      fprintf(stderr, "cu_main: ERROR during tcgetattr(): %d\n", errno);
      goto errout_with_outfd;
    }

  /* remember std termios attributes if it is a tty. Try to select
   * right descriptor that is used to refer to tty
   */

  if (isatty(fileno(stderr)))
    {
      fd_std_tty = fileno(stderr);
    }
  else if (isatty(fileno(stdout)))
    {
      fd_std_tty = fileno(stdout);
    }
  else if (isatty(fileno(stdin)))
    {
      fd_std_tty = fileno(stdin);
    }
  else
    {
      fd_std_tty = -1;
    }

  if (fd_std_tty >= 0)
    {
      tcgetattr(fd_std_tty, &g_tio_std);
    }

  if (set_termios(g_cu.outfd, baudrate, parity, rtscts, nocrlf) != 0)
    {
      goto errout_with_outfd_retrieve;
    }
#endif

  /* Open the serial device for reading.  Since we are already connected,
   * this should not fail.
   */

  g_cu.infd = open(devname, O_RDONLY);
  if (g_cu.infd < 0)
    {
      fprintf(stderr, "cu_main: ERROR: Failed to open %s for reading: %d\n",
             devname, errno);
      goto errout_with_outfd;
    }

  /* Start the serial receiver thread */

  ret = pthread_attr_init(&attr);
  if (ret != OK)
    {
      fprintf(stderr, "cu_main: pthread_attr_init failed: %d\n", ret);
      goto errout_with_fds;
    }

  /* Set priority of listener to configured value */

  attr.priority = CONFIG_SYSTEM_CUTERM_PRIORITY;

  ret = pthread_create(&g_cu.listener, &attr,
                       cu_listener, (pthread_addr_t)0);
  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      fprintf(stderr, "cu_main: Error in thread creation: %d\n", ret);
      goto errout_with_fds;
    }

  /* Send messages and get responses -- forever */

  for (; ; )
    {
      int ch = getc(stdin);

      if (ch < 0)
        {
          continue;
        }

      if (nobreak == 1)
        {
          write(g_cu.outfd, &ch, 1);
          continue;
        }

      if (start_of_line == 1 && ch == '~')
        {
          /* We've seen and escape (~) character, echo it to local
           * terminal and read the next char from serial
           */

          fputc(ch, stdout);
          bcmd = getc(stdin);
          if (bcmd == ch)
            {
              /* Escaping a tilde: handle like normal char */

              write(g_cu.outfd, &ch, 1);
              continue;
            }
          else
            {
              if (cu_cmd(bcmd) == 1)
                {
                  break;
                }
            }
        }

      /* Normal character */

      write(g_cu.outfd, &ch, 1);

      /* Determine if we are now at the start of a new line or not */

      if (ch == '\n' || ch == '\r')
        {
          start_of_line = 1;
        }
      else
        {
          start_of_line = 0;
        }
    }

  pthread_cancel(g_cu.listener);
  exitval = EXIT_SUCCESS;

  /* Error exits */

errout_with_fds:
  close(g_cu.infd);
#ifdef CONFIG_SERIAL_TERMIOS
errout_with_outfd_retrieve:
  retrieve_termios(g_cu.outfd);
#endif
errout_with_outfd:
  close(g_cu.outfd);
errout_with_devinit:
  return exitval;
}
