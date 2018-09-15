/****************************************************************************
 * apps/interpreters/bas/bas_vt100.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_INTERPRETERS_BAS_BAS_VT100_H
#define __APPS_INTERPRETERS_BAS_BAS_VT100_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: vt100_blinkon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_blinkon(int chn);
#endif

/****************************************************************************
 * Name: vt100_boldon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_boldon(int chn);
#endif

/****************************************************************************
 * Name: vt100_reverseon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_reverseon(int chn);
#endif

/****************************************************************************
 * Name: vt100_attriboff
 *
 * Description:
 *   Disable all previously selected attributes.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_attriboff(int chn);
#endif

/****************************************************************************
 * Name: vt100_cursoron
 *
 * Description:
 *   Turn on the cursor
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_cursoron(int chn);
#endif

/****************************************************************************
 * Name: vt100_cursoroff
 *
 * Description:
 *   Turn off the cursor
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_cursoroff(int chn);
#endif

/****************************************************************************
 * Name: vt100_cursorhome
 *
 * Description:
 *   Move the current cursor to the upper left hand corner of the display
 *
 ****************************************************************************/

void vt100_cursorhome(int chn);

/****************************************************************************
 * Name: vt100_setcursor
 *
 * Description:
 *   Move the current cursor position to position (row,col)
 *
 ****************************************************************************/

void vt100_setcursor(int chn, uint16_t row, uint16_t column);

/****************************************************************************
 * Name: vt100_clrtoeol
 *
 * Description:
 *   Clear the display from the current cursor position to the end of the
 *   current line.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_clrtoeol(int chn);
#endif

/****************************************************************************
 * Name: vt100_clrscreen
 *
 * Description:
 *   Clear the entire display
 *
 ****************************************************************************/

void vt100_clrscreen(int chn);

/****************************************************************************
 * Name: vt100_scrollup
 *
 * Description:
 *   Scroll the display up 'nlines' by sending the VT100 INDEX command.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_scrollup(int chn, uint16_t nlines);
#endif

/****************************************************************************
 * Name: vt100_scrolldown
 *
 * Description:
 *   Scroll the display down 'nlines' by sending the VT100 REVINDEX command.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_scrolldown(int chn, uint16_t nlines);
#endif

/****************************************************************************
 * Name: vt100_foreground_color
 *
 * Description:
 *   Set the foreground color
 *
 ****************************************************************************/

void vt100_foreground_color(int chn, uint8_t color);

/****************************************************************************
 * Name: vt100_background_color
 *
 * Description:
 *   Set the background color
 *
 ****************************************************************************/

void vt100_background_color(int chn, uint8_t color);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INTERPRETERS_BAS_BAS_VT100_H */
