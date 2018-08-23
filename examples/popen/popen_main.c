/****************************************************************************
 * examples/popen/popen_main.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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
#include <time.h>
#include <errno.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: popen_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int popen_main(int argc, char *argv[])
#endif
{
  struct itimerspec value;
  struct sigevent ev;
  FILE *stream;
  timer_t timerid;
  char buffer[256];
  size_t nread;
  int exitcode;
  int ret;

  /* Create a POSIX timer */

  ev.sigev_notify            = SIGEV_SIGNAL;
  ev.sigev_signo             = SIGALRM;
  ev.sigev_value.sival_int   = 0;
#ifdef CONFIG_SIG_EVTHREAD
  ev.sigev_notify_function   = NULL;
  ev.sigev_notify_attributes = NULL;
#endif

  ret = timer_create(CLOCK_REALTIME, &ev, &timerid);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: timer_create() failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Ask for help from the NSH shell */

  printf("Calling popen(\"help\")\n");

  stream = popen("help", "r");
  if (stream == NULL)
    {
      fprintf(stderr, "ERROR: popen() failed: %d\n", errno);
      (void)timer_delete(timerid);
      return EXIT_FAILURE;
    }

  /* Loop until the shell stops sending data */

  for (; ; )
    {
      /* Set up a signal to wake-up the fread if there is no data */

      value.it_value.tv_sec     = 2;
      value.it_value.tv_nsec    = 0;
      value.it_interval.tv_sec  = 2;
      value.it_interval.tv_nsec = 0;

      ret = timer_settime(timerid, 0, &value, NULL);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: timer_settime() failed: %d\n", errno);
          exitcode = EXIT_FAILURE;
          break;
        }

      /* Read data from the shell */

      nread = fread(buffer, 1, 256, stream);

      /* Cancel the timer by setting the timeout to zero. */

      value.it_value.tv_sec     = 0;
      value.it_value.tv_nsec    = 0;
      value.it_interval.tv_sec  = 0;
      value.it_interval.tv_nsec = 0;

      ret = timer_settime(timerid, 0, &value, NULL);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: timer_settime() failed: %d\n", errno);
          exitcode = EXIT_FAILURE;
          break;
        }

      /* Check for read errors */

      if (nread == 0)
        {
          /* Did an error occur? */

          if (ferror(stream))
            {
              int errcode = errno;

              if (errcode == EINTR)
                {
                  printf("Timeout... end of data\n");
                  exitcode = EXIT_SUCCESS;
                }
              else
                {
                  fprintf(stderr, "ERROR: fread() failed: %d\n", errcode);
                  exitcode = EXIT_FAILURE;
                }

              break;
            }
          else
            {
              /* No.. must be EOF */

              printf("End of data\n");
              exitcode = EXIT_SUCCESS;
              break;
            }
        }

      /* Dump what we read from the shell */

      fwrite(buffer, 1, nread, stdout);
    }

  printf("Calling pclose()\n");

  ret = pclose(stream);
  if (ret < 0)
    {
      int errcode = errno;

      /* ECHILD is not really an error.  It simply means that the child
       * thread has already exited and exit status is not available.
       */

      if (errcode == ECHILD)
        {
          printf("The shell has already exited (and exit status is not available)\n");
        }
      else
        {
          fprintf(stderr, "ERROR: pclose() failed: %d\n", errcode);
          exitcode = EXIT_FAILURE;
        }
    }

  (void)timer_delete(timerid);
  return exitcode;
}
