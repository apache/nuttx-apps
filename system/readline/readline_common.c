/****************************************************************************
 * apps/system/readline/readline_common.c
 *
 *   Copyright (C) 2007-2008, 2011-2013, 2015 Gregory Nutt. All rights reserved.
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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/ascii.h>
#include <nuttx/vt100.h>
#include <nuttx/binfmt/builtin.h>

#include <apps/readline.h>
#include "readline.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* <esc>[K is the VT100 command erases to the end of the line. */

static const char g_erasetoeol[] = VT100_CLEAREOL;

#ifdef CONFIG_READLINE_TABCOMPLETION
/* Prompt string to present at the beginning of the line */

static FAR const char *g_readline_prompt = NULL;

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
static FAR const struct extmatch_vtable_s *g_extmatch_vtbl = NULL;
#endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: count_builtin_maches
 *
 * Description:
 *   Count the number of builtin commands
 *
 * Input Parameters:
 *   matches - Array to save builtin command index.
 *   len - The length of the matching name to try
 *
 * Returned Value:
 *   The number of matching names
 *
 **************************************************************************/

#if defined(CONFIG_READLINE_TABCOMPLETION) && defined(CONFIG_BUILTIN)
static int count_builtin_maches(FAR char *buf, FAR int *matches, int namelen)
{
  FAR const char *name;
  int nr_matches = 0;
  int i;

  for (i = 0; (name = builtin_getname(i)) != NULL; i++)
    {
      if (strncmp(buf, name, namelen) == 0)
        {
          matches[nr_matches] = i;
          nr_matches++;

          if (nr_matches >= CONFIG_READLINE_MAX_BUILTINS)
            {
              break;
            }
        }
    }

  return nr_matches;
}
#endif

/****************************************************************************
 * Name: tab_completion
 *
 * Description:
 *   Nghia - Unix like tab completion, only for builtin apps
 *
 * Input Parameters:
 *   vtbl   - vtbl used to access implementation specific interface
 *   buf     - The user allocated buffer to be filled.
 *   buflen  - the size of the buffer.
 *
 * Returned Value:
 *   None.
 *
 **************************************************************************/

#ifdef CONFIG_READLINE_TABCOMPLETION
static void tab_completion(FAR struct rl_common_s *vtbl, char *buf,
                           int *nch)
{
  FAR const char *name = NULL;
#ifdef CONFIG_BUILTIN
  int nr_builtin_matches = 0;
  int builtin_matches[CONFIG_READLINE_MAX_BUILTINS];
#endif
#ifdef CONFIG_BUILTIN
  int nr_ext_matches = 0;
  int ext_matches[CONFIG_READLINE_MAX_EXTCMDS];
#endif
  int nr_matches;
  int len = *nch;
  int i;
  int j;

  if (len >= 1)
    {
#ifdef CONFIG_BUILTIN
      /* Count the matching builtin commands */

      nr_builtin_matches = count_builtin_maches(buf, builtin_matches, len);
      nr_matches         = nr_builtin_matches;
#else
      nr_matches         = 0;
#endif

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
      /* Is there registered external handling logic? */

      nr_ext_matches     = 0;
      if (g_extmatch_vtbl != NULL)
        {
          /* Count the number of external commands */

          nr_ext_matches = g_extmatch_vtbl->count_matches(buf, ext_matches, len);
          nr_matches    += nr_ext_matches;
        }

#endif

      /* Is there only one matching name? */

      if (nr_matches ==  1)
        {
          int name_len;

          /* Yes... that that is the one we want.  Was it a match with a
           * builtin command?  Or with an external command.
           */

#ifdef CONFIG_BUILTIN
#ifdef CONFIG_READLINE_HAVE_EXTMATCH
          if (nr_builtin_matches ==  1)
#endif
            {
              /* It is a match with a builtin command */

              name = builtin_getname(builtin_matches[0]);
            }
#endif

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
#ifdef CONFIG_BUILTIN
          else
#endif
            {
              /* It is a match with an external command */

              name = g_extmatch_vtbl->getname(ext_matches[0]);
            }
#endif

          /* Copy the name to the command buffer and to the display. */

          name_len = strlen(name);

          for (j = len; j < name_len; j++)
            {
              buf[j] = name[j];
              RL_PUTC(vtbl, name[j]);
            }

          /* Don't remove extra characters after the completed word, if any. */

          if (len < name_len)
            {
              *nch = name_len;
            }
        }

      /* Are there multiple matching names? */

      else if (nr_matches > 1)
        {
          RL_PUTC(vtbl, '\n');

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
          /* Show the possible external completions */

          for (i = 0; i < nr_ext_matches; i++)
            {
              name = g_extmatch_vtbl->getname(ext_matches[i]);

              RL_PUTC(vtbl, ' ');
              RL_PUTC(vtbl, ' ');

              for (j = 0; j < strlen(name); j++)
                {
                  RL_PUTC(vtbl, name[j]);
                }

              RL_PUTC(vtbl, '\n');
            }
#endif

#ifdef CONFIG_BUILTIN
          /* Show the possible builtin completions */

          for (i = 0; i < nr_builtin_matches; i++)
            {
              name = builtin_getname(builtin_matches[i]);

              RL_PUTC(vtbl, ' ');
              RL_PUTC(vtbl, ' ');

              for (j = 0; j < strlen(name); j++)
                {
                  RL_PUTC(vtbl, name[j]);
                }

              RL_PUTC(vtbl, '\n');
            }
#endif

          /* Output the original prompt */

          if (g_readline_prompt != NULL)
            {
              for (i = 0; i < strlen(g_readline_prompt); i++)
                {
                  RL_PUTC(vtbl, g_readline_prompt[i]);
                }
            }

          for (i = 0; i < len; i++)
            {
              RL_PUTC(vtbl, buf[i]);
            }
        }
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: readline_prompt
 *
 *   If a prompt string is used by the application, then the application
 *   must provide the prompt string to readline by calling this function.
 *   This is needed only for tab completion in cases where is it necessary
 *   to reprint the prompt string.
 *
 * Input Parameters:
 *   prompt    - The prompt string. This function may then be
 *   called with that value in order to restore the previous vtable.
 *
 * Returned values:
 *   Returns the previous value of the prompt string
 *
 * Assumptions:
 *   The prompt string is statically allocated a global.  readline will
 *   simply remember the pointer to the string.  The string must stay
 *   allocated and available.  Only one prompt string is supported.  If
 *   there are multiple clients of readline, they must all share the same
 *   prompt string (with exceptions in the case of the kernel build).
 *
 **************************************************************************/

#ifdef CONFIG_READLINE_TABCOMPLETION
FAR const char *readline_prompt(FAR const char *prompt)
{
  FAR const char *ret = g_readline_prompt;
  g_readline_prompt = prompt;
  return ret;
}
#endif

/****************************************************************************
 * Name: readline_extmatch
 *
 *   If the applications supports a command set, then it may call this
 *   function in order to provide support for tab complete on these\
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
 *   The vtbl string is statically allocated a global.  readline will
 *   simply remember the pointer to the structure.  The structure must stay
 *   allocated and available.  Only one instance of such a structure is 
 *   upported.  If there are multiple clients of readline, they must all
 *   share the same tab-completion logic (with exceptions in the case of
 *   the kernel build).
 *
 **************************************************************************/

#if defined(CONFIG_READLINE_TABCOMPLETION) && defined(CONFIG_READLINE_HAVE_EXTMATCH)
FAR const struct extmatch_vtable_s *
  readline_extmatch(FAR const struct extmatch_vtable_s *vtbl)
{
  FAR const struct extmatch_vtable_s *ret = g_extmatch_vtbl;
  g_extmatch_vtbl = vtbl;
  return ret;
}
#endif

/****************************************************************************
 * Name: readline_common
 *
 * Description:
 *   readline() reads in at most one less than 'buflen' characters from
 *   'instream' and stores them into the buffer pointed to by 'buf'.
 *   Characters are echoed on 'outstream'.  Reading stops after an EOF or a
 *   newline.  If a newline is read, it is stored into the buffer.  A null
 *   terminator is stored after the last character in the buffer.
 *
 *   This version of realine assumes that we are reading and writing to
 *   a VT100 console.  This will not work well if 'instream' or 'outstream'
 *   corresponds to a raw byte steam.
 *
 *   This function is inspired by the GNU readline but is an entirely
 *   different creature.
 *
 * Input Parameters:
 *   vtbl   - vtbl used to access implementation specific interface
 *   buf     - The user allocated buffer to be filled.
 *   buflen  - the size of the buffer.
 *
 * Returned Value:
 *   On success, the (positive) number of bytes transferred is returned.
 *   EOF is returned to indicate either an end of file condition or a
 *   failure.
 *
 **************************************************************************/

ssize_t readline_common(FAR struct rl_common_s *vtbl, FAR char *buf, int buflen)
{
  int  escape;
  int  nch;

  /* Sanity checks */

  DEBUGASSERT(buf && buflen > 0);

  if (buflen < 2)
    {
      *buf = '\0';
      return 0;
    }

  /* <esc>[K is the VT100 command that erases to the end of the line. */

#ifdef CONFIG_READLINE_ECHO
  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
#endif

  /* Read characters until we have a full line. On each the loop we must
   * be assured that there are two free bytes in the line buffer:  One for
   * the next character and one for the null terminator.
   */

  escape = 0;
  nch    = 0;

  for (;;)
    {
      /* Get the next character. readline_rawgetc() returns EOF on any
       * errors or at the end of file.
       */

      int ch = RL_GETC(vtbl);

      /* Check for end-of-file or read error */

      if (ch == EOF)
        {
          /* Did we already received some data? */

          if (nch > 0)
            {
              /* Yes.. Terminate the line (which might be zero length)
               * and return the data that was received.  The end-of-file
               * or error condition will be reported next time.
               */

              buf[nch] = '\0';
              return nch;
            }

          return EOF;
        }

      /* Are we processing a VT100 escape sequence */

      else if (escape)
        {
          /* Yes, is it an <esc>[, 3 byte sequence */

          if (ch != ASCII_LBRACKET || escape == 2)
            {
              /* We are finished with the escape sequence */

              escape = 0;
              ch = 'a';
            }
          else
            {
              /* The next character is the end of a 3-byte sequence.
               * NOTE:  Some of the <esc>[ sequences are longer than
               * 3-bytes, but I have not encountered any in normal use
               * yet and, so, have not provided the decoding logic.
               */

              escape = 2;
            }
        }

      /* Check for backspace
       *
       * There are several notions of backspace, for an elaborate summary see
       * http://www.ibb.net/~anne/keyboard.html. There is no clean solution.
       * Here both DEL and backspace are treated like backspace here.  The
       * Unix/Linux screen terminal by default outputs  DEL (0x7f) when the
       * backspace key is pressed.
       */

      else if (ch == ASCII_BS || ch == ASCII_DEL)
        {
          /* Eliminate that last character in the buffer. */

          if (nch > 0)
            {
              nch--;

#ifdef CONFIG_READLINE_ECHO
              /* Echo the backspace character on the console.  Always output
               * the backspace character because the VT100 terminal doesn't
               * understand DEL properly.
               */

              RL_PUTC(vtbl, ASCII_BS);
              RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
#endif
            }
        }

      /* Check for the beginning of a VT100 escape sequence */

      else if (ch == ASCII_ESC)
        {
          /* The next character is escaped */

          escape = 1;
        }

      /* Check for end-of-line.  This is tricky only in that some
       * environments may return CR as end-of-line, others LF, and
       * others both.
       */

#if  defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF)
      else if (ch == '\n')
#elif defined(CONFIG_EOL_IS_CR)
      else if (ch == '\r')
#elif CONFIG_EOL_IS_EITHER_CRLF
      else if (ch == '\n' || ch == '\r')
#endif
        {
          /* The newline is stored in the buffer along with the null
           * terminator.
           */

          buf[nch++] = '\n';
          buf[nch]   = '\0';

#ifdef CONFIG_READLINE_ECHO
          /* Echo the newline to the console */

          RL_PUTC(vtbl, '\n');
#endif
          return nch;
        }

      /* Otherwise, check if the character is printable and, if so, put the
       * character in the line buffer
       */

      else if (isprint(ch))
        {
          buf[nch++] = ch;

#ifdef CONFIG_READLINE_ECHO
          /* Echo the character to the console */

          RL_PUTC(vtbl, ch);
#endif
          /* Check if there is room for another character and the line's
           * null terminator.  If not then we have to end the line now.
           */

          if (nch + 1 >= buflen)
            {
              buf[nch] = '\0';
              return nch;
            }
        }
#ifdef CONFIG_READLINE_TABCOMPLETION
     else if (ch == '\t') /* Nghia - TAB character */
        {
          tab_completion(vtbl, buf, &nch);
        }
#endif
    }
}
