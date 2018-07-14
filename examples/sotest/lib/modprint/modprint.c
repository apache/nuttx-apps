/****************************************************************************
 * apps/examples/sotest/lib/modprint/modprint.c
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
  (void)vsyslog(LOG_INFO, fmt, ap);
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
