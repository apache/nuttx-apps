/****************************************************************************
 * apps/graphics/nuttx/pdcutil.c
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

#include <unistd.h>
#include <poll.h>

#include "pdcnuttx.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_beep
 *
 * Description:
 *   Emits a short audible beep.  If this is not possible on your platform,
 *   you must set SP->audible to false during initialization (i.e., from
 *   PDC_scr_open() -- not here); otherwise, set it to true.  This function
 *   is called from beep().
 *
 ****************************************************************************/

void PDC_beep(void)
{
  PDC_LOG(("PDC_beep() - called\n"));
}

/****************************************************************************
 * Name: PDC_napms
 *
 * Description:
 *   This is the core delay routine, called by napms().  It pauses for about
 *   (the X/Open spec says "at least") ms milliseconds, then returns.  High
 *   degrees of accuracy and precision are not expected (though desirable, if
 *   you can achieve them).  More important is that this function gives back
 *   the process time slice to the OS, so that PDCurses idles at low CPU
 *   usage.
 *
 ****************************************************************************/

void PDC_napms(int ms)
{
  PDC_LOG(("PDC_napms() - called: ms=%d\n", ms));
  usleep(ms * 1000);
}

/****************************************************************************
 * Name: PDC_sysname
 *
 * Description:
 *   Returns a short string describing the platform, such as "DOS" or "X11".
 *   This is used by longname(). It must be no more than 100 characters; it
 *   should be much, much shorter (existing platforms use no more than 5).
 *
 ****************************************************************************/

const char *PDC_sysname(void)
{
  return "NuttX";
}
