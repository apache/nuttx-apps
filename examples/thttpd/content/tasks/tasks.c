/****************************************************************************
 * apps/examples/thttpd/content/tasks/tasks.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef THTTPD_PROCFS_MOUNTPOINT
#  define THTTPD_PROCFS_MOUNTPOINT "/proc"
#endif

#define THTTPD_STATUS_SIZE 512

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct thttpd_task_status_s
{
  FAR const char *name;
  FAR const char *type;
  FAR const char *state;
  FAR const char *priority;
  FAR const char *policy;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tasks_trim_value
 ****************************************************************************/

static FAR char *tasks_trim_value(FAR char *line)
{
  FAR char *end;

  while (isblank((unsigned char)*line) && *line != '\0')
    {
      line++;
    }

  end = line + strlen(line);
  while (end > line &&
         (*(end - 1) == '\n' || *(end - 1) == '\r' ||
          isblank((unsigned char)*(end - 1))))
    {
      end--;
    }

  *end = '\0';
  return line;
}

/****************************************************************************
 * Name: tasks_is_pid_dir
 ****************************************************************************/

static bool tasks_is_pid_dir(FAR const struct dirent *entryp)
{
  FAR const char *name = entryp->d_name;

  if (!DIRENT_ISDIRECTORY(entryp->d_type) || *name == '\0')
    {
      return false;
    }

  for (; *name != '\0'; name++)
    {
      if (!isdigit((unsigned char)*name))
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: tasks_parse_status_line
 ****************************************************************************/

static void tasks_parse_status_line(FAR char *line,
                                    FAR struct thttpd_task_status_s *status)
{
  if (strncmp(line, "Name:", 5) == 0)
    {
      status->name = tasks_trim_value(&line[5]);
    }
  else if (strncmp(line, "Type:", 5) == 0)
    {
      status->type = tasks_trim_value(&line[5]);
    }
  else if (strncmp(line, "State:", 6) == 0)
    {
      status->state = tasks_trim_value(&line[6]);
    }
  else if (strncmp(line, "Priority:", 9) == 0)
    {
      status->priority = tasks_trim_value(&line[9]);
    }
  else if (strncmp(line, "Scheduler:", 10) == 0)
    {
      status->policy = tasks_trim_value(&line[10]);
    }
}

/****************************************************************************
 * Name: tasks_parse_status
 ****************************************************************************/

static void tasks_parse_status(FAR char *buffer,
                               FAR struct thttpd_task_status_s *status)
{
  FAR char *line = buffer;

  while (line != NULL && *line != '\0')
    {
      FAR char *next = strchr(line, '\n');
      if (next != NULL)
        {
          *next++ = '\0';
        }

      tasks_parse_status_line(line, status);
      line = next;
    }
}

/****************************************************************************
 * Name: tasks_read_status
 ****************************************************************************/

static int tasks_read_status(FAR const char *pid, FAR char *buffer,
                             size_t buflen,
                             FAR struct thttpd_task_status_s *status)
{
  char filepath[sizeof(THTTPD_PROCFS_MOUNTPOINT) + NAME_MAX +
                sizeof("/status") + 1];
  ssize_t nread;
  size_t total = 0;
  int ret;
  int fd;

  ret = snprintf(filepath, sizeof(filepath), "%s/%s/status",
                 THTTPD_PROCFS_MOUNTPOINT, pid);
  if (ret < 0 || ret >= sizeof(filepath))
    {
      return -ENAMETOOLONG;
    }

  fd = open(filepath, O_RDONLY);
  if (fd < 0)
    {
      return -errno;
    }

  while (total < buflen - 1)
    {
      nread = read(fd, &buffer[total], buflen - 1 - total);
      if (nread < 0)
        {
          ret = -errno;
          close(fd);
          return ret;
        }
      else if (nread == 0)
        {
          break;
        }

      total += nread;
    }

  close(fd);
  buffer[total] = '\0';
  tasks_parse_status(buffer, status);
  return OK;
}

/****************************************************************************
 * Name: show_task
 ****************************************************************************/

static void show_task(FAR const char *pid,
                      FAR const struct thttpd_task_status_s *status)
{
  printf("%5s %-12s %-14s %-7s %-18s %s\n",
         pid, status->priority, status->policy, status->type,
         status->state, status->name);
}

/****************************************************************************
 * Name: show_tasks
 ****************************************************************************/

static int show_tasks(void)
{
  FAR struct dirent *entryp;
  DIR *dirp;

  dirp = opendir(THTTPD_PROCFS_MOUNTPOINT);
  if (dirp == NULL)
    {
      printf("Unable to open %s (is procfs mounted?): %d\n",
             THTTPD_PROCFS_MOUNTPOINT, errno);
      return -errno;
    }

  while ((entryp = readdir(dirp)) != NULL)
    {
      char buffer[THTTPD_STATUS_SIZE];
      struct thttpd_task_status_s status =
      {
        "", "", "", "", ""
      };

      if (!tasks_is_pid_dir(entryp))
        {
          continue;
        }

      if (tasks_read_status(entryp->d_name, buffer, sizeof(buffer),
                            &status) < 0)
        {
          continue;
        }

      show_task(entryp->d_name, &status);
    }

  closedir(dirp);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_THTTPD_BINFS
int tasks_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  puts(
    "Content-type: text/html\r\n"
    "Status: 200/html\r\n"
    "\r\n"
    "<html>\r\n"
    "<head>\r\n"
    "<title>NuttX Tasks</title>\r\n"
    "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\r\n"
    "</head>\r\n"
    "<body bgcolor=\"#fffeec\" text=\"black\">\r\n"
    "<div class=\"menu\">\r\n"
    "<div class=\"menubox\"><a href=\"/index.html\">Front page</a></div>\r\n"
    "<div class=\"menubox\"><a href=\"hello\">Say Hello</a></div>\r\n"
    "<div class=\"menubox\"><a href=\"tasks\">Tasks</a></div>\r\n"
    "<br>\r\n"
    "</div>\r\n"
    "<div class=\"contentblock\">\r\n"
    "<pre>\r\n");

  printf("%5s %-12s %-14s %-7s %-18s %s\n",
         "PID", "PRI", "SCHEDULER", "TYPE", "STATE", "NAME");

  show_tasks();

  puts(
    "</pre>\r\n"
    "</body>\r\n"
    "</html>\r\n");

  return 0;
}
