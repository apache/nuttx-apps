/****************************************************************************
 * apps/graphics/nuttx/pdcutil.c
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
