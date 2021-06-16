/****************************************************************************
 * apps/examples/popen/popen_main.c
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
#include <time.h>
#include <errno.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: popen_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
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
      timer_delete(timerid);
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

  timer_delete(timerid);
  return exitcode;
}
