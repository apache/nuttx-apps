/****************************************************************************
 * apps/graphics/nuttx/pdcdisp.c
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

#include <string.h>

#include "pdcnuttx.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_gotoyx
 *
 * Description:
 *   Move the physical cursor (as opposed to the logical cursor affected by
 *   wmove()) to the given location. This is called mainly from doupdate().
 *   In general, this function need not compare the old location with the new
 *   one, and should just move the cursor unconditionally.
 *
 ****************************************************************************/

/* Position hardware cursor at (y, x) */

void PDC_gotoyx(int row, int col)
{
  PDC_LOG(("PDC_gotoyx() - called: row %d col %d\n", row, col));
#warning Missing logic
}

/****************************************************************************
 * Name: PDC_transform_line
 *
 * Description:
 *   The core output routine. It takes len chtype entities from srcp (a
 *   pointer into curscr) and renders them to the physical screen at line
 *   lineno, column x. It must also translate characters 0-127 via acs_map[],
 *   if they're flagged with A_ALTCHARSET in the attribute portion of the
 *   chtype.
 *
 ****************************************************************************/

void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
  PDC_LOG(("PDC_transform_line() - called: line %d\n", lineno));
#warning Missing logic
}
