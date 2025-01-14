/****************************************************************************
 * apps/system/readline/readline.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "system/readline.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: readline
 *
 *   readline will read a line from the terminal and return it, using
 *   prompt as a prompt.  If prompt is NULL or the empty string, no prompt
 *   is issued.  The line returned is allocated with malloc(3);
 *   the caller must free it when finished.  The line re‐turned has the
 *   final newline removed, so only the text of the line remains.
 *
 ****************************************************************************/

FAR char *readline(FAR const char *prompt)
{
  FAR char *line = malloc(LINE_MAX);

  if (line != NULL)
    {
#ifdef CONFIG_READLINE_TABCOMPLETION
      FAR const char *orig = readline_prompt(prompt);
#endif
      if (readline_fd(line, LINE_MAX,
                      STDIN_FILENO, STDOUT_FILENO) == 0)
        {
          free(line);
          line = NULL;
        }

#ifdef CONFIG_READLINE_TABCOMPLETION
      readline_prompt(orig);
#endif
    }

  return line;
}
