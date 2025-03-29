/****************************************************************************
 * apps/examples/sotest/sotest/sotest.c
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

#include <sys/types.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <syslog.h>

#include <nuttx/symtab.h>
#include <nuttx/lib/elf.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#if CONFIG_LIBC_ELF_MAXDEPEND > 0
void modprint(FAR const char *fmt, ...) printf_like(1, 2);
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

visibility_default const char g_msg1[] = "Hello to you too!";
visibility_default const char g_msg2[] = "Not so bad so far.";
visibility_default const char g_msg3[] = "Yes, don't be a stranger!";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modprint
 ****************************************************************************/

#if CONFIG_LIBC_ELF_MAXDEPEND < 1
static void modprint(FAR const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vsyslog(LOG_INFO, fmt, ap);
  va_end(ap);
}
#endif

/****************************************************************************
 * Name: testfunc1
 ****************************************************************************/

visibility_default void testfunc1(FAR const char *msg)
{
  modprint("testfunc1: Hello, everyone!\n");
  modprint("   caller: %s\n", msg);
}

/****************************************************************************
 * Name: testfunc2
 ****************************************************************************/

visibility_default void testfunc2(FAR const char *msg)
{
  modprint("testfunc2: Hope you are having a great day!\n");
  modprint("   caller: %s\n", msg);
}

/****************************************************************************
 * Name: testfunc3
 ****************************************************************************/

visibility_default void testfunc3(FAR const char *msg)
{
  modprint("testfunc3: Let's talk again very soon\n");
  modprint("   caller: %s\n", msg);
}

/****************************************************************************
 * Name: module_uninitialize
 ****************************************************************************/

destructor_function static void module_uninitialize(void)
{
  modprint("module_uninitialize\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: module_initialize
 *
 * Description:
 *   Register /dev/sotest
 *
 ****************************************************************************/

constructor_fuction static void module_initialize(void)
{
  modprint("module_initialize\n");
}
