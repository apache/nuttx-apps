/****************************************************************************
 * apps/nshlib/nsh_telnetlogin.c
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

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>

#include "fsutils/passwd.h"

#include "nsh.h"
#include "nsh_console.h"
#include "nshlib/nshlib.h"
#include "system/readline.h"

#ifdef CONFIG_NSH_TELNET_LOGIN

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TELNET_IAC              255
#define TELNET_WILL             251
#define TELNET_WONT             252
#define TELNET_DO               253
#define TELNET_DONT             254
#define TELNET_USE_ECHO         1
#define TELNET_NOTUSE_ECHO      0

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_telnettoken
 ****************************************************************************/

static void nsh_telnettoken(FAR struct console_stdio_s *pstate,
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

  strlcpy(buffer, start, buflen);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_telnetlogin
 ****************************************************************************/

int nsh_telnetlogin(FAR struct console_stdio_s *pstate)
{
  char username[16];
  char password[128];
#ifdef CONFIG_NSH_PLATFORM_CHALLENGE
  char challenge[128];
#endif
  struct termios cfg;
  int i;
  int ret;

#ifdef CONFIG_NSH_PLATFORM_SKIP_LOGIN
  if (platform_skip_login() == OK)
    {
      return OK;
    }
#endif

  /* Present the NSH Telnet greeting */

  write(OUTFD(pstate), g_telnetgreeting, strlen(g_telnetgreeting));

  /* Loop for the configured number of retries */

  for (i = 0; i < CONFIG_NSH_LOGIN_FAILCOUNT; i++)
    {
      /* Ask for the login username */

      write(OUTFD(pstate), g_userprompt, strlen(g_userprompt));

      username[0] = '\0';
      if (readline_fd(pstate->cn_line, LINE_MAX,
                      INFD(pstate), OUTFD(pstate)) >= 0)

        {
          /* Parse out the username */

          nsh_telnettoken(pstate, username, sizeof(username));
        }

      if (username[0] == '\0')
        {
          i--;
          continue;
        }

#ifdef CONFIG_NSH_PLATFORM_CHALLENGE
      platform_challenge(challenge, sizeof(challenge));
      write(OUTFD(pstate), challenge, strlen(challenge));
#endif

      /* Ask for the login password */

      write(OUTFD(pstate), g_passwordprompt, strlen(g_passwordprompt));

      /* Disable ECHO if its a tty device */

      if (isatty(INFD(pstate)))
        {
          if (tcgetattr(INFD(pstate), &cfg) == 0)
            {
              cfg.c_lflag &= ~ECHO;
              tcsetattr(INFD(pstate), TCSANOW, &cfg);
            }
        }

      password[0] = '\0';
      ret = readline_fd(pstate->cn_line, LINE_MAX,
                      INFD(pstate), OUTFD(pstate));

      /* Enable echo again after password */

      if (isatty(INFD(pstate)))
        {
          if (tcgetattr(INFD(pstate), &cfg) == 0)
            {
              cfg.c_lflag |= ECHO;
              tcsetattr(INFD(pstate), TCSANOW, &cfg);
            }
        }

      if (ret >= 0)
        {
          /* Parse out the password */

          nsh_telnettoken(pstate, password, sizeof(password));

          /* Verify the username and password */

#if defined(CONFIG_NSH_LOGIN_PASSWD)
          if (PASSWORD_VERIFY_MATCH(passwd_verify(username, password)))
#elif defined(CONFIG_NSH_LOGIN_PLATFORM)
#  ifdef CONFIG_NSH_PLATFORM_CHALLENGE
          if (PASSWORD_VERIFY_MATCH(platform_user_verify(username,
                                                         challenge,
                                                         password)))
#  else
          if (PASSWORD_VERIFY_MATCH(platform_user_verify(username,
                                                         password)))
#  endif
#elif defined(CONFIG_NSH_LOGIN_FIXED)
          if (strcmp(password, CONFIG_NSH_LOGIN_PASSWORD) == 0 &&
              strcmp(username, CONFIG_NSH_LOGIN_USERNAME) == 0)
#else
#  error No user verification method selected
#endif
            {
              write(OUTFD(pstate), g_loginsuccess, strlen(g_loginsuccess));
              return OK;
            }
          else
            {
              write(OUTFD(pstate), g_badcredentials,
                    strlen(g_badcredentials));
#if CONFIG_NSH_LOGIN_FAILDELAY > 0
              usleep(CONFIG_NSH_LOGIN_FAILDELAY * 1000L);
#endif
            }
        }
    }

  /* Too many failed login attempts */

  write(OUTFD(pstate), g_loginfailure, strlen(g_loginfailure));
  return -1;
}

#endif /* CONFIG_NSH_TELNET_LOGIN */
