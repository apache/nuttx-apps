/****************************************************************************
 * apps/examples/pty_test/pty_test.c
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <nuttx/serial/pty.h>

#include "nshlib/nshlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_PTYTEST_SERIALDEV
#  define CONFIG_EXAMPLES_PTYTEST_SERIALDEV "/dev/ttyS1"
#endif

#ifndef CONFIG_EXAMPLES_PTYTEST_DAEMONPRIO
#  define CONFIG_EXAMPLES_PTYTEST_DAEMONPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_EXAMPLES_PTYTEST_STACKSIZE
#  define CONFIG_EXAMPLES_PTYTEST_STACKSIZE 2048
#endif

#define POLL_TIMEOUT  200

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct term_pair_s
{
  int fd_uart;
  int fd_pty;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: serial_in
 *
 * Description:
 *   Read data from serial and write to pts
 *
 ****************************************************************************/

static void serial_in(struct term_pair_s *tp)
{
#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
  struct pollfd fdp;
#endif
  char buffer[16];
  int ret;

  if (!tp)
    {
      fprintf(stderr, "ERROR: terminal pair struct is NULL!\n");
      return;
    }

#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
  fdp.fd     = tp->fd_uart;
  fdp.events = POLLIN;
#endif

  /* Run forever */

  while (true)
    {
#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
      ret = poll((struct pollfd *)&fdp, 1, POLL_TIMEOUT);
      if (ret > 0)
        {
          if ((fdp.revents & POLLIN) != 0)
#endif
            {
              ssize_t len;

              len = read(tp->fd_uart, buffer, 16);
              if (len < 0)
                {
                  fprintf(stderr,
                          "ERROR Failed to read from serial: %d\n",
                          errno);
                }
              else if (len > 0)
                {
                  ret = write(tp->fd_pty, buffer, len);
                  if (ret < 0)
                    {
                      fprintf(stderr,
                              "Failed to write to serial: %d\n",
                              errno);
                    }
                }
            }
#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
        }

      /* Timeout */

      else if (ret == 0)
        {
          continue;
        }

      /* Poll error */

      else if (ret < 0)
        {
          fprintf(stderr, "ERROR: poll failed: %d\n", errno);
          continue;
        }
#endif
    }
}

/****************************************************************************
 * Name: serial_out
 *
 * Description:
 *   Read data from pts and write to serial
 *
 ****************************************************************************/

static void serial_out(struct term_pair_s *tp)
{
#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
  struct pollfd fdp;
#endif
  char buffer[16];
  int ret;

  if (!tp)
    {
      fprintf(stderr, "Error: terminal pair struct is NULL!\n");
      return;
    }

#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
  fdp.fd = tp->fd_pty;
  fdp.events = POLLIN;
#endif

  /* Run forever */

  while (true)
    {
#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
      ret = poll((struct pollfd *)&fdp, 1, POLL_TIMEOUT);
      if (ret > 0)
        {
          if ((fdp.revents & POLLIN) != 0)
#endif
            {
              ssize_t len;

              len = read(tp->fd_pty, buffer, 16);
              if (len < 0)
                {
                  fprintf(stderr,
                          "ERROR: Failed to read from pseudo-terminal: %d\n",
                          errno);
                }
              else if (len > 0)
                {
                  ret = write(tp->fd_uart, buffer, len);
                  if (ret < 0)
                    {
                      fprintf(stderr,
                              "ERROR: Failed to write to serial: %d\n",
                              errno);
                    }
                }
            }
#ifdef CONFIG_EXAMPLES_PTYTEST_POLL
        }

      /* Timeout */

      else if (ret == 0)
        {
          continue;
        }

      /* poll error */

      else if (ret < 0)
        {
          fprintf(stderr, "ERROR: poll failed: %d\n", errno);
          continue;
        }
#endif
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pty_test_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct term_pair_s termpair;
  struct termios tio;
  pthread_t si_thread;
  pthread_t so_thread;
  char buffer[16];
  pid_t pid;
  int fd_pts;
  int ret;

  printf("Create pseudo-terminal\n");

  /* Open pseudo-terminal master (pts factory) */

  termpair.fd_pty = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  if (termpair.fd_pty < 0)
    {
      fprintf(stderr, "ERROR: Failed to opening /dev/ptmx: %d\n",
              errno);
      return EXIT_FAILURE;
    }

  /* Grant access and unlock the slave PTY */

  ret = grantpt(termpair.fd_pty);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: grantpt() failed: %d\n", errno);
      goto error_pts;
    }

  ret = unlockpt(termpair.fd_pty);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: unlockpt() failed: %d\n", errno);
      goto error_pts;
    }

  /* Get the create pts file-name */

  ret = ptsname_r(termpair.fd_pty, buffer, 16);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ptsname_r() failed: %d\n", errno);
      goto error_pts;
    }

  /* Open the created pts */

  fd_pts = open(buffer, O_RDWR);
  if (fd_pts < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", buffer, errno);
      goto error_pts;
    }

  /* Open the second serial port to create a new console there */

  termpair.fd_uart = open(CONFIG_EXAMPLES_PTYTEST_SERIALDEV, O_RDWR);
  if (termpair.fd_uart < 0)
    {
#ifdef CONFIG_EXAMPLES_PTYTEST_WAIT_CONNECTED
      /* if ENOTCONN is received, re-attempt to open periodically */

      if (errno == ENOTCONN)
        {
          fprintf(stderr, "ERROR: device not connected, will continue"
                          " trying\n");
        }

      while (termpair.fd_uart < 0 && errno == ENOTCONN)
        {
          sleep(1);

          termpair.fd_uart = open(CONFIG_EXAMPLES_PTYTEST_SERIALDEV, O_RDWR);
        }

      /* if we exited due to an error different than ENOTCONN */

      if (termpair.fd_uart < 0)
        {
          fprintf(stderr, "Failed to open %s: %i\n",
                 CONFIG_EXAMPLES_PTYTEST_SERIALDEV, errno);
          goto error_serial;
        }
#else
      fprintf(stderr, "Failed to open %s: %i\n",
             CONFIG_EXAMPLES_PTYTEST_SERIALDEV, errno);
      goto error_serial;
#endif
    }

#ifdef CONFIG_SERIAL_TERMIOS
  /* Enable \n -> \r\n conversion during write */

  ret = tcgetattr(termpair.fd_uart, &tio);
  if (ret)
    {
      fprintf(stderr, "ERROR: tcgetattr() failed: %d\n", errno);
      goto error_serial;
    }

  tio.c_oflag = OPOST | ONLCR;
  ret = tcsetattr(termpair.fd_uart, TCSANOW, &tio);
  if (ret)
    {
      fprintf(stderr, "ERROR: tcsetattr() failed: %d\n", errno);
      goto error_serial;
    }
#endif

  printf("Starting a new NSH Session using %s\n", buffer);

  /* Close default stdin, stdout and stderr */

  close(0);
  close(1);
  close(2);

  /* Use this pts file as stdin, stdout, and stderr */

  dup2(fd_pts, 0);
  dup2(fd_pts, 1);
  dup2(fd_pts, 2);

  /* Create a new console using this /dev/pts/N */

  pid = task_create("NSH Console", CONFIG_EXAMPLES_PTYTEST_DAEMONPRIO,
                    CONFIG_EXAMPLES_PTYTEST_STACKSIZE, nsh_consolemain,
                    &argv[1]);
  if (pid < 0)
    {
      /* Can't do output because stdout and stderr are redirected */

      goto error_console;
    }

  /* Start a thread to read from serial */

  ret = pthread_create(&si_thread, NULL,
                       (pthread_startroutine_t)serial_in,
                       (pthread_addr_t)&termpair);
  if (ret != 0)
    {
      /* Can't do output because stdout and stderr are redirected */

      goto error_thread1;
    }

  /* Start a thread to write to serial */

  ret = pthread_create(&so_thread, NULL,
                       (pthread_startroutine_t)serial_out,
                       (pthread_addr_t)&termpair);
  if (ret != 0)
    {
      /* Can't do output because stdout and stderr are redirected */

      goto error_thread2;
    }

  /* Stay here to keep the threads running */

  while (true)
    {
      /* Nothing to do, then sleep to avoid eating all cpu time */

      pause();
    }

  return EXIT_SUCCESS;

error_thread2:
error_thread1:
error_console:
  close(termpair.fd_uart);
error_serial:
  close(fd_pts);
error_pts:
  close(termpair.fd_pty);

  return EXIT_FAILURE;
}
