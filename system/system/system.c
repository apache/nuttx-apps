/****************************************************************************
 * apps/system/system/system.c
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
#include <assert.h>

#include "nshlib/nshlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: system
 *
 * Description:
 *   Use system to pass a command string to the NSH parser and wait for it
 *   to finish executing.
 *
 *   This is an experimental version with known incompatibilities:
 *
 *   1. It is not a part of libc due to its close association with NSH.  The
 *      function is still prototyped in nuttx/include/stdlib.h, however.
 *   2. It cannot use /bin/sh since that program will not exist in most
 *      embedded configurations.  Rather, it will spawn a shell-specific
 *      system command -- currently only NSH.
 *   3. REVISIT: There may be some issues with returned values.
 *   4. Of course, only NSH commands will be supported so that still means
 *      that many leveraged system() calls will still not be functional.
 *
 ****************************************************************************/

int system(FAR const char *cmd)
{
  FAR char *argv[4];

  /* REVISIT: If cmd is NULL, then system() should return a non-zero value to
   * indicate if the command processor is available or zero if it is not.
   */

  DEBUGASSERT(cmd != NULL);

#ifdef CONFIG_SYSTEM_SYSTEM_SHPATH
  argv[0] = CONFIG_SYSTEM_SYSTEM_SHPATH;
#else
  argv[0] = "system";
#endif

  /* Spawn nsh_system() which will execute the command under the shell. */

  argv[1] = "-c";
  argv[2] = (FAR char *)cmd;
  argv[3] = NULL;

  return nsh_spawn(argv[0], nsh_system, argv, CONFIG_SYSTEM_SYSTEM_PRIORITY,
                   CONFIG_SYSTEM_SYSTEM_STACKSIZE, NULL, NULL, 0, true);
}
