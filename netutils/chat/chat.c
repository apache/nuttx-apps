/****************************************************************************
 * apps/netutils/chat/chat.c
 *
 *   Copyright (C) 2016 Vladimir Komendantskiy. All rights reserved.
 *   Author: Vladimir Komendantskiy <vladimir@moixaenergy.com>
 *   Partly based on code by Max Nekludov <macscomp@gmail.com>
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

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chat.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHAT_TOKEN_SIZE    128

/****************************************************************************
 * Pivate types
 ****************************************************************************/

/* Type of singly-linked list of tokens */

struct chat_token
{
  FAR char* string;
  bool no_termin;
  FAR struct chat_token* next;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void chat_init(FAR struct chat *priv, FAR struct chat_ctl *ctl)
{
  DEBUGASSERT(priv != NULL && ctl != NULL && ctl->timeout >= 0);

  memcpy(&priv->ctl, ctl, sizeof(struct chat_ctl));
  priv->script  = NULL;
}

/* Linear one-pass tokenizer. */

static int chat_tokenise(FAR struct chat *priv,
                         FAR const char *script,
                         FAR struct chat_token **first_tok)
{
  FAR const char *cursor = script;   /* pointer to the current character */
  unsigned int quoted = 0;           /* two-valued:
                                      * 1: quoted (expecting a closing quote)
                                      * 0: not quoted */
  unsigned int escaped = 0;          /* two-valued */
  char tok_str[CHAT_TOKEN_SIZE];     /* current token buffer */
  int tok_pos = 0;                   /* current length of the current token */
  FAR struct chat_token *tok = NULL; /* the last complete token */
  bool no_termin;                    /* line termination property:
                                      * true if line terminator is deasserted */
  int ret = 0;

  /* Delimiter handler */

  int tok_on_delimiter(void)
  {
    if (!tok_pos && !quoted && !no_termin)
      {
        /* a) the first character in the script is a delimiter or
         * b) the previous character was a delimiter,
         * and in both cases it is not the empty string token,
         * hence skipping.
         */

        return 0;
      }

    /* Terminate the temporary */

    tok_str[tok_pos] = '\0';
    if (tok)
      {
        tok->next = malloc(sizeof(struct chat_token));

        /* The terminated token becomes previous */

        tok = tok->next;
      }
    else
      {
        /* There was no previous token */

        *first_tok = malloc(sizeof(struct chat_token));
        tok = *first_tok;
      }

    if (!tok)
      {
        /* out of memory */

        return -ENOMEM;
      }

    /* Copy the temporary */

    tok->string = strdup(tok_str);
    tok->no_termin = no_termin;

    /* Initialize the next token */

    tok->next = NULL;

    /* Reset the buffer position */

    tok_pos = 0;

    _info("%s (%d)\n", tok->string, tok->no_termin);
    return 0;
  }

  /* Tokenizer start */

  DEBUGASSERT(script != NULL);
  _info("%s\n", script);

  while (!ret && *cursor != '\0')
    {
      /* Assert line terminator */

      no_termin = false;
      switch (*cursor)
        {
        case '\\':
          if (!quoted)
            {
              if (escaped)
                {
                  tok_str[tok_pos++] = '\\';
                }

              escaped ^= 1;
            }
          else
            {
              /* Quoted */

              tok_str[tok_pos++] = '\\';
            }

          break;

        case '"':
          if (escaped)
            {
              tok_str[tok_pos++] = '"';
              escaped = 0;
            }
          else
            {
              /* Not escaped */

              if (quoted && !tok_pos)
                {
                  /* Empty string token */

                  ret = tok_on_delimiter();
                }

              quoted ^= 1;

              /* No effect on the position in the temporary */
            }

          break;

        case ' ':
        case '\n':
        case '\r':
        case '\t':
          if (quoted)
            {
              /* Append the quoted character to the temporary */

              tok_str[tok_pos++] = *cursor;
            }
          else
            {
              ret = tok_on_delimiter();
            }

          break;

        default:
          if (escaped)
            {
              switch (*cursor)
                {
                case 'n':
                  tok_str[tok_pos++] = '\n';
                  break;

                case 'r':
                  tok_str[tok_pos++] = '\r';
                  break;

                case 'c':
                  /* Deassert line terminator */

                  no_termin = true;
                  break;

                default:
                  ret = -EINVAL;
                  break;
                }

              escaped = 0;
            }
          else
            {
              /* Append the regular character to the temporary */

              tok_str[tok_pos++] = *cursor;
            }

          break;
        }

      /* Shift the cursor right through the input string */

      cursor++;
    }

  if (!ret && *cursor == '\0')
    {
      /* Treat the null terminator as an EOF delimiter */

      ret = tok_on_delimiter();
    }

  _info("result %d\n", ret);
  return ret;
}

/* Creates the internal representation of a tokenised chat script. */

static int chat_internalise(FAR struct chat *priv,
                            FAR struct chat_token *tok)
{
  unsigned int rhs = 0;  /* 1 if processing the right-hand side,
                          * 0 otherwise */
  FAR struct chat_line *line = NULL;
  int len; /* length of the destination string when variable */
  int ret = 0;

  while (tok && !ret)
    {
      DEBUGASSERT(tok->string);
      _info("(%c) %s\n", rhs ? 'R' : 'L', tok->string);

      if (!rhs)
        {
          /* Create a new line */

          if (!line)
            {
              /* First line */

              line = malloc(sizeof(struct chat_line));
              priv->script = line;
            }
          else
            {
              /* Subsequent line */

              line->next = malloc(sizeof(struct chat_line));
              line = line->next;
            }

          if (!line)
            {
              ret = -ENOMEM;
              break;
            }

          line->next = NULL;
        }

      if (rhs)
        {
          len = strlen(tok->string);
          if (!tok->no_termin)
            {
              /* Add space for the line terminator */

              len += 2;
            }

          line->rhs = malloc(len + 1);
          if (line->rhs)
            {
              /* Copy the token and add the line terminator as appropriate */

              sprintf(line->rhs, tok->no_termin ? "%s" : "%s\r\n", tok->string);
            }
          else
            {
              ret = -ENOMEM;
            }
        }
      else
        {
          if (!strcmp(tok->string, "ABORT"))
            {
              line->type = CHAT_LINE_TYPE_COMMAND;
              line->lhs.command = CHAT_COMMAND_ABORT;
            }
          else if (!strcmp(tok->string, "ECHO"))
            {
              line->type = CHAT_LINE_TYPE_COMMAND;
              line->lhs.command = CHAT_COMMAND_ECHO;
            }
          else if (!strcmp(tok->string, "PAUSE"))
            {
              line->type = CHAT_LINE_TYPE_COMMAND;
              line->lhs.command = CHAT_COMMAND_PAUSE;
            }
          else if (!strcmp(tok->string, "SAY"))
            {
              line->type = CHAT_LINE_TYPE_COMMAND;
              line->lhs.command = CHAT_COMMAND_SAY;
            }
          else if (!strcmp(tok->string, "TIMEOUT"))
            {
              line->type = CHAT_LINE_TYPE_COMMAND;
              line->lhs.command = CHAT_COMMAND_TIMEOUT;
            }
          else
            {
              /* Not a command, hence an expectation */

              line->type = CHAT_LINE_TYPE_EXPECT_SEND;
              line->lhs.expect = strdup(tok->string);
              if (!line->lhs.expect)
                {
                  ret = -ENOMEM;
                }
            }

          /* Initialise the rhs - for 'free' in case of error */

          line->rhs = NULL;
        }

      /* Alternate between left-hand side and right-hand side */

      rhs ^= 1;
      tok = tok->next;
    }

  if (!ret && rhs)
    {
      /* The right-hand side of a line is missing */

      ret = -ENODATA;
    }

  _info("result %d, rhs %d\n", ret, rhs);
  return ret;
}

/* Chat token list deallocator */

static void chat_tokens_free(FAR struct chat_token *first_tok)
{
  FAR struct chat_token* next_tok;

  while (first_tok)
    {
      next_tok = first_tok->next;

      DEBUGASSERT(first_tok->string != NULL);
      free(first_tok->string);
      free(first_tok);
      first_tok = next_tok;
    }

  _info("tokens freed\n");
}

/* Main parsing function. */

static int chat_script_parse(FAR struct chat *priv, FAR const char *script)
{
  FAR struct chat_token *first_tok;
  int ret;

  ret = chat_tokenise(priv, script, &first_tok);
   if (!ret)
     {
       ret = chat_internalise(priv, first_tok);
     }

  chat_tokens_free(first_tok);
  return ret;
}

/* Polls the file descriptor for a specified number of milliseconds and, on
 * successful read, stores 1 read byte at location pointed by 'c'. Otherwise
 * returns a negative error code.
 */

static int chat_readb(FAR struct chat *priv, FAR char *c, int timeout_ms)
{
  struct pollfd fds;
  int ret;

  fds.fd = priv->ctl.fd;
  fds.events = POLLIN;
  fds.revents = 0;

  ret = poll(&fds, 1, timeout_ms);
  if (ret <= 0)
    {
      _info("poll timed out\n");
      return -ETIMEDOUT;
    }

  ret = read(priv->ctl.fd, c, 1);
  if (ret != 1)
    {
      _info("read failed\n");
      return -EPERM;
    }

  if (priv->ctl.echo)
    {
      fputc(*c, stderr);
    }

  _info("read \'%c\' (0x%02X)\n", *c, *c);
  return 0;
}

static void chat_flush(FAR struct chat *priv)
{
  char c;

  _info("starting\n");
  while (chat_readb(priv, (FAR char *) &c, 0) == 0);
  _info("done\n");
}

static int chat_expect(FAR struct chat *priv, FAR const char *s)
{
  char c;
  struct timespec abstime;
  struct timespec endtime;
  int timeout_ms;
  int s_len = strlen(s);
  int i_match = 0; /* index of the next character to be matched in s */
  int ret = 0;

  /* Get initial time and set the end time */

  clock_gettime(CLOCK_REALTIME, (FAR struct timespec*) &abstime);
  endtime.tv_sec  = abstime.tv_sec + priv->ctl.timeout;
  endtime.tv_nsec = abstime.tv_nsec;

  while (!ret && i_match < s_len &&
         (abstime.tv_sec < endtime.tv_sec ||
          (abstime.tv_sec == endtime.tv_sec &&
           abstime.tv_nsec < endtime.tv_nsec)))
    {
      timeout_ms =
        (endtime.tv_sec - abstime.tv_sec) * 1000 +
        (endtime.tv_nsec - abstime.tv_nsec) / 1000000;

      DEBUGASSERT(timeout_ms >= 0);

      ret = chat_readb(priv, &c, timeout_ms);
      if (!ret)
        {
          if (c == s[i_match])
            {
              /* Continue matching the next character */

              i_match++;
            }
          else
            {
              /* Match failed; start anew */

              i_match = 0;
            }

          /* Update current time */

          clock_gettime(CLOCK_REALTIME, (FAR struct timespec*) &abstime);
        }
    }

  _info("result %d\n", ret);
  return ret;
}

static int chat_send(FAR struct chat *priv, FAR const char *s)
{
  int ret = 0;
  int len = strlen(s);

  /* 'write' returns the number of successfully written characters */

  ret = write(priv->ctl.fd, s, len);
  _info("wrote %d out of %d bytes of \'%s\'\n", ret, len, s);
  if (ret > 0)
    {
      /* Just SUCCESS */

      ret = 0;
    }

  return ret;
}

static int chat_line_run(FAR struct chat* priv,
                         FAR const struct chat_line* line)
{
  int ret = 0;
  int numarg;

  _info("type %d, rhs %s\n", line->type, line->rhs);

  switch (line->type)
    {
    case CHAT_LINE_TYPE_COMMAND:
      if (priv->ctl.verbose)
        {
          fprintf(stderr, "chat: cmd %d, arg %s\n",
                  line->lhs.command, line->rhs);
        }

      switch (line->lhs.command)
        {
        case CHAT_COMMAND_ABORT:
          /* TODO */
          break;

        case CHAT_COMMAND_ECHO:
          if (strcmp(line->rhs, "ON"))
            {
              priv->ctl.echo = true;
            }
          else
            {
              priv->ctl.echo = false;
            }

          break;

        case CHAT_COMMAND_PAUSE:
          numarg = atoi(line->rhs);
          if (numarg < 0)
            {
              numarg = 0;
            }

          sleep(numarg);
          break;

        case CHAT_COMMAND_SAY:
          fprintf(stderr, "%s\n", line->rhs);
          break;

        case CHAT_COMMAND_TIMEOUT:
          numarg = atoi(line->rhs);
          if (numarg < 0)
            {
              _info("invalid timeout string %s\n", line->rhs);
            }
          else
            {
              _info("timeout is %d s\n", numarg);
              priv->ctl.timeout = numarg;
            }

          break;

        default:
          break;
        }

      break;

    case CHAT_LINE_TYPE_EXPECT_SEND:
      if (priv->ctl.verbose)
        {
          fprintf(stderr, "chat: %s %s\n", line->lhs.expect, line->rhs);
        }

      ret = chat_expect(priv, line->lhs.expect);
      if (!ret)
        {
          /* Discard anything after the confirmed expectation */

          chat_flush(priv);
          ret = chat_send(priv, line->rhs);
        }

      break;

    default:
      ret = -EINVAL;
      break;
    }

  return ret;
}

static int chat_script_run(FAR struct chat *priv)
{
  FAR struct chat_line *line = priv->script;
  int ret = 0;
#ifdef CONFIG_DEBUG_INFO
  int line_num = 0;
#endif

  while (!ret && line)
    {
      ret = chat_line_run(priv, line);
      if (!ret)
        {
          line = line->next;
#ifdef CONFIG_DEBUG_INFO
          line_num++;
#endif
        }
    }

  _info("Script result %d, exited on line %d\n", ret, line_num);
  return ret;
}

static int chat_script_free(FAR struct chat *priv)
{
  FAR struct chat_line* line = priv->script;
  FAR struct chat_line* next_line;
  int ret = 0;

  while (line)
    {
      next_line = line->next;
      if (line->type == CHAT_LINE_TYPE_EXPECT_SEND)
        {
          free(line->lhs.expect);
        }

      free(line->rhs);
      free(line);
      line = next_line;
    }

  priv->script = NULL;
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int chat(FAR struct chat_ctl *ctl, FAR const char *script)
{
  int ret = 0;
  struct chat priv;

  DEBUGASSERT(script != NULL);

  chat_init(&priv, ctl);
  ret = chat_script_parse(&priv, script);
  if (!ret)
    {
      /* TODO: optionally, free 'script' here */

      ret = chat_script_run(&priv);
    }

  chat_script_free(&priv);
  return ret;
}
