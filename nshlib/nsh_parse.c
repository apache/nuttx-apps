/****************************************************************************
 * apps/nshlib/nsh_parse.c
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

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#ifdef CONFIG_NSH_CMDPARMS
#  include <sys/stat.h>
#endif

#include <nuttx/version.h>
#include "nshlib/nshlib.h"

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* If CONFIG_NSH_CMDPARMS or CONFIG_NSH_ARGCAT is enabled, then we will need
 * retain a list of memory allocations to be freed at the completion of
 * command processing.
 */

#undef HAVE_MEMLIST
#if defined(CONFIG_NSH_CMDPARMS) || defined(CONFIG_NSH_ARGCAT)
#  define HAVE_MEMLIST 1
#endif

#if defined(HAVE_MEMLIST) && !defined(CONFIG_NSH_MAXALLOCS)
#  ifdef CONFIG_NSH_ARGCAT
#    define CONFIG_NSH_MAXALLOCS (2*CONFIG_NSH_MAXARGUMENTS)
#  else
#    define CONFIG_NSH_MAXALLOCS CONFIG_NSH_MAXARGUMENTS
#  endif
#endif

/* Allocation list helper macros */

#ifdef HAVE_MEMLIST
#  define NSH_MEMLIST_TYPE      struct nsh_memlist_s
#  define NSH_MEMLIST_INIT(m)   memset(&(m), 0, sizeof(struct nsh_memlist_s));
#  define NSH_MEMLIST_ADD(m,a)  nsh_memlist_add(m,a)
#  define NSH_MEMLIST_FREE(m)   nsh_memlist_free(m)
#else
#  define NSH_MEMLIST_TYPE      uint8_t
#  define NSH_MEMLIST_INIT(m)   do { (m) = 0; } while (0)
#  define NSH_MEMLIST_ADD(m,a)
#  define NSH_MEMLIST_FREE(m)
#endif

/* Do we need g_nullstring[]? */

#undef NEED_NULLSTRING
#if defined(NSH_HAVE_VARS) || defined(CONFIG_NSH_CMDPARMS)
#  define NEED_NULLSTRING       1
#elif !defined(CONFIG_NSH_ARGCAT) || !defined(HAVE_MEMLIST)
#  define NEED_NULLSTRING       1
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* These structure describes the parsed command line */

#ifndef CONFIG_NSH_DISABLEBG
struct cmdarg_s
{
  FAR struct nsh_vtbl_s *vtbl;      /* For front-end interaction */
  int fd;                           /* FD for output redirection */
  int argc;                         /* Number of arguments in argv */
  FAR char *argv[MAX_ARGV_ENTRIES]; /* Argument list */
};
#endif

/* This structure describes the allocation list */

#ifdef HAVE_MEMLIST
struct nsh_memlist_s
{
  int nallocs;                      /* Number of allocations */
  FAR char *allocations[CONFIG_NSH_MAXALLOCS];
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef HAVE_MEMLIST
static void nsh_memlist_add(FAR struct nsh_memlist_s *memlist,
              FAR char *allocation);
static void nsh_memlist_free(FAR struct nsh_memlist_s *memlist);
#endif

#ifndef CONFIG_NSH_DISABLEBG
static void nsh_releaseargs(struct cmdarg_s *arg);
static pthread_addr_t nsh_child(pthread_addr_t arg);
static struct cmdarg_s *nsh_cloneargs(FAR struct nsh_vtbl_s *vtbl,
               int fd, int argc, char *argv[]);
#endif

static int nsh_saveresult(FAR struct nsh_vtbl_s *vtbl, bool result);
static int nsh_execute(FAR struct nsh_vtbl_s *vtbl,
               int argc, FAR char *argv[], FAR const char *redirfile,
               int oflags);

#ifdef CONFIG_NSH_CMDPARMS
static FAR char *nsh_filecat(FAR struct nsh_vtbl_s *vtbl, FAR char *s1,
               FAR const char *filename);
static FAR char *nsh_cmdparm(FAR struct nsh_vtbl_s *vtbl, FAR char *cmdline,
               FAR char **allocation);
#endif

#ifdef CONFIG_NSH_ARGCAT
static FAR char *nsh_strcat(FAR struct nsh_vtbl_s *vtbl, FAR char *s1,
               FAR const char *s2);
#endif

#if defined(CONFIG_NSH_QUOTE) && defined(CONFIG_NSH_ARGCAT)
static FAR char *nsh_strchr(FAR const char *str, int ch);
#else
#  define nsh_strchr(s,c) strchr(s,c)
#endif

#ifdef NSH_HAVE_VARS
static FAR char *nsh_envexpand(FAR struct nsh_vtbl_s *vtbl,
               FAR char *varname);
#endif

#if defined(CONFIG_NSH_QUOTE) && defined(CONFIG_NSH_ARGCAT)
static void nsh_dequote(FAR char *cmdline);
#else
#  define nsh_dequote(c)
#endif

static FAR char *nsh_argexpand(FAR struct nsh_vtbl_s *vtbl,
               FAR char *cmdline, FAR char **allocation, FAR int *isenvvar);
static FAR char *nsh_argument(FAR struct nsh_vtbl_s *vtbl, char **saveptr,
               FAR NSH_MEMLIST_TYPE *memlist, FAR int *isenvvar);

#ifndef CONFIG_NSH_DISABLESCRIPT
#ifndef CONFIG_NSH_DISABLE_LOOPS
static bool nsh_loop_enabled(FAR struct nsh_vtbl_s *vtbl);
#endif
#ifndef CONFIG_NSH_DISABLE_ITEF
static bool nsh_itef_enabled(FAR struct nsh_vtbl_s *vtbl);
#endif
static bool nsh_cmdenabled(FAR struct nsh_vtbl_s *vtbl);
#ifndef CONFIG_NSH_DISABLE_LOOPS
static int nsh_loop(FAR struct nsh_vtbl_s *vtbl, FAR char **ppcmd,
                    FAR char **saveptr, FAR NSH_MEMLIST_TYPE *memlist);
#endif
#ifndef CONFIG_NSH_DISABLE_ITEF
static int nsh_itef(FAR struct nsh_vtbl_s *vtbl, FAR char **ppcmd,
                    FAR char **saveptr, FAR NSH_MEMLIST_TYPE *memlist);
#endif
#endif

#ifndef CONFIG_NSH_DISABLEBG
static int nsh_nice(FAR struct nsh_vtbl_s *vtbl, FAR char **ppcmd,
               FAR char **saveptr, FAR NSH_MEMLIST_TYPE *memlist);
#endif

#ifdef CONFIG_NSH_CMDPARMS
static int nsh_parse_cmdparm(FAR struct nsh_vtbl_s *vtbl, FAR char *cmdline,
               FAR const char *redirfile);
#endif

static int nsh_parse_command(FAR struct nsh_vtbl_s *vtbl, FAR char *cmdline);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_token_separator[] = " \t\n";
#ifndef NSH_DISABLE_SEMICOLON
static const char g_line_separator[]  = "\"#;\n";
#endif
#ifdef CONFIG_NSH_ARGCAT
static const char g_arg_separator[]   = "`$";
#endif
static const char g_redirect1[]       = ">";
static const char g_redirect2[]       = ">>";
#ifdef NSH_HAVE_VARS
static const char g_exitstatus[]      = "?";
static const char g_success[]         = "0";
static const char g_failure[]         = "1";
#endif
#ifdef NEED_NULLSTRING
static const char g_nullstring[]      = "";
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* If NuttX versioning information is available, Include that information
 * in the NSH greeting.
 */

#if CONFIG_VERSION_MAJOR != 0 || CONFIG_VERSION_MINOR != 0
const char g_nshgreeting[]       =
  "\nNuttShell (NSH) NuttX-" CONFIG_VERSION_STRING "\n";
#else
const char g_nshgreeting[]       = "\nNuttShell (NSH)\n";
#endif

/* Fixed Message of the Day (MOTD) */

#if defined(CONFIG_NSH_MOTD) && !defined(CONFIG_NSH_PLATFORM_MOTD)
const char g_nshmotd[]           = CONFIG_NSH_MOTD_STRING;
#endif

/* Telnet login prompts */

#ifdef CONFIG_NSH_LOGIN
#if defined(CONFIG_NSH_TELNET_LOGIN) && defined(CONFIG_NSH_TELNET)
const char g_telnetgreeting[]    =
  "\nWelcome to NuttShell(NSH) Telnet Server...\n";
#endif
const char g_userprompt[]        = "login: ";
const char g_passwordprompt[]    = "password: ";
const char g_loginsuccess[]      = "\nUser Logged-in!\n";
const char g_badcredentials[]    = "\nInvalid username or password\n";
const char g_loginfailure[]      = "Login failed!\n";
#endif

/* The NSH prompt */

const char g_nshprompt[]         = CONFIG_NSH_PROMPT_STRING;

/* Common, message formats */

const char g_fmtsyntax[]         = "nsh: %s: syntax error\n";
const char g_fmtargrequired[]    = "nsh: %s: missing required argument(s)\n";
const char g_fmtnomatching[]     = "nsh: %s: no matching %s\n";
const char g_fmtarginvalid[]     = "nsh: %s: argument invalid\n";
const char g_fmtargrange[]       = "nsh: %s: value out of range\n";
const char g_fmtcmdnotfound[]    = "nsh: %s: command not found\n";
const char g_fmtnosuch[]         = "nsh: %s: no such %s: %s\n";
const char g_fmttoomanyargs[]    = "nsh: %s: too many arguments\n";
const char g_fmtdeepnesting[]    = "nsh: %s: nesting too deep\n";
const char g_fmtcontext[]        = "nsh: %s: not valid in this context\n";
#ifdef CONFIG_NSH_STRERROR
const char g_fmtcmdfailed[]      = "nsh: %s: %s failed: %s\n";
#else
const char g_fmtcmdfailed[]      = "nsh: %s: %s failed: %d\n";
#endif
const char g_fmtcmdoutofmemory[] = "nsh: %s: out of memory\n";
const char g_fmtinternalerror[]  = "nsh: %s: Internal error\n";
const char g_fmtsignalrecvd[]    = "nsh: %s: Interrupted by signal\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_memlist_add
 ****************************************************************************/

#ifdef HAVE_MEMLIST
static void nsh_memlist_add(FAR struct nsh_memlist_s *memlist,
                            FAR char *allocation)
{
  if (memlist && allocation)
    {
      int index = memlist->nallocs;
      if (index < CONFIG_NSH_MAXALLOCS)
        {
          memlist->allocations[index] = allocation;
          memlist->nallocs = index + 1;
        }
    }
}
#endif

/****************************************************************************
 * Name: nsh_memlist_free
 ****************************************************************************/

#ifdef HAVE_MEMLIST
static void nsh_memlist_free(FAR struct nsh_memlist_s *memlist)
{
  if (memlist)
    {
      int index;

      for (index = 0; index < memlist->nallocs; index++)
        {
          free(memlist->allocations[index]);
          memlist->allocations[index] = NULL;
        }

      memlist->nallocs = 0;
    }
}
#endif

/****************************************************************************
 * Name: nsh_releaseargs
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLEBG
static void nsh_releaseargs(struct cmdarg_s *arg)
{
  FAR struct nsh_vtbl_s *vtbl = arg->vtbl;
  int i;

#ifdef CONFIG_FILE_STREAM
  /* If the output was redirected, then file descriptor should
   * be closed.  The created task has its one, independent copy of
   * the file descriptor
   */

  if (vtbl->np.np_redirect)
    {
      close(arg->fd);
    }
#endif

  /* Released the cloned vtbl instance */

  nsh_release(vtbl);

  /* Release the cloned args */

  for (i = 0; i < arg->argc; i++)
    {
      free(arg->argv[i]);
    }

  free(arg);
}
#endif

/****************************************************************************
 * Name: nsh_child
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLEBG
static pthread_addr_t nsh_child(pthread_addr_t arg)
{
  struct cmdarg_s *carg = (struct cmdarg_s *)arg;
  int ret;

  _info("BG %s\n", carg->argv[0]);

  /* Execute the specified command on the child thread */

  ret = nsh_command(carg->vtbl, carg->argc, carg->argv);

  /* Released the cloned arguments */

  _info("BG %s complete\n", carg->argv[0]);
  nsh_releaseargs(carg);
  return (pthread_addr_t)((uintptr_t)ret);
}
#endif

/****************************************************************************
 * Name: nsh_cloneargs
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLEBG
static struct cmdarg_s *nsh_cloneargs(FAR struct nsh_vtbl_s *vtbl,
                                      int fd, int argc, char *argv[])
{
  struct cmdarg_s *ret = (struct cmdarg_s *)zalloc(sizeof(struct cmdarg_s));
  int i;

  if (ret)
    {
      ret->vtbl = vtbl;
      ret->fd   = fd;
      ret->argc = argc;

      for (i = 0; i < argc; i++)
        {
          ret->argv[i] = strdup(argv[i]);
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_saveresult
 ****************************************************************************/

static int nsh_saveresult(FAR struct nsh_vtbl_s *vtbl, bool result)
{
  struct nsh_parser_s *np = &vtbl->np;

#ifndef CONFIG_NSH_DISABLESCRIPT
#ifndef CONFIG_NSH_DISABLE_LOOPS
  /* Check if we are waiting for the condition associated with a while
   * token.
   *
   *   while <test-cmd>; do <cmd-sequence>; done
   *
   *  Execute <cmd-sequence> as long as <test-cmd> has an exit status of
   *  zero.
   */

  if (np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_WHILE)
    {
      np->np_fail = false;
      np->np_lpstate[np->np_lpndx].lp_enable = (result == OK);
      return OK;
    }

  /* Check if we are waiting for the condition associated with an until
   * token.
   *
   *   until <test-cmd>; do <cmd-sequence>; done
   *
   *  Execute <cmd-sequence> as long as <test-cmd> has a non-zero exit
   * status.
   */

  else if (np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_UNTIL)
    {
      np->np_fail = false;
      np->np_lpstate[np->np_lpndx].lp_enable = (result != OK);
      return OK;
    }
  else
#endif

#ifndef CONFIG_NSH_DISABLE_ITEF
  /* Check if we are waiting for the condition associated with an if token */

  if (np->np_iestate[np->np_iendx].ie_state == NSH_ITEF_IF)
    {
      np->np_fail = false;
      np->np_iestate[np->np_iendx].ie_ifcond =
        np->np_iestate[np->np_iendx].ie_inverted ^ result;
      return OK;
    }
  else
#endif
#endif
    {
      np->np_fail = result;
      return result ? ERROR : OK;
    }
}

/****************************************************************************
 * Name: nsh_execute
 ****************************************************************************/

static int nsh_execute(FAR struct nsh_vtbl_s *vtbl,
                       int argc, FAR char *argv[],
                       FAR const char *redirfile, int oflags)
{
#if defined(CONFIG_FILE_STREAM) || !defined(CONFIG_NSH_DISABLEBG)
  int fd = -1;
#endif
  int ret;

  /* DO NOT CHANGE THE ORDERING OF THE FOLLOWING STEPS
   *
   * They must work as follows:
   *
   * 1. Load a file from file system if possible.  An external command on a
   *    file system with the provided name (and on the defined PATH) takes
   *    precendence over any other source of a command by that name.  This
   *    allows the user to replace a built-in command with a command on a`
   *    file system
   *
   * 2. If not, run a built-in application of that name if possible.  A
   *    built-in application will take precendence over any NSH command.
   *
   * 3. If not, run an NSH command line command if possible.
   *
   * 4. If not, report that the command was not found.
   */

  /* Does this command correspond to a built-in command?
   * nsh_builtin() returns:
   *
   *   -1 (ERROR)  if the application task corresponding to 'argv[0]' could
   *               not be started (possibly because it does not exist).
   *    0 (OK)     if the application task corresponding to 'argv[0]' was
   *               and successfully started.  If CONFIG_SCHED_WAITPID is
   *               defined, this return value also indicates that the
   *               application returned successful status (EXIT_SUCCESS)
   *    1          If CONFIG_SCHED_WAITPID is defined, then this return value
   *               indicates that the application task was spawned
   *               successfully but returned failure exit status.
   *
   * Note the priority is not effected by nice-ness.
   */

#ifdef CONFIG_NSH_BUILTIN_APPS
#ifdef CONFIG_FILE_STREAM
  ret = nsh_builtin(vtbl, argv[0], argv, redirfile, oflags);
#else
  ret = nsh_builtin(vtbl, argv[0], argv, NULL, 0);
#endif
  if (ret >= 0)
    {
      /* nsh_builtin() returned 0 or 1.  This means that the built-in
       * command was successfully started (although it may not have ran
       * successfully).  So certainly it is not an NSH command.
       */

      /* Save the result:  success if 0; failure if 1 */

      return nsh_saveresult(vtbl, ret != OK);
    }

  /* No, not a built in command (or, at least, we were unable to start a
   * built-in command of that name). Maybe it is a built-in application
   * or an NSH command.
   */

#endif

  /* Does this command correspond to an application filename?
   * nsh_fileapp() returns:
   *
   *   -1 (ERROR)  if the application task corresponding to 'argv[0]' could
   *               not be started (possibly because it doesn not exist).
   *    0 (OK)     if the application task corresponding to 'argv[0]' was
   *               and successfully started.  If CONFIG_SCHED_WAITPID is
   *               defined, this return value also indicates that the
   *               application returned successful status (EXIT_SUCCESS)
   *    1          If CONFIG_SCHED_WAITPID is defined, then this return value
   *               indicates that the application task was spawned
   *               successfully but returned failure exit status.
   *
   * Note the priority is not effected by nice-ness.
   */

#ifdef CONFIG_NSH_FILE_APPS
  ret = nsh_fileapp(vtbl, argv[0], argv, redirfile, oflags);
  if (ret >= 0)
    {
      /* nsh_fileapp() returned 0 or 1.  This means that the built-in
       * command was successfully started (although it may not have ran
       * successfully).  So certainly it is not an NSH command.
       */

      /* Save the result:  success if 0; failure if 1 */

      return nsh_saveresult(vtbl, ret != OK);
    }

  /* No, not a file name command (or, at least, we were unable to start a
   * program of that name). Treat it like an NSH command.
   */

#endif

#ifdef CONFIG_FILE_STREAM
  /* Redirected output? */

  if (vtbl->np.np_redirect)
    {
      /* Open the redirection file.  This file will eventually
       * be closed by a call to either nsh_release (if the command
       * is executed in the background) or by nsh_undirect if the
       * command is executed in the foreground.
       */

      fd = open(redirfile, oflags, 0666);
      if (fd < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
          goto errout;
        }
    }
#endif

  /* Handle the case where the command is executed in background.
   * However is app is to be started as built-in new process will
   * be created anyway, so skip this step.
   */

#ifndef CONFIG_NSH_DISABLEBG
  if (vtbl->np.np_bg)
    {
      struct sched_param param;
      struct nsh_vtbl_s *bkgvtbl;
      struct cmdarg_s *args;
      pthread_attr_t attr;
      pthread_t thread;

      /* Get a cloned copy of the vtbl with reference count=1.
       * after the command has been processed, the nsh_release() call
       * at the end of nsh_child() will destroy the clone.
       */

      bkgvtbl = nsh_clone(vtbl);
      if (!bkgvtbl)
        {
          goto errout_with_redirect;
        }

      /* Create a container for the command arguments */

      args = nsh_cloneargs(bkgvtbl, fd, argc, argv);
      if (!args)
        {
          nsh_release(bkgvtbl);
          goto errout_with_redirect;
        }

#ifdef CONFIG_FILE_STREAM
      /* Handle redirection of output via a file descriptor */

      if (vtbl->np.np_redirect)
        {
          nsh_redirect(bkgvtbl, fd, NULL);
        }
#endif

      /* Get the execution priority of this task */

      ret = sched_getparam(0, &param);
      if (ret != 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "sched_getparm",
                    NSH_ERRNO);

          /* NOTE: bkgvtbl is released in nsh_relaseargs() */

          nsh_releaseargs(args);
          goto errout;
        }

      /* Determine the priority to execute the command */

      if (vtbl->np.np_nice != 0)
        {
          int priority = param.sched_priority - vtbl->np.np_nice;
          if (vtbl->np.np_nice < 0)
            {
              int max_priority = sched_get_priority_max(SCHED_NSH);
              if (priority > max_priority)
                {
                  priority = max_priority;
                }
            }
          else
            {
              int min_priority = sched_get_priority_min(SCHED_NSH);
              if (priority < min_priority)
                {
                  priority = min_priority;
                }
            }

          param.sched_priority = priority;
        }

      /* Set up the thread attributes */

      pthread_attr_init(&attr);
      pthread_attr_setschedpolicy(&attr, SCHED_NSH);
      pthread_attr_setschedparam(&attr, &param);

      /* Execute the command as a separate thread at the appropriate
       * priority.
       */

      ret = pthread_create(&thread, &attr, nsh_child, (pthread_addr_t)args);
      if (ret != 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "pthread_create",
                    NSH_ERRNO_OF(ret));

          /* NOTE: bkgvtbl is released in nsh_relaseargs() */

          nsh_releaseargs(args);
          goto errout;
        }

      /* Detach from the pthread since we are not going to join with it.
       * Otherwise, we would have a memory leak.
       */

      pthread_detach(thread);

      nsh_output(vtbl, "%s [%d:%d]\n", argv[0], thread,
                 param.sched_priority);
    }
  else
#endif
    {
#ifdef CONFIG_FILE_STREAM
      uint8_t save[SAVE_SIZE];

      /* Handle redirection of output via a file descriptor */

      if (vtbl->np.np_redirect)
        {
          nsh_redirect(vtbl, fd, save);
        }
#endif

      /* Then execute the command in "foreground" -- i.e., while the user
       * waits for the next prompt.  nsh_command will return:
       *
       * -1 (ERROR) if the command was unsuccessful
       *  0 (OK)     if the command was successful
       */

      ret = nsh_command(vtbl, argc, argv);

#ifdef CONFIG_FILE_STREAM
      /* Restore the original output.  Undirect will close the redirection
       * file descriptor.
       */

      if (vtbl->np.np_redirect)
        {
          nsh_undirect(vtbl, save);
        }
#endif

      /* Mark errors so that it is possible to test for non-zero return
       * values in nsh scripts.
       */

      if (ret < 0)
        {
          goto errout;
        }
    }

  /* Return success if the command succeeded (or at least, starting of the
   * command task succeeded).
   */

  return nsh_saveresult(vtbl, false);

#ifndef CONFIG_NSH_DISABLEBG
errout_with_redirect:
#ifdef CONFIG_FILE_STREAM
  if (vtbl->np.np_redirect)
    {
      close(fd);
    }
#endif
#endif

errout:
  return nsh_saveresult(vtbl, true);
}

/****************************************************************************
 * Name: nsh_filecat
 ****************************************************************************/

#ifdef CONFIG_NSH_CMDPARMS
static FAR char *nsh_filecat(FAR struct nsh_vtbl_s *vtbl, FAR char *s1,
                             FAR const char *filename)
{
  struct stat buf;
  size_t s1size = 0;
  size_t allocsize;
  ssize_t nbytesread;
  FAR char *argument;
  int index;
  int fd;
  int ret;

  /* Get the size of the string */

  if (s1)
    {
      s1size = (size_t)strlen(s1);
    }

  /* Get the size of file */

  ret = stat(filename, &buf);
  if (ret != 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "``", "stat", NSH_ERRNO);
      return NULL;
    }

  /* Get the total allocation size */

  allocsize = s1size + (size_t)buf.st_size + 1;
  argument = (FAR char *)realloc(s1, allocsize);
  if (!argument)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, "``");
      return NULL;
    }

  /* Open the source file for reading */

  fd = open(filename, O_RDONLY);
  if (fd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "``", "open", NSH_ERRNO);
      goto errout_with_alloc;
    }

  /* Now copy the file.  Loop until the entire file has been transferred to
   * the allocated string (after the original contents of s1size bytes.
   */

  for (index = s1size; index < allocsize - 1; )
    {
      /* Loop until we successfully read something , we encounter the
       * end-of-file, or until a read error occurs
       */

      do
        {
          nbytesread = read(fd, &argument[index], IOBUFFERSIZE);
          if (nbytesread == 0)
            {
              /* Unexpected end of file -- Break out of the loop */

              break;
            }
          else if (nbytesread < 0)
            {
              /* EINTR is not an error (but will still stop the copy) */

              if (errno == EINTR)
                {
                  nsh_error(vtbl, g_fmtsignalrecvd, "``");
                }
              else
                {
                  /* Read error */

                  nsh_error(vtbl, g_fmtcmdfailed, "``", "read", NSH_ERRNO);
                }

              goto errout_with_fd;
            }
        }
      while (nbytesread <= 0);

      /* Update the index based upon the number of bytes read */

      index += nbytesread;
    }

  /* Remove trailing whitespace */

  for (;
       index > s1size &&
       strchr(g_token_separator, argument[index - 1]) != NULL;
       index--);

  /* Make sure that the new string is null terminated */

  argument[index] = '\0';

  /* Close the temporary file and return the concatenated value */

  close (fd);
  return argument;

errout_with_fd:
  close(fd);

errout_with_alloc:
  free(argument);
  return NULL;
}
#endif

/****************************************************************************
 * Name: nsh_cmdparm
 ****************************************************************************/

#ifdef CONFIG_NSH_CMDPARMS
static FAR char *nsh_cmdparm(FAR struct nsh_vtbl_s *vtbl, FAR char *cmdline,
                             FAR char **allocation)
{
  FAR char *tmpfile;
  FAR char *argument;
  int ret;

  /* We cannot process the command argument if there is no allocation
   * pointer.
   */

  if (!allocation)
    {
      return (FAR char *)g_nullstring;
    }

  /* Create a unique file name using the task ID */

  tmpfile = NULL;
  ret = asprintf(&tmpfile, "%s/TMP%d.dat", CONFIG_LIBC_TMPDIR, getpid());
  if (ret < 0 || !tmpfile)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, "``");
      return (FAR char *)g_nullstring;
    }

  /* Execute the command that will re-direct the output of the command to
   * the temporary file.  This is a simple command that can't handle most
   * options.
   */

  ret = nsh_parse_cmdparm(vtbl, cmdline, tmpfile);
  if (ret != OK)
    {
      /* Report the failure */

      nsh_error(vtbl, g_fmtcmdfailed, "``", "exec", NSH_ERRNO);
      free(tmpfile);
      return (FAR char *)g_nullstring;
    }

  /* Concatenate the file contents with the current allocation */

  argument = nsh_filecat(vtbl, *allocation, tmpfile);
  if (argument == NULL)
    {
      argument = (FAR char *)g_nullstring;
    }
  else
    {
      *allocation = argument;
    }

  /* We can now unlink the tmpfile and free the tmpfile string */

  ret = unlink(tmpfile);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "``", "unlink", NSH_ERRNO);
    }

  free(tmpfile);
  return argument;
}
#endif

/****************************************************************************
 * Name: nsh_strcat
 ****************************************************************************/

#ifdef CONFIG_NSH_ARGCAT
static FAR char *nsh_strcat(FAR struct nsh_vtbl_s *vtbl, FAR char *s1,
                            FAR const char *s2)
{
  FAR char *argument;
  int s1size = 0;
  int allocsize;

  /* Get the size of the first string... it might be NULL */

  if (s1)
    {
      s1size = strlen(s1);
    }

  /* Then reallocate the first string so that it is large enough to hold
   * both (including the NUL terminator).
   */

  allocsize = s1size + strlen(s2) + 1;
  argument  = (FAR char *)realloc(s1, allocsize);
  if (!argument)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, "$");
      argument = s1;
    }
  else
    {
      argument[s1size] = '\0';  /* (In case s1 was NULL) */
      strcat(argument, s2);
    }

  return argument;
}
#endif

/****************************************************************************
 * Name: nsh_strchr
 ****************************************************************************/

#if defined(CONFIG_NSH_QUOTE) && defined(CONFIG_NSH_ARGCAT)
static FAR char *nsh_strchr(FAR const char *str, int ch)
{
  FAR const char *ptr;
  bool quoted = false;

  for (ptr = str; ; ptr++)
    {
      if (*ptr == '\\' && !quoted)
        {
          quoted = true;
        }
      else if ((int)*ptr == ch && !quoted)
        {
          return (FAR char *)ptr;
        }
      else if (*ptr == '\0')
        {
          return NULL;
        }
      else
        {
          quoted = false;
        }
    }
}
#endif

/****************************************************************************
 * Name: nsh_envexpand
 ****************************************************************************/

#ifdef NSH_HAVE_VARS
static FAR char *nsh_envexpand(FAR struct nsh_vtbl_s *vtbl,
                               FAR char *varname)
{
  /* Check for built-in variables */

  if (strcmp(varname, g_exitstatus) == 0)
    {
      if (vtbl->np.np_fail)
        {
          return (FAR char *)g_failure;
        }
      else
        {
          return (FAR char *)g_success;
        }
    }
  else
    {
      FAR char *value;

#ifdef CONFIG_NSH_VARS
      /* Not a built-in? Return the value of the NSH variable with this
       * name.
       */

      value = nsh_getvar(vtbl, varname);
      if (value != NULL)
        {
          return value;
        }
#endif

#ifndef CONFIG_DISABLE_ENVIRON
      /* Not an NSH variable? Return the value of the NSH variable
       * environment variable with this name.
       */

      value = getenv(varname);
      if (value != NULL)
        {
          return value;
        }
#endif

      return (FAR char *)g_nullstring;
    }
}
#endif

/****************************************************************************
 * Name: nsh_dequote
 ****************************************************************************/

#if defined(CONFIG_NSH_QUOTE) && defined(CONFIG_NSH_ARGCAT)
static void nsh_dequote(FAR char *cmdline)
{
  FAR char *ptr;
  bool quoted;

  quoted = false;

  for (ptr = cmdline; *ptr != '\0'; )
    {
      if (*ptr == '\\' && !quoted)
        {
          FAR char *dest = ptr;
          FAR const char *src = ptr + 1;
          char ch;

          /* Move the data to eliminate the quote from the command line */

          do
            {
              ch      = *src++;
              *dest++ = ch;
            }
          while (ch != '\0');

          /* Remember that the next character is quote (in case it is
           * another back-slash character).
           */

          quoted = true;
          continue;
        }
      else
        {
          /* The next character is not quoted because either (1) it was not
           * preceded by a back-slash, or (2) it was preceded by a quoted
           * back-slash.
           */

          quoted = false;
          ptr++;
        }
    }

  /* Make sure that the new, possibly shorted string is NUL terminated */

  *ptr = '\0';
}
#endif

/****************************************************************************
 * Name: nsh_argexpand
 ****************************************************************************/

#if defined(CONFIG_NSH_ARGCAT) && defined(HAVE_MEMLIST)
static FAR char *nsh_argexpand(FAR struct nsh_vtbl_s *vtbl,
                               FAR char *cmdline, FAR char **allocation,
                               FAR int *isenvvar)
{
  FAR char *working = cmdline;
#ifdef CONFIG_NSH_QUOTE
  FAR char *nextwork;
#endif
  FAR char *argument = NULL;
  FAR char *ptr;
  size_t len;

  /* Loop until all of the commands on the command line have been processed */

  for (; ; )
    {
      /* Look for interesting things within the command string. */

      len      = strcspn(working, g_arg_separator);
      ptr      = working + len;
#ifdef CONFIG_NSH_QUOTE
      nextwork = ptr + 1;

      /* But ignore these interesting things if they are quoted */

      while (len > 0 && *ptr != '\0')
        {
          FAR char *prev = working + len - 1;
          int bcount;
          bool quoted;

          /* Check if the current character is quoted */

          for (bcount = 0, quoted = false;
               bcount < len && *prev == '\\';
               bcount++, prev--)
            {
              quoted ^= true;
            }

          if (quoted)
            {
              /* Yes.. skip over it */

              len     += strcspn(ptr + 1, g_arg_separator) + 1;
              ptr      = working + len;
              nextwork = ptr + 1;
            }
          else
            {
              /* Not quoted.. subject to normal processing */

              break;
            }
        }
#endif

      /* If ptr points to the NUL terminator, then there is nothing else
       * interesting in the argument.
       */

      if (*ptr == '\0')
        {
          /* Was anything previously concatenated? */

          if (argument)
            {
              /* Yes, then we probably need to add the last part of the
               * argument beginning at the last working pointer to the
               * concatenated argument.
               *
               * On failures to allocation memory, nsh_strcat will just
               * return old value of argument
               */

              argument    = nsh_strcat(vtbl, argument, working);
              *allocation = argument;

              /* De-quote the returned string */

              nsh_dequote(argument);
              return argument;
            }
          else
            {
              /* No.. just return the original string from the command
               * line.
               */

              nsh_dequote(cmdline);
              return cmdline;
            }
        }
      else

#ifdef CONFIG_NSH_CMDPARMS
      /* Check for a back-quoted command embedded within the argument
       * string.
       */

      if (*ptr == '`')
        {
          FAR char *tmpalloc = NULL;
          FAR char *result;
          FAR char *rptr;

          /* Replace the back-quote with a NUL terminator and add the
           * intervening character to the concatenated string.
           */

          *ptr++      = '\0';
          argument    = nsh_strcat(vtbl, argument, working);
          *allocation = argument;

          /* Find the closing back-quote (must be unquoted) */

          rptr = nsh_strchr(ptr, '`');
          if (!rptr)
            {
              nsh_error(vtbl, g_fmtnomatching, "`", "`");
              return (FAR char *)g_nullstring;
            }

          /* Replace the final back-quote with a NUL terminator */

          *rptr = '\0';

          /* Then execute the command to get the sub-string value.  On
           * error, nsh_cmdparm may return g_nullstring but never NULL.
           */

          result = nsh_cmdparm(vtbl, ptr, &tmpalloc);

          /* Concatenate the result of the operation with the accumulated
           * string.  On failures to allocation memory, nsh_strcat will
           * just return old value of argument
           */

          argument    = nsh_strcat(vtbl, argument, result);
          *allocation = argument;
          working     = rptr + 1;

          /* And free any temporary allocations */

          if (tmpalloc)
            {
              free(tmpalloc);
            }
        }
      else
#endif

#ifdef NSH_HAVE_VARS
      /* Check if we encountered a reference to an environment variable */

      if (*ptr == '$')
        {
          FAR const char *envstr;
          FAR char *rptr;

          /* Replace the dollar sign with a NUL terminator and add the
           * intervening character to the concatenated string.
           */

          *ptr++      = '\0';
          argument    = nsh_strcat(vtbl, argument, working);
          *allocation = argument;

          /* Find the end of the environment variable reference.  If the
           * dollar sign ('$') is followed by a left bracket ('{') then the
           * variable name is terminated with the right bracket character
           * ('}').  Otherwise, the variable name goes to the end of the
           * argument.
           */

          if (*ptr == '{')
            {
              /* Skip over the left bracket */

              ptr++;

              /* Find the closing right bracket */

              rptr = nsh_strchr(ptr, '}');
              if (!rptr)
                {
                  nsh_error(vtbl, g_fmtnomatching, "${", "}");
                  return (FAR char *)g_nullstring;
                }

              /* Replace the right bracket with a NUL terminator and set the
               * working pointer to the character after the bracket.
               */

              *rptr   = '\0';
              working = rptr + 1;
            }
          else
            {
              /* Set working to the NUL terminator at the end of the string.
               *
               * REVISIT:  Needs logic to get the size of the variable name
               * based on parsing the name string which must be of the form
               * [a-zA-Z_]+[a-zA-Z0-9_]*
               */

              working = ptr + strlen(ptr);
            }

          /* Then get the value of the environment variable.  On errors,
           * nsh_envexpand will return the NULL string.
           */

          if (isenvvar != NULL)
            {
              *isenvvar = 1;
            }

          envstr = nsh_envexpand(vtbl, ptr);

#ifndef CONFIG_NSH_DISABLESCRIPT
          if ((vtbl->np.np_flags & NSH_PFLAG_SILENT) == 0)
#endif
            {
              nsh_output(vtbl, "  %s=%s\n", ptr, envstr ? envstr : "(null)");
            }

          /* Concatenate the result of the operation with the accumulated
           * string.  On failures to allocation memory, nsh_strcat will
           * just return old value of argument
           */

          argument    = nsh_strcat(vtbl, argument, envstr);
          *allocation = argument;
        }
      else
#endif
        {
          /* Not a special character... skip to the next character in the
           * cmdline.
           */

#ifdef CONFIG_NSH_QUOTE
          working = nextwork;
#else
          working++;
#endif
        }
    }
}

#else
static FAR char *nsh_argexpand(FAR struct nsh_vtbl_s *vtbl,
                               FAR char *cmdline, FAR char **allocation,
                               FAR int *isenvvar)
{
  FAR char *argument = (FAR char *)g_nullstring;
#ifdef CONFIG_NSH_QUOTE
  char ch = *cmdline;

  /* A single backslash at the beginning of the line is support, nothing
   * more.
   */

  nsh_dequote(cmdline);
  if (ch == '\\')
    {
      argument = cmdline;
    }
  else
#endif

#ifdef CONFIG_NSH_CMDPARMS
  /* Are we being asked to use the output from another command or program
   * as an input parameters for this command?
   */

  if (*cmdline == '`')
    {
      /* Verify that the final character is also a back-quote */

      FAR char *rptr = nsh_strchr(cmdline + 1, '`');
      if (!rptr || rptr[1] != '\0')
        {
          nsh_error(vtbl, g_fmtnomatching, "`", "`");
          return (FAR char *)g_nullstring;
        }

      /* Replace the final back-quote with a NUL terminator */

      *rptr = '\0';

      /* Then execute the command to get the parameter value */

      argument = nsh_cmdparm(vtbl, cmdline + 1, allocation);
    }
  else
#endif

#ifdef NSH_HAVE_VARS
  /* Check for references to environment variables */

  if (*cmdline == '$')
    {
      if (isenvvar != NULL)
        {
          *isenvvar = 1;
        }

      argument = nsh_envexpand(vtbl, cmdline + 1);
    }
  else
#endif
    {
      /* The argument to be returned is simply the beginning of the
       * delimited string.
       */

      argument = cmdline;
    }

  return argument;
}
#endif

/****************************************************************************
 * Name: nsh_argument
 ****************************************************************************/

static FAR char *nsh_argument(FAR struct nsh_vtbl_s *vtbl,
                              FAR char **saveptr,
                              FAR NSH_MEMLIST_TYPE *memlist,
                              FAR int *isenvvar)
{
  FAR char *pbegin     = *saveptr;
  FAR char *pend       = NULL;
  FAR char *allocation = NULL;
  FAR char *argument   = NULL;
  FAR const char *term;
#ifdef CONFIG_NSH_QUOTE
  FAR char *prev;
  bool quoted;
#endif
#ifdef CONFIG_NSH_CMDPARMS
  bool backquote;
#endif

  /* Find the beginning of the next token */

  for (;
       *pbegin && strchr(g_token_separator, *pbegin) != NULL;
       pbegin++);

  /* If we are at the end of the string with nothing but delimiters found,
   * then return NULL, meaning that there are no further arguments on the
   * line.
   */

  if (!*pbegin)
    {
      return NULL;
    }

  /* Does the token begin with '>' -- redirection of output? */

  if (*pbegin == '>')
    {
      /* Yes.. does it begin with ">>"? */

      if (*(pbegin + 1) == '>')
        {
          *saveptr = pbegin + 2;
          argument = (FAR char *)g_redirect2;
        }
      else
        {
          *saveptr = pbegin + 1;
          argument = (FAR char *)g_redirect1;
        }
    }

  /* Does the token begin with '#' -- comment */

  else if (*pbegin == '#')
    {
      /* Return NULL meaning that we are at the end of the line */

      *saveptr = pbegin;
      argument = NULL;
    }

  /* Otherwise, it is a normal argument and we have to parse using the normal
   * rules to find the end of the argument.
   */

  else
    {
      /* However, the rules are a little different if the next argument is
       * a quoted string.
       */

      if (*pbegin == '"')
        {
          /* A quoted string can only be terminated with another quotation
           * mark.  Set pbegin to point at the character after the opening
           * quote mark.
           */

          pbegin++;
          term = "\"";

          /* If this is an environment variable in double quotes, we don't
           * want it split into multiple arguments. So just invalidate the
           * flag pointer which would otherwise communicate such back up
           * the call tree.
           */

          isenvvar = NULL;
        }
      else
        {
          /* No, then any of the usual separators will terminate the
           * argument.  In this case, pbegin points for the first character
           * of the token following the previous separator.
           */

          term = g_token_separator;
        }

      /* Find the end of the string */

#ifdef CONFIG_NSH_CMDPARMS
      /* Some special care must be exercised to make sure that we do not
       * break up any back-quote delimited substrings.  NOTE that the
       * absence of a closing back-quote is not detected;  That case should
       * be detected later.
       */

#ifdef CONFIG_NSH_QUOTE
      quoted    = false;
      backquote = false;

      for (prev = NULL, pend = pbegin; *pend != '\0'; prev = pend, pend++)
        {
          /* Check if the current character is quoted */

          if (prev != NULL && *prev == '\\' && !quoted)
            {
              /* Do no special checks on the quoted character */

              quoted = true;
              continue;
            }

          quoted = false;

          /* Check if the current character is an (unquoted) back-quote */

          if (*pend == '\\' && !quoted)
            {
              /* Yes.. Do no special processing on the backspace character */

              continue;
            }

          /* Toggle the back-quote flag when one is encountered? */

          if (*pend == '`')
            {
              backquote = !backquote;
            }

          /* Check for a delimiting character only if we are not in a
           * back-quoted sub-string.
           */

          else if (!backquote && nsh_strchr(term, *pend) != NULL)
            {
              /* We found a delimiter outside of any back-quoted substring.
               * Now we can break out of the loop.
               */

              break;
            }
        }
#else
      backquote = false;

      for (pend = pbegin; *pend != '\0'; pend++)
        {
          /* Toggle the back-quote flag when one is encountered? */

          if (*pend == '`')
            {
              backquote = !backquote;
            }

          /* Check for a delimiting character only if we are not in a
           * back-quoted sub-string.
           */

          else if (!backquote && nsh_strchr(term, *pend) != NULL)
            {
              /* We found a delimiter outside of any back-quoted substring.
               * Now we can break out of the loop.
               */

              break;
            }
        }

#endif /* CONFIG_NSH_QUOTE */
#else  /* CONFIG_NSH_CMDPARMS */

      /* Search the next occurrence of a terminating character (or the end
       * of the line).
       */

#ifdef CONFIG_NSH_QUOTE
      quoted = false;

      for (prev = NULL, pend = pbegin; *pend != '\0'; prev = pend, pend++)
        {
          /* Check if the current character is quoted */

          if (prev != NULL && *prev == '\\' && !quoted)
            {
              /* Do no special checks on the quoted character */

              quoted = true;
              continue;
            }

          quoted = false;

          /* Check if the current character is an (unquoted) back-quote */

          if (*pend == '\\' && !quoted)
            {
              /* Yes.. Do no special processing on the backspace character */

              continue;
            }

          /* Check for a delimiting character */

          if (nsh_strchr(term, *pend) != NULL)
            {
              /* We found a delimiter. Now we can break out of the loop. */

              break;
            }
        }

#else

      for (pend = pbegin;
          *pend != '\0' && nsh_strchr(term, *pend) == NULL;
           pend++)
        {
        }

#endif /* CONFIG_NSH_QUOTE */
#endif /* CONFIG_NSH_CMDPARMS */

      /* pend either points to the end of the string or to the first
       * delimiter after the string.
       */

      if (*pend)
        {
          /* Turn the delimiter into a NUL terminator */

          *pend++ = '\0';
        }

      /* Save the pointer where we left off */

      *saveptr = pend;

      /* Perform expansions as necessary for the argument */

      argument = nsh_argexpand(vtbl, pbegin, &allocation, isenvvar);
    }

  /* If any memory was allocated for this argument, make sure that it is
   * added to the list of memory to be freed at the end of command
   * processing.
   */

  NSH_MEMLIST_ADD(memlist, allocation);

  /* Return the parsed argument. */

  return argument;
}

/****************************************************************************
 * Name: nsh_loop_enabled
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
static bool nsh_loop_enabled(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct nsh_parser_s *np = &vtbl->np;

  /* If we are looping and the disable bit is set, then we are skipping
   * all data until we next get to the 'done' token at the end of the
   * loop.
   */

  if (np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_DO)
    {
      /* We have parsed 'do', looking for 'done' */

      return (bool)np->np_lpstate[np->np_lpndx].lp_enable;
    }

  return true;
}
#else
#  define nsh_loop_enabled(vtbl) true
#endif

/****************************************************************************
 * Name: nsh_itef_enabled
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_ITEF)
static bool nsh_itef_enabled(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct nsh_parser_s *np = &vtbl->np;
  bool ret = !np->np_iestate[np->np_iendx].ie_disabled;
  if (ret)
    {
      switch (np->np_iestate[np->np_iendx].ie_state)
        {
          case NSH_ITEF_NORMAL:
          case NSH_ITEF_IF:
          default:
            break;

          case NSH_ITEF_THEN:
            ret = !np->np_iestate[np->np_iendx].ie_ifcond;
            break;

          case NSH_ITEF_ELSE:
            ret = np->np_iestate[np->np_iendx].ie_ifcond;
            break;
        }
    }

  return ret;
}
#else
#  define nsh_itef_enabled(vtbl) true
#endif

/****************************************************************************
 * Name: nsh_cmdenabled
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLESCRIPT
static bool nsh_cmdenabled(FAR struct nsh_vtbl_s *vtbl)
{
  /* Return true if command processing is enabled on this pass through the
   * loop AND if command processing is enabled in this part of the if-then-
   * else-fi sequence.
   */

  return (nsh_loop_enabled(vtbl) && nsh_itef_enabled(vtbl));
}
#endif

/****************************************************************************
 * Name: nsh_loop
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
static int nsh_loop(FAR struct nsh_vtbl_s *vtbl, FAR char **ppcmd,
                    FAR char **saveptr, FAR NSH_MEMLIST_TYPE *memlist)
{
  FAR struct nsh_parser_s *np = &vtbl->np;
  FAR char *cmd = *ppcmd;
  long offset;
  bool whilematch;
  bool untilmatch;
  bool enable;
  int ret;

  if (cmd != NULL)
    {
      /* Check if the command is preceded by "while" or "until" */

      whilematch = strcmp(cmd, "while") == 0;
      untilmatch = strcmp(cmd, "until") == 0;

      if (whilematch || untilmatch)
        {
          uint8_t state;

          /* Get the cmd following the "while" or "until" */

          *ppcmd = nsh_argument(vtbl, saveptr, memlist, 0);
          if (*ppcmd == NULL || **ppcmd == '\0')
            {
              nsh_error(vtbl, g_fmtarginvalid, cmd);
              goto errout;
            }

          /* Verify that "while" or "until" is valid in this context */

          if (
#ifndef CONFIG_NSH_DISABLE_ITEF
              np->np_iestate[np->np_iendx].ie_state == NSH_ITEF_IF ||
#endif
              np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_WHILE ||
              np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_UNTIL ||
              np->np_stream == NULL || np->np_foffs < 0)
            {
              nsh_error(vtbl, g_fmtcontext, cmd);
              goto errout;
            }

          /* Check if we have exceeded the maximum depth of nesting */

          if (np->np_lpndx >= CONFIG_NSH_NESTDEPTH - 1)
            {
              nsh_error(vtbl, g_fmtdeepnesting, cmd);
              goto errout;
            }

          /* "Push" the old state and set the new state */

          state  = whilematch ? NSH_LOOP_WHILE : NSH_LOOP_UNTIL;
          enable = nsh_cmdenabled(vtbl);
#ifdef NSH_DISABLE_SEMICOLON
          offset = np->np_foffs;
#else
          offset = np->np_foffs + np->np_loffs;
#endif

#ifndef NSH_DISABLE_SEMICOLON
          np->np_jump                             = false;
#endif
          np->np_lpndx++;
          np->np_lpstate[np->np_lpndx].lp_state   = state;
          np->np_lpstate[np->np_lpndx].lp_enable  = enable;
#ifndef CONFIG_NSH_DISABLE_ITEF
          np->np_lpstate[np->np_lpndx].lp_iendx   = np->np_iendx;
#endif
          np->np_lpstate[np->np_lpndx].lp_topoffs = offset;
        }

      /* Check if the token is "do" */

      else if (strcmp(cmd, "do") == 0)
        {
          /* Get the cmd following the "do" -- there may or may not be one */

          *ppcmd = nsh_argument(vtbl, saveptr, memlist, NULL);

          /* Verify that "do" is valid in this context */

          if (np->np_lpstate[np->np_lpndx].lp_state != NSH_LOOP_WHILE &&
              np->np_lpstate[np->np_lpndx].lp_state != NSH_LOOP_UNTIL)
            {
              nsh_error(vtbl, g_fmtcontext, "do");
              goto errout;
            }

          np->np_lpstate[np->np_lpndx].lp_state = NSH_LOOP_DO;
        }

      /* Check if the token is "done" */

      else if (strcmp(cmd, "done") == 0)
        {
          /* Get the cmd following the "done" -- there should be one */

          *ppcmd = nsh_argument(vtbl, saveptr, memlist, NULL);
          if (*ppcmd)
            {
              nsh_error(vtbl, g_fmtarginvalid, "done");
              goto errout;
            }

          /* Verify that "done" is valid in this context */

          if (np->np_lpstate[np->np_lpndx].lp_state != NSH_LOOP_DO)
            {
              nsh_error(vtbl, g_fmtcontext, "done");
              goto errout;
            }

          if (np->np_lpndx < 1) /* Shouldn't happen */
            {
              nsh_error(vtbl, g_fmtinternalerror, "done");
              goto errout;
            }

          /* Now what do we do?  We either:  Do go back to the top of the
           * loop (if lp_enable == true) or continue past the end of the
           * loop (if lp_enable == false)
           */

          if (np->np_lpstate[np->np_lpndx].lp_enable)
            {
              /* Set the new file position to the top of the loop offset */

              ret = fseek(np->np_stream,
                          np->np_lpstate[np->np_lpndx].lp_topoffs,
                          SEEK_SET);
              if (ret < 0)
                {
                  nsh_error(vtbl, g_fmtcmdfailed, "done", "fseek",
                            NSH_ERRNO);
                }

#ifndef NSH_DISABLE_SEMICOLON
              /* Signal nsh_parse that we need to stop processing the
               * current line and jump back to the top of the loop.
               */

              np->np_jump = true;
#endif
            }
          else
            {
              np->np_lpstate[np->np_lpndx].lp_enable = true;
            }

          /* "Pop" the previous state.  We do this no matter what we
           * decided to do
           */

          np->np_lpstate[np->np_lpndx].lp_state = NSH_LOOP_NORMAL;
          np->np_lpndx--;
        }

      /* If we just parsed "while" or "until", then nothing is acceptable
       * other than "do"
       */

      else if (np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_WHILE ||
               np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_UNTIL)
        {
          nsh_error(vtbl, g_fmtcontext, cmd);
          goto errout;
        }
    }

  return OK;

errout:
#ifndef NSH_DISABLE_SEMICOLON
  np->np_jump                  = false;
#endif
  np->np_lpndx                 = 0;
  np->np_lpstate[0].lp_state   = NSH_LOOP_NORMAL;
  np->np_lpstate[0].lp_enable  = true;
  np->np_lpstate[0].lp_topoffs = 0;
  return ERROR;
}
#endif

/****************************************************************************
 * Name: nsh_itef
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_ITEF)
static int nsh_itef(FAR struct nsh_vtbl_s *vtbl, FAR char **ppcmd,
                    FAR char **saveptr, FAR NSH_MEMLIST_TYPE *memlist)
{
  FAR struct nsh_parser_s *np = &vtbl->np;
  FAR char *cmd = *ppcmd;
  bool disabled;
  bool inverted = false;

  if (cmd != NULL)
    {
      /* Check if the command is preceded by "if" */

      if (strcmp(cmd, "if") == 0)
        {
          /* Get the cmd following the if */

          *ppcmd = nsh_argument(vtbl, saveptr, memlist, NULL);
          if (*ppcmd == NULL || **ppcmd == '\0')
            {
              nsh_error(vtbl, g_fmtarginvalid, "if");
              goto errout;
            }

          /* Check for inverted logic */

          if (strcmp(*ppcmd, "!") == 0)
            {
              inverted = true;

              /* Get the next cmd */

              *ppcmd = nsh_argument(vtbl, saveptr, memlist, 0);
              if (*ppcmd == NULL || **ppcmd == '\0')
                {
                  nsh_error(vtbl, g_fmtarginvalid, "if");
                  goto errout;
                }
            }

          /* Verify that "if" is valid in this context */

          if (np->np_iestate[np->np_iendx].ie_state == NSH_ITEF_IF)
            {
              nsh_error(vtbl, g_fmtcontext, "if");
              goto errout;
            }

          /* Check if we have exceeded the maximum depth of nesting */

          if (np->np_iendx >= CONFIG_NSH_NESTDEPTH - 1)
            {
              nsh_error(vtbl, g_fmtdeepnesting, "if");
              goto errout;
            }

          /* "Push" the old state and set the new state */

          disabled                                 = !nsh_cmdenabled(vtbl);
          np->np_iendx++;
          np->np_iestate[np->np_iendx].ie_state    = NSH_ITEF_IF;
          np->np_iestate[np->np_iendx].ie_disabled = disabled;
          np->np_iestate[np->np_iendx].ie_ifcond   = false;
          np->np_iestate[np->np_iendx].ie_inverted = inverted;
        }

      /* Check if the token is "then" */

      else if (strcmp(cmd, "then") == 0)
        {
          /* Get the cmd following the "then" -- there may or may not be
           * one.
           */

          *ppcmd = nsh_argument(vtbl, saveptr, memlist, NULL);

          /* Verify that "then" is valid in this context */

          if (np->np_iestate[np->np_iendx].ie_state != NSH_ITEF_IF)
            {
              nsh_error(vtbl, g_fmtcontext, "then");
              goto errout;
            }

          np->np_iestate[np->np_iendx].ie_state = NSH_ITEF_THEN;
        }

      /* Check if the token is "else" */

      else if (strcmp(cmd, "else") == 0)
        {
          /* Get the cmd following the "else" -- there may or may not be
           * one.
           */

          *ppcmd = nsh_argument(vtbl, saveptr, memlist, NULL);

          /* Verify that "else" is valid in this context */

          if (np->np_iestate[np->np_iendx].ie_state != NSH_ITEF_THEN)
            {
              nsh_error(vtbl, g_fmtcontext, "else");
              goto errout;
            }

          np->np_iestate[np->np_iendx].ie_state = NSH_ITEF_ELSE;
        }

      /* Check if the token is "fi" */

      else if (strcmp(cmd, "fi") == 0)
        {
          /* Get the cmd following the fi -- there should be one */

          *ppcmd = nsh_argument(vtbl, saveptr, memlist, NULL);
          if (*ppcmd)
            {
              nsh_error(vtbl, g_fmtarginvalid, "fi");
              goto errout;
            }

          /* Verify that "fi" is valid in this context */

          if (np->np_iestate[np->np_iendx].ie_state != NSH_ITEF_THEN &&
              np->np_iestate[np->np_iendx].ie_state != NSH_ITEF_ELSE)
            {
              nsh_error(vtbl, g_fmtcontext, "fi");
              goto errout;
            }

          if (np->np_iendx < 1) /* Shouldn't happen */
            {
              nsh_error(vtbl, g_fmtinternalerror, "if");
              goto errout;
            }

          /* "Pop" the previous state */

          np->np_iendx--;
        }

      /* If we just parsed "if", then nothing is acceptable other than
       * "then".
       */

      else if (np->np_iestate[np->np_iendx].ie_state == NSH_ITEF_IF)
        {
          nsh_error(vtbl, g_fmtcontext, cmd);
          goto errout;
        }
    }

  return OK;

errout:
  np->np_iendx                  = 0;
  np->np_iestate[0].ie_state    = NSH_ITEF_NORMAL;
  np->np_iestate[0].ie_disabled = false;
  np->np_iestate[0].ie_ifcond   = false;
  np->np_iestate[0].ie_inverted = false;
  return ERROR;
}
#endif

/****************************************************************************
 * Name: nsh_nice
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLEBG
static int nsh_nice(FAR struct nsh_vtbl_s *vtbl, FAR char **ppcmd,
                    FAR char **saveptr, FAR NSH_MEMLIST_TYPE *memlist)
{
  FAR char *cmd = *ppcmd;

  vtbl->np.np_nice = 0;
  if (cmd)
    {
      /* Check if the command is preceded by "nice" */

      if (strcmp(cmd, "nice") == 0)
        {
          /* Nicenesses range from -20 (most favorable scheduling) to 19
           * (least  favorable).  Default is 10.
           */

          vtbl->np.np_nice = 10;

          /* Get the cmd (or -d option of nice command) */

          cmd = nsh_argument(vtbl, saveptr, memlist, NULL);
          if (cmd && strcmp(cmd, "-d") == 0)
            {
              FAR char *val = nsh_argument(vtbl, saveptr, memlist, NULL);
              if (val)
                {
                  char *endptr;
                  vtbl->np.np_nice = (int)strtol(val, &endptr, 0);
                  if (vtbl->np.np_nice > 19 || vtbl->np.np_nice < -20 ||
                      endptr == val || *endptr != '\0')
                    {
                      nsh_error(vtbl, g_fmtarginvalid, "nice");
                      return ERROR;
                    }

                  cmd = nsh_argument(vtbl, saveptr, memlist, NULL);
                }
            }

          /* Return the real command name */

          *ppcmd = cmd;
        }
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: nsh_parse_cmdparm
 *
 * Description:
 *   This function parses and executes a simple NSH command.  Output is
 *   always redirected.  This function supports command parameters like
 *
 *     set FOO `hello`
 *
 *   which would set the environment variable FOO to the output from
 *   the hello program
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_CMDPARMS
static int nsh_parse_cmdparm(FAR struct nsh_vtbl_s *vtbl, FAR char *cmdline,
                             FAR const char *redirfile)
{
  NSH_MEMLIST_TYPE memlist;
  FAR char *argv[MAX_ARGV_ENTRIES];
  FAR char *saveptr;
  FAR char *cmd;
#ifndef CONFIG_NSH_DISABLEBG
  bool bgsave;
#endif
  bool redirsave;
  int argc;
  int ret;

  /* Initialize parser state */

  memset(argv, 0, MAX_ARGV_ENTRIES*sizeof(FAR char *));
  NSH_MEMLIST_INIT(memlist);

  /* If any options like nice, redirection, or backgrounding are attempted,
   * these will not be recognized and will just be passed through as
   * normal, invalid commands or parameters.
   */

#ifndef CONFIG_NSH_DISABLEBG
  /* The command is never backgrounded .  Remember the current backgrounding
   * state
   */

  bgsave = vtbl->np.np_bg;
  vtbl->np.np_bg = false;
#endif

  /* Output is always redirected.  Remember the current redirection state */

  redirsave = vtbl->np.np_redirect;
  vtbl->np.np_redirect = true;

  /* Parse out the command at the beginning of the line */

  saveptr = cmdline;
  cmd = nsh_argument(vtbl, &saveptr, &memlist, NULL);

  /* Check if any command was provided -OR- if command processing is
   * currently disabled.
   */

#ifndef CONFIG_NSH_DISABLESCRIPT
  if (!cmd || !nsh_cmdenabled(vtbl))
#else
  if (!cmd)
#endif
    {
      /* An empty line is not an error and an unprocessed command cannot
       * generate an error, but neither should it change the last command
       * status.
       */

      ret = 0;
      goto exit;
    }

  /* Parse all of the arguments following the command name.  The form
   * of argv is:
   *
   *   argv[0]:      The command name.
   *   argv[1]:      The beginning of argument (up to
   *                 CONFIG_NSH_MAXARGUMENTS)
   *   argv[argc]:   NULL terminating pointer
   *
   * Maximum size is CONFIG_NSH_MAXARGUMENTS+1
   */

  argv[0] = cmd;
  for (argc = 1; argc < MAX_ARGV_ENTRIES - 1; argc++)
    {
      argv[argc] = nsh_argument(vtbl, &saveptr, &memlist, NULL);
      if (!argv[argc])
        {
          break;
        }
    }

  argv[argc] = NULL;

  /* Check if the maximum number of arguments was exceeded */

  if (argc > CONFIG_NSH_MAXARGUMENTS)
    {
      nsh_error(vtbl, g_fmttoomanyargs, cmd);
    }

  /* Then execute the command */

  ret = nsh_execute(vtbl, argc, argv, redirfile,
                    O_WRONLY | O_CREAT | O_TRUNC);

  /* Restore the backgrounding and redirection state */

exit:
#ifndef CONFIG_NSH_DISABLEBG
  vtbl->np.np_bg       = bgsave;
#endif
  vtbl->np.np_redirect = redirsave;

  NSH_MEMLIST_FREE(&memlist);
  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_parse_command
 *
 * Description:
 *   This function parses and executes one NSH command from the command line.
 *
 ****************************************************************************/

static int nsh_parse_command(FAR struct nsh_vtbl_s *vtbl, FAR char *cmdline)
{
  NSH_MEMLIST_TYPE memlist;
  FAR char *argv[MAX_ARGV_ENTRIES];
  FAR char *saveptr;
  FAR char *cmd;
  FAR char *redirfile = NULL;
  int       oflags = 0;
  int       argc;
  int       ret;
#ifdef CONFIG_FILE_STREAM
  bool      redirect_save = false;
#endif

  /* Initialize parser state */

  memset(argv, 0, MAX_ARGV_ENTRIES*sizeof(FAR char *));
  NSH_MEMLIST_INIT(memlist);

#ifndef CONFIG_NSH_DISABLEBG
  vtbl->np.np_bg       = false;
#endif

#ifdef CONFIG_FILE_STREAM
  vtbl->np.np_redirect = false;
#endif

  /* Parse out the command at the beginning of the line */

  saveptr = cmdline;
  cmd = nsh_argument(vtbl, &saveptr, &memlist, NULL);

#ifndef CONFIG_NSH_DISABLESCRIPT
#ifndef CONFIG_NSH_DISABLE_LOOPS
  /* Handle while-do-done and until-do-done loops */

  if (nsh_loop(vtbl, &cmd, &saveptr, &memlist) != 0)
    {
      NSH_MEMLIST_FREE(&memlist);
      return nsh_saveresult(vtbl, true);
    }
#endif

#ifndef CONFIG_NSH_DISABLE_ITEF
  /* Handle if-then-else-fi */

  if (nsh_itef(vtbl, &cmd, &saveptr, &memlist) != 0)
    {
      NSH_MEMLIST_FREE(&memlist);
      return nsh_saveresult(vtbl, true);
    }

#endif
#endif

  /* Handle nice */

#ifndef CONFIG_NSH_DISABLEBG
  if (nsh_nice(vtbl, &cmd, &saveptr, &memlist) != 0)
    {
      NSH_MEMLIST_FREE(&memlist);
      return nsh_saveresult(vtbl, true);
    }
#endif

  /* Check if any command was provided -OR- if command processing is
   * currently disabled.
   */

#ifndef CONFIG_NSH_DISABLESCRIPT
  if (!cmd || !nsh_cmdenabled(vtbl))
#else
  if (!cmd)
#endif
    {
      /* An empty line is not an error and an unprocessed command cannot
       * generate an error, but neither should it change the last command
       * status.
       */

      NSH_MEMLIST_FREE(&memlist);
      return OK;
    }

  /* Parse all of the arguments following the command name.  The form
   * of argv is:
   *
   *   argv[0]:      The command name.
   *   argv[1]:      The beginning of argument (up to
   *                 CONFIG_NSH_MAXARGUMENTS)
   *   argv[argc-3]: Possibly '>' or '>>'
   *   argv[argc-2]: Possibly <file>
   *   argv[argc-1]: Possibly '&'
   *   argv[argc]:   NULL terminating pointer
   *
   * Maximum size is CONFIG_NSH_MAXARGUMENTS+5
   */

  argv[0] = cmd;
  for (argc = 1; argc < MAX_ARGV_ENTRIES - 1; argc++)
    {
      int isenvvar = 0; /* flag for if an environment variable gets expanded */

      argv[argc] = nsh_argument(vtbl, &saveptr, &memlist, &isenvvar);

      if (!argv[argc])
        {
          break;
        }

      if (isenvvar != 0)
        {
          while (argc < MAX_ARGV_ENTRIES - 1)
            {
              FAR char *pbegin = argv[argc];

              /* Find the end of the current token */

              for (; *pbegin && !strchr(g_token_separator, *pbegin);
                   pbegin++)
                {
                }

              /* If end of string, we've processed the last token and we're
               * done.
               */

              if ('\0' == *pbegin)
                {
                  break;
                }

              /* Terminate the token to complete the argv variable */

              *pbegin = '\0';

              /* We've inserted an extra parameter, so bump the count */

              argc++;

              /* Move to the next character in the string of tokens */

              pbegin++;

              /* Throw away any extra separator chars between tokens */

              for (; *pbegin && strchr(g_token_separator, *pbegin) != NULL;
                   pbegin++)
                {
                }

              /* Prepare to loop again on the next argument token */

              argv[argc] = pbegin;
            }
        }
    }

  /* Check if the command should run in background */

#ifndef CONFIG_NSH_DISABLEBG
  if (argc > 1 && strcmp(argv[argc - 1], "&") == 0)
    {
      vtbl->np.np_bg = true;
      argc--;
    }
#endif

#ifdef CONFIG_FILE_STREAM
  /* Check if the output was re-directed using > or >> */

  if (argc > 2)
    {
      /* Check for redirection to a new file */

      if (strcmp(argv[argc - 2], g_redirect1) == 0)
        {
          redirect_save        = vtbl->np.np_redirect;
          vtbl->np.np_redirect = true;
          oflags               = O_WRONLY | O_CREAT | O_TRUNC;
          redirfile            = nsh_getfullpath(vtbl, argv[argc - 1]);
          argc                -= 2;
        }

      /* Check for redirection by appending to an existing file */

      else if (strcmp(argv[argc - 2], g_redirect2) == 0)
        {
          redirect_save        = vtbl->np.np_redirect;
          vtbl->np.np_redirect = true;
          oflags               = O_WRONLY | O_CREAT | O_APPEND;
          redirfile            = nsh_getfullpath(vtbl, argv[argc - 1]);
          argc                -= 2;
        }
    }
#endif

  /* Last argument vector must be empty */

  argv[argc] = NULL;

  /* Check if the maximum number of arguments was exceeded */

  if (argc > CONFIG_NSH_MAXARGUMENTS)
    {
      nsh_error(vtbl, g_fmttoomanyargs, cmd);
    }

  /* Then execute the command */

  ret = nsh_execute(vtbl, argc, argv, redirfile, oflags);

  /* Free any allocated resources */

#ifdef CONFIG_FILE_STREAM
  /* Free the redirected output file path */

  if (redirfile)
    {
      nsh_freefullpath(redirfile);
      vtbl->np.np_redirect = redirect_save;
    }
#endif

  NSH_MEMLIST_FREE(&memlist);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_parse
 *
 * Description:
 *   This function parses and executes the line of text received from the
 *   user.  This may consist of one or more NSH commands.  Multiple NSH
 *   commands are separated by semi-colons.
 *
 ****************************************************************************/

int nsh_parse(FAR struct nsh_vtbl_s *vtbl, FAR char *cmdline)
{
#ifdef NSH_DISABLE_SEMICOLON
  return nsh_parse_command(vtbl, cmdline);

#else
#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
  FAR struct nsh_parser_s *np = &vtbl->np;
#endif
  FAR char *start   = cmdline;
  FAR char *working = cmdline;
  FAR char *ptr;
  size_t len;
  int ret;

  /* Loop until all of the commands on the command line have been processed
   * OR until the end-of-loop has been encountered and we need to reload the
   * line at the top of the loop.
   */

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
  for (np->np_jump = false; !np->np_jump; )
#else
  for (; ; )
#endif
    {
#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
      /* Save the offset on the line to the start of the command */

      np->np_loffs = (uint16_t)(working - cmdline);
#endif
      /* A command may be terminated with a newline character, the end of the
       * line, a semicolon, or a '#' character.  NOTE that the set of
       * delimiting characters includes the quotation mark.  We need to
       * handle quotation marks here because any other delimiter within a
       * quoted string must be treated as normal text.
       */

      len = strcspn(working, g_line_separator);
      ptr = working + len;

      /* Check for the last command on the line.  This means that the none
       * of the delimiting characters was found or that the newline or '#'
       * character was found.  Anything after the newline or '#' character
       * is ignored (there should not be anything after a newline, of
       * course).
       */

      if (*ptr == '\0' || *ptr == '\n' || *ptr == '#')
        {
          /* Parse the last command on the line */

          return nsh_parse_command(vtbl, start);
        }

      /* Check for a command terminated with ';'.  There is probably another
       * command on the command line after this one.
       */

      else if (*ptr == ';')
        {
          /* Terminate the line */

          *ptr++ = '\0';

          /* Parse this command */

          ret = nsh_parse_command(vtbl, start);
          if (ret != OK)
            {
              /* nsh_parse_command may return (1) -1 (ERROR) meaning that the
               * command failed or we failed to start the command application
               * or (2) 1 meaning that the application task was spawned
               * successfully but returned failure exit status.
               */

              return ret;
            }

          /* Then set the start of the next command on the command line */

          start   = ptr;
          working = ptr;
        }

      /* Check if we encountered a quoted string */

      else /* if (*ptr == '"') */
        {
          /* Find the closing quotation mark */

          FAR char *tmp = nsh_strchr(ptr + 1, '"');
          if (!tmp)
            {
              /* No closing quotation mark! */

              nsh_error(vtbl, g_fmtnomatching, "\"", "\"");
              return ERROR;
            }

          /* Otherwise, continue parsing after the closing quotation mark */

          working = ++tmp;
        }
    }

#ifndef CONFIG_NSH_DISABLESCRIPT
  return OK;
#endif
#endif
}

/****************************************************************************
 * Name: cmd_break
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
int cmd_break(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR struct nsh_parser_s *np = &vtbl->np;

  /* Break outside of a loop is ignored */

  if (np->np_lpstate[np->np_lpndx].lp_state == NSH_LOOP_DO)
    {
#ifndef CONFIG_NSH_DISABLE_ITEF
      /* Yes... pop the original if-then-else-if state */

      np->np_iendx = np->np_lpstate[np->np_lpndx].lp_iendx;
#endif
      /* Disable all command processing until 'done' is encountered.  */

      np->np_lpstate[np->np_lpndx].lp_enable = false;
    }

  /* No syntax errors are detected(?).  Break is a nop everywhere except
   * the supported context.
   */

  return OK;
}
#endif
