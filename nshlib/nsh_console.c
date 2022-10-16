/****************************************************************************
 * apps/nshlib/nsh_console.c
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_FILE_STREAM
struct serialsave_s
{
  int   cn_errfd;     /* Re-directed error output file descriptor */
  int   cn_outfd;     /* Re-directed output file descriptor */
  FILE *cn_errstream; /* Re-directed error output stream */
  FILE *cn_outstream; /* Re-directed output stream */
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLEBG
static FAR struct nsh_vtbl_s *nsh_consoleclone(FAR struct nsh_vtbl_s *vtbl);
#endif
static void nsh_consolerelease(FAR struct nsh_vtbl_s *vtbl);
static ssize_t nsh_consolewrite(FAR struct nsh_vtbl_s *vtbl,
  FAR const void *buffer, size_t nbytes);
static int nsh_consoleoutput(FAR struct nsh_vtbl_s *vtbl,
  FAR const char *fmt, ...) printflike(2, 3);
static int nsh_erroroutput(FAR struct nsh_vtbl_s *vtbl,
  FAR const char *fmt, ...) printflike(2, 3);
static FAR char *nsh_consolelinebuffer(FAR struct nsh_vtbl_s *vtbl);
static void nsh_consoleredirect(FAR struct nsh_vtbl_s *vtbl, int fd,
  FAR uint8_t *save);
static void nsh_consoleundirect(FAR struct nsh_vtbl_s *vtbl,
  FAR uint8_t *save);
static void nsh_consoleexit(FAR struct nsh_vtbl_s *vtbl, int exitstatus)
  noreturn_function;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_openifnotopen
 ****************************************************************************/

static int nsh_openifnotopen(struct console_stdio_s *pstate)
{
  /* The stream is open in a lazy fashion.  This is done because the file
   * descriptor may be opened on a different task than the stream.
   */

  if (!pstate->cn_outstream)
    {
      pstate->cn_outstream = fdopen(pstate->cn_outfd, "w");
      if (!pstate->cn_outstream)
        {
          return ERROR;
        }

#if !defined(CONFIG_NSH_ALTCONDEV)
      /* If the alternative console is not enabled then stderr = stdout */

      pstate->cn_errfd     = pstate->cn_outfd;
      pstate->cn_errstream = pstate->cn_outstream;
#endif
    }

  return 0;
}

/****************************************************************************
 * Name: nsh_closeifnotclosed
 *
 * Description:
 *   Close the output stream if it is not the standard output stream.
 *
 ****************************************************************************/

#ifdef CONFIG_FILE_STREAM
static void nsh_closeifnotclosed(struct console_stdio_s *pstate)
{
  if (pstate->cn_outstream == OUTSTREAM(pstate))
    {
      fflush(OUTSTREAM(pstate));
      pstate->cn_outfd = OUTFD(pstate);
    }
  else
    {
      if (pstate->cn_outstream)
        {
          fflush(pstate->cn_outstream);
          fclose(pstate->cn_outstream);
        }
      else if (pstate->cn_outfd >= 0 && pstate->cn_outfd != OUTFD(pstate))
        {
          close(pstate->cn_outfd);
        }

      pstate->cn_errfd     = -1;
      pstate->cn_outfd     = -1;
      pstate->cn_errstream = NULL;
      pstate->cn_outstream = NULL;
    }
}
#endif

/****************************************************************************
 * Name: nsh_consolewrite
 *
 * Description:
 *   write a buffer to the remote shell window.
 *
 *   Currently only used by cat.
 *
 ****************************************************************************/

static ssize_t nsh_consolewrite(FAR struct nsh_vtbl_s *vtbl,
                                FAR const void *buffer, size_t nbytes)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  ssize_t ret;

  /* The stream is open in a lazy fashion.  This is done because the file
   * descriptor may be opened on a different task than the stream.  The
   * actual open will then occur with the first output from the new task.
   */

  if (nsh_openifnotopen(pstate) != 0)
    {
      return ERROR;
    }

  /* Write the data to the output stream */

  ret = fwrite(buffer, 1, nbytes, pstate->cn_outstream);
  if (ret < 0)
    {
      _err("ERROR: [%d] Failed to send buffer: %d\n",
          pstate->cn_outfd, errno);
    }

  /* Flush the data to the output stream */

  fflush(pstate->cn_outstream);

  return ret;
}

/****************************************************************************
 * Name: nsh_consoleoutput
 *
 * Description:
 *   Print a string to the currently selected stream.
 *
 ****************************************************************************/

static int nsh_consoleoutput(FAR struct nsh_vtbl_s *vtbl,
                             FAR const char *fmt, ...)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  va_list ap;
  int ret;

  /* The stream is open in a lazy fashion.  This is done because the file
   * descriptor may be opened on a different task than the stream.  The
   * actual open will then occur with the first output from the new task.
   */

  if (nsh_openifnotopen(pstate) != 0)
    {
      return ERROR;
    }

  va_start(ap, fmt);
  ret = vfprintf(pstate->cn_outstream, fmt, ap);
  va_end(ap);

  return ret;
}

/****************************************************************************
 * Name: nsh_erroroutput
 *
 * Description:
 *   Print a string to the currently selected error stream.
 *
 ****************************************************************************/

static int nsh_erroroutput(FAR struct nsh_vtbl_s *vtbl,
                           FAR const char *fmt, ...)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  va_list ap;
  int ret;

  /* The stream is open in a lazy fashion.  This is done because the file
   * descriptor may be opened on a different task than the stream.  The
   * actual open will then occur with the first output from the new task.
   */

  if (nsh_openifnotopen(pstate) != 0)
    {
      return ERROR;
    }

  va_start(ap, fmt);
  ret = vfprintf(pstate->cn_errstream, fmt, ap);
  va_end(ap);

  return ret;
}

/****************************************************************************
 * Name: nsh_consolelinebuffer
 *
 * Description:
 *   Return a reference to the current line buffer
 *
 ****************************************************************************/

static FAR char *nsh_consolelinebuffer(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  return pstate->cn_line;
}

/****************************************************************************
 * Name: nsh_consoleclone
 *
 * Description:
 *   Make an independent copy of the vtbl
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLEBG
static FAR struct nsh_vtbl_s *nsh_consoleclone(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct console_stdio_s *pclone = nsh_newconsole(vtbl->isctty);
  return &pclone->cn_vtbl;
}
#endif

/****************************************************************************
 * Name: nsh_consolerelease
 *
 * Description:
 *   Release the cloned instance
 *
 ****************************************************************************/

static void nsh_consolerelease(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;

  /* Close the output stream */

  nsh_closeifnotclosed(pstate);

  /* Close the console stream */

#ifdef CONFIG_NSH_ALTCONDEV
  fclose(pstate->cn_constream);
#endif

#ifdef CONFIG_NSH_VARS
  /* Free any NSH variables */

  if (pstate->varp != NULL)
    {
      free(pstate->varp);
    }
#endif

  /* Then release the vtable container */

  free(pstate);
}

/****************************************************************************
 * Name: nsh_consoleredirect
 *
 * Description:
 *   Set up for redirected output.  This function is called from nsh_parse()
 *   in two different contexts:
 *
 *   1) Redirected background commands of the form:  command > xyz.text &
 *
 *      In this case:
 *      - vtbl: A newly allocated and initialized instance created by
 *        nsh_consoleclone,
 *      - fd:- The file descriptor of the redirected output
 *      - save: NULL
 *
 *      nsh_consolerelease() will perform the clean-up when the clone is
 *      destroyed.
 *
 *   2) Redirected foreground commands of the form:  command > xyz.txt
 *
 *      In this case:
 *      - vtbl: The current state structure,
 *      - fd: The file descriptor of the redirected output
 *      - save: Where to save the re-directed registers.
 *
 *      nsh_consoleundirect() will perform the clean-up after the redirected
 *      command completes.
 *
 ****************************************************************************/

static void nsh_consoleredirect(FAR struct nsh_vtbl_s *vtbl, int fd,
                                FAR uint8_t *save)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  FAR struct serialsave_s *ssave  = (FAR struct serialsave_s *)save;

  /* Case 1: Redirected foreground commands */

  if (ssave)
    {
      /* pstate->cn_outstream and cn_outfd refer refer to the
       * currently opened output stream.  If the output stream is open, flush
       * any pending output.
       */

      if (pstate->cn_outstream)
        {
          fflush(pstate->cn_errstream);
          fflush(pstate->cn_outstream);
        }

      /* Save the current fd and stream values.  These will be restored
       * when nsh_consoleundirect() is called.
       */

      ssave->cn_errfd     = pstate->cn_errfd;
      ssave->cn_outfd     = pstate->cn_outfd;
      ssave->cn_errstream = pstate->cn_errstream;
      ssave->cn_outstream = pstate->cn_outstream;
    }
  else
    {
      /* nsh_consoleclone() set pstate->cn_outfd and cn_outstream to refer
       * to standard out.  We just want to leave these alone and overwrite
       * them with the fd for the re-directed stream.
       */
    }

  /* In either case, set the fd of the new, re-directed output and nullify
   * the output stream (it will be fdopen'ed if it is used).
   */

  pstate->cn_outfd     = fd;
  pstate->cn_outstream = NULL;
}

/****************************************************************************
 * Name: nsh_consoleundirect
 *
 * Description:
 *   Set up for redirected output
 *
 ****************************************************************************/

static void nsh_consoleundirect(FAR struct nsh_vtbl_s *vtbl,
                                FAR uint8_t *save)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  FAR struct serialsave_s *ssave  = (FAR struct serialsave_s *)save;

  nsh_closeifnotclosed(pstate);
  pstate->cn_errfd     = ssave->cn_errfd;
  pstate->cn_outfd     = ssave->cn_outfd;
  pstate->cn_errstream = ssave->cn_errstream;
  pstate->cn_outstream = ssave->cn_outstream;
}

/****************************************************************************
 * Name: nsh_consoleexit
 *
 * Description:
 *   Exit the shell task
 *
 ****************************************************************************/

static void nsh_consoleexit(FAR struct nsh_vtbl_s *vtbl, int exitstatus)
{
  /* Destroy ourself then exit with the provided status */

  nsh_consolerelease(vtbl);
  exit(exitstatus);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_newconsole
 ****************************************************************************/

FAR struct console_stdio_s *nsh_newconsole(bool isctty)
{
  FAR struct console_stdio_s *pstate =
    (FAR struct console_stdio_s *)zalloc(sizeof(struct console_stdio_s));

  if (pstate)
    {
      /* Initialize the call table */

#ifndef CONFIG_NSH_DISABLEBG
      pstate->cn_vtbl.clone       = nsh_consoleclone;
#endif
      pstate->cn_vtbl.release     = nsh_consolerelease;
      pstate->cn_vtbl.write       = nsh_consolewrite;
      pstate->cn_vtbl.output      = nsh_consoleoutput;
      pstate->cn_vtbl.error       = nsh_erroroutput;
      pstate->cn_vtbl.linebuffer  = nsh_consolelinebuffer;
      pstate->cn_vtbl.exit        = nsh_consoleexit;
      pstate->cn_vtbl.isctty      = isctty;

#ifndef CONFIG_NSH_DISABLESCRIPT
      /* Set the initial option flags */

      pstate->cn_vtbl.np.np_flags = NSH_NP_SET_OPTIONS_INIT;
#endif

#ifdef CONFIG_FILE_STREAM
      pstate->cn_vtbl.redirect    = nsh_consoleredirect;
      pstate->cn_vtbl.undirect    = nsh_consoleundirect;

      /* Initialize the error stream */

      pstate->cn_errfd            = ERRFD(pstate);
      pstate->cn_errstream        = ERRSTREAM(pstate);

      /* Initialize the output stream */

      pstate->cn_outfd            = OUTFD(pstate);
      pstate->cn_outstream        = OUTSTREAM(pstate);
#endif
    }

  return pstate;
}
