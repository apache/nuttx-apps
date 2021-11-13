/****************************************************************************
 * apps/system/readline/readline_fd.c
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

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "system/readline.h"
#include "readline.h"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct readline_s
{
  struct rl_common_s vtbl;
  int infd;
#ifdef CONFIG_READLINE_ECHO
  int outfd;
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: readline_getc
 ****************************************************************************/

static int readline_getc(FAR struct rl_common_s *vtbl)
{
  FAR struct readline_s *priv = (FAR struct readline_s *)vtbl;
  char buffer;
  ssize_t nread;

  DEBUGASSERT(priv);

  /* Loop until we successfully read a character (or until an unexpected
   * error occurs).
   */

  do
    {
      /* Read one character from the incoming stream */

      nread = read(priv->infd, &buffer, 1);

      /* Check for end-of-file. */

      if (nread == 0)
        {
          /* Return EOF on end-of-file */

          return EOF;
        }

      /* Check if an error occurred */

      else if (nread < 0)
        {
          /* EINTR is not really an error; it simply means that a signal was
           * received while waiting for input.
           */

          int errcode = errno;
          if (errcode != EINTR)
            {
              /* Return EOF on any errors that we cannot handle */

              return EOF;
            }
        }
    }
  while (nread < 1);

  /* On success, return the character that was read */

  return (int)buffer;
}

/****************************************************************************
 * Name: readline_putc
 ****************************************************************************/

#ifdef CONFIG_READLINE_ECHO
static void readline_putc(FAR struct rl_common_s *vtbl, int ch)
{
  FAR struct readline_s *priv = (FAR struct readline_s *)vtbl;
  char buffer = ch;
  ssize_t nwritten;

  DEBUGASSERT(priv);

  /* Loop until we successfully write a character (or until an unexpected
   * error occurs).
   */

  do
    {
      /* Write the character to the outgoing stream */

      nwritten = write(priv->outfd, &buffer, 1);

      /* Check for irrecoverable write errors. */

      if (nwritten < 0 && errno != EINTR)
        {
          break;
        }
    }
  while (nwritten < 1);
}
#endif

/****************************************************************************
 * Name: readline_write
 ****************************************************************************/

#ifdef CONFIG_READLINE_ECHO
static void readline_write(FAR struct rl_common_s *vtbl,
                           FAR const char *buffer, size_t buflen)
{
  FAR struct readline_s *priv = (FAR struct readline_s *)vtbl;
  DEBUGASSERT(priv && buffer && buflen > 0);

  write(priv->outfd, buffer, buflen);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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

ssize_t readline_fd(FAR char *buf, int buflen, int infd, int outfd)
{
  struct readline_s vtbl;

  /* Set up the vtbl structure */

  vtbl.vtbl.rl_getc  = readline_getc;
  vtbl.infd          = infd;

#ifdef CONFIG_READLINE_ECHO
  vtbl.vtbl.rl_putc  = readline_putc;
  vtbl.vtbl.rl_write = readline_write;
  vtbl.outfd         = outfd;
#endif

  /* The let the common readline logic do the work */

  return readline_common(&vtbl.vtbl, buf, buflen);
}
