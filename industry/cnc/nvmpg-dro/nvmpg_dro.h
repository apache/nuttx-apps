/****************************************************************************
 * apps/industry/cnc/nvmpg-dro/nvmpg_dro.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_INDUSTRY_CNC_NVMPG_DRO_NVMPG_DRO_H
#define __APPS_INDUSTRY_CNC_NVMPG_DRO_NVMPG_DRO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* RGB565 colors */

#define NVDRO_BLACK      0x0000
#define NVDRO_WHITE      0xffff
#define NVDRO_RED        0xf800
#define NVDRO_GREEN      0x07e0
#define NVDRO_YELLOW     0xffe0
#define NVDRO_BG         0x3a6f   /* Blue-violet background */
#define NVDRO_PANEL      0xa7f4   /* Light green "LCD" panel */
#define NVDRO_BOX        0xce59   /* Light grey label boxes */
#define NVDRO_DIM        0x6b6d   /* Greyed out text */
#define NVDRO_LAMP_OFF   0x2104

/* Glyph geometry of the built-in 5x7 font at scale 1 */

#define NVDRO_FONT_W     5
#define NVDRO_FONT_H     7

/* The NuttX logo is a 160x160 image */

#define NVDRO_LOGO_SIZE  160

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Open the LCD and query its geometry.  Returns 0 or a negated errno. */

int nvdro_lcd_open(FAR const char *path);
void nvdro_lcd_close(void);
int nvdro_lcd_width(void);
int nvdro_lcd_height(void);

/* Fill the rectangle with its top-left corner at (x, y) */

void nvdro_fill(int x, int y, int w, int h, uint16_t color);

/* Copy one row of w pixels to the screen at (x, y) */

void nvdro_putrow(int x, int y, int w, FAR const uint16_t *pixels);

/* Draw a hollow rectangle with a border 't' pixels thick */

void nvdro_frame(int x, int y, int w, int h, int t, uint16_t color);

/* Draw a string with the 5x7 font magnified 'scale' times.  Each character
 * cell is (5 * scale + gap) pixels wide and 7 * scale pixels tall, and the
 * background of the cell is painted too, so redrawing a string erases
 * what was there.  Returns the width drawn.
 */

int nvdro_text(int x, int y, FAR const char *str, int scale, int gap,
               uint16_t fg, uint16_t bg);

/* Width in pixels of 'nchars' characters at the given scale and gap */

int nvdro_text_width(int nchars, int scale, int gap);

#ifdef CONFIG_INDUSTRY_CNC_NVMPG_DRO_SPLASH
/* Clear the screen and draw the NuttX logo centered */

void nvdro_logo(void);
#endif

#endif /* __APPS_INDUSTRY_CNC_NVMPG_DRO_NVMPG_DRO_H */
