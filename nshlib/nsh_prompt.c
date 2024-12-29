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
