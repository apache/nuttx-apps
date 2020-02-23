/****************************************************************************
 * examples/chrono/chrono_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include <nuttx/input/buttons.h>

#include <nuttx/lcd/slcd_ioctl.h>
#include <nuttx/lcd/slcd_codec.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUTTON_SIGNO 13
#define BUTTON_STACKSIZE 2048
#define BUTTON_PRIORITY 100
#define BUTTON_DEVPATH "/dev/buttons"

#define SLCD_DEVNAME "/dev/slcd0"
#define SLCD_BUFSIZE 8

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* What is the current chronometer status? */

enum chronostate_e
{
  CHRONO_RESETED = 0,
  CHRONO_RUNNING,
  CHRONO_STOPPED,
};

/* All state information for this test is kept within the following structure
 * in order create a namespace and to minimize the possibility of name
 * collisions.
 *
 * NOTE: stream must be the first element of struct slcd_chrono_s to support
 * casting between these two types.
 */

struct slcd_chrono_s
{
  struct lib_outstream_s   stream;      /* Stream to use for all output */
  struct slcd_attributes_s attr;        /* Size of the SLCD (rows x columns) */
  int                      fd;          /* File descriptor or the open SLCD device */
  bool                     initialized; /* TRUE:  Initialized */
  struct timespec          ts_start;    /* Store the initial time */
  struct timespec          ts_end;      /* Store the final time */
  int                      state;

  /* The I/O buffer */

  uint8_t buffer[SLCD_BUFSIZE];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct slcd_chrono_s g_slcd;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: button_daemon
 ****************************************************************************/

static int button_daemon(int argc, char *argv[])
{
  FAR struct slcd_chrono_s *priv = &g_slcd;
  struct btn_notify_s btnevents;
  btn_buttonset_t supported;

  int ret;
  int fd;
  int i;

  UNUSED(i);

  printf("button_daemon: Running\n");

  /* Open the BUTTON driver */

  printf("button_daemon: Opening %s\n", BUTTON_DEVPATH);
  fd = open(BUTTON_DEVPATH, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      int errcode = errno;
      printf("button_daemon: ERROR: Failed to open %s: %d\n",
             BUTTON_DEVPATH, errcode);
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

  printf("button_daemon: Supported BUTTONs 0x%02x\n",
         (unsigned int)supported);

  /* Define the notifications events */

  btnevents.bn_press   = supported;
  btnevents.bn_release = supported;

  btnevents.bn_event.sigev_notify = SIGEV_SIGNAL;
  btnevents.bn_event.sigev_signo  = BUTTON_SIGNO;

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

  /* Now loop forever, waiting BUTTONs events */

  for (; ; )
    {
      struct siginfo value;
      sigset_t set;

      /* Wait for a signal */

      sigemptyset(&set);
      sigaddset(&set, BUTTON_SIGNO);
      ret = sigwaitinfo(&set, &value);
      if (ret < 0)
        {
          int errcode = errno;
          printf("button_daemon: ERROR: sigwaitinfo() failed: %d\n", errcode);
          goto errout_with_fd;
        }

      if (priv->state == CHRONO_STOPPED)
        {
          clock_gettime(CLOCK_MONOTONIC, &priv->ts_start);
          priv->state = CHRONO_RUNNING;
        }
      else
        {
          priv->state = CHRONO_STOPPED;
        }

      /* Make sure that everything is displayed */

      fflush(stdout);

      usleep(1000);
    }

errout_with_fd:
  close(fd);

errout:

  printf("button_daemon: Terminating\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Name: slcd_flush
 ****************************************************************************/

static int slcd_flush(FAR struct lib_outstream_s *stream)
{
  FAR struct slcd_chrono_s *priv = (FAR struct slcd_chrono_s *)stream;
  FAR const uint8_t *buffer;
  ssize_t nwritten;
  ssize_t remaining;

  /* Loop until all bytes were written (handling both partial and interrupted
   * writes).
   */

  remaining = stream->nput;
  buffer    = priv->buffer;

  while (remaining > 0)
    {
      nwritten = write(priv->fd, buffer, remaining);
      if (nwritten < 0)
        {
          int errcode = errno;
          printf("write failed: %d\n", errcode);

          if (errcode != EINTR)
            {
              exit(EXIT_FAILURE);
            }
        }
      else
        {
          remaining -= nwritten;
          buffer    += nwritten;
        }
    }

  /* Reset the stream */

  stream->nput = 0;
  return OK;
}

/****************************************************************************
 * Name: slcd_putc
 ****************************************************************************/

static void slcd_putc(FAR struct lib_outstream_s *stream, int ch)
{
  FAR struct slcd_chrono_s *priv = (FAR struct slcd_chrono_s *)stream;

  /* Write the character to the buffer */

  priv->buffer[stream->nput] = ch;
  stream->nput++;
  priv->buffer[stream->nput] = '\0';

  /* If the buffer is full, flush it */

  if (stream->nput >= CONFIG_EXAMPLES_SLCD_BUFSIZE)
    {
      slcd_flush(stream);
    }
}

/****************************************************************************
 * Name: slcd_puts
 ****************************************************************************/

static void slcd_puts(FAR struct lib_outstream_s *outstream,
                      FAR const char *str)
{
  for (; *str; str++)
    {
      slcd_put((int)*str, outstream);
    }
}

/****************************************************************************
 * chrono_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct slcd_chrono_s *priv = &g_slcd;
  FAR char str[8] = "00:00.0";
  int fd;
  int ret;
  long sec;
  long min;

  /* Create a thread to wait for the button events */

  ret = task_create("button_daemon", BUTTON_PRIORITY,
                    BUTTON_STACKSIZE, button_daemon, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("buttons_main: ERROR: Failed to start button_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  /* Open the SLCD device */

  printf("Opening %s for read/write access\n", SLCD_DEVNAME);

  fd = open(SLCD_DEVNAME, O_RDWR);
  if (fd < 0)
    {
      printf("Failed to open %s: %d\n", CONFIG_EXAMPLES_SLCD_DEVNAME, errno);
      goto errout;
    }

  /* Get the attributes of the SCLD device */

  ret = ioctl(fd, SLCDIOC_GETATTRIBUTES, (unsigned long)&priv->attr);
  if (ret < 0)
    {
      printf("ioctl(SLCDIOC_GETATTRIBUTES) failed: %d\n", errno);
      goto errout_with_fd;
    }

  /* Are we already initialized? */

  if (!priv->initialized)
    {
      /* Initialize the output stream */

      memset(priv, 0, sizeof(struct slcd_chrono_s));
      priv->stream.put   = slcd_putc;
#ifdef CONFIG_STDIO_LINEBUFFER
      priv->stream.flush = slcd_flush;
#endif

      /* Get the attributes of the SCLD device */

      ret = ioctl(fd, SLCDIOC_GETATTRIBUTES, (unsigned long)&priv->attr);
      if (ret < 0)
        {
          printf("ioctl(SLCDIOC_GETATTRIBUTES) failed: %d\n", errno);
          goto errout_with_fd;
        }

      printf("Attributes:\n");
      printf("  rows: %d columns: %d nbars: %d\n",
             priv->attr.nrows, priv->attr.ncolumns, priv->attr.nbars);
      printf("  max contrast: %d max brightness: %d\n",
             priv->attr.maxcontrast, priv->attr.maxbrightness);

      /* Home the cursor and clear the display */

      printf("Clear screen\n");
      slcd_encode(SLCDCODE_CLEAR, 0, &priv->stream);
      slcd_flush(&priv->stream);

      priv->initialized = true;
    }

  /* Save the file descriptor in a place where slcd_flush can find it */

  priv->fd = fd;

  /* Set the cursor to the beginning of the current row by homing the cursor
   * then going down as necessary, and erase to the end of the line.
   */

  slcd_encode(SLCDCODE_HOME, 0, &priv->stream);
  slcd_encode(SLCDCODE_ERASEEOL, 0, &priv->stream);

  priv->state = CHRONO_RESETED;

  for (; ; )
    {
      /* If the device is reset, show 00.. and assume it is initial time */

      if (priv->state == CHRONO_RESETED)
        {
          /* Copy the initial value */

          strncpy(str, "00:00.0", 7);

          /* Print the initial reset value */

          slcd_puts(&priv->stream, str);
          slcd_flush(&priv->stream);

          /* Move to stopped state to avoid wasting time here */

          priv->state = CHRONO_STOPPED;
        }

      /* If the device is stopped nothing to do here */

      if (priv->state == CHRONO_STOPPED)
        {
        }

      /* If the chronometer is running, get the elapsed time and how it */

      if (priv->state == CHRONO_RUNNING)
        {
          /* Get the current time */

          clock_gettime(CLOCK_MONOTONIC, &priv->ts_end);

          /* How many seconds passed from initial time? */

          sec = priv->ts_end.tv_sec - priv->ts_start.tv_sec;

          /* Get the amount of minutes */

          min = sec / 60;

          /* Get the remaining seconds */

          sec = sec % 60;

          sprintf(str, "%02d:%02d:%01d", min, sec,
                  (priv->ts_end.tv_nsec / 100000000));

          /* Print it into LCD */

          slcd_encode(SLCDCODE_HOME, 0, &priv->stream);
          slcd_puts(&priv->stream, str);
          slcd_flush(&priv->stream);
        }

      /* usleep(1000); */
    }

  /* Normal exit */

  close(fd);
  return 0;

errout_with_fd:
  close(priv->fd);
errout:
  priv->initialized = false;
  exit(EXIT_FAILURE);
}
