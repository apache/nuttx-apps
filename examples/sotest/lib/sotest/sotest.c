/****************************************************************************
 * apps/examples/sotest/lib/sotest/sotest.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdarg.h>
#include <dllfcn.h>
#include <syslog.h>

#include <nuttx/symtab.h>
#include <nuttx/lib/modlib.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#if CONFIG_MODLIB_MAXDEPEND > 0
void modprint(FAR const char *fmt, ...);
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
  (void)vsyslog(LOG_INFO, fmt, ap);
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
