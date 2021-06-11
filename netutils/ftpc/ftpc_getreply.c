/****************************************************************************
 * apps/netutils/ftpc/ftpc_getreply.c
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

#include "ftpc_config.h"

#include <stdlib.h>
#include <string.h>
#include <debug.h>

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_gets
 ****************************************************************************/

static int ftpc_gets(struct ftpc_session_s *session)
{
  int ch;
  int ndx = 0;

  /* Start with an empty response string */

  session->reply[0] = '\0';

  /* Verify that the command channel is still connected */

  if (!ftpc_sockconnected(&session->cmd))
    {
      nwarn("WARNING: Cmd channel disconnected\n");
      return ERROR;
    }

  /* Loop until the full line is obtained */

  for (; ; )
    {
      /* Get the next character from incoming command stream */

      ch = ftpc_sockgetc(&session->cmd);

      /* Check if the command stream was closed */

      if (ch == EOF)
        {
          nwarn("WARNING: EOF: Server closed command stream\n");
          ftpc_reset(session);
          return ERROR;
        }

      /* Handle embedded Telnet stuff */

      else if (ch == TELNET_IAC)
        {
          /* Handle TELNET commands */

          switch (ch = ftpc_sockgetc(&session->cmd))
            {
            case TELNET_WILL:
            case TELNET_WONT:
              ch = ftpc_sockgetc(&session->cmd);
              ftpc_sockprintf(&session->cmd, "%c%c%c",
                              TELNET_IAC, TELNET_DONT, ch);
              ftpc_sockflush(&session->cmd);
              break;

            case TELNET_DO:
            case TELNET_DONT:
              ch = ftpc_sockgetc(&session->cmd);
              ftpc_sockprintf(&session->cmd, "%c%c%c",
                              TELNET_IAC, TELNET_WONT, ch);
              ftpc_sockflush(&session->cmd);
              break;

            default:
            break;
            }

          continue;
        }

      /* Deal with carriage returns */

      else if (ch == ISO_CR)
        {
          /* What follows the carriage return? */

          ch = ftpc_sockgetc(&session->cmd);
          if (ch == '\0')
            {
              /* If it is followed by a NUL then keep it */

              ch = ISO_CR;
            }

          /* If it is followed by a newline then break out of the loop. */

          else if (ch == ISO_NL)
            {
              /* Newline terminates the reply */

              break;
            }

          /* If we did not lose the connection, then push the character
           * following the carriage back on the "stack" and continue to
           * examine it from scratch (if could be part of the Telnet
           * protocol).
           */

          else if (ch != EOF)
            {
              ungetc(ch, session->cmd.instream);
              continue;
            }
        }

      else if (ch == ISO_NL)
        {
          /* The ISO newline character terminates the string.  Just break
           * out of the loop.
           */

          break;
        }

      /* Put the character into the response buffer.  Is there space for
       * another character in the reply buffer?
       */

      if (ndx < CONFIG_FTP_MAXREPLY)
        {
          /* Yes.. put the character in the reply buffer */

          session->reply[ndx++] = (char)ch;
        }
      else
        {
          nwarn("WARNING: Reply truncated\n");
        }
    }

  session->reply[ndx] = '\0';
  session->code = atoi(session->reply);
  return session->code;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_getreply
 ****************************************************************************/

int fptc_getreply(struct ftpc_session_s *session)
{
  char tmp[5] = "xxx ";
  int ret;

  /* Set up a timeout */

  if (session->replytimeo)
    {
      ret = wd_start(&session->wdog, session->replytimeo,
                     ftpc_timeout, (wdparm_t)session);
    }

  /* Get the next line from the server */

  ret = ftpc_gets(session);

  /* Do we still have a connection? */

  if (!ftpc_sockconnected(&session->cmd))
    {
      /* No.. cancel the timer and return an error */

      wd_cancel(&session->wdog);
      ninfo("Lost connection\n");
      return ERROR;
    }

  /* Did an error occur? */

  if (ret < 0)
    {
      /* No.. cancel the timer and return an error */

      wd_cancel(&session->wdog);
      ninfo("ftpc_gets failed\n");
      return ERROR;
    }

  ninfo("Reply: %s\n", session->reply);

  if (session->reply[3] == '-')
    {
      /* Multi-line response */

      strncpy(tmp, session->reply, 3);
      do
        {
          if (ftpc_gets(session) == -1)
            {
              break;
            }

          ninfo("Reply: %s\n", session->reply);
        }
      while (strncmp(tmp, session->reply, 4) != 0);
    }

  wd_cancel(&session->wdog);
  return ret;
}
