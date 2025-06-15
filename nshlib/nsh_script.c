/****************************************************************************
 * apps/nshlib/nsh_script.c
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

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

#include "nsh.h"
#include "nsh_console.h"

#include <system/readline.h>

#ifndef CONFIG_NSH_DISABLESCRIPT

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_ETC_ROMFS
static bool g_nsh_script_initialized;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_ETC_ROMFS) || defined(CONFIG_NSH_ROMFSRC)
static int nsh_script_redirect(FAR struct nsh_vtbl_s *vtbl,
                               FAR const char *cmd,
                               FAR const char *path,
                               bool log)
{
  uint8_t save[SAVE_SIZE];
  int fd = -1;
  int ret;

  if (CONFIG_NSH_SCRIPT_REDIRECT_PATH[0])
    {
      fd = open(CONFIG_NSH_SCRIPT_REDIRECT_PATH, 0666);
      if (fd > 0)
        {
          nsh_redirect(vtbl, 0, fd, save);
        }
    }

  ret = nsh_script(vtbl, cmd, path, log);
  if (CONFIG_NSH_SCRIPT_REDIRECT_PATH[0])
    {
      if (fd > 0)
        {
          nsh_undirect(vtbl, save);
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_script
 *
 * Description:
 *   Execute the NSH script at path.
 *
 ****************************************************************************/

int nsh_script(FAR struct nsh_vtbl_s *vtbl, FAR const FAR char *cmd,
               FAR const char *path, bool log)
{
  FAR char *fullpath;
  int savestream;
  FAR char *buffer;
  int ret = ERROR;

  /* The path to the script may relative to the current working directory */

  fullpath = nsh_getfullpath(vtbl, path);
  if (!fullpath)
    {
      return ERROR;
    }

  /* Get a reference to the common input buffer */

  buffer = nsh_linebuffer(vtbl);
  if (buffer)
    {
      /* Save the parent stream in case of nested script processing */

      savestream = vtbl->np.np_fd;

      /* Open the file containing the script */

      vtbl->np.np_fd = open(fullpath, O_RDOK | O_CLOEXEC);
      if (vtbl->np.np_fd < 0)
        {
          if (log)
            {
              nsh_error(vtbl, g_fmtcmdfailed, cmd, "open", NSH_ERRNO);
            }

          /* Free the allocated path */

          nsh_freefullpath(fullpath);

          /* Restore the parent script stream */

          vtbl->np.np_fd = savestream;
          return ERROR;
        }

      /* Loop, processing each command line in the script file (or
       * until an error occurs)
       */

      do
        {
#ifndef CONFIG_NSH_DISABLE_LOOPS
          /* Get the current file position.  This is used to control
           * looping.  If a loop begins in the next line, then this file
           * offset will be needed to locate the top of the loop in the
           * script file.  Note that lseek will return -1 on failure.
           */

          vtbl->np.np_foffs = lseek(vtbl->np.np_fd, 0, SEEK_CUR);
          vtbl->np.np_loffs = 0;

          if (vtbl->np.np_foffs < 0 && log)
            {
              nsh_error(vtbl, g_fmtcmdfailed, "loop", "lseek", NSH_ERRNO);
            }
#endif

          /* Now read the next line from the script file */

          ret = readline_fd(buffer, LINE_MAX, vtbl->np.np_fd, -1);
          if (ret >= 0)
            {
              /* Parse process the command.  NOTE:  this is recursive...
               * we got to cmd_source via a call to nsh_parse.  So some
               * considerable amount of stack may be used.
               */

              if ((vtbl->np.np_flags & NSH_PFLAG_SILENT) == 0)
                {
                  nsh_output(vtbl, "%s", buffer);
                }

              if (vtbl->np.np_flags & NSH_PFLAG_IGNORE)
                {
                  nsh_parse(vtbl, buffer);
                }
              else
                {
                  ret = nsh_parse(vtbl, buffer);
                }
            }
        }
      while (ret >= 0);

      /* Close the script file */

      close(vtbl->np.np_fd);

      /* Restore the parent script stream */

      vtbl->np.np_fd = savestream;
    }

  /* Free the allocated path */

  nsh_freefullpath(fullpath);
  return ret;
}

/****************************************************************************
 * Name: nsh_sysinitscript
 *
 * Description:
 *   Attempt to execute the configured system initialization script. This
 *   script should be executed once when NSH starts.
 *
 ****************************************************************************/

#ifdef CONFIG_ETC_ROMFS
int nsh_sysinitscript(FAR struct nsh_vtbl_s *vtbl)
{
  /* Since most existing systems only use the NSH_INITPATH script file.
   * Do not log error output for a missing NSH_SYSINITPATH script file.
   */

  return nsh_script_redirect(vtbl, "sysinit", NSH_SYSINITPATH, false);
}
#endif

/****************************************************************************
 * Name: nsh_initscript
 *
 * Description:
 *   Attempt to execute the configured initialization script.  This script
 *   should be executed once when NSH starts.  nsh_initscript is idempotent
 *   and may, however, be called multiple times (the script will be executed
 *   once.
 *
 ****************************************************************************/

#ifdef CONFIG_ETC_ROMFS
int nsh_initscript(FAR struct nsh_vtbl_s *vtbl)
{
  bool already;
  int ret = OK;

  /* Atomic test and set of the g_nsh_script_initialized flag */

  sched_lock();
  already                  = g_nsh_script_initialized;
  g_nsh_script_initialized = true;
  sched_unlock();

  /* If we have not already executed the init script, then do so now */

  if (!already)
    {
      ret = nsh_script_redirect(vtbl, "init", NSH_INITPATH, true);
#ifndef CONFIG_NSH_DISABLESCRIPT
      /* Reset the option flags */

      vtbl->np.np_flags = NSH_NP_SET_OPTIONS_INIT;
#endif
    }

  return ret;
}

/****************************************************************************
 * Name: nsh_loginscript
 *
 * Description:
 *   Attempt to execute the configured login script.  This script
 *   should be executed when each NSH session starts.
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_ROMFSRC
int nsh_loginscript(FAR struct nsh_vtbl_s *vtbl)
{
  return nsh_script_redirect(vtbl, "login", NSH_RCPATH, true);
}
#endif
#endif /* CONFIG_ETC_ROMFS */

#endif /* CONFIG_NSH_DISABLESCRIPT */
