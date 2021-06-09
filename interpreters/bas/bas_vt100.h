/****************************************************************************
 * apps/interpreters/bas/bas_vt100.h
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
