/****************************************************************************
 * examples/buttons/buttons_main.c
 *
 *   Copyright (C) 2016-2017 Gregory Nutt. All rights reserved.
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

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>

#include <nuttx/input/buttons.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_BUTTONS
#  error "CONFIG_BUTTONS is not defined in the configuration"
#endif

#if defined(CONFIG_DISABLE_SIGNALS) && defined(CONFIG_DISABLE_POLL)
#  error "You need at least SIGNALS or POLL support to read buttons"
#endif

#if !defined(CONFIG_DISABLE_SIGNALS) && !defined(CONFIG_DISABLE_POLL)
#  define USE_NOTIFY_SIGNAL 1
#else
#  define USE_NOTIFY_POLL 1
#endif

#ifndef CONFIG_BUTTONS_NPOLLWAITERS
#  define CONFIG_BUTTONS_NPOLLWAITERS 2
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_SIGNO
#  define CONFIG_EXAMPLES_BUTTONS_SIGNO 13
#endif

#ifndef CONFIG_BUTTONS_POLL_DELAY
#  define CONFIG_BUTTONS_POLL_DELAY 1000
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME0
#  define CONFIG_EXAMPLES_BUTTONS_NAME0 "BUTTON0"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME1
#  define CONFIG_EXAMPLES_BUTTONS_NAME1 "BUTTON1"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME2
#  define CONFIG_EXAMPLES_BUTTONS_NAME2 "BUTTON2"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME3
#  define CONFIG_EXAMPLES_BUTTONS_NAME3 "BUTTON3"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME4
#  define CONFIG_EXAMPLES_BUTTONS_NAME4 "BUTTON4"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME5
#  define CONFIG_EXAMPLES_BUTTONS_NAME5 "BUTTON5"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME6
#  define CONFIG_EXAMPLES_BUTTONS_NAME6 "BUTTON6"
#endif

#ifndef CONFIG_EXAMPLES_BUTTONS_NAME7
#  define CONFIG_EXAMPLES_BUTTONS_NAME7 "BUTTON7"
#endif

#define BUTTON_MAX 8

#ifndef CONFIG_EXAMPLES_BUTTONS_QTD
#  define CONFIG_EXAMPLES_BUTTONS_QTD BUTTON_MAX
#endif

#if CONFIG_EXAMPLES_BUTTONS_QTD > 8
#  error "CONFIG_EXAMPLES_BUTTONS_QTD > 8"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_BUTTONS_NAMES
static const char button_name[CONFIG_EXAMPLES_BUTTONS_QTD][16] =
{
  CONFIG_EXAMPLES_BUTTONS_NAME0
#if CONFIG_EXAMPLES_BUTTONS_QTD > 1
  , CONFIG_EXAMPLES_BUTTONS_NAME1
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 2
  , CONFIG_EXAMPLES_BUTTONS_NAME2
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 3
  , CONFIG_EXAMPLES_BUTTONS_NAME3
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 4
  , CONFIG_EXAMPLES_BUTTONS_NAME4
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 5
  , CONFIG_EXAMPLES_BUTTONS_NAME5
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 6
  , CONFIG_EXAMPLES_BUTTONS_NAME6
#endif
#if CONFIG_EXAMPLES_BUTTONS_QTD > 7
  , CONFIG_EXAMPLES_BUTTONS_NAME7
#endif
};
#endif

static bool g_button_daemon_started;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: button_daemon
 ****************************************************************************/

static int button_daemon(int argc, char *argv[])
{
#ifdef USE_NOTIFY_POLL
  struct pollfd fds[CONFIG_BUTTONS_NPOLLWAITERS];
#endif

#ifdef USE_NOTIFY_SIGNAL
  struct btn_notify_s btnevents;
#endif

  btn_buttonset_t supported;
  btn_buttonset_t sample = 0;

#ifdef CONFIG_EXAMPLES_BUTTONS_NAMES
  btn_buttonset_t oldsample = 0;
#endif

  int ret;
  int fd;
  int i;

  UNUSED(i);

  /* Indicate that we are running */

  g_button_daemon_started = true;
  printf("button_daemon: Running\n");

  /* Open the BUTTON driver */

  printf("button_daemon: Opening %s\n", CONFIG_EXAMPLES_BUTTONS_DEVPATH);
  fd = open(CONFIG_EXAMPLES_BUTTONS_DEVPATH, O_RDONLY|O_NONBLOCK);
  if (fd < 0)
    {
      int errcode = errno;
      printf("button_daemon: ERROR: Failed to open %s: %d\n",
             CONFIG_EXAMPLES_BUTTONS_DEVPATH, errcode);
      goto errout;
    }

  /* Get the set of BUTTONs supported */

  ret = ioctl(fd, BTNIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&supported));
  if (ret < 0)
    {
      int errcode = errno;
      printf("button_daemon: ERROR: ioctl(BTNIOC_SUPPORTED) failed: %d\n",
             errcode);
      goto errout_with_fd;
    }

  printf("button_daemon: Supported BUTTONs 0x%02x\n", (unsigned int)supported);

#ifdef USE_NOTIFY_SIGNAL
  /* Define the notifications events */

  btnevents.bn_press   = supported;
  btnevents.bn_release = supported;
  btnevents.bn_signo   = CONFIG_EXAMPLES_BUTTONS_SIGNO;

  /* Register to receive a signal when buttons are pressed/released */

  ret = ioctl(fd, BTNIOC_REGISTER,
              (unsigned long)((uintptr_t)&btnevents));
  if (ret < 0)
    {
      int errcode = errno;
      printf("button_daemon: ERROR: ioctl(BTNIOC_SUPPORTED) failed: %d\n",
             errcode);
      goto errout_with_fd;
    }
#endif

  /* Now loop forever, waiting BUTTONs events */

  for (; ; )
    {
#ifdef USE_NOTIFY_SIGNAL
      struct siginfo value;
      sigset_t set;
#endif

#ifdef USE_NOTIFY_POLL
      bool timeout;
      bool pollin;
      int nbytes;
#endif

#ifdef USE_NOTIFY_SIGNAL
      /* Wait for a signal */

      (void)sigemptyset(&set);
      (void)sigaddset(&set, CONFIG_EXAMPLES_BUTTONS_SIGNO);
      ret = sigwaitinfo(&set, &value);
      if (ret < 0)
        {
          int errcode = errno;
          printf("button_daemon: ERROR: sigwaitinfo() failed: %d\n", errcode);
          goto errout_with_fd;
        }

      sample = (btn_buttonset_t)value.si_value.sival_int;
#endif

#ifdef USE_NOTIFY_POLL
      /* Prepare the File Descriptor for poll */

      memset(fds, 0, sizeof(struct pollfd)*CONFIG_BUTTONS_NPOLLWAITERS);

      fds[0].fd      = fd;
      fds[0].events  = POLLIN;

      timeout        = false;
      pollin         = false;

      ret = poll(fds, CONFIG_BUTTONS_NPOLLWAITERS, CONFIG_BUTTONS_POLL_DELAY);

      printf("\nbutton_daemon: poll returned: %d\n", ret);
      if (ret < 0)
        {
          int errcode = errno;
          printf("button_daemon: ERROR poll failed: %d\n", errcode);
        }
      else if (ret == 0)
        {
          printf("button_daemon: Timeout\n");
          timeout = true;
        }
      else if (ret > CONFIG_BUTTONS_NPOLLWAITERS)
        {
          printf("button_daemon: ERROR poll reported: %d\n", errno);
        }
      else
        {
          pollin = true;
        }

      /* In any event, read until the pipe is empty */

      for (i = 0; i < CONFIG_BUTTONS_NPOLLWAITERS; i++)
        {
          do
            {
              nbytes = read(fds[i].fd, (void *)&sample,
                            sizeof(btn_buttonset_t));

              if (nbytes <= 0)
                {
                  if (nbytes == 0 || errno == EAGAIN)
                    {
                      if ((fds[i].revents & POLLIN) != 0)
                        {
                          printf("button_daemon: ERROR no read data[%d]\n", i);
                        }
                    }
                  else if (errno != EINTR)
                    {
                      printf("button_daemon: read[%d] failed: %d\n", i, errno);
                    }

                  nbytes = 0;
                }
              else
                {
                  if (timeout)
                    {
                      printf("button_daemon: ERROR? Poll timeout, but data read[%d]\n", i);
                      printf("               (might just be a race condition)\n");
                    }
                }

              /* Suppress error report if no read data on the next time through */

              fds[i].revents = 0;
            }
          while (nbytes > 0);
        }
#endif

#ifdef CONFIG_EXAMPLES_BUTTONS_NAMES
      /* Print name of all pressed/release button */

      for (i = 0; i < CONFIG_EXAMPLES_BUTTONS_QTD; i++)
        {
          if ((sample & (1 << i)) && !(oldsample & (1 << i)))
            {
              printf("%s was pressed\n", button_name[i]);
            }

          if (!(sample & (1 << i)) && (oldsample & (1 << i)))
            {
              printf("%s was released\n", button_name[i]);
            }
        }

      oldsample = sample;
#else
      printf("Sample = %d\n", sample);
#endif

      /* Make sure that everything is displayed */

      fflush(stdout);

      usleep(1000);
    }

errout_with_fd:
  (void)close(fd);

errout:
  g_button_daemon_started = false;

  printf("button_daemon: Terminating\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * buttons_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int buttons_main(int argc, FAR char *argv[])
#endif
{
  int ret;

  printf("buttons_main: Starting the button_daemon\n");
  if (g_button_daemon_started)
    {
      printf("buttons_main: button_daemon already running\n");
      return EXIT_SUCCESS;
    }

  ret = task_create("button_daemon", CONFIG_EXAMPLES_BUTTONS_PRIORITY,
                    CONFIG_EXAMPLES_BUTTONS_STACKSIZE, button_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("buttons_main: ERROR: Failed to start button_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  printf("buttons_main: button_daemon started\n");
  return EXIT_SUCCESS;
}
