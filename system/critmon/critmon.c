/****************************************************************************
 * apps/system/critmon/critmon.c
 *
 *   Copyright (C) 2013, 2018 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sched.h>
#include <syslog.h>
#include <errno.h>

#ifdef CONFIG_SYSTEM_CRITMONITOR

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SYSTEM_CRITMONITOR_DAEMON_STACKSIZE
#  define CONFIG_SYSTEM_CRITMONITOR_DAEMON_STACKSIZE 2048
#endif

#ifndef CONFIG_SYSTEM_CRITMONITOR_DAEMON_PRIORITY
#  define CONFIG_SYSTEM_CRITMONITOR_DAEMON_PRIORITY 50
#endif

#ifndef CONFIG_SYSTEM_CRITMONITOR_INTERVAL
#  define CONFIG_SYSTEM_CRITMONITOR_INTERVAL 2
#endif

#ifndef CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT
#  define CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT "/proc"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct critmon_state_s
{
  volatile bool started;
  volatile bool stop;
  pid_t pid;
  char line[80];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct critmon_state_s g_critmon;

#if CONFIG_TASK_NAME_SIZE > 0
static const char g_name[] = "Name:";
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: critmon_isolate_value
 ****************************************************************************/

static FAR char *critmon_isolate_value(FAR char *line)
{
  FAR char *ptr;

  while (isblank(*line) && *line != '\0')
    {
      line++;
    }

  ptr = line;
  while (*ptr != '\n' && *ptr != '\r' && *ptr != '\0')
    {
      ptr++;
    }

  *ptr = '\0';
  return line;
}

/****************************************************************************
 * Name: critmon_process_directory
 ****************************************************************************/

static int critmon_process_directory(FAR struct dirent *entryp)

{
  FAR const char *tmpstr;
  FAR char *filepath;
  FAR char *maxpreemp;
  FAR char *maxcrit;
  FAR char *endptr;
  FILE *stream;
  int errcode;
  int len;
  int ret;

#if CONFIG_TASK_NAME_SIZE > 0
  FAR char *name = NULL;

  /* Read the task status to get the task name */

  filepath = NULL;
  ret = asprintf(&filepath, CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT "/%s/status",
                 entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr, "Csection Monitor: Failed to create path to status file: %d\n",
              errcode);
      return -errcode;
    }

  /* Open the status file */

  stream = fopen(filepath, "r");
  if (stream == NULL)
    {
      ret = -errno;
      fprintf(stderr, "Csection Monitor: Failed to open %s: %d\n",
              filepath, ret);
      goto errout_with_filepath;
    }

  while (fgets(g_critmon.line, 80, stream) != NULL)
    {
      g_critmon.line[79] = '\n';
      len = strlen(g_name);
      if (strncmp(g_critmon.line, g_name, len) == 0)
        {
          tmpstr = critmon_isolate_value(&g_critmon.line[len]);
          if (*tmpstr == '\0')
            {
              ret = -EINVAL;
              goto errout_with_stream;
            }

          name = strdup(tmpstr);
          if (name == NULL)
            {
              ret = -EINVAL;
              goto errout_with_stream;
            }
        }
    }

  free(filepath);
  fclose(stream);
#endif

  /* Read critical section information */

  filepath = NULL;

  ret = asprintf(&filepath, CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT "/%s/critmon",
                 entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr, "Csection Monitor: Failed to create path to Csection file: %d\n",
              errcode);
      ret = -EINVAL;
      goto errout_with_name;
    }

  /* Open the Csection file */

  stream = fopen(filepath, "r");
  if (stream == NULL)
    {
      ret = -errno;
      fprintf(stderr, "Csection Monitor: Failed to open %s: %d\n",
              filepath, ret);
      goto errout_with_filepath;
    }

  /* Read the line containing the Csection max durations */

  if (fgets(g_critmon.line, 80, stream) == NULL)
    {
      ret = -errno;
      fprintf(stderr, "Csection Monitor: Failed to read from %s: %d\n",
              filepath, ret);
      goto errout_with_filepath;
    }

  /* Input Format:   X.XXXXXXXXX,X.XXXXXXXXX
   * Output Format:  X.XXXXXXXXX X.XXXXXXXXX NNNNN <name>
   */

  maxpreemp = g_critmon.line;
  maxcrit   = strchr(g_critmon.line, ',');

  if (maxcrit != NULL)
    {
      *maxcrit++ = '\0';
      endptr = strchr(maxcrit, '\n');
      if (endptr != NULL)
        {
          *endptr = '\0';
        }
    }
  else
    {
      maxcrit = "None";
    }

  /* Finally, output the stack info that we gleaned from the procfs */

#if CONFIG_TASK_NAME_SIZE > 0
  printf("%11s %11s %5s %s\n",
         maxpreemp, maxcrit, entryp->d_name, name);
#else
  printf("%11s %11s %5s\n",
         maxpreemp, maxcrit, entryp->d_name);
#endif

  ret = OK;

errout_with_stream:
  fclose(stream);

errout_with_filepath:
  free(filepath);

errout_with_name:
#if CONFIG_TASK_NAME_SIZE > 0
  if (name != NULL)
    {
      free(name);
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: critmon_check_name
 ****************************************************************************/

static bool critmon_check_name(FAR char *name)
{
  int i;

  /* Check each character in the name */

  for (i = 0; i < NAME_MAX && name[i] != '\0'; i++)
    {
      if (!isdigit(name[i]))
        {
          /* Name contains something other than a decimal numeric character */

          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: critmon_global_crit
 ****************************************************************************/

static void critmon_global_crit(void)
{
  FAR char *filepath;
  FAR char *cpu;
  FAR char *maxpreemp;
  FAR char *maxcrit;
  FAR char *endptr;
  FILE *stream;
  int errcode;
  int ret;

  /* Read critical section information */

  filepath = NULL;

  ret = asprintf(&filepath, CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT "/critmon");
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr, "Csection Monitor: Failed to create path to Csection file: %d\n",
              errcode);
      return;
    }

  /* Open the Csection file */

  stream = fopen(filepath, "r");
  if (stream == NULL)
    {
      errcode = errno;
      fprintf(stderr, "Csection Monitor: Failed to open %s: %d\n",
              filepath, errcode);
      goto errout_with_filepath;
    }

  /* Read the line containing the Csection max durations for each CPU */

  while (fgets(g_critmon.line, 80, stream) != NULL)
    {
      /* Input Format:  X,X.XXXXXXXXX,X.XXXXXXXXX
       * Output Format: X.XXXXXXXXX X.XXXXXXXXX       CPU X
       */

      cpu       = g_critmon.line;
      maxpreemp = strchr(g_critmon.line, ',');

      if (maxpreemp != NULL)
        {
          *maxpreemp++ = '\0';
          maxcrit = strchr(maxpreemp, ',');
          if (maxcrit != NULL)
            {
              *maxcrit++ = '\0';
              endptr = strchr(maxcrit, '\n');
              if (endptr != NULL)
                {
                  *endptr = '\0';
                }
            }
          else
            {
              maxcrit = "None";
            }
        }
      else
        {
          maxpreemp = "None";
          maxcrit   = "None";
        }

      /* Finally, output the stack info that we gleaned from the procfs */

      printf("%11s %11s  ---  CPU %s\n", maxpreemp, maxcrit, cpu);
    }

  fclose(stream);

errout_with_filepath:
  free(filepath);
}

/****************************************************************************
 * Name: critmon_daemon
 ****************************************************************************/

static int critmon_daemon(int argc, char **argv)
{
  DIR *dirp;
  int exitcode = EXIT_SUCCESS;
  int errcount = 0;
  int ret;

  printf("Csection Monitor: Running: %d\n", g_critmon.pid);

  /* Loop until we detect that there is a request to stop. */

  while (!g_critmon.stop)
    {
      /* Wait for the next sample interval */

      sleep(CONFIG_SYSTEM_CRITMONITOR_INTERVAL);

      /* Output a Header */

#if CONFIG_TASK_NAME_SIZE > 0
      printf("PRE-EMPTION CSECTION    PID   DESCRIPTION\n");
#else
      printf("PRE-EMPTION CSECTION    PID\n");
#endif
      printf("MAX DISABLE MAX TIME\n");

      /* Should global usage first */

      critmon_global_crit();

      /* Open the top-level procfs directory */

      dirp = opendir(CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT);
      if (dirp == NULL)
        {
          /* Failed to open the directory */

          fprintf(stderr, "Csection Monitor: Failed to open directory: %s\n",
                  CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT);

          if (++errcount > 100)
            {
              fprintf(stderr, "Csection Monitor: Too many errors ... exiting\n");
              exitcode = EXIT_FAILURE;
              break;
            }
        }

      /* Read each directory entry */

      for (; ; )
        {
          FAR struct dirent *entryp = readdir(dirp);
          if (entryp == NULL)
            {
              /* Finished with this directory */

              break;
            }

          /* Task/thread entries in the /proc directory will all be (1)
           * directories with (2) all numeric names.
           */

          if (DIRENT_ISDIRECTORY(entryp->d_type) &&
              critmon_check_name(entryp->d_name))
            {
              /* Looks good -- process the directory */

              ret = critmon_process_directory(entryp);
              if (ret < 0)
                {
                  /* Failed to process the thread directory */

                  fprintf(stderr,
                          "Csection Monitor: Failed to process sub-directory: %s\n",
                          entryp->d_name);

                  if (++errcount > 100)
                    {
                      fprintf(stderr, "Csection Monitor: Too many errors ... exiting\n");
                      exitcode = EXIT_FAILURE;
                      break;
                    }
                }
            }
        }

      closedir(dirp);
      fputc('\n', stdout);
    }

  /* Stopped */

  g_critmon.stop    = false;
  g_critmon.started = false;
  printf("Csection Monitor: Stopped: %d\n", g_critmon.pid);

  return exitcode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  /* Has the monitor already started? */

  sched_lock();
  if (!g_critmon.started)
    {
      int ret;

      /* No.. start it now */

      /* Then start the stack monitoring daemon */

      g_critmon.started = true;
      g_critmon.stop    = false;

      ret = task_create("Csection Monitor", CONFIG_SYSTEM_CRITMONITOR_DAEMON_PRIORITY,
                        CONFIG_SYSTEM_CRITMONITOR_DAEMON_STACKSIZE,
                        (main_t)critmon_daemon, (FAR char * const *)NULL);
      if (ret < 0)
        {
          int errcode = errno;
          printf("Csection Monitor ERROR: Failed to start the stack monitor: %d\n",
                 errcode);
        }
      else
        {
          g_critmon.pid = ret;
          printf("Csection Monitor: Started: %d\n", g_critmon.pid);
        }

      sched_unlock();
      return 0;
    }

  sched_unlock();
  printf("Csection Monitor: %s: %d\n",
         g_critmon.stop ? "Stopping" : "Running", g_critmon.pid);
  return 0;
}

int critmon_stop_main(int argc, char **argv)
{
  /* Has the monitor already started? */

  if (g_critmon.started)
    {
      /* Stop the stack monitor.  The next time the monitor wakes up,
       * it will see the stop indication and will exist.
       */

      printf("Csection Monitor: Stopping: %d\n", g_critmon.pid);
      g_critmon.stop = true;
    }

  printf("Csection Monitor: Stopped: %d\n", g_critmon.pid);
  return 0;
}

#endif /* CONFIG_SYSTEM_CRITMONITOR */
