/****************************************************************************
 * apps/nshlib/nsh_console.h
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

#ifndef __APPS_NSHLIB_NSH_CONSOLE_H
#define __APPS_NSHLIB_NSH_CONSOLE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <nuttx/queue.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Method access macros */

#define nsh_clone(v)            (v)->clone(v)
#define nsh_release(v)          (v)->release(v)
#define nsh_write(v,b,n)        (v)->write(v,b,n)
#define nsh_read(v,b,n)         (v)->read(v,b,n)
#define nsh_ioctl(v,c,a)        (v)->ioctl(v,c,a)
#define nsh_linebuffer(v)       (v)->linebuffer(v)
#define nsh_redirect(v,fi,fo,s) (v)->redirect(v,fi,fo,s)
#define nsh_undirect(v,s)       (v)->undirect(v,s)
#define nsh_exit(v,s)           (v)->exit(v,s)

#ifdef CONFIG_CPP_HAVE_VARARGS
#  define nsh_error(v, ...)     (v)->error(v, ##__VA_ARGS__)
#  define nsh_output(v, ...)    (v)->output(v, ##__VA_ARGS__)
#  define nsh_none(v, ...)     \
     do { if (0) nsh_output_none(v, ##__VA_ARGS__); } while (0)
#else
#  define nsh_error            vtbl->error
#  define nsh_output           vtbl->output
#  define nsh_none             (void)
#endif

#ifdef CONFIG_NSH_DISABLE_ERROR_PRINT
#  undef nsh_error
#  define nsh_error            nsh_none
#endif

/* Size of info to be saved in call to nsh_redirect
 * See struct serialsave_s in nsh_console.c
 */

#define SAVE_SIZE (3 * sizeof(int))

/* Are we using the NuttX console for I/O?  Or some other character device? */

#ifdef CONFIG_NSH_ALTCONDEV

#  if !defined(CONFIG_NSH_ALTSTDIN) && !defined(CONFIG_NSH_ALTSTDOUT) && \
      !defined(CONFIG_NSH_ALTSTDERR)
#    error CONFIG_NSH_ALTCONDEV selected but CONFIG_NSH_ALTSTDxxx not provided
#  endif

#  define INFD(p)      ((p)->cn_confd)

#else

#  define INFD(p)      ((p)->cn_infd)

#endif

#  define OUTFD(p)     ((p)->cn_outfd)
#  define ERRFD(p)     ((p)->cn_errfd)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This describes a generic console front-end */

struct nsh_vtbl_s
{
  /* This function pointers are "hooks" into the front end logic to
   * handle things like output of command results, redirection, etc.
   * -- all of which must be done in a way that is unique to the nature
   * of the front end.
   */

#ifndef CONFIG_NSH_DISABLEBG
  FAR struct nsh_vtbl_s *(*clone)(FAR struct nsh_vtbl_s *vtbl);
  void (*addref)(FAR struct nsh_vtbl_s *vtbl);
#endif
  void (*release)(FAR struct nsh_vtbl_s *vtbl);
  ssize_t (*write)(FAR struct nsh_vtbl_s *vtbl, FAR const void *buffer,
                   size_t nbytes);
  ssize_t (*read)(FAR struct nsh_vtbl_s *vtbl, FAR void *buffer,
                   size_t nbytes);
  int (*ioctl)(FAR struct nsh_vtbl_s *vtbl, int cmd, unsigned long arg);
#ifndef CONFIG_NSH_DISABLE_ERROR_PRINT
  int (*error)(FAR struct nsh_vtbl_s *vtbl, FAR const char *fmt, ...)
      printf_like(2, 3);
#endif
  int (*output)(FAR struct nsh_vtbl_s *vtbl, FAR const char *fmt, ...)
      printf_like(2, 3);
  FAR char *(*linebuffer)(FAR struct nsh_vtbl_s *vtbl);
  void (*redirect)(FAR struct nsh_vtbl_s *vtbl, int fd_in, int fd_out,
                   FAR uint8_t *save);
  void (*undirect)(FAR struct nsh_vtbl_s *vtbl, FAR uint8_t *save);
  void (*exit)(FAR struct nsh_vtbl_s *vtbl, int status) noreturn_function;

#ifdef NSH_HAVE_IOBUFFER
  /* Common buffer for file I/O. */

  char iobuffer[IOBUFFERSIZE];
#endif

#ifdef CONFIG_NSH_ALIAS
  /* Shell alias support */

  struct nsh_alias_s atab[CONFIG_NSH_ALIAS_MAX_AMOUNT];
  struct sq_queue_s  alist;
  struct sq_queue_s  afreelist;
#endif

  /* Parser state data */

  struct nsh_parser_s np;

  /* Ctrl tty or not */

  bool isctty;

  /* Current working directory */

#ifdef CONFIG_DISABLE_ENVIRON
  char cwd[PATH_MAX];
#endif
};

/* This structure describes a console front-end that is based on stdin and
 * stdout (which is all of the supported console types at the time being).
 */

struct console_stdio_s
{
  /* NSH front-end call table */

  struct nsh_vtbl_s cn_vtbl;

  /* NSH input/output streams */

#ifdef CONFIG_NSH_ALTCONDEV
  int   cn_confd;     /* Console I/O file descriptor */
#endif
  int   cn_infd;      /* Input file descriptor (possibly redirected) */
  int   cn_outfd;     /* Output file descriptor (possibly redirected) */
  int   cn_errfd;     /* Error Output file descriptor (possibly redirected) */

#ifdef CONFIG_NSH_VARS
  /* Allocation and size of NSH variables */

  FAR char *varp;
  size_t varsz;
#endif

  /* Line input buffer */

  char   cn_line[LINE_MAX];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Inline functions
 ****************************************************************************/

#ifdef CONFIG_CPP_HAVE_VARARGS
/* Can be used to suppress any nsh output */

static inline void nsh_output_none(FAR struct nsh_vtbl_s *vtbl, ...)
{
  UNUSED(vtbl);
}
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Defined in nsh_console.c *************************************************/

FAR struct console_stdio_s *nsh_newconsole(bool isctty);

#endif /* __APPS_NSHLIB_NSH_CONSOLE_H */
