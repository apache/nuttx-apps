/****************************************************************************
 * apps/examples/fdpicxip/modules/libcounter.c
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

/* A shared library with its own state.
 *
 * g_calls is the interesting part: it lives in the library's writable
 * segment, and there is one of it.  A library named in DT_NEEDED is opened
 * with dlopen(), which hands back the instance already in the module
 * registry, so every task that names this library counts into the same
 * g_calls.  The library's code, meanwhile, is mapped once and executed in
 * place.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <syslog.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_calls;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int counter_bump(int by);
int counter_total(void);

int counter_bump(int by)
{
  g_calls += by;
  return g_calls;
}

int counter_total(void)
{
  return g_calls;
}
