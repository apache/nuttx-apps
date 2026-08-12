/****************************************************************************
 * apps/nshlib/nsh_prompt.c
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
#include <stdlib.h>
#include <assert.h>

#ifdef CONFIG_SCHED_USER_IDENTITY
#  include <stdbool.h>
#  include <unistd.h>
#endif

#include "nsh.h"

/****************************************************************************
 * Preprocessor Macros
 ****************************************************************************/

#define NSH_PROMPT_SIZE      (CONFIG_NSH_PROMPT_MAX + 1 - \
                              sizeof(CONFIG_NSH_PROMPT_SUFFIX))

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static char g_nshprompt[CONFIG_NSH_PROMPT_MAX] = CONFIG_NSH_PROMPT_STRING;

#ifdef CONFIG_SCHED_USER_IDENTITY
static bool g_nsh_privilege_prompt;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_SCHED_USER_IDENTITY

/****************************************************************************
 * Name: nsh_apply_privilege_marker
 *
 * Description:
 *   Replace the last '>' in the prompt with the privilege marker ('#' or
 *   '$').  When no '>' is present, append the marker instead.
 *
 ****************************************************************************/

static void nsh_apply_privilege_marker(FAR char *prompt, char marker)
{
  size_t len;
  FAR char *p;

  len = strlen(prompt);
  for (p = prompt + len; p > prompt; p--)
    {
      if (*(p - 1) == '>')
        {
          *(p - 1) = marker;
          return;
        }
    }

  if (len + 1 < CONFIG_NSH_PROMPT_MAX)
    {
      prompt[len]     = marker;
      prompt[len + 1] = '\0';
    }
}

/****************************************************************************
 * Name: nsh_ensure_trailing_space
 *
 * Description:
 *   Ensure the prompt ends with a separating space before command input.
 *
 ****************************************************************************/

static void nsh_ensure_trailing_space(FAR char *prompt)
{
  size_t len;

  len = strlen(prompt);
  if (len > 0 && prompt[len - 1] != ' ' &&
      len + 1 < CONFIG_NSH_PROMPT_MAX)
    {
      prompt[len]     = ' ';
      prompt[len + 1] = '\0';
    }
}

#endif /* CONFIG_SCHED_USER_IDENTITY */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_update_prompt
 *
 * Description:
 *   This updates g_nshprompt from multiple sources, in the following order:
 *
 *   - non-empty NSH_PROMPT_ENV variable and suffix
 *   - non-empty NSH_PROMPT_STRING
 *   - non-empty HOSTNAME and suffix
 *
 *   When SCHED_USER_IDENTITY is enabled, NSH_PROMPT_STRING_ROOT or
 *   NSH_PROMPT_STRING_USER replace the prompt when non-empty.
 *
 *   After login (see nsh_update_prompt_after_login()), when those overrides
 *   are empty, the last '>' in the prompt is replaced with '#' (euid 0) or
 *   '$' (non-zero euid), or the marker is appended when no '>' is present.
 *
 * Note that suffix has higher priority when used to help clearly separate
 * prompts from command line inputs.
 *
 * Results:
 *   - updated g_nsh_prompt value.
 *
 ****************************************************************************/

void nsh_update_prompt(void)
{
  static_assert(CONFIG_NSH_PROMPT_MAX > sizeof(CONFIG_NSH_PROMPT_STRING),
                                          "NSH_PROMPT_STRING too long!");
  static_assert(CONFIG_NSH_PROMPT_MAX > sizeof(CONFIG_NSH_PROMPT_SUFFIX),
                                          "NSH_PROMPT_SUFFIX too long!");
  #ifndef CONFIG_DISABLE_ENVIRON
  if (getenv(CONFIG_NSH_PROMPT_ENV))
    {
      strlcpy(g_nshprompt, getenv(CONFIG_NSH_PROMPT_ENV), NSH_PROMPT_SIZE);
      strcat(g_nshprompt, CONFIG_NSH_PROMPT_SUFFIX);
    }
  else
  #endif
  if (CONFIG_NSH_PROMPT_STRING[0])
    {
      strcpy(g_nshprompt, CONFIG_NSH_PROMPT_STRING);
    }
  else
    {
      gethostname(g_nshprompt, NSH_PROMPT_SIZE);
      strcat(g_nshprompt, CONFIG_NSH_PROMPT_SUFFIX);
    }

#ifdef CONFIG_SCHED_USER_IDENTITY
  if (geteuid() == 0)
    {
      bool applied = false;

#ifdef CONFIG_NSH_PROMPT_STRING_ROOT
      if (CONFIG_NSH_PROMPT_STRING_ROOT[0] != '\0')
        {
          strlcpy(g_nshprompt, CONFIG_NSH_PROMPT_STRING_ROOT,
                  CONFIG_NSH_PROMPT_MAX);
          applied = true;
        }

#endif

      if (!applied && g_nsh_privilege_prompt)
        {
          nsh_apply_privilege_marker(g_nshprompt, '#');
        }
    }
  else
    {
      bool applied = false;

#ifdef CONFIG_NSH_PROMPT_STRING_USER
      if (CONFIG_NSH_PROMPT_STRING_USER[0] != '\0')
        {
          strlcpy(g_nshprompt, CONFIG_NSH_PROMPT_STRING_USER,
                  CONFIG_NSH_PROMPT_MAX);
          applied = true;
        }

#endif

      if (!applied && g_nsh_privilege_prompt)
        {
          nsh_apply_privilege_marker(g_nshprompt, '$');
        }
    }

  if (g_nsh_privilege_prompt)
    {
      nsh_ensure_trailing_space(g_nshprompt);
    }
#endif
}

/****************************************************************************
 * Name: nsh_update_prompt_after_login
 *
 * Description:
 *   Enable privilege markers in the prompt and refresh it.  Boot and
 *   no-login sessions keep NSH_PROMPT_STRING (for example, "nsh> ").
 *
 ****************************************************************************/

void nsh_update_prompt_after_login(void)
{
#ifdef CONFIG_SCHED_USER_IDENTITY
  g_nsh_privilege_prompt = true;
#endif

  nsh_update_prompt();
}

/****************************************************************************
 * Name: nsh_prompt
 *
 * Description:
 *   This function returns latest prompt string.
 *   It is needed as g_nshprompt is no longer public.
 *
 ****************************************************************************/

FAR const char *nsh_prompt(void)
{
  return g_nshprompt;
}
