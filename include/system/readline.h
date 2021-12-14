/****************************************************************************
 * apps/include/system/readline.h
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

#ifndef __APPS_INCLUDE_SYSTEM_READLINE_H
#define __APPS_INCLUDE_SYSTEM_READLINE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>

#ifdef CONFIG_SYSTEM_READLINE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Tab completion cannot be supported if there is no console echo */

#ifndef CONFIG_READLINE_ECHO
#  undef CONFIG_READLINE_TABCOMPLETION
#endif

/* Make sure that the are valid values for all tab-completion settings */

#ifdef CONFIG_READLINE_TABCOMPLETION
#  ifndef CONFIG_READLINE_MAX_BUILTINS
#    define CONFIG_READLINE_MAX_BUILTINS 64
#  endif

#  ifndef CONFIG_READLINE_MAX_EXTCMDS
#    define CONFIG_READLINE_MAX_EXTCMDS 64
#  endif
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#if defined(CONFIG_READLINE_TABCOMPLETION) && \
    defined(CONFIG_READLINE_HAVE_EXTMATCH)
struct extmatch_vtable_s
{
  CODE int (*count_matches)(FAR char *name, FAR int *matches, int namelen);
  CODE FAR const char *(*getname)(int index);
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: readline_prompt
 *
 *   If a prompt string is used by the application, then the application
 *   must provide the prompt string to readline() by calling this function.
 *   This is needed only for tab completion in cases where is it necessary
 *   to reprint the prompt string.
 *
 * Input Parameters:
 *   prompt    - The prompt string. This function may then be
 *   called with that value in order to restore the previous vtable.
 *
 * Returned values:
 *   Returns the previous value of the prompt string.  This function may
 *   then be called with that value in order to restore the previous prompt.
 *
 * Assumptions:
 *   The prompt string is statically allocated a global.  readline() will
 *   simply remember the pointer to the string.  The string must stay
 *   allocated and available.  Only one prompt string is supported.  If
 *   there are multiple clients of readline(), they must all share the same
 *   prompt string (with exceptions in the case of the kernel build).
 *
 ****************************************************************************/

#ifdef CONFIG_READLINE_TABCOMPLETION
FAR const char *readline_prompt(FAR const char *prompt);
#else
#  define readline_prompt(p)
#endif

/****************************************************************************
 * Name: readline_extmatch
 *
 *   If the applications supports a command set, then it may call this
 *   function in order to provide support for tab complete on these
 *   "external"  commands
 *
 * Input Parameters:
 *   vtbl - Callbacks to access the external names.
 *
 * Returned values:
 *   Returns the previous vtable pointer.  This function may then be
 *   called with that value in order to restore the previous vtable.
 *
 * Assumptions:
 *   The vtbl string is statically allocated a global.  readline() will
 *   simply remember the pointer to the structure.  The structure must stay
 *   allocated and available.  Only one instance of such a structure is
 *   supported.  If there are multiple clients of readline(), they must all
 *   share the same tab-completion logic (with exceptions in the case of
 *   the kernel build).
 *
 ****************************************************************************/

#if defined(CONFIG_READLINE_TABCOMPLETION) && \
    defined(CONFIG_READLINE_HAVE_EXTMATCH)
FAR const struct extmatch_vtable_s *
  readline_extmatch(FAR const struct extmatch_vtable_s *vtbl);
#endif

/****************************************************************************
 * Name: readline_fd
 *
 *   readline_fd() reads in at most one less than 'buflen' characters from
 *   'infd' and stores them into the buffer pointed to by 'buf'.
 *   Characters are echoed on 'outfd'.  Reading stops after an EOF or a
 *   newline.  If a newline is read, it is stored into the buffer.  A null
 *   terminator is stored after the last character in the buffer.
 *
 *   This version of readline_fd assumes that we are reading and writing to
 *   a VT100 console.  This will not work well if 'infd' or 'outfd'
 *   corresponds to a raw byte steam.
 *
 *   This function is inspired by the GNU readline but is an entirely
 *   different creature.
 *
 * Input Parameters:
 *   buf    - The user allocated buffer to be filled.
 *   buflen - the size of the buffer.
 *   infd   - The file to read characters from
 *   outfd  - The file to each characters to.
 *
 * Returned values:
 *   On success, the (positive) number of bytes transferred is returned.
 *   EOF is returned to indicate either an end of file condition or a
 *   failure.
 *
 ****************************************************************************/

ssize_t readline_fd(FAR char *buf, int buflen, int infd, int outfd);

/****************************************************************************
 * Name: readline
 *
 *   readline() is same to readline_fd() but accept a file stream instead
 *   of a file handle.
 *
 ****************************************************************************/

#ifdef CONFIG_FILE_STREAM
ssize_t readline(FAR char *buf, int buflen, FILE *instream, FILE *outstream);
#endif

/****************************************************************************
 * Name: std_readline
 *
 *   readline() reads in at most one less than 'buflen' characters from
 *   'stdin' and stores them into the buffer pointed to by 'buf'.
 *   Characters are echoed on 'stdout'.  Reading stops after an EOF or a
 *   newline.  If a newline is read, it is stored into the buffer.  A null
 *   terminator is stored after the last character in the buffer.
 *
 *   This version of realine assumes that we are reading and writing to
 *   a VT100 console.  This will not work well if 'stdin' or 'stdout'
 *   corresponds to a raw byte steam.
 *
 *   This function is inspired by the GNU readline but is an entirely
 *   different creature.
 *
 * Input Parameters:
 *   buf       - The user allocated buffer to be filled.
 *   buflen    - the size of the buffer.
 *
 * Returned values:
 *   On success, the (positive) number of bytes transferred is returned.
 *   EOF is returned to indicate either an end of file condition or a
 *   failure.
 *
 ****************************************************************************/

#define std_readline(b,s) readline(b,s,stdin,stdout)

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_SYSTEM_READLINE */
#endif /* __APPS_INCLUDE_SYSTEM_READLINE_H */
