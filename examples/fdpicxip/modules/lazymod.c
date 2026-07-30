/****************************************************************************
 * apps/examples/fdpicxip/modules/lazymod.c
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

/* Linked without -z now (BINDNOW= in the makefile), so its imported function
 * descriptors land in DT_JMPREL instead of DT_REL.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <syslog.h>
#include <stdlib.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int seed = (argc > 1) ? atoi(argv[1]) : 1;

  syslog(LOG_INFO, "[lazymod] called through a DT_JMPREL descriptor, "
         "seed %d\n", seed);
  return 0;
}
