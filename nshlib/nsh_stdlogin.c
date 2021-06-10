/****************************************************************************
 * apps/nshlib/nsh_stdlogin.c
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "fsutils/passwd.h"
#ifdef CONFIG_NSH_CLE
#  include "system/cle.h"
#else
#  include "system/readline.h"
#endif

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_CONSOLE_LOGIN

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_stdtoken
 ****************************************************************************/

static void nsh_stdtoken(FAR struct console_stdio_s *pstate,
                      FAR char *buffer, size_t buflen)
{
  FAR char *start;
  FAR char *endp1;
  bool quoted = false;

  /* Find the start of token.  Either (1) the first non-white space
   * character on the command line or (2) the character immediately after
   * a quotation mark.
   */

  for (start = pstate->cn_line; *start; start++)
    {
      /* Does the token open with a quotation mark */

      if (*start == '"')
        {
          /* Yes.. break out with start set to the character after the
           * quotation mark.
           */

          quoted = true;
          start++;
          break;
        }

      /* No, then any non-whitespace is the first character of the token */

      else if (!isspace(*start))
        {
          /* Break out with start set to the first character of the token */

          break;
        }
    }

  /* Find the terminating character after the token on the command line.  The
   * terminating character is either (1) the matching quotation mark, or (2)
   * any whitespace.
   */

  for (endp1 = start; *endp1; endp1++)
    {
      /* Did the token begin with a quotation mark? */

      if (quoted)
        {
          /* Yes.. then only the matching quotation mark (or end of string)
           * terminates
           */

          if (*endp1 == '"')
            {
              /* Break out... endp1 points to closing quotation mark */

              break;
            }
        }

      /* No.. any whitespace (or end of string) terminates */

      else if (isspace(*endp1))
        {
          /* Break out... endp1 points to first while space encountered */

          break;
        }
    }

  /* Replace terminating character with a NUL terminator */

  *endp1 = '\0';

  /* Copied the token into the buffer */

  strncpy(buffer, start, buflen);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_stdlogin
 *
 * Description:
 *   Prompt the user for a username and password.  Return a failure if valid
 *   credentials are not returned (after some retries.
 *
 ****************************************************************************/

int nsh_stdlogin(FAR struct console_stdio_s *pstate)
{
  char username[16];
  char password[16];
  int ret;
  int i;

  /* Loop for the configured number of retries */

  for (i = 0; i < CONFIG_NSH_LOGIN_FAILCOUNT; i++)
    {
      /* Get the response, handling all possible cases */

      username[0] = '\0';

#ifdef CONFIG_NSH_CLE
      /* cle() returns a negated errno value on failure */

      ret = cle(pstate->cn_line, g_userprompt, CONFIG_NSH_LINELEN,
                stdin, stdout);
      if (ret >= 0)
#else
      /* Ask for the login username */

      printf("%s", g_userprompt);

      /* readline() returns EOF on failure */

      ret = std_readline(pstate->cn_line, CONFIG_NSH_LINELEN);
      if (ret != EOF)
#endif
        {
          /* Parse out the username */

          nsh_stdtoken(pstate, username, sizeof(username));
        }

      /* Ask for the login password */

      printf("%s", g_passwordprompt);

      password[0] = '\0';
      if (fgets(pstate->cn_line, CONFIG_NSH_LINELEN, stdin) != NULL)
        {
          /* Parse out the password */

          nsh_stdtoken(pstate, password, sizeof(password));

          /* Verify the username and password */

#if defined(CONFIG_NSH_LOGIN_PASSWD)
          ret = passwd_verify(username, password);
          if (PASSWORD_VERIFY_MATCH(ret))

#elif defined(CONFIG_NSH_LOGIN_PLATFORM)
          ret = platform_user_verify(username, password);
          if (PASSWORD_VERIFY_MATCH(ret))

#elif defined(CONFIG_NSH_LOGIN_FIXED)
          if (strcmp(password, CONFIG_NSH_LOGIN_PASSWORD) == 0 &&
              strcmp(username, CONFIG_NSH_LOGIN_USERNAME) == 0)
#else
#  error No user verification method selected
#endif
            {
              printf("%s", g_loginsuccess);
              return OK;
            }
          else
            {
              printf("%s", g_badcredentials);
#if CONFIG_NSH_LOGIN_FAILDELAY > 0
              usleep(CONFIG_NSH_LOGIN_FAILDELAY * 1000L);
#endif
            }
        }
    }

  /* Too many failed login attempts */

  printf("%s", g_loginfailure);
  return -1;
}

#endif /* CONFIG_NSH_CONSOLE_LOGIN */
