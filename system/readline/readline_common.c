/****************************************************************************
 * apps/system/readline/readline_common.c
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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/ascii.h>
#include <nuttx/vt100.h>
#include <nuttx/lib/builtin.h>

#include "system/readline.h"
#include "readline.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_READLINE_CMD_HISTORY
#  define RL_CMDHIST_LEN        CONFIG_READLINE_CMD_HISTORY_LEN
#  define RL_CMDHIST_LINELEN    CONFIG_READLINE_CMD_HISTORY_LINELEN
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_READLINE_CMD_HISTORY
struct cmdhist_s
{
  char buf[RL_CMDHIST_LEN][RL_CMDHIST_LINELEN];  /* Circular buffer */
  int  head;                                     /* Head of the circular buffer */
  int  offset;                                   /* Offset from head */
  int  len;                                      /* Size of the circular buffer */
};
#endif /* CONFIG_READLINE_CMD_HISTORY */

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
#endif /* CONFIG_READLINE_TABCOMPLETION */

#ifdef CONFIG_READLINE_CMD_HISTORY
static struct cmdhist_s g_cmdhist;
#endif /* CONFIG_READLINE_CMD_HISTORY */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: count_builtin_matches
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
 ****************************************************************************/

#if defined(CONFIG_READLINE_TABCOMPLETION) && defined(CONFIG_BUILTIN)
static int count_builtin_matches(FAR char *buf, FAR int *matches,
                                 int namelen)
{
#if CONFIG_READLINE_MAX_BUILTINS > 0
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

#else
  return 0;
#endif
}
#endif

/****************************************************************************
 * Name: tab_completion
 *
 * Description:
 *   Unix like tab completion, only for builtin apps
 *
 * Input Parameters:
 *   vtbl   - vtbl used to access implementation specific interface
 *   buf     - The user allocated buffer to be filled.
 *   buflen  - the size of the buffer.
 *   nch     - the number of characters.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

#ifdef CONFIG_READLINE_TABCOMPLETION
static void tab_completion(FAR struct rl_common_s *vtbl, char *buf,
                           int buflen, int *nch)
{
  FAR const char *name = NULL;
  char tmp_name[CONFIG_TASK_NAME_SIZE + 1];
#ifdef CONFIG_BUILTIN
  int nr_builtin_matches = 0;
  int builtin_matches[CONFIG_READLINE_MAX_BUILTINS];
#endif
#ifdef CONFIG_READLINE_HAVE_EXTMATCH
  int nr_ext_matches = 0;
  int ext_matches[CONFIG_READLINE_MAX_EXTCMDS];
#endif
  int nr_matches;
  int len = *nch;
  int name_len;
  int i;
  int j;

  if (len >= 1)
    {
#ifdef CONFIG_BUILTIN
      /* Count the matching builtin commands */

      nr_builtin_matches = count_builtin_matches(buf, builtin_matches, len);
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

          nr_ext_matches =
            g_extmatch_vtbl->count_matches(buf, ext_matches, len);
          nr_matches    += nr_ext_matches;
        }

#endif

      /* Is there only one matching name? */

      if (nr_matches == 1)
        {
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

          /* Don't remove extra characters after the completed word,
           * if any.
           */

          if (len < name_len)
            {
              *nch = name_len;
            }
        }

      /* Are there multiple matching names? */

      else if (nr_matches > 1)
        {
          RL_PUTC(vtbl, '\n');

          /* See how many characters we can auto complete for the user
           * For example, if we have the following commands:
           * - prog1
           * - prog2
           * - prog3
           * then it should automatically complete up to prog.
           * We do this in one pass using a temp.
           */

          memset(tmp_name, 0, sizeof(tmp_name));

#ifdef CONFIG_READLINE_HAVE_EXTMATCH
          /* Show the possible external completions */

          for (i = 0; i < nr_ext_matches; i++)
            {
              name = g_extmatch_vtbl->getname(ext_matches[i]);

              /* Initialize temp */

              if (tmp_name[0] == '\0')
                {
                  strncpy(tmp_name, name, sizeof(tmp_name) - 1);
                }

              RL_PUTC(vtbl, ' ');
              RL_PUTC(vtbl, ' ');

              for (j = 0; j < strlen(name); j++)
                {
                  /* Removing characters that aren't common to all the
                   * matches.
                   */

                  if (j < sizeof(tmp_name) && name[j] != tmp_name[j])
                    {
                      tmp_name[j] = '\0';
                    }

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

              /* Initialize temp */

              if (tmp_name[0] == '\0')
                {
                  strncpy(tmp_name, name, sizeof(tmp_name) - 1);
                }

              RL_PUTC(vtbl, ' ');
              RL_PUTC(vtbl, ' ');

              for (j = 0; j < strlen(name); j++)
                {
                  /* Removing characters that aren't common to all the
                   * matches.
                   */

                  if (j < sizeof(tmp_name) && name[j] != tmp_name[j])
                    {
                      tmp_name[j] = '\0';
                    }

                  RL_PUTC(vtbl, name[j]);
                }

              RL_PUTC(vtbl, '\n');
            }

#endif
          strncpy(buf, tmp_name, buflen - 1);

          name_len = strlen(tmp_name);

          /* Output the original prompt */

          if (g_readline_prompt != NULL)
            {
              for (i = 0; i < strlen(g_readline_prompt); i++)
                {
                  RL_PUTC(vtbl, g_readline_prompt[i]);
                }
            }

          for (i = 0; i < name_len; i++)
            {
              RL_PUTC(vtbl, buf[i]);
            }

          /* Don't remove extra characters after the completed word,
           * if any
           */

          if (len < name_len)
            {
              *nch = name_len;
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

#if defined(CONFIG_READLINE_TABCOMPLETION) && defined(CONFIG_READLINE_HAVE_EXTMATCH)
FAR const struct extmatch_vtable_s *
  readline_extmatch(FAR const struct extmatch_vtable_s *vtbl)
{
#if (CONFIG_READLINE_MAX_EXTCMDS > 0)
  FAR const struct extmatch_vtable_s *ret = g_extmatch_vtbl;
  g_extmatch_vtbl = vtbl;
  return ret;
#else
  return NULL;
#endif
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
 *   vtbl    - vtbl used to access implementation specific interface
 *   buf     - The user allocated buffer to be filled.
 *   buflen  - the size of the buffer.
 *
 * Returned Value:
 *   On success, the (positive) number of bytes transferred is returned.
 *   EOF is returned to indicate either an end of file condition or a
 *   failure.
 *
 ****************************************************************************/

ssize_t readline_common(FAR struct rl_common_s *vtbl, FAR char *buf,
                        int buflen)
{
  int escape;
  int nch;
#ifdef CONFIG_READLINE_CMD_HISTORY
  int i;
#endif

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

  for (; ; )
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

#ifdef CONFIG_READLINE_CMD_HISTORY
              /* Intercept up and down arrow keys */

              if (g_cmdhist.len > 0)
                {
                  if (ch == 'A') /* up arrow */
                    {
                      /* Go to the past command in history */

                      g_cmdhist.offset--;

                      if (-g_cmdhist.offset >= g_cmdhist.len)
                        {
                          g_cmdhist.offset = -(g_cmdhist.len - 1);
                        }
                    }
                  else if (ch == 'B') /* down arrow */
                    {
                      /* Go to the recent command in history */

                      g_cmdhist.offset++;

                      if (g_cmdhist.offset > 1)
                        {
                          g_cmdhist.offset = 1;
                        }
                    }

                  /* Clear out current command from the prompt */

                  while (nch > 0)
                    {
                      nch--;

#ifdef CONFIG_READLINE_ECHO
                      RL_PUTC(vtbl, ASCII_BS);
                      RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
#endif
                    }

                  if (g_cmdhist.offset != 1)
                    {
                      int idx = g_cmdhist.head + g_cmdhist.offset;

                      /* Circular buffer wrap around */

                      if (idx < 0)
                        {
                          idx = idx + RL_CMDHIST_LEN;
                        }
                      else if (idx >= RL_CMDHIST_LEN)
                        {
                          idx = idx - RL_CMDHIST_LEN;
                        }

                      for (i = 0; g_cmdhist.buf[idx][i] != '\0'; i++)
                        {
                          buf[nch++] = g_cmdhist.buf[idx][i];
                          RL_PUTC(vtbl, g_cmdhist.buf[idx][i]);
                        }

                      buf[nch] = '\0';
                    }
                }
#endif /* CONFIG_READLINE_CMD_HISTORY */

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
#elif defined(CONFIG_EOL_IS_EITHER_CRLF)
      else if (ch == '\n' || ch == '\r')
#endif
        {
#ifdef CONFIG_READLINE_CMD_HISTORY
          /* Save history of command, only if there was something
           * typed besides return character.
           */

          if (nch >= 1)
            {
              /* If this command is the one at the top of the circular
               * buffer, don't save it again.
               */

              if (strncmp(buf, g_cmdhist.buf[g_cmdhist.head], nch + 1) != 0)
                {
                  g_cmdhist.head = (g_cmdhist.head + 1) % RL_CMDHIST_LEN;

                  for (i = 0; (i < nch) && i < (RL_CMDHIST_LINELEN - 1); i++)
                    {
                      g_cmdhist.buf[g_cmdhist.head][i] = buf[i];
                    }

                  g_cmdhist.buf[g_cmdhist.head][i] = '\0';

                  if (g_cmdhist.len < RL_CMDHIST_LEN)
                    {
                      g_cmdhist.len++;
                    }
                }

              g_cmdhist.offset = 1;
            }
#endif /* CONFIG_READLINE_CMD_HISTORY */

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

      /* Otherwise, put the character in the line buffer if the
       * character is not a control byte
       */

      else if (!iscntrl(ch & 0xff))
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
      else if (ch == '\t') /* TAB character */
        {
          tab_completion(vtbl, buf, buflen, &nch);
        }
#endif
    }
}
