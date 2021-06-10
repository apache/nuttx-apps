/****************************************************************************
 * apps/system/readline/readline.h
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

#ifndef __APPS_SYSTEM_READLINE_READLINE_H
#define __APPS_SYSTEM_READLINE_READLINE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Some environments may return CR as end-of-line, others LF, and others
 * both.  If not specified, the logic here assumes either (but not both) as
 * the default.
 */

#if defined(CONFIG_EOL_IS_CR)
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#  undef  CONFIG_EOL_IS_EITHER_CRLF
#elif defined(CONFIG_EOL_IS_LF)
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#  undef  CONFIG_EOL_IS_EITHER_CRLF
#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_EITHER_CRLF
#elif defined(CONFIG_EOL_IS_EITHER_CRLF)
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#else
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#  define CONFIG_EOL_IS_EITHER_CRLF 1
#endif

/* Helper macros */

#define RL_GETC(v)      ((v)->rl_getc(v))

#ifdef CONFIG_READLINE_ECHO
#  define RL_PUTC(v,ch)   ((v)->rl_putc(v,ch))
#  define RL_WRITE(v,b,s) ((v)->rl_write(v,b,s))
#endif

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

struct rl_common_s
{
  int  (*rl_getc)(FAR struct rl_common_s *vtbl);
#ifdef CONFIG_READLINE_ECHO
  void (*rl_putc)(FAR struct rl_common_s *vtbl, int ch);
  void (*rl_write)(FAR struct rl_common_s *vtbl, FAR const char *buffer,
                   size_t buflen);
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: readline_common
 *
 *   Common logic shared by various readline frone-ends (currently only
 *   readline()).
 *
 * Input Parameters:
 *   buf       - The user allocated buffer to be filled.
 *   buflen    - the size of the buffer.
 *   instream  - The stream to read characters from
 *   outstream - The stream to each characters to.
 *
 * Returned values:
 *   On success, the (positive) number of bytes transferred is returned.
 *   EOF is returned to indicate either an end of file condition or a
 *   failure.
 *
 ****************************************************************************/

ssize_t readline_common(FAR struct rl_common_s *vtbl,
                        FAR char *buf, int buflen);

#endif /* __APPS_SYSTEM_READLINE_READLINE_H */
