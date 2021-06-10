/****************************************************************************
 * apps/system/stackmonitor/stackmonitor.c
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

#ifdef CONFIG_SYSTEM_STACKMONITOR

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SYSTEM_STACKMONITOR_STACKSIZE
#  define CONFIG_SYSTEM_STACKMONITOR_STACKSIZE 2048
#endif

#ifndef CONFIG_SYSTEM_STACKMONITOR_PRIORITY
#  define CONFIG_SYSTEM_STACKMONITOR_PRIORITY 50
#endif

#ifndef CONFIG_SYSTEM_STACKMONITOR_INTERVAL
#  define CONFIG_SYSTEM_STACKMONITOR_INTERVAL 2
#endif

#ifndef CONFIG_SYSTEM_STACKMONITOR_MOUNTPOINT
#  define CONFIG_SYSTEM_STACKMONITOR_MOUNTPOINT "/proc"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stkmon_state_s
{
  volatile bool started;
  volatile bool stop;
  pid_t pid;
  char line[80];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stkmon_state_s g_stackmonitor;
#if CONFIG_TASK_NAME_SIZE > 0
static const char g_name[] = "Name:";
#endif
static const char g_stacksize[] = "StackSize:";
static const char g_stackused[] = "StackUsed:";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stkmon_isolate_value
 ****************************************************************************/

static FAR char *stkmon_isolate_value(FAR char *line)
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
 * Name: stkmon_process_directory
 ****************************************************************************/

static int stkmon_process_directory(FAR struct dirent *entryp)
{
  FAR char *filepath;
  FAR char *endptr;
  FAR const char *tmpstr;
  FILE *stream;
  unsigned long stack_size;
  unsigned long stack_used;
  int errcode;
  int len;
  int ret;

#if CONFIG_TASK_NAME_SIZE > 0
  FAR char *name = NULL;

  /* Read the task status to get the task name */

  filepath = NULL;
  ret = asprintf(&filepath,
                 CONFIG_SYSTEM_STACKMONITOR_MOUNTPOINT "/%s/status",
                 entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr,
              "Stack Monitor: Failed to create path to status file: %d\n",
              errcode);
      return -errcode;
    }

  /* Open the status file */

  stream = fopen(filepath, "r");
  if (stream == NULL)
    {
      ret = -errno;
      fprintf(stderr, "Stack Monitor: Failed to open %s: %d\n",
              filepath, ret);
      goto errout_with_filepath;
    }

  while (fgets(g_stackmonitor.line, 80, stream) != NULL)
    {
      g_stackmonitor.line[79] = '\n';
      len = strlen(g_name);
      if (strncmp(g_stackmonitor.line, g_name, len) == 0)
        {
          tmpstr = stkmon_isolate_value(&g_stackmonitor.line[len]);
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

  /* Read stack information */

  stack_size = 0;
  stack_used = 0;
  filepath   = NULL;

  ret = asprintf(&filepath,
                 CONFIG_SYSTEM_STACKMONITOR_MOUNTPOINT "/%s/stack",
                 entryp->d_name);
  if (ret < 0 || filepath == NULL)
    {
      errcode = errno;
      fprintf(stderr,
              "Stack Monitor: Failed to create path to stack file: %d\n",
              errcode);
      ret = -EINVAL;
      goto errout_with_name;
    }

  /* Open the stack file */

  stream = fopen(filepath, "r");
  if (stream == NULL)
    {
      ret = -errno;
      fprintf(stderr, "Stack Monitor: Failed to open %s: %d\n",
              filepath, ret);
      goto errout_with_filepath;
    }

  while (fgets(g_stackmonitor.line, 80, stream) != NULL)
    {
      g_stackmonitor.line[79] = '\n';
      len = strlen(g_stacksize);
      if (strncmp(g_stackmonitor.line, g_stacksize, len) == 0)
        {
          tmpstr = stkmon_isolate_value(&g_stackmonitor.line[len]);
          if (*tmpstr == '\0')
            {
              ret = -EINVAL;
              goto errout_with_stream;
            }

          stack_size = (uint32_t)strtoul(tmpstr, &endptr, 10);
          if (*endptr != '\0')
            {
              fprintf(stderr,
                      "Stack Monitor: Bad numeric value %s\n", tmpstr);
              ret = -EINVAL;
              goto errout_with_stream;
            }
        }
      else
        {
          len = strlen(g_stackused);
          if (strncmp(g_stackmonitor.line, g_stackused, len) == 0)
            {
              tmpstr = stkmon_isolate_value(&g_stackmonitor.line[len]);
              if (*tmpstr == '\0')
                {
                  ret = -EINVAL;
                  goto errout_with_stream;
                }

              stack_used = (uint32_t)strtoul(tmpstr, &endptr, 10);
              if (*endptr != '\0')
                {
                  fprintf(stderr,
                          "Stack Monitor: Bad numeric value %s\n", tmpstr);
                  ret = -EINVAL;
                  goto errout_with_stream;
                }
            }
        }
    }

  /* Finally, output the stack info that we gleaned from the procfs */

#if CONFIG_TASK_NAME_SIZE > 0
  printf("%5s %6lu %6lu %s\n",
         entryp->d_name, stack_size, stack_used, name);
#else
  printf("%5s %6lu %6lu\n",
         entryp->d_name, stack_size, stack_used);
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
 * Name: stackmonitor_check_name
 ****************************************************************************/

static bool stackmonitor_check_name(FAR char *name)
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
 * Name: stackmonitor_daemon
 ****************************************************************************/

static int stackmonitor_daemon(int argc, char **argv)
{
  DIR *dirp;
  int exitcode = EXIT_SUCCESS;
  int errcount = 0;
  int ret;

  printf("Stack Monitor: Running: %d\n", g_stackmonitor.pid);

  /* Loop until we detect that there is a request to stop. */

  while (!g_stackmonitor.stop)
    {
      /* Wait for the next sample interval */

      sleep(CONFIG_SYSTEM_STACKMONITOR_INTERVAL);

      /* Open the top-level procfs directory */

      dirp = opendir(CONFIG_SYSTEM_STACKMONITOR_MOUNTPOINT);
      if (dirp == NULL)
        {
          /* Failed to open the directory */

          fprintf(stderr, "Stack Monitor: Failed to open directory: %s\n",
                  CONFIG_SYSTEM_STACKMONITOR_MOUNTPOINT);

          if (++errcount > 100)
            {
              fprintf(stderr,
                      "Stack Monitor: Too many errors ... exiting\n");
              exitcode = EXIT_FAILURE;
              break;
            }
        }

      /* Output the header */

#if CONFIG_TASK_NAME_SIZE > 0
      printf("%-5s %-6s %-6s %s\n", "PID", "SIZE", "USED", "THREAD NAME");
#else
      printf("%-5s %-6s %-6s\n", "PID", "SIZE", "USED");
#endif

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
              stackmonitor_check_name(entryp->d_name))
            {
              /* Looks good -- process the directory */

              ret = stkmon_process_directory(entryp);
              if (ret < 0)
                {
                  /* Failed to process the thread directory */

                  fprintf(stderr, "Stack Monitor: "
                          "Failed to process sub-directory: %s\n",
                          entryp->d_name);

                  if (++errcount > 100)
                    {
                      fprintf(stderr,
                             "Stack Monitor: Too many errors ... exiting\n");
                      exitcode = EXIT_FAILURE;
                      break;
                    }
                }
            }
        }

      closedir(dirp);
    }

  /* Stopped */

  g_stackmonitor.stop    = false;
  g_stackmonitor.started = false;
  printf("Stack Monitor: Stopped: %d\n", g_stackmonitor.pid);

  return exitcode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  /* Has the monitor already started? */

  sched_lock();
  if (!g_stackmonitor.started)
    {
      int ret;

      /* No.. start it now */

      /* Then start the stack monitoring daemon */

      g_stackmonitor.started = true;
      g_stackmonitor.stop    = false;

      ret = task_create("Stack Monitor", CONFIG_SYSTEM_STACKMONITOR_PRIORITY,
                        CONFIG_SYSTEM_STACKMONITOR_STACKSIZE,
                        (main_t)stackmonitor_daemon,
                        (FAR char * const *)NULL);
      if (ret < 0)
        {
          int errcode = errno;
          printf("Stack Monitor ERROR: "
                 "Failed to start the stack monitor: %d\n",
                 errcode);
        }
      else
        {
          g_stackmonitor.pid = ret;
          printf("Stack Monitor: Started: %d\n", g_stackmonitor.pid);
        }

      sched_unlock();
      return 0;
    }

  sched_unlock();
  printf("Stack Monitor: %s: %d\n",
         g_stackmonitor.stop ? "Stopping" : "Running", g_stackmonitor.pid);
  return 0;
}

int stackmonitor_stop_main(int argc, char **argv)
{
  /* Has the monitor already started? */

  if (g_stackmonitor.started)
    {
      /* Stop the stack monitor.  The next time the monitor wakes up,
       * it will see the stop indication and will exist.
       */

      printf("Stack Monitor: Stopping: %d\n", g_stackmonitor.pid);
      g_stackmonitor.stop = true;
    }

  printf("Stack Monitor: Stopped: %d\n", g_stackmonitor.pid);
  return 0;
}

#endif /* CONFIG_SYSTEM_STACKMONITOR */
