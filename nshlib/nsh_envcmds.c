/****************************************************************************
 * apps/nshlib/nsh_envcmds.c
 *
 *   Copyright (C) 2007-2009, 2011-2012, 2018 Gregory Nutt. All rights
 *   reserved.
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
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
static const char g_pwd[]    = "PWD";
#ifndef CONFIG_NSH_DISABLE_CD
static const char g_oldpwd[] = "OLDPWD";
#endif
static const char g_home[]   = CONFIG_LIB_HOMEDIR;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_getwd
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
static inline FAR const char *nsh_getwd(const char *wd)
{
  const char *val;

  /* If no working directory is defined, then default to the home directory */

  val = getenv(wd);
  if (val == NULL)
    {
      val = g_home;
    }

  return val;
}
#endif

/****************************************************************************
 * Name: nsh_getdirpath
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
static inline char *nsh_getdirpath(FAR struct nsh_vtbl_s *vtbl,
                                   const char *dirpath, const char *relpath)
{
  FAR char *alloc;
  int len;

  /* Handle the special case where the dirpath is simply "/" */

  if (strcmp(dirpath, "/") == 0)
    {
      len   = strlen(relpath) + 2;
      alloc = (FAR char *)malloc(len);
      if (alloc)
        {
          sprintf(alloc, "/%s", relpath);
        }
    }
  else
    {
      len = strlen(dirpath) + strlen(relpath) + 2;
      alloc = (FAR char *)malloc(len);
      if (alloc)
        {
          sprintf(alloc, "%s/%s", dirpath, relpath);
        }
    }

  if (!alloc)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, "nsh_getdirpath");
    }

  return alloc;
}
#endif

/****************************************************************************
 * Name: nsh_dumpvar
 ****************************************************************************/

#if defined(CONFIG_NSH_VARS) && !defined(CONFIG_NSH_DISABLE_SET)
static int nsh_dumpvar(FAR struct nsh_vtbl_s *vtbl, FAR void *arg,
                       FAR const char *pair)
{
  nsh_output(vtbl, "%s\n", pair);
  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_getwd
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
FAR const char *nsh_getcwd(void)
{
  return nsh_getwd(g_pwd);
}
#endif

/****************************************************************************
 * Name: nsh_getfullpath
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
FAR char *nsh_getfullpath(FAR struct nsh_vtbl_s *vtbl,
                          FAR const char *relpath)
{
  const char *wd;

  /* Handle some special cases */

  if (!relpath || relpath[0] == '\0')
    {
      /* No relative path provided */

      return strdup(g_home);
    }
  else if (relpath[0] == '/')
    {
      return strdup(relpath);
    }

  /* Get the path to the current working directory */

  wd = nsh_getcwd();

  /* Fake the '.' directory */

  if (strcmp(relpath, ".") == 0)
    {
      return strdup(wd);
    }

  /* Return the full path */

  return nsh_getdirpath(vtbl, wd, relpath);
}
#endif

/****************************************************************************
 * Name: nsh_freefullpath
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
void nsh_freefullpath(FAR char *fullpath)
{
  if (fullpath)
    {
      free(fullpath);
    }
}
#endif

/****************************************************************************
 * Name: cmd_cd
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
#ifndef CONFIG_NSH_DISABLE_CD
int cmd_cd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR const char *path = argv[1];
  FAR char *alloc = NULL;
  FAR char *fullpath = NULL;
  int ret = OK;

  /* Check for special arguments */

  if (argc < 2 || strcmp(path, "~") == 0)
    {
      path = g_home;
    }
  else if (strcmp(path, "-") == 0)
    {
      alloc = strdup(nsh_getwd(g_oldpwd));
      path  = alloc;
    }
  else if (strcmp(path, "..") == 0)
    {
      alloc = strdup(nsh_getcwd());
      path  = dirname(alloc);
    }
  else
    {
      fullpath = nsh_getfullpath(vtbl, path);
      path     = fullpath;
    }

  /* Set the new working directory */

  ret = chdir(path);
  if (ret != 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "chdir", NSH_ERRNO);
      ret = ERROR;
    }

  /* Free any memory that was allocated */

  if (alloc)
    {
      free(alloc);
    }

  if (fullpath)
    {
      nsh_freefullpath(fullpath);
    }

  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_echo
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_ECHO
int cmd_echo(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  int i;
  int s = 1;

  if (argc > 1 && 0 == strncmp(argv[1], "-n", 2))
    {
      s = 2;
    }

  /* echo each argument, separated by a space as it must have been on the
   * command line.
   */

  for (i = s; i < argc; i++)
    {
      if (i != s)
        {
          nsh_output(vtbl, " ");
        }

      nsh_output(vtbl, "%s", argv[i]);
    }

  if (1 == s)
    {
      nsh_output(vtbl, "\n");
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_env
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_ENV
int cmd_env(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  return nsh_catfile(vtbl, argv[0],
                     CONFIG_NSH_PROC_MOUNTPOINT "/self/group/env");
}
#endif

/****************************************************************************
 * Name: cmd_pwd
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ENVIRON
#ifndef CONFIG_NSH_DISABLE_PWD
int cmd_pwd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  nsh_output(vtbl, "%s\n", nsh_getcwd());
  return OK;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_set
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_SET
int cmd_set(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *value;
  int ret = OK;
#ifdef NSH_HAVE_VARS
  int ndx = 1;
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
  FAR char *popt;
  const char opts[] = NSH_NP_SET_OPTIONS;
  int op;

#ifdef CONFIG_NSH_VARS
  /* Set with no arguments will show all of the NSH variables */

  if (argc == 1)
    {
      ret = nsh_foreach_var(vtbl, nsh_dumpvar, NULL);
      nsh_output(vtbl, "\n");
      return ret < 0 ? ERROR : OK;
    }
  else
#endif
#ifdef NSH_HAVE_VARS
  /* Support set [{+|-}{e|x|xe|ex}] [<name> <value>] */

  if (argc == 2 || argc == 4)
#else
  /* Support set [{+|-}{e|x|xe|ex}] */

#endif
    {
      if (strlen(argv[1]) < 2)
        {
          ret = -EINVAL;
          nsh_error(vtbl, g_fmtargrequired, argv[0], "set", NSH_ERRNO);
        }
      else
        {
          op = argv[1][0];
          if (op != '-' && op != '+')
            {
              ret = -EINVAL;
              nsh_error(vtbl, g_fmtarginvalid, argv[0], "set", NSH_ERRNO);
            }
          else
            {
              value = &argv[1][1];
              while (*value && *value != ' ')
                {
                  popt = strchr(opts, *value++);
                  if (popt == NULL)
                    {
                      nsh_error(vtbl, g_fmtarginvalid,
                                argv[0], "set", NSH_ERRNO);
                      ret = -EINVAL;
                      break;
                    }

                  if (op == '+')
                    {
                      vtbl->np.np_flags |= 1 << (popt - opts);
                    }
                  else
                    {
                      vtbl->np.np_flags &= ~(1 << (popt - opts));
                    }
                }

#ifdef NSH_HAVE_VARS
              if (ret == OK)
                {
                  ndx = 2;
                }
#endif
            }
        }
    }

#ifdef NSH_HAVE_VARS
  if (ret == OK && (argc == 3 || argc == 4))
#endif
#endif /* CONFIG_NSH_DISABLESCRIPT */
#ifdef NSH_HAVE_VARS
    {
#if defined(CONFIG_NSH_VARS) && !defined(CONFIG_DISABLE_ENVIRON)
      FAR char *oldvalue;
#endif

      /* Trim whitespace from the value */

      value = nsh_trimspaces(argv[ndx + 1]);

#ifdef CONFIG_NSH_VARS
#ifndef CONFIG_DISABLE_ENVIRON
      /* Check if the NSH variable has already been promoted to an group-
       * wide environment variable.
       *
       * REVISIT:  Is this the correct behavior?  Bash would create/modify
       * a local variable that shadows the environment variable.
       */

      oldvalue = getenv(argv[ndx]);
      if (oldvalue == NULL)
#endif
        {
          /* Set the NSH variable */

          ret = nsh_setvar(vtbl, argv[ndx], value);
          if (ret < 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0], "nsh_setvar",
                         NSH_ERRNO_OF(-ret));
            }
        }
#endif /* CONFIG_NSH_VARS */

#if !defined(CONFIG_DISABLE_ENVIRON)
#ifdef CONFIG_NSH_VARS
      else
#endif
        {
          /* Set the environment variable */

          ret = setenv(argv[ndx], value, TRUE);
          if (ret < 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0], "setenv",
                               NSH_ERRNO);
            }
        }
#endif /* !CONFIG_DISABLE_ENVIRON */
    }
#endif /* NSH_HAVE_VARS */

  return ret;
}
#endif /* CONFIG_NSH_DISABLE_SET */

/****************************************************************************
 * Name: cmd_unset
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_UNSET
int cmd_unset(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
#if defined(CONFIG_NSH_VARS) || !defined(CONFIG_DISABLE_ENVIRON)
  int status;
#endif
  int ret = OK;

#if defined(CONFIG_NSH_VARS)
  /* Unset NSH variable */

  status = nsh_unsetvar(vtbl, argv[1]);
  if (status < 0 && status != -ENOENT)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "nsh_unsetvar",
                NSH_ERRNO_OF(-status));
      ret = ERROR;
    }
#endif

#if !defined(CONFIG_DISABLE_ENVIRON)
  /* Unset environment variable */

  status = unsetenv(argv[1]);
  if (status < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "unsetenv", NSH_ERRNO);
      ret = ERROR;
    }
#endif

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_export
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_EXPORT
int cmd_export(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR const char *value = "";
  int status;
  int ret = OK;

  /* Get the value from the command line if provided.  argc may be either 2
   * or 3
   */

  if (argc == 3)
    {
      value = argv[2];
    }
  else
    {
      FAR const char *tmp;

      /* Try to get the value from the NSH variable */

      tmp = nsh_getvar(vtbl, argv[1]);
      if (tmp != NULL)
        {
          value = tmp;
        }
    }

  /* Set the environment variable to the selected value */

  status = setenv(argv[1], value, TRUE);
  if (status < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "unsetenv", NSH_ERRNO);
      ret = ERROR;
    }
  else
    {
      /* Unset NSH variable.
       *
       * REVISIT:  Is this the correct behavior?  Bash would retain
       * a local variable that shadows the environment variable.
       */

      status = nsh_unsetvar(vtbl, argv[1]);
      if (status < 0 && status != -ENOENT)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "nsh_unsetvar",
                     NSH_ERRNO_OF(-status));
          ret = ERROR;
        }
    }

  return ret;
}
#endif
