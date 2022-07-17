/****************************************************************************
 * apps/examples/sotest/lib/sotest/sotest.c
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
#include <nuttx/lib/modlib.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#if CONFIG_MODLIB_MAXDEPEND > 0
void modprint(FAR const char *fmt, ...) printflike(1, 2);
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void testfunc1(FAR const char *msg);
static void testfunc2(FAR const char *msg);
static void testfunc3(FAR const char *msg);
static int module_uninitialize(FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_msg1[] = "Hello to you too!";
static const char g_msg2[] = "Not so bad so far.";
static const char g_msg3[] = "Yes, don't be a stranger!";

static const struct symtab_s g_sotest_exports[6] =
{
  {
    (FAR const char *)"testfunc1", (FAR const void *)testfunc1,
  },
  {
    (FAR const char *)"testfunc2", (FAR const void *)testfunc2,
  },
  {
    (FAR const char *)"testfunc3", (FAR const void *)testfunc3,
  },
  {
    (FAR const char *)"g_msg1",    (FAR const void *)g_msg1,
  },
  {
    (FAR const char *)"g_msg2",    (FAR const void *)g_msg2,
  },
  {
    (FAR const char *)"g_msg3",    (FAR const void *)g_msg3,
  },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modprint
 ****************************************************************************/

#if CONFIG_MODLIB_MAXDEPEND < 1
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

static void testfunc1(FAR const char *msg)
{
  modprint("testfunc1: Hello, everyone!\n");
  modprint("   caller: %s\n", msg);
}

/****************************************************************************
 * Name: testfunc2
 ****************************************************************************/

static void testfunc2(FAR const char *msg)
{
  modprint("testfunc2: Hope you are having a great day!\n");
  modprint("   caller: %s\n", msg);
}

/****************************************************************************
 * Name: testfunc3
 ****************************************************************************/

static void testfunc3(FAR const char *msg)
{
  modprint("testfunc3: Let's talk again very soon\n");
  modprint("   caller: %s\n", msg);
}

/****************************************************************************
 * Name: module_uninitialize
 ****************************************************************************/

static int module_uninitialize(FAR void *arg)
{
  modprint("module_uninitialize: arg=%p\n", arg);
  return OK;
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

int module_initialize(FAR struct mod_info_s *modinfo)
{
  modprint("module_initialize:\n");

  modinfo->uninitializer = module_uninitialize;
  modinfo->arg           = NULL;
  modinfo->exports       = g_sotest_exports;
  modinfo->nexports      = 6;

  return OK;
}
