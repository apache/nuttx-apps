/****************************************************************************
 * apps/system/critmon/critmon.c
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
  FAR char *maxrun;
  FAR char *endptr;
  FILE *stream;
  int errcode;
  int len;
  int ret;

#if CONFIG_TASK_NAME_SIZE > 0
  FAR char *name = NULL;

  /* Read the task status to get the task name */

  filepath = NULL;
  ret = asprintf(&filepath,
                 CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT "/%s/status",
                 entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr,
              "Csection Monitor: Failed to create path to status file: %d\n",
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

  ret = asprintf(&filepath,
                 CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT "/%s/critmon",
                 entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr, "Csection Monitor: "
              "Failed to create path to Csection file: %d\n",
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

  /* Input Format:   X.XXXXXXXXX,X.XXXXXXXXX,X.XXXXXXXXX
   * Output Format:  X.XXXXXXXXX X.XXXXXXXXX X.XXXXXXXXX NNNNN <name>
   */

  maxpreemp = g_critmon.line;
  maxcrit   = strchr(g_critmon.line, ',');

  if (maxcrit != NULL)
    {
      *maxcrit++ = '\0';

      maxrun = strchr(maxcrit, ',');
      if (maxrun != NULL)
        {
          *maxrun++ = '\0';

          endptr = strchr(maxrun, '\n');
          if (endptr != NULL)
            {
              *endptr = '\0';
            }
        }
      else
        {
          maxrun = "None";
        }
    }
  else
    {
      maxcrit = "None";
      maxrun  = "None";
    }

  /* Finally, output the stack info that we gleaned from the procfs */

#if CONFIG_TASK_NAME_SIZE > 0
  printf("%11s %11s %11s %5s %s\n",
         maxpreemp, maxcrit, maxrun, entryp->d_name, name);
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

  ret = asprintf(&filepath,
                 CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT "/critmon");
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr, "Csection Monitor: "
              "Failed to create path to Csection file: %d\n",
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

      printf("%11s %11s ----------- ----- CPU %s\n",
              maxpreemp, maxcrit, cpu);
    }

  fclose(stream);

errout_with_filepath:
  free(filepath);
}

/****************************************************************************
 * Name: critmon_list_once
 ****************************************************************************/

static int critmon_list_once(void)
{
  int exitcode = EXIT_SUCCESS;
  int errcount = 0;
  DIR *dirp;
  int ret;

  /* Output a Header */

#if CONFIG_TASK_NAME_SIZE > 0
  printf("PRE-EMPTION CSECTION    RUN         PID   DESCRIPTION\n");
#else
  printf("PRE-EMPTION CSECTION    RUN         PID\n");
#endif

  /* Should global usage first */

  critmon_global_crit();

  /* Open the top-level procfs directory */

  dirp = opendir(CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT);
  if (dirp == NULL)
    {
      /* Failed to open the directory */

      fprintf(stderr, "Csection Monitor: Failed to open directory: %s\n",
              CONFIG_SYSTEM_CRITMONITOR_MOUNTPOINT);
      return EXIT_FAILURE;
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

              fprintf(stderr, "Csection Monitor: "
                      "Failed to process sub-directory: %s\n",
                      entryp->d_name);

              if (++errcount > 100)
                {
                  fprintf(stderr, "Csection Monitor: "
                          "Too many errors ... exiting\n");
                  exitcode = EXIT_FAILURE;
                  break;
                }
            }
        }
    }

  closedir(dirp);
  fputc('\n', stdout);
  return exitcode;
}

/****************************************************************************
 * Name: critmon_daemon
 ****************************************************************************/

static int critmon_daemon(int argc, char **argv)
{
  int exitcode = EXIT_SUCCESS;

  printf("Csection Monitor: Running: %d\n", g_critmon.pid);

  /* Loop until we detect that there is a request to stop. */

  while (!g_critmon.stop)
    {
      exitcode = critmon_list_once();
      if (exitcode != EXIT_SUCCESS)
        {
          break;
        }

      /* Wait for the next sample interval */

      sleep(CONFIG_SYSTEM_CRITMONITOR_INTERVAL);
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

int critmon_start_main(int argc, char **argv)
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

      ret = task_create("Csection Monitor",
                        CONFIG_SYSTEM_CRITMONITOR_DAEMON_PRIORITY,
                        CONFIG_SYSTEM_CRITMONITOR_DAEMON_STACKSIZE,
                        (main_t)critmon_daemon, (FAR char * const *)NULL);
      if (ret < 0)
        {
          int errcode = errno;
          printf("Csection Monitor ERROR: "
                 "Failed to start the stack monitor: %d\n",
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

int critmon_main(int argc, char **argv)
{
  return critmon_list_once();
}

#endif /* CONFIG_SYSTEM_CRITMONITOR */
