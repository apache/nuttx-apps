/****************************************************************************
 * apps/nshlib/nsh_wait.c
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

#include <nuttx/sched.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#include "nsh.h"
#include "nsh_console.h"

#if !defined(CONFIG_NSH_DISABLE_WAIT) && defined(CONFIG_SCHED_WAITPID) && \
    !defined(CONFIG_DISABLE_PTHREAD) && defined(CONFIG_FS_PROCFS) && \
    !defined(CONFIG_FS_PROCFS_EXCLUDE_PROCESS)

static const char g_groupid[] = "Group:";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_wait
 *
 * Description:
 *   Handle 'cmd_wait' command from terminal.
 *   wait pid1 [pid2 [pid3] ..] - wait for a pid to exit.
 *
 * Input Parameters:
 *   vtbl - The NSH console.
 *   argc - Amount of argument strings in command.
 *   argv - The argument strings.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int cmd_wait(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *nextline;
  FAR char *line;
  char buf[128];
  char path[32];
  int status = 0;
  int ret = OK;
  pid_t self;
  pid_t tid;
  int fd;
  int i;

  if (argc == 1)
    {
      return OK;
    }

  self = getpid();
  for (i = 1; i < argc; i++)
    {
      tid = atoi(argv[i]);
      if (tid == 0)
        {
          continue;
        }

      snprintf(path, sizeof(path), "/proc/%d/status", tid);
      fd = open(path, O_RDONLY);
      if (fd < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "wait", NSH_ERRNO);
          continue;
        }

      ret = read(fd, buf, sizeof(buf) - 1);
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "wait", NSH_ERRNO);
          close(fd);
          continue;
        }

      close(fd);
      nextline = buf;
      do
        {
          line = nextline;
          for (nextline++;
               *nextline != '\0' && *nextline != '\n';
               nextline++);

          if (*nextline == '\n')
            {
              *nextline++ = '\0';
            }
          else
            {
              nextline = NULL;
            }

          if (strncmp(line, g_groupid, sizeof(g_groupid) - 1) == 0)
            {
              if (atoi(line + sizeof(g_groupid)) == self)
                {
                  ret = pthread_join(tid, (FAR pthread_addr_t *)&status);
                }
              else
                {
                  ret = waitpid(tid, &status, 0);
                }

              if (ret < 0)
                {
                  nsh_error(vtbl, g_fmtcmdfailed,
                            argv[0], "wait", NSH_ERRNO);
                }

              break;
            }
        }
      while (nextline != NULL);
    }

  return ret < 0 ? ret : status;
}

#endif /* CONFIG_NSH_DISABLE_WAIT */
