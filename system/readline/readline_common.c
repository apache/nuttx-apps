/****************************************************************************
 * apps/system/readline/readline_common.c
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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <nuttx/debug.h>

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

#ifdef CONFIG_READLINE_ECHO
static const char g_erasetoeol[] = VT100_CLEAREOL;
#endif
#ifdef CONFIG_READLINE_EDIT
static const char g_curleft[]  = {ASCII_ESC, '[', 'D'};
static const char g_curright[] = {ASCII_ESC, '[', 'C'};
#endif

#ifdef CONFIG_READLINE_EDIT_EMACS
/* Emacs-style control key codes */

#  define CTRL_A  1   /* ^A - Home */
#  define CTRL_B  2   /* ^B - Left */
#  define CTRL_D  4   /* ^D - Delete at cursor */
#  define CTRL_E  5   /* ^E - End */
#  define CTRL_F  6   /* ^F - Right */
#  define CTRL_K  11  /* ^K - Kill to end of line */
#  define CTRL_U  21  /* ^U - Kill to beginning of line */
#  define CTRL_W  23  /* ^W - Kill word backward */
#  ifdef CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH
#    define CTRL_G  7   /* ^G - Cancel incremental search */
#    define CTRL_R  18  /* ^R - Reverse incremental search */
#  endif
#endif

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
 *   True if the completion changed 'buf'/'*nch' and/or echoed anything
 *   to the terminal (a single unambiguous match was appended, or the
 *   multiple-match list was printed -- which always ends with a full
 *   reprint of the prompt and buffer, even if the common prefix did
 *   not grow).  False if nothing happened at all (no match, or an
 *   exact single match with nothing left to add), in which case the
 *   terminal's cursor never moved.
 *
 ****************************************************************************/

#ifdef CONFIG_READLINE_TABCOMPLETION
static bool tab_completion(FAR struct rl_common_s *vtbl, char *buf,
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
              return true;
            }

          return false;
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
                  strlcpy(tmp_name, name, sizeof(tmp_name));
                }

              RL_PUTC(vtbl, ' ');
              RL_PUTC(vtbl, ' ');

              for (j = 0; j < (int)strlen(name); j++)
                {
                  /* Removing characters that aren't common to all the
                   * matches.
                   */

                  if (j < (int)sizeof(tmp_name) && name[j] != tmp_name[j])
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
                  strlcpy(tmp_name, name, sizeof(tmp_name));
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
          strlcpy(buf, tmp_name, buflen);
          name_len = strlen(tmp_name);

          /* Output the original prompt */

          if (g_readline_prompt != NULL)
            {
              for (i = 0; i < (int)strlen(g_readline_prompt); i++)
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

          /* Don't remove extra characters after the completed word,
           * if any
           */

          if (len < name_len)
            {
              *nch = name_len;
            }

          /* Whether or not the common prefix grew, the prompt and
           * buffer were just reprinted from scratch above, so the
           * terminal's cursor is now at the end of the line either
           * way.
           */

          return true;
        }
    }

  return false;
}
#endif

#ifdef CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH
/****************************************************************************
 * Name: isearch_find
 *
 * Description:
 *   Used by Ctrl+R (reverse incremental search).  Search backward
 *   through the command history, starting just before 'startoffset',
 *   for the most recent entry containing 'search' as a substring (using
 *   the same head/offset addressing as the up/down arrow history
 *   recall code above).  If a match is found, it is copied into 'buf'
 *   (updating '*nch'), and its offset is returned via '*foundoffset' so
 *   that a subsequent call can continue the search further back in
 *   history.  If no match is found, 'buf'/'*nch' are left unmodified.
 *
 * Returned Value:
 *   True if a match was found, false otherwise.
 *
 ****************************************************************************/

static bool isearch_find(FAR const char *search, int searchlen,
                          int startoffset, FAR int *foundoffset,
                          FAR char *buf, int buflen, FAR int *nch)
{
  int minoffset;
  int offset;
  int idx;
  int len;
  int i;
  int j;
  bool matched;

  if (searchlen == 0 || g_cmdhist.len == 0)
    {
      return false;
    }

  minoffset = -(g_cmdhist.len - 1);

  for (offset = startoffset - 1; offset >= minoffset; offset--)
    {
      idx = g_cmdhist.head + offset;

      if (idx < 0)
        {
          idx += RL_CMDHIST_LEN;
        }
      else if (idx >= RL_CMDHIST_LEN)
        {
          idx -= RL_CMDHIST_LEN;
        }

      len = strlen(g_cmdhist.buf[idx]);

      /* Does this history entry contain 'search' anywhere?  'search'
       * is a raw character buffer that is never null-terminated (the
       * caller only tracks its length in 'searchlen'), so this cannot
       * use strstr() -- do a plain bounded substring search instead.
       */

      matched = false;

      for (i = 0; i + searchlen <= len; i++)
        {
          for (j = 0; j < searchlen; j++)
            {
              if (g_cmdhist.buf[idx][i + j] != search[j])
                {
                  break;
                }
            }

          if (j == searchlen)
            {
              matched = true;
              break;
            }
        }

      if (!matched)
        {
          continue;
        }

      if (len > buflen - 1)
        {
          len = buflen - 1;
        }

      for (i = 0; i < len; i++)
        {
          buf[i] = g_cmdhist.buf[idx][i];
        }

      buf[len]     = '\0';
      *nch         = len;
      *foundoffset = offset;
      return true;
    }

  return false;
}
#endif

#ifdef CONFIG_READLINE_ECHO
#ifdef CONFIG_READLINE_EDIT
/****************************************************************************
 * Name: redraw_line
 *
 * Description:
 *   Redraw the prompt and the line buffer from scratch: return to the
 *   true start of the terminal line, erase to the end of line, reprint
 *   the prompt (if any), reprint the buffer, then move the cursor left
 *   as many times as needed to visually land on 'cursor'.  This does
 *   not depend on where the terminal's cursor happened to be beforehand
 *   -- unlike a backspace-based erase, it is correct no matter where
 *   the cursor was left by whatever came before it.
 *
 *   This is the common tail end of every full-line redraw in this file
 *   (Home, End, Ctrl+Left/Right, history recall, ...); consolidating it
 *   here instead of repeating the same half-dozen lines at each call
 *   site saves a fair amount of code space.
 *
 ****************************************************************************/

static void redraw_line(FAR struct rl_common_s *vtbl, FAR const char *buf,
                         int nch, int cursor)
{
  int i;

  RL_PUTC(vtbl, '\r');
  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));

#ifdef CONFIG_READLINE_TABCOMPLETION
  if (g_readline_prompt != NULL)
    {
      for (i = 0; g_readline_prompt[i] != '\0'; i++)
        {
          RL_PUTC(vtbl, g_readline_prompt[i]);
        }
    }
#endif

  if (nch > 0)
    {
      RL_WRITE(vtbl, buf, nch);
    }

  for (i = nch; i > cursor; i--)
    {
      RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
    }
}
#endif /* CONFIG_READLINE_EDIT */

#ifdef CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH
/****************************************************************************
 * Name: isearch_redraw
 *
 * Description:
 *   Redraw the reverse-incremental-search prompt line:
 *   "(reverse-i-search)`<search>': <match>", or
 *   "(failed reverse-i-search)`<search>': <match>" if the current
 *   search string has no match (in which case 'buf'/'nch' still hold
 *   whatever the last successful match was, exactly as bash does).
 *
 ****************************************************************************/

static void isearch_redraw(FAR struct rl_common_s *vtbl,
                            FAR const char *search, int searchlen,
                            FAR const char *buf, int nch, bool failed)
{
  static const char matchlabel[] = "(reverse-i-search)`";
  static const char faillabel[]  = "(failed reverse-i-search)`";

  RL_PUTC(vtbl, '\r');
  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));

  if (failed)
    {
      RL_WRITE(vtbl, faillabel, sizeof(faillabel) - 1);
    }
  else
    {
      RL_WRITE(vtbl, matchlabel, sizeof(matchlabel) - 1);
    }

  if (searchlen > 0)
    {
      RL_WRITE(vtbl, search, searchlen);
    }

  RL_PUTC(vtbl, '\'');
  RL_PUTC(vtbl, ':');
  RL_PUTC(vtbl, ' ');

  if (nch > 0)
    {
      RL_WRITE(vtbl, buf, nch);
    }
}
#endif /* CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH */
#endif /* CONFIG_READLINE_ECHO */

/****************************************************************************
 * Name: submit_line
 *
 * Description:
 *   Finish a line of input: record it in the command history (if
 *   enabled and it isn't a duplicate of the most recent entry),
 *   terminate 'buf' with '\n' and a null terminator, and return the
 *   number of characters in 'buf' (including the trailing '\n').  This
 *   is shared by the normal Enter key handling and by Ctrl+R
 *   (reverse-i-search), which also submits the matched line directly
 *   on Enter.
 *
 ****************************************************************************/

static ssize_t submit_line(FAR char *buf, int nch)
{
#ifdef CONFIG_READLINE_CMD_HISTORY
  int i;

  /* Save history of command, only if there was something typed besides
   * the return character.
   */

  if (nch >= 1)
    {
      /* If this command is the one at the top of the circular buffer,
       * don't save it again.
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

  return nch;
}

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
#ifdef CONFIG_READLINE_EDIT
  volatile int cursor;
#endif
#ifdef CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH
  bool insearch;
  char search[RL_CMDHIST_LINELEN];
  int  searchlen;
  int  searchoffset;
  char savedbuf[RL_CMDHIST_LINELEN];
  int  savednch;
  int  savedcursor;
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
#ifdef CONFIG_READLINE_EDIT
  cursor  = 0;
#endif
#ifdef CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH
  insearch  = false;
  searchlen = 0;
#endif

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

#ifdef CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH
      /* Are we in reverse incremental search mode (Ctrl+R)?  If so,
       * every subsequent keystroke is interpreted as part of the
       * search until it is accepted, submitted, or cancelled.
       */

      else if (insearch)
        {
          if (ch == CTRL_R)
            {
              /* Repeat: search further back for another match of the
               * same search string.
               */

              bool found = isearch_find(search, searchlen, searchoffset,
                                         &searchoffset, buf, buflen, &nch);
#ifdef CONFIG_READLINE_ECHO
              isearch_redraw(vtbl, search, searchlen, buf, nch, !found);
#endif
            }
          else if (ch == ASCII_BS || ch == ASCII_DEL)
            {
              if (searchlen > 0)
                {
                  searchlen--;
                  searchoffset = 1;

                  if (searchlen > 0)
                    {
                      isearch_find(search, searchlen, searchoffset,
                                    &searchoffset, buf, buflen, &nch);
                    }
                  else
                    {
                      nch = 0;
                    }
                }

#ifdef CONFIG_READLINE_ECHO
              isearch_redraw(vtbl, search, searchlen, buf, nch,
                              searchlen > 0 && nch == 0);
#endif
            }
          else if (ch == CTRL_G || ch == ASCII_ETX)  /* ^G or ^C: cancel */
            {
              /* Cancel: restore the line exactly as it was before
               * Ctrl+R was pressed.
               */

              insearch = false;
              nch      = savednch;
              cursor   = savedcursor;

              for (i = 0; i < nch; i++)
                {
                  buf[i] = savedbuf[i];
                }

              buf[nch] = '\0';

#ifdef CONFIG_READLINE_ECHO
              redraw_line(vtbl, buf, nch, cursor);
#endif
            }
          else if (ch == '\n')
            {
              /* Accept the current match and submit it immediately,
               * exactly as bash does.
               */

              insearch = false;
              return submit_line(buf, nch);
            }
          else if (ch == ASCII_ESC)
            {
              /* Accept the current match into the line, then let the
               * escape sequence that follows (if any) be processed
               * normally against it on the next iteration(s).
               */

              insearch = false;
              cursor   = nch;
              escape   = 1;
            }
          else if (!iscntrl(ch & 0xff) && searchlen < RL_CMDHIST_LINELEN - 1)
            {
              search[searchlen++] = (char)ch;
              searchoffset = 1;

              bool found = isearch_find(search, searchlen, searchoffset,
                                         &searchoffset, buf, buflen, &nch);
#ifdef CONFIG_READLINE_ECHO
              isearch_redraw(vtbl, search, searchlen, buf, nch, !found);
#endif
            }
          else
            {
              /* Anything else (an unhandled control character) just
               * accepts the current match and returns to normal
               * editing.
               */

              insearch = false;
              cursor   = nch;

#ifdef CONFIG_READLINE_ECHO
              redraw_line(vtbl, buf, nch, cursor);
#endif
            }

          continue;
        }
#endif

      /* Are we processing a VT100 escape sequence */

      else if (escape)
        {
#ifdef CONFIG_READLINE_EDIT
          /* Delete key: ESC [ 3 ~ — waiting for '~' */

          if (escape == 3)
            {
              escape = 0;
              if (ch == '~' && cursor < nch)
                {
                  int k;
                  for (k = cursor + 1; k < nch; k++)
                    buf[k - 1] = buf[k];
                  nch--;
#  ifdef CONFIG_READLINE_ECHO
                  /* Back up 1 — terminal echo of '~' advanced cursor */

                  RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
                  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
                  if (cursor < nch)
                    {
                      RL_WRITE(vtbl, buf + cursor, nch - cursor);
                      for (k = nch; k > cursor; k--)
                        RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
                    }
#  endif
                }
              continue;
            }

          if (escape == 4 || escape == 5)
            {
              if (ch == '~')
                {
                  /* Home (1~) or End (4~) */

                  cursor = (escape == 4) ? 0 : nch;
                  redraw_line(vtbl, buf, nch, cursor);
                }

              if (ch == ';')
                {
                  escape = 8;
                  continue;
                }

              escape = 0;
              continue;
            }
#endif

#ifdef CONFIG_READLINE_EDIT
          if (escape == 8)          /* CSI ; — waiting for modifier digit */
            {
              if (ch == '5')
                {
                  escape = 9;       /* Ready for final char */
                  continue;
                }

              if (ch == ';')
                {
                  escape = 8;
                  continue;
                }

              escape = 0;
              continue;
            }

          if (escape == 9)          /* CSI ;5 — waiting for D or C */
            {
              escape = 0;

              if (ch == 'D')        /* Ctrl+Left */
                {
                  while (cursor > 0 && buf[cursor - 1] == ' ')
                    cursor--;
                  while (cursor > 0 && buf[cursor - 1] != ' ')
                    cursor--;

                  redraw_line(vtbl, buf, nch, cursor);
                }
              else if (ch == 'C')   /* Ctrl+Right */
                {
                  while (cursor < nch && buf[cursor] != ' ')
                    cursor++;
                  while (cursor < nch && buf[cursor] == ' ')
                    cursor++;

                  redraw_line(vtbl, buf, nch, cursor);
                }

              continue;
            }
#endif /* CONFIG_READLINE_EDIT */

          /* Some terminals (e.g. xterm) use the SS3 introducer "ESC O"
           * rather than CSI "ESC [" for Home/End (and, in application
           * cursor key mode, for the arrow keys too).  Recognize the
           * "ESC O" prefix here and fall into exactly the same
           * "terminator character" handling used for "ESC [ <x>" below,
           * by advancing to a dedicated state (6) that is treated the
           * same way state 2 (saw "ESC [") is treated once the final
           * byte arrives.  Without this, the 'O' is silently dropped
           * (falling through the unrecognized-sequence path below) and
           * the *next* byte (e.g. 'F' for End, 'H' for Home) is left to
           * be reprocessed as an ordinary printable character, which is
           * what produced the stray inserted "OF"/"OH" text.
           */

          if (escape == 1 && ch == ASCII_O)
            {
              escape = 6;
              continue;
            }

          /* Yes, is it an <esc>[, 3 byte sequence */

          if (ch != ASCII_LBRACKET || escape == 2 || escape == 6)
            {
              /* We are finished with the escape sequence */

#ifdef CONFIG_READLINE_CMD_HISTORY
              /* Intercept up and down arrow keys.  This must only run
               * for the up/down arrow keys themselves -- previously the
               * clear-and-reload logic below ran for *any* completed
               * escape sequence (Left, Right, Home, End, Delete, ...)
               * as long as some history existed, silently wiping
               * whatever the user was currently typing.
               */

              if (g_cmdhist.len > 0 && (ch == 'A' || ch == 'B'))
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
                  else /* down arrow */
                    {
                      /* Go to the recent command in history */

                      g_cmdhist.offset++;

                      if (g_cmdhist.offset > 1)
                        {
                          g_cmdhist.offset = 1;
                        }
                    }

                  /* Clear out current command from the prompt.
                   *
                   * This cannot assume the terminal's cursor is
                   * sitting at the end of the currently displayed
                   * text (i.e. at column 'nch') and simply backspace
                   * 'nch' times -- the cursor can be anywhere in the
                   * line (e.g. the user pressed Left one or more
                   * times before pressing Up/Down again), and
                   * backspacing more times than the cursor's actual
                   * distance from the end of the prompt walks back
                   * into and erases part of the prompt itself.
                   * Instead, return to the true start of the
                   * terminal line and erase to the end of line, then
                   * reprint the prompt -- this does not depend on
                   * where the cursor happened to be.
                   */

                  nch = 0;

#ifdef CONFIG_READLINE_ECHO
                  RL_PUTC(vtbl, '\r');
                  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
#ifdef CONFIG_READLINE_TABCOMPLETION
                  if (g_readline_prompt != NULL)
                    {
                      int k;
                      for (k = 0; g_readline_prompt[k] != '\0'; k++)
                        {
                          RL_PUTC(vtbl, g_readline_prompt[k]);
                        }
                    }
#endif
#endif

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

                      for (i = 0;
                           g_cmdhist.buf[idx][i] != '\0' &&
                           nch + 1 < buflen;
                           i++)
                        {
                          buf[nch++] = g_cmdhist.buf[idx][i];
                          RL_PUTC(vtbl, g_cmdhist.buf[idx][i]);
                        }

                      buf[nch] = '\0';
                    }

                  /* The line was just wholesale replaced -- the cursor
                   * must always end up in sync with the new length, or
                   * later edits (insert/delete) will index into buf[]
                   * using a stale, out-of-range cursor value.
                   */

#ifdef CONFIG_READLINE_EDIT
                  cursor = nch;
#endif
                }
#endif /* CONFIG_READLINE_CMD_HISTORY */

#ifdef CONFIG_READLINE_EDIT
              if (ch == '1')
                {
                  escape = 4;
                  continue;
                }

              if (ch == ';')
                {
                  escape = 8;
                  continue;
                }

              if (ch == '3')
                {
                  escape = 3;
                  continue;
                }

              if (ch == '4')
                {
                  escape = 5;
                  continue;
                }

              if (ch == 'F')
                {
                  escape = 0;
                  cursor = nch;

                  /* Redraw full line — cursor ends at end */

                  redraw_line(vtbl, buf, nch, cursor);
                  continue;
                }

              if (ch == 'D' && cursor > 0)
                {
                  cursor--;
                  escape = 0;
                  RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
                  continue;
                }

              if (ch == 'C' && cursor < nch)
                {
                  cursor++;
                  escape = 0;
                  RL_WRITE(vtbl, g_curright, sizeof(g_curright));
                  continue;
                }

              if (ch == 'H')
                {
                  escape = 0;
                  cursor = 0;

                  /* Redraw full line from column 0 */

                  redraw_line(vtbl, buf, nch, cursor);

                  continue;
                }
#endif

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
#ifdef CONFIG_READLINE_EDIT
          /* Delete character before cursor */

          if (cursor > 0)
            {
              int k;
              for (k = cursor; k < nch; k++)
                buf[k - 1] = buf[k];
              cursor--;
              nch--;

#  ifdef CONFIG_READLINE_ECHO
              /* Redraw: backspace, clear EOL, redraw tail, restore cursor */

              RL_PUTC(vtbl, ASCII_BS);
              RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
              if (cursor < nch)
                {
                  RL_WRITE(vtbl, buf + cursor, nch - cursor);
                  for (k = nch; k > cursor; k--)
                    RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
                }
#  endif
            }
#else
          if (nch > 0)
            {
              nch--;

#  ifdef CONFIG_READLINE_ECHO
              RL_PUTC(vtbl, ASCII_BS);
              RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
#  endif
            }
#endif
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

      else if (ch == '\n')
        {
          return submit_line(buf, nch);
        }

      /* Emacs-style control keys */

#ifdef CONFIG_READLINE_EDIT_EMACS
      else if (ch == CTRL_A)        /* Ctrl+A = Home */
        {
          cursor = 0;
          redraw_line(vtbl, buf, nch, cursor);
        }
      else if (ch == CTRL_B)            /* Ctrl+B = Left */
        {
          if (cursor > 0)
            {
              cursor--;
              RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
            }
        }
      else if (ch == CTRL_D)            /* Ctrl+D = Delete at cursor */
        {
          if (cursor < nch)
            {
              int k;
              for (k = cursor + 1; k < nch; k++)
                buf[k - 1] = buf[k];
              nch--;
              RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
              if (cursor < nch)
                {
                  RL_WRITE(vtbl, buf + cursor, nch - cursor);
                  for (k = nch; k > cursor; k--)
                    RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
                }
            }
        }
      else if (ch == CTRL_E)            /* Ctrl+E = End */
        {
          while (cursor < nch)
            {
              cursor++;
              RL_WRITE(vtbl, g_curright, sizeof(g_curright));
            }
        }
      else if (ch == CTRL_F)            /* Ctrl+F = Right */
        {
          if (cursor < nch)
            {
              cursor++;
              RL_WRITE(vtbl, g_curright, sizeof(g_curright));
            }
        }
      else if (ch == CTRL_K)           /* Ctrl+K = Kill to EOL */
        {
          nch = cursor;
          buf[nch] = '\0';
          RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
        }
      else if (ch == CTRL_U)           /* Ctrl+U = Kill to BOL */
        {
          int j;
          for (j = cursor; j < nch; j++)
            buf[j - cursor] = buf[j];
          nch -= cursor;
          cursor = 0;
          buf[nch] = '\0';
          redraw_line(vtbl, buf, nch, cursor);
        }
      else if (ch == CTRL_W)           /* Ctrl+W = Kill word backward (FIXME) */
        {
          int start, k;
          start = cursor;
          while (start > 0 && buf[start - 1] == ' ')
            start--;
          while (start > 0 && buf[start - 1] != ' ')
            start--;
          for (k = start; k < nch; k++)
            buf[k - (cursor - start)] = buf[k];
          nch -= (cursor - start);
          cursor = start;
          buf[nch] = '\0';
          RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
          if (cursor < nch)
            {
              RL_WRITE(vtbl, buf + cursor, nch - cursor);
              for (k = nch; k > cursor; k--)
                RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
            }
        }

#ifdef CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH
      else if (ch == CTRL_R && g_cmdhist.len > 0)
        {
          /* Ctrl+R = start a reverse incremental search through the
           * command history, bash-style.  Save the line currently
           * being edited so it can be restored if the search is
           * cancelled (Ctrl+G).
           */

          insearch     = true;
          searchlen    = 0;
          searchoffset = 1;

          savednch = nch;
          if (savednch > RL_CMDHIST_LINELEN - 1)
            {
              savednch = RL_CMDHIST_LINELEN - 1;
            }

          for (i = 0; i < savednch; i++)
            {
              savedbuf[i] = buf[i];
            }

          savedcursor = cursor;
          nch         = 0;

#ifdef CONFIG_READLINE_ECHO
          isearch_redraw(vtbl, search, searchlen, buf, nch, false);
#endif
        }
#endif /* CONFIG_READLINE_EDIT_EMACS_REVERSE_SEARCH */
#endif /* CONFIG_READLINE_EDIT_EMACS */

      /* Otherwise, put the character in the line buffer if the
       * character is not a control byte
       */

      else if (!iscntrl(ch & 0xff))
        {
#ifdef CONFIG_READLINE_EDIT
          /* Defensive check: 'cursor' must always satisfy
           * 0 <= cursor <= nch.  It should never be able to get out of
           * that range now, but clamp it rather than trust it blindly
           * -- indexing buf[] with an out-of-range cursor is a
           * memory-safety bug (a stale cursor previously let this
           * write arbitrarily far past the end of the caller's
           * buffer).
           */

          if (cursor < 0 || cursor > nch)
            {
              cursor = nch;
            }

          /* Only insert if there is room for the new character plus
           * the line's null terminator.  Checking this before writing
           * (rather than only after, as the non-editing path below
           * does) is what actually prevents the out-of-bounds write.
           */

          if (nch + 1 < buflen)
            {
              int j;
              for (j = nch; j > cursor; j--) buf[j] = buf[j - 1];
              buf[cursor] = (char)ch; nch++; cursor++;
#  ifdef CONFIG_READLINE_ECHO
              if (cursor < nch)
                {
                  RL_WRITE(vtbl, g_erasetoeol, sizeof(g_erasetoeol));
                  RL_WRITE(vtbl, buf + cursor, nch - cursor);
                  for (j = nch; j > cursor; j--)
                    RL_WRITE(vtbl, g_curleft, sizeof(g_curleft));
                }
#  endif
            }
#else
          buf[nch++] = ch;
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
          /* When tab_completion() finds a match (or lists several), it
           * always leaves the terminal's real cursor at the end of the
           * (possibly completed) line -- either by echoing the
           * appended characters one at a time, or, when it lists
           * multiple matches, by reprinting the prompt and the whole
           * buffer from scratch.  'cursor' has to be resynced to
           * match in that case, or the very next Left/Right/Home/End
           * keypress will move the terminal's cursor from column
           * 'nch' while still believing it is moving from wherever
           * 'cursor' was left (typically the length of the word
           * before completion) -- the two then stay out of sync for
           * the rest of the line.
           *
           * But if there was no match at all, tab_completion() does
           * not touch the terminal or the buffer, and 'cursor' must
           * be left exactly where it was -- unconditionally resyncing
           * it here would be just as wrong as never resyncing it,
           * only in the opposite direction.
           */

#ifdef CONFIG_READLINE_EDIT
          if (tab_completion(vtbl, buf, buflen, &nch))
            {
              cursor = nch;
            }
#else
          tab_completion(vtbl, buf, buflen, &nch);
#endif
        }
#endif
    }
}
