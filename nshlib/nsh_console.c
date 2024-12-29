/****************************************************************************
 * apps/nshlib/nsh_console.c
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

#include <sys/ioctl.h>
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

struct serialsave_s
{
#ifdef CONFIG_NSH_ALTCONDEV
  int   cn_confd;     /* Console I/O file descriptor */
#else
  int   cn_infd;      /* Re-directed input file descriptor */
#endif
  int   cn_errfd;     /* Re-directed error output file descriptor */
  int   cn_outfd;     /* Re-directed output file descriptor */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLEBG
static FAR struct nsh_vtbl_s *nsh_consoleclone(FAR struct nsh_vtbl_s *vtbl);
#endif
static void nsh_consolerelease(FAR struct nsh_vtbl_s *vtbl);
static ssize_t nsh_consolewrite(FAR struct nsh_vtbl_s *vtbl,
                                FAR const void *buffer, size_t nbytes);
static int nsh_consoleioctl(FAR struct nsh_vtbl_s *vtbl,
                            int cmd, unsigned long arg);
static int nsh_consoleoutput(FAR struct nsh_vtbl_s *vtbl,
                             FAR const char *fmt, ...) printf_like(2, 3);
#ifndef CONFIG_NSH_DISABLE_ERROR_PRINT
static int nsh_erroroutput(FAR struct nsh_vtbl_s *vtbl,
                           FAR const char *fmt, ...) printf_like(2, 3);
#endif
static FAR char *nsh_consolelinebuffer(FAR struct nsh_vtbl_s *vtbl);
static void nsh_consoleredirect(FAR struct nsh_vtbl_s *vtbl, int fd_in,
                                int fd_out, FAR uint8_t *save);
static void nsh_consoleundirect(FAR struct nsh_vtbl_s *vtbl,
                                FAR uint8_t *save);
static void nsh_consoleexit(FAR struct nsh_vtbl_s *vtbl,
                            int exitstatus) noreturn_function;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_closeifnotclosed
 *
 * Description:
 *   Close the output stream if it is not the standard output stream.
 *
 ****************************************************************************/

static void nsh_closeifnotclosed(struct console_stdio_s *pstate)
{
  if (OUTFD(pstate) >= 0 && OUTFD(pstate) != STDOUT_FILENO)
    {
      close(OUTFD(pstate));
    }

  if (ERRFD(pstate) >= 0 && ERRFD(pstate) != STDERR_FILENO
      && ERRFD(pstate) != OUTFD(pstate))
    {
      close(ERRFD(pstate));
    }

  if (INFD(pstate) >= 0 && INFD(pstate) != STDIN_FILENO)
    {
      close(INFD(pstate));
    }

  ERRFD(pstate) = -1;
  OUTFD(pstate) = -1;
  INFD(pstate) = -1;
}

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

  /* Write the data to the output stream */

  ret = write(OUTFD(pstate), buffer, nbytes);
  if (ret < 0)
    {
      _err("ERROR: [%d] Failed to send buffer: %d\n",
          OUTFD(pstate), errno);
    }

  return ret;
}

/****************************************************************************
 * Name: nsh_consoleread
 *
 * Description:
 *   read a buffer to the remote shell window.
 *
 *   Currently only used by cat.
 *
 ****************************************************************************/

static ssize_t nsh_consoleread(FAR struct nsh_vtbl_s *vtbl,
                                FAR void *buffer, size_t nbytes)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  ssize_t ret;

  /* Read the data to the output stream */

  ret = read(INFD(pstate), buffer, nbytes);
  if (ret < 0)
    {
      _err("ERROR: [%d] Failed to read buffer: %d\n",
          INFD(pstate), errno);
    }

  return ret;
}

/****************************************************************************
 * Name: nsh_consolewrite
 *
 * Description:
 *   Issue ioctl to the currently selected stream.
 *
 ****************************************************************************/

static int nsh_consoleioctl(FAR struct nsh_vtbl_s *vtbl,
                            int cmd, unsigned long arg)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;

  return ioctl(OUTFD(pstate), cmd, arg);
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

  va_start(ap, fmt);
  ret = vdprintf(OUTFD(pstate), fmt, ap);
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

#ifndef CONFIG_NSH_DISABLE_ERROR_PRINT
static int nsh_erroroutput(FAR struct nsh_vtbl_s *vtbl,
                           FAR const char *fmt, ...)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vdprintf(ERRFD(pstate), fmt, ap);
  va_end(ap);

  return ret;
}
#endif

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
  close(pstate->cn_confd);
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

static void nsh_consoleredirect(FAR struct nsh_vtbl_s *vtbl, int fd_in,
                                int fd_out, FAR uint8_t *save)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  FAR struct serialsave_s *ssave  = (FAR struct serialsave_s *)save;

  /* Redirected foreground commands */

  if (ssave)
    {
      /* Save the current fd and stream values.  These will be restored
       * when nsh_consoleundirect() is called.
       */

      ERRFD(ssave) = ERRFD(pstate);
      OUTFD(ssave) = OUTFD(pstate);
      INFD(ssave) = INFD(pstate);
    }

  /* Set the fd of the new. */

  OUTFD(pstate) = fd_out;
  INFD(pstate) = fd_in;
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
  FAR struct serialsave_s *ssave = (FAR struct serialsave_s *)save;

  nsh_closeifnotclosed(pstate);
  ERRFD(pstate) = ERRFD(ssave);
  OUTFD(pstate) = OUTFD(ssave);
  INFD(pstate) = INFD(ssave);
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
      pstate->cn_vtbl.read        = nsh_consoleread;
      pstate->cn_vtbl.ioctl       = nsh_consoleioctl;
      pstate->cn_vtbl.output      = nsh_consoleoutput;
#ifndef CONFIG_NSH_DISABLE_ERROR_PRINT
      pstate->cn_vtbl.error       = nsh_erroroutput;
#endif
      pstate->cn_vtbl.linebuffer  = nsh_consolelinebuffer;
      pstate->cn_vtbl.exit        = nsh_consoleexit;
      pstate->cn_vtbl.isctty      = isctty;

#ifndef CONFIG_NSH_DISABLESCRIPT
      /* Set the initial option flags */

      pstate->cn_vtbl.np.np_flags = NSH_NP_SET_OPTIONS_INIT;
#endif

      pstate->cn_vtbl.redirect    = nsh_consoleredirect;
      pstate->cn_vtbl.undirect    = nsh_consoleundirect;

      /* Initialize the error stream */

      ERRFD(pstate)               = STDERR_FILENO;

      /* Initialize the output stream */

      OUTFD(pstate)               = STDOUT_FILENO;

      /* Initialize the input stream */

      INFD(pstate)               = STDIN_FILENO;

      /* Initialize current working directory */

#ifdef CONFIG_DISABLE_ENVIRON
      strlcpy(pstate->cn_vtbl.cwd, CONFIG_LIBC_HOMEDIR,
              sizeof(pstate->cn_vtbl.cwd));
#endif
    }

  return pstate;
}
