/****************************************************************************
 * apps/graphics/pdcurses/pdc_debug.c
 * Public Domain Curses
 * RCSID("$Id: debug.c,v 1.7 2008/07/13 16:08:18 wmcbrine Exp $")
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Adapted by: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt and
 * released as part of NuttX under the 3-clause BSD license:
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
