/****************************************************************************
 * apps/graphics/pdcurses/pdc_debug.c
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
 * Adapted from the original public domain pdcurses by Gregory Nutt
 ****************************************************************************/

/* Name: debug
 *
 * Synopsis:
 *       void traceon(void);
 *       void traceoff(void);
 *       void PDC_debug(const char *, ...);
 *
 * Description:
 *       traceon() and traceoff() toggle the recording of debugging
 *       information to the file "trace". Although not standard, similar
 *       functions are in some other curses implementations.
 *
 *       PDC_debug() is the function that writes to the file, based on
 *       whether traceon() has been called. It's used from the PDC_LOG()
 *       macro.
 *
 * Portability                                X/Open    BSD    SYS V
 *       traceon                                 -       -       -
 *       traceoff                                -       -       -
 *       PDC_debug                               -       -       -
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdarg.h>
#include <syslog.h>

#include "curspriv.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
bool pdc_trace_on = false;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void PDC_debug(const char *fmt, ...)
{
  va_list args;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  if (pdc_trace_on)
    {
      va_start(args, fmt);
      vsyslog(LOG_NOTICE, fmt, args);
      va_end(args);
    }
}

void traceon(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("traceon() - called\n"));

  pdc_trace_on = true;
}

void traceoff(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("traceoff() - called\n"));

  pdc_trace_on = false;
}
