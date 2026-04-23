/****************************************************************************
 * apps/system/vncviewer/lcd_render.h
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

#ifndef __APPS_SYSTEM_VNCVIEWER_LCD_RENDER_H
#define __APPS_SYSTEM_VNCVIEWER_LCD_RENDER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct lcd_ctx_s
{
  int fd;           /* File descriptor for /dev/lcdN */
  uint16_t xres;    /* LCD horizontal resolution */
  uint16_t yres;    /* LCD vertical resolution */
  uint8_t fmt;      /* Pixel format (FB_FMT_*) */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: lcd_init
 *
 * Description:
 *   Open and initialize the LCD device.
 *
 * Input Parameters:
 *   ctx   - LCD context to initialize
 *   devno - LCD device number (0 for /dev/lcd0)
 *
 * Returned Value:
 *   OK on success, negative errno on failure.
 *
 ****************************************************************************/

int lcd_init(struct lcd_ctx_s *ctx, int devno);

/****************************************************************************
 * Name: lcd_put_row
 *
 * Description:
 *   Write a single row of pixels to the LCD.
 *
 * Input Parameters:
 *   ctx    - LCD context
 *   x      - Starting X coordinate
 *   y      - Y coordinate (row)
 *   w      - Width in pixels
 *   pixels - Pixel data matching LCD native format
 *
 * Returned Value:
 *   OK on success, negative errno on failure.
 *
 ****************************************************************************/

int lcd_put_row(struct lcd_ctx_s *ctx, uint16_t x, uint16_t y,
                uint16_t w, const uint16_t *pixels);

/****************************************************************************
 * Name: lcd_fill
 *
 * Description:
 *   Fill the entire LCD with a solid color.
 *
 ****************************************************************************/

int lcd_fill(struct lcd_ctx_s *ctx, uint16_t color);

/****************************************************************************
 * Name: lcd_uninit
 *
 * Description:
 *   Close the LCD device.
 *
 ****************************************************************************/

void lcd_uninit(struct lcd_ctx_s *ctx);

#endif /* __APPS_SYSTEM_VNCVIEWER_LCD_RENDER_H */
