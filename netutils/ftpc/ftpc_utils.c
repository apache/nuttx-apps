/****************************************************************************
 * apps/netutils/ftpc/ftpc_utils.c
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

#include "ftpc_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_nibble
 *
 * Description:
 *   Convert a ASCII hex 'digit' to binary
 *
 ****************************************************************************/

int ftpc_nibble(char ch)
{
  if (ch >= '0' && ch <= '9')
    {
      return (unsigned int)ch - '0';
    }
  else if (ch >= 'A' && ch <= 'F')
    {
      return (unsigned int)ch - 'A' + 10;
    }
  else if (ch >= 'a' && ch <= 'f')
    {
      return (unsigned int)ch - 'a' + 10;
    }

  return ERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_reset
 *
 * Description:
 *   Reset the FTP session.
 *
 ****************************************************************************/

void ftpc_reset(struct ftpc_session_s *session)
{
  ftpc_sockclose(&session->data);
  ftpc_sockclose(&session->cmd);
  free(session->uname);
  session->uname      = NULL;
  free(session->pwd);
  session->pwd        = NULL;
  free(session->initrdir);
  session->initrdir   = NULL;
  session->flags     &= ~FTPC_FLAGS_CLEAR;
  session->flags     |= FTPC_FLAGS_SET;
  session->xfrmode    = FTPC_XFRMODE_UNKNOWN;
  session->code       = 0;
  session->replytimeo = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
  session->conntimeo  = CONFIG_FTP_DEFTIMEO * CLOCKS_PER_SEC;
}

/****************************************************************************
 * Name: ftpc_lpwd
 *
 * Description:
 *   Return the local current working directory.  NOTE:  This is a peek at
 *   a global copy.  The caller should call strdup if it wants to keep it.
 *
 ****************************************************************************/

FAR const char *ftpc_lpwd(void)
{
#ifndef CONFIG_DISABLE_ENVIRON
  FAR const char *val;

  val = getenv("PWD");
  if (!val)
    {
      val = CONFIG_FTP_TMPDIR;
    }

  return val;
#else
  return CONFIG_FTP_TMPDIR;
#endif
}

/****************************************************************************
 * Name: ftpc_stripcrlf
 *
 * Description:
 *   Strip any trailing carriage returns or line feeds from a string (by
 *   overwriting them with NUL characters).
 *
 ****************************************************************************/

void ftpc_stripcrlf(FAR char *str)
{
  FAR char *ptr;
  int len;

  if (str)
    {
      len = strlen(str);
      if (len > 0)
        {
          for (ptr = str + len - 1;
               len > 0 && (*ptr == '\r' || *ptr == '\n');
               ptr--, len--)
            {
              *ptr = '\0';
            }
        }
    }
}

/****************************************************************************
 * Name: ftpc_stripslash
 *
 * Description:
 *   Strip single trailing slash from a string (by overwriting it with NUL
 *   character).
 *
 ****************************************************************************/

void ftpc_stripslash(FAR char *str)
{
  FAR char *ptr;
  int len;

  if (str)
    {
      len = strlen(str);
      if (len > 1)
        {
          ptr = str + len - 1;
          if (*ptr == '/')
            {
              *ptr = '\0';
            }
        }
    }
}

/****************************************************************************
 * Name: ftpc_dequote
 *
 * Description:
 *   Convert quoted hexadecimal constants to binary values.
 *
 ****************************************************************************/

FAR char *ftpc_dequote(FAR const char *str)
{
  FAR char *allocstr = NULL;
  FAR char *ptr;
  int ms;
  int ls;
  int len;

  if (str)
    {
      /* Allocate space for a modifiable copy of the string */

      len      = strlen(str);
      allocstr = (FAR char *)malloc(len + 1);
      if (allocstr)
        {
          /* Search the string */

          ptr = allocstr;
          while (*str != '\0')
            {
              /* Check for a quoted hex value (make sure that there are
               * least 3 characters remaining in the string.
               */

              if (len > 2 && str[0] == '%')
                {
                  /* Extract the hex value */

                  ms = ftpc_nibble(str[1]);
                  if (ms >= 0)
                    {
                      ls = ftpc_nibble(str[2]);
                      if (ls >= 0)
                        {
                          /* Save the binary value and skip ahead by 3 */

                          *ptr++ = (char)(ms << 4 | ls);
                          str += 3;
                          len -= 3;
                          continue;
                        }
                    }
                }

              /* Just transfer the character */

              *ptr++ = *str++;
              len--;
            }

          /* NUL terminate */

          *ptr = '\0';
        }
    }

  return allocstr;
}
