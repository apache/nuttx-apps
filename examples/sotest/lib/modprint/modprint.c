/****************************************************************************
 * apps/examples/sotest/lib/modprint/modprint.c
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
 * Private Function Prototypes
 ****************************************************************************/

static void modprint(FAR const char *fmt, ...);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct symtab_s g_modprint_exports[1] =
{
  {
    (FAR const char *)"modprint", (FAR const void *)modprint,
  }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modprint
 ****************************************************************************/

static void modprint(FAR const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vsyslog(LOG_INFO, fmt, ap);
  va_end(ap);
}

/****************************************************************************
 * Name: module_uninitialize
 ****************************************************************************/

static int module_uninitialize(FAR void *arg)
{
  syslog(LOG_INFO, "module_uninitialize: arg=%p\n", arg);
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
  syslog(LOG_INFO, "module_initialize:\n");

  modinfo->uninitializer = module_uninitialize;
  modinfo->arg           = NULL;
  modinfo->exports       = g_modprint_exports;
  modinfo->nexports      = 1;

  return OK;
}
