/****************************************************************************
 * apps/system/nsh/sh_main.c
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

#include "nshlib/nshlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sh_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* There are two modes that NSH can be executed in:
   *
   * 1) As a normal, interactive shell.  In this case, no arguments are
   *    expected on the command line.  OR
   * 2) As a single command processor.  In this case, the single command is
   *    is provided in argv[1].
   *
   * NOTE:  The latter mode is only available if CONFIG_SYSTEM_NSH=m.
   * In that case, this main() function will be built as a process.  The
   * process will be started with a command by the implementations of the
   * system() and popen() interfaces.
   */

  return nsh_system(argc, argv);
}
