/****************************************************************************
 * apps/graphics/littlevgl/lv_conf.h
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gábor Kiss-Vámosi <kisvegabor@gmail.com>
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

#ifndef __APPS_GRAPHICS_LITTLEVGL_LV_CONF_H
#define __APPS_GRAPHICS_LITTLEVGL_LV_CONF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Compiler settings */

#define LV_COMPILER_VLA_SUPPORTED   1

/* Dynamic memory */

#define LV_MEM_CUSTOM       0
#define LV_MEM_SIZE         CONFIG_LV_MEM_SIZE
#define LV_MEM_AUTO_DEFRAG  1               /* Automatically defrag on free */
#define LV_MEM_ATTR

/* Graphical settings */

#define LV_DEMO_SLIDE_SHOW  0

/* Horizontal and vertical resolution of the library.*/

#define LV_HOR_RES          CONFIG_LV_HOR_RES
#define LV_VER_RES          CONFIG_LV_VER_RES
#define LV_DPI              CONFIG_LV_DPI

/* Buffered rendering:
 *
 * No antialasing >= LV_HOR_RES
 * Anti aliasing  >= 4 * LV_HOR_RES
 *
 * Place VDB to a specific address (e.g. in external RAM) (0: allocate into
 * RAM)
 */

#define LV_VDB_SIZE         CONFIG_LV_VDB_SIZE
#define LV_VDB_ADR          CONFIG_LV_VDB_ADR

/* Use two Virtual Display buffers (VDB) parallelize rendering and flushing
 * The flushing should use DMA to write the frame buffer in the background,
 */

#ifdef CONFIG_LV_VDB_DOUBLE
#  define LV_VDB_DOUBLE     CONFIG_LV_VDB_DOUBLE
#else
#  define LV_VDB_DOUBLE     0
#endif

/* LV_VDB2_ADR - Place VDB2 to a specific address (e.g. in external RAM)
 * (0: allocate into RAM)
 */

#define LV_VDB2_ADR         CONFIG_VDB2_ADR

/* Enable anti aliasing
 */

#ifdef CONFIG_LV_ANTIALIAS
#  define LV_ANTIALIAS      CONFIG_LV_ANTIALIAS
#else
#  define LV_ANTIALIAS      0
#endif

/* Screen refresh settings
 *
 * LV_REFR_PERIOD   - Screen refresh period in milliseconds
 * LV_INV_FIFO_SIZE - The average count of objects on a screen
 */

#define LV_REFR_PERIOD      CONFIG_LV_REFR_PERIOD
#define LV_INV_FIFO_SIZE    CONFIG_LV_INV_FIFO_SIZE

/* Misc. settings */
/* Input device settings
 *
 * LV_INDEV_READ_PERIOD         - Input device read period in milliseconds
 * LV_INDEV_POINT_MARKER        - Mark the pressed points
 * LV_INDEV_DRAG_LIMIT          - Drag threshold in pixels
 * LV_INDEV_DRAG_THROW          - Drag throw slow-down in [%]. Greater value
 *                                means faster slow-down
 * LV_INDEV_LONG_PRESS_TIME     - Long press time in milliseconds
 * LV_INDEV_LONG_PRESS_REP_TIME - Repeated trigger period in long press [ms]
 */

#define LV_INDEV_READ_PERIOD         CONFIG_LV_INDEV_READ_PERIOD

#ifdef CONFIG_LV_INDEV_POINT_MARKER
#  define LV_INDEV_POINT_MARKER      CONFIG_LV_INDEV_POINT_MARKER
#else
#  define LV_INDEV_POINT_MARKER      0
#endif

#define LV_INDEV_DRAG_LIMIT          CONFIG_LV_INDEV_DRAG_LIMIT
#define LV_INDEV_DRAG_THROW          CONFIG_LV_INDEV_DRAG_THROW
#define LV_INDEV_LONG_PRESS_TIME     CONFIG_LV_INDEV_LONG_PRESS_TIME
#define LV_INDEV_LONG_PRESS_REP_TIME CONFIG_LV_INDEV_LONG_PRESS_REP_TIME

/* Color settings
 *
 * LV_COLOR_TRANSP - Images pixels with this color will not be drawn (chroma
 *                   keying)
 */

#define LV_COLOR_DEPTH       CONFIG_LV_COLOR_DEPTH
#define LV_COLOR_TRANSP      LV_COLOR_HEX(CONFIG_LV_COLOR_TRANSP)

/* Text settings
 *
 * LV_TXT_BREAK_CHARS - Can break texts on these chars
 */

#ifdef CONFIG_LV_TXT_UTF8
#  define LV_TXT_UTF8        CONFIG_LV_TXT_UTF8
#else
#  define LV_TXT_UTF8        0
#endif

#define LV_TXT_BREAK_CHARS   CONFIG_LV_TXT_BREAK_CHARS

/* Graphics feature usage */

#ifdef CONFIG_USE_LV_ANIMATION
#  define USE_LV_ANIMATION   CONFIG_USE_LV_ANIMATION
#else
#  define USE_LV_ANIMATION   0
#endif

#ifdef CONFIG_USE_LV_SHADOW
#  define USE_LV_SHADOW      CONFIG_USE_LV_SHADOW
#else
#  define USE_LV_SHADOW      0
#endif

#ifdef CONFIG_USE_LV_GROUP
  #define USE_LV_GROUP       CONFIG_USE_LV_GROUP
#else
#  define USE_LV_GROUP       0
#endif

#ifdef CONFIG_USE_LV_GPU
#  define USE_LV_GPU         CONFIG_USE_LV_GPU
#else
#  define USE_LV_GPU         0
#endif

#ifdef CONFIG_USE_LV_REAL_DRAW
#  define USE_LV_REAL_DRAW   CONFIG_USE_LV_REAL_DRAW
#else
#  define USE_LV_REAL_DRAW   0
#endif


#ifdef CONFIG_USE_LV_FILESYSTEM
#  define USE_LV_FILESYSTEM  CONFIG_USE_LV_FILESYSTEM
#else
#  define USE_LV_FILESYSTEM  0
#endif

/* THEME USAGE */
/* Just for test */

#ifdef CONFIG_USE_LV_THEME_TEMPL
#  define USE_LV_THEME_TEMPL     CONFIG_USE_LV_THEME_TEMPL
#else
#  define USE_LV_THEME_TEMPL     0
#endif

/* Built mainly from the built-in styles. Consumes very few RAM */

#ifdef CONFIG_USE_LV_THEME_DEFAULT
#  define USE_LV_THEME_DEFAULT   CONFIG_USE_LV_THEME_DEFAULT
#else
#  define USE_LV_THEME_DEFAULT   0
#endif

/* Dark futuristic theme */

#ifdef CONFIG_USE_LV_THEME_ALIEN
#  define USE_LV_THEME_ALIEN     CONFIG_USE_LV_THEME_ALIEN
#else
#  define USE_LV_THEME_ALIEN     0
#endif

/* Dark elegant theme */

#ifdef CONFIG_USE_LV_THEME_NIGHT
#  define USE_LV_THEME_NIGHT     CONFIG_USE_LV_THEME_NIGHT
#else
#  define USE_LV_THEME_NIGHT     0
#endif

/* Mono color theme for monochrome displays */

#ifdef CONFIG_USE_LV_THEME_MONO
#  define USE_LV_THEME_MONO      CONFIG_USE_LV_THEME_MONO
#else
#  define USE_LV_THEME_MONO      0
#endif

/* Flat theme with bold colors and light shadows (Planned) */

#ifdef CONFIG_USE_LV_THEME_MATERIAL
#  define USE_LV_THEME_MATERIAL  CONFIG_USE_LV_THEME_MATERIAL
#else
#  define USE_LV_THEME_MATERIAL  0
#endif

/* Peaceful, mainly black and white theme (Planned) */

#ifdef CONFIG_USE_LV_THEME_ZEN
#  define USE_LV_THEME_ZEN       CONFIG_USE_LV_THEME_ZEN
#else
#  define USE_LV_THEME_ZEN       0
#endif

/* FONT USAGE */

#ifdef CONFIG_USE_LV_FONT_DEJAVU_10
#  define USE_LV_FONT_DEJAVU_10             CONFIG_USE_LV_FONT_DEJAVU_10
#  if USE_LV_FONT_DEJAVU_10
#    define LV_FONT_DEFAULT                 &lv_font_dejavu_10
#  endif
#else
#  define USE_LV_FONT_DEJAVU_10             0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_10_LATIN_SUP
#  define USE_LV_FONT_DEJAVU_10_LATIN_SUP CONFIG_USE_LV_FONT_DEJAVU_10_LATIN_SUP
#else
#  define USE_LV_FONT_DEJAVU_10_LATIN_SUP 0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_10_CYRILLIC
#  define USE_LV_FONT_DEJAVU_10_CYRILLIC    CONFIG_USE_LV_FONT_DEJAVU_10_CYRILLIC
#else
#  define USE_LV_FONT_DEJAVU_10_CYRILLIC    0
#endif

#ifdef CONFIG_USE_LV_FONT_SYMBOL_10
#  define USE_LV_FONT_SYMBOL_10       CONFIG_USE_LV_FONT_SYMBOL_10
#else
#  define USE_LV_FONT_SYMBOL_10       0
#endif

/* Size 20 */

#ifdef CONFIG_USE_LV_FONT_DEJAVU_20
#  define USE_LV_FONT_DEJAVU_20             CONFIG_USE_LV_FONT_DEJAVU_20
#  if USE_LV_FONT_DEJAVU_20
#    define LV_FONT_DEFAULT                 &lv_font_dejavu_20
#  endif
#else
#  define USE_LV_FONT_DEJAVU_20             0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_20_LATIN_SUP
#  define USE_LV_FONT_DEJAVU_20_LATIN_SUP CONFIG_USE_LV_FONT_DEJAVU_20_LATIN_SUP
#else
#  define USE_LV_FONT_DEJAVU_20_LATIN_SUP 0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_20_CYRILLIC
#  define USE_LV_FONT_DEJAVU_20_CYRILLIC    CONFIG_USE_LV_FONT_DEJAVU_20_CYRILLIC
#else
#  define USE_LV_FONT_DEJAVU_20_CYRILLIC    0
#endif

#ifdef CONFIG_USE_LV_FONT_SYMBOL_20
#  define USE_LV_FONT_SYMBOL_20       CONFIG_USE_LV_FONT_SYMBOL_20
#else
#  define USE_LV_FONT_SYMBOL_20       0
#endif

/* Size 30 */

#ifdef CONFIG_USE_LV_FONT_DEJAVU_30
#  define USE_LV_FONT_DEJAVU_30             CONFIG_USE_LV_FONT_DEJAVU_30
#  if USE_LV_FONT_DEJAVU_30
#    ifndef LV_FONT_DEFAULT
#      define   LV_FONT_DEFAULT               &lv_font_dejavu_30
#    endif
#  endif
#else
#  define USE_LV_FONT_DEJAVU_30             0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_30_LATIN_SUP
#  define USE_LV_FONT_DEJAVU_30_LATIN_SUP CONFIG_USE_LV_FONT_DEJAVU_30_LATIN_SUP
#else
#  define USE_LV_FONT_DEJAVU_30_LATIN_SUP 0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_30_CYRILLIC
#  define USE_LV_FONT_DEJAVU_30_CYRILLIC    CONFIG_USE_LV_FONT_DEJAVU_30_CYRILLIC
#else
#  define USE_LV_FONT_DEJAVU_30_CYRILLIC    0
#endif

#ifdef CONFIG_USE_LV_FONT_SYMBOL_30
#  define USE_LV_FONT_SYMBOL_30       CONFIG_USE_LV_FONT_SYMBOL_30
#else
#  define USE_LV_FONT_SYMBOL_30       0
#endif

/*Size 40*/
#ifdef CONFIG_USE_LV_FONT_DEJAVU_40
#  define USE_LV_FONT_DEJAVU_40             CONFIG_USE_LV_FONT_DEJAVU_40
#  if USE_LV_FONT_DEJAVU_40
#    ifndef LV_FONT_DEFAULT
#      define LV_FONT_DEFAULT               &lv_font_dejavu_40
#    endif
#  endif
#else
#  define USE_LV_FONT_DEJAVU_40             0
#endif

#ifdef CONFIG_USE_LV_FONT_SYMBOL_40
#  define USE_LV_FONT_SYMBOL_40       CONFIG_USE_LV_FONT_SYMBOL_40
#else
#  define USE_LV_FONT_SYMBOL_40       0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_40_LATIN_SUP
#  define USE_LV_FONT_DEJAVU_40_LATIN_SUP CONFIG_USE_LV_FONT_DEJAVU_40_LATIN_SUP
#else
#  define USE_LV_FONT_DEJAVU_40_LATIN_SUP 0
#endif

#ifdef CONFIG_USE_LV_FONT_DEJAVU_40_CYRILLIC
#  define USE_LV_FONT_DEJAVU_40_CYRILLIC    CONFIG_USE_LV_FONT_DEJAVU_40_CYRILLIC
#else
#  define USE_LV_FONT_DEJAVU_40_CYRILLIC    0
#endif

/* LV_OBJ SETTINGS
 *
 * LV_OBJ_FREE_NUM_TYPE - Type of free number attribute (comment out disable free number)
 * LV_OBJ_FREE_PTR      - Enable the free pointer attribut
 */

#define LV_OBJ_FREE_NUM_TYPE int

#ifdef CONFIG_LV_OBJ_FREE_PTR
#  define LV_OBJ_FREE_PTR    CONFIG_LV_OBJ_FREE_PTR
#else
#  define LV_OBJ_FREE_PTR    0
#endif

/* LV OBJ X USAGE */
/* Simple object */

#ifdef CONFIG_USE_LV_LABEL
#  define USE_LV_LABEL       CONFIG_USE_LV_LABEL
#else
#  define USE_LV_LABEL       0
#endif

/* Label (dependencies: - */

#ifdef CONFIG_USE_LV_LABEL
#  define USE_LV_LABEL       CONFIG_USE_LV_LABEL
#else
#  define USE_LV_LABEL       0
#endif

/* Image (dependencies: lv_label (if symbols are enabled) from misc: FSINT,
 * UFS)
 */

#ifdef CONFIG_USE_LV_IMG
#  define USE_LV_IMG         CONFIG_USE_LV_IMG
#else
#  define USE_LV_IMG         0
#endif

/* Line (dependencies: - */

#ifdef CONFIG_USE_LV_LINE
#  define USE_LV_LINE        CONFIG_USE_LV_LINE
#else
#  define USE_LV_LINE        0
#endif

/* Container objects */
/* Container (dependencies: - */

#ifdef CONFIG_USE_LV_CONT
#  define USE_LV_CONT        CONFIG_USE_LV_CONT
#else
#  define USE_LV_CONT        0
#endif

/* Page (dependencies: lv_cont) */

#ifdef CONFIG_USE_LV_PAGE
#  define USE_LV_PAGE        CONFIG_USE_LV_PAGE
#else
#  define USE_LV_PAGE        0
#endif

/* Window (dependencies: lv_cont, lv_btn, lv_label, lv_img, lv_page) */

#ifdef CONFIG_USE_LV_WIN
#  define USE_LV_WIN         CONFIG_USE_LV_WIN
#else
#  define USE_LV_WIN         0
#endif

/* Tab (dependencies: lv_page, lv_btnm) */

#ifdef CONFIG_USE_LV_TABVIEW
#  define USE_LV_TABVIEW     CONFIG_USE_LV_TABVIEW
#else
#  define USE_LV_TABVIEW     0
#endif

/* Data visualizer objects */
/* Bar (dependencies: -) */

#ifdef CONFIG_USE_LV_BAR
#  define USE_LV_BAR        CONFIG_USE_LV_BAR
#else
#  define USE_LV_BAR        0
#endif

/* Line meter (dependencies: bar; misc: trigo) */

#ifdef CONFIG_USE_LV_LMETER
#  define USE_LV_LMETER      CONFIG_USE_LV_LMETER
#else
#  define USE_LV_LMETER      0
#endif

/* Gauge (dependencies:bar, lmeter; misc: trigo) */

#ifdef CONFIG_USE_LV_GAUGE
#  define USE_LV_GAUGE       CONFIG_USE_LV_GAUGE
#else
#  define USE_LV_GAUGE       0
#endif

/* Chart (dependencies: -) */

#ifdef CONFIG_USE_LV_CHART
#  define USE_LV_CHART       CONFIG_USE_LV_CHART
#else
#  define USE_LV_CHART       0
#endif

/* LED (dependencies: -) */

#ifdef CONFIG_USE_LV_LED
#  define USE_LV_LED         CONFIG_USE_LV_LED
#else
#  define USE_LV_LED         0
#endif

/* Message box (dependencies: lv_rect, lv_btnm, lv_label) */

#ifdef CONFIG_USE_LV_MBOX
#  define USE_LV_MBOX        CONFIG_USE_LV_MBOX
#else
#  define USE_LV_MBOX        0
#endif

/* Text area (dependencies: lv_label, lv_page) */

#ifdef CONFIG_USE_LV_TA
#  define USE_LV_TA          CONFIG_USE_LV_TA
#else
#  define USE_LV_TA          0
#endif

/* User input objects */
/* Button (dependencies: lv_cont */

#ifdef CONFIG_USE_LV_BTN
#  define USE_LV_BTN         CONFIG_USE_LV_BTN
#else
#  define USE_LV_BTN         0
#endif

/* Button matrix (dependencies: -) */

#ifdef CONFIG_USE_LV_BTNM
#  define USE_LV_BTNM        CONFIG_USE_LV_BTNM
#else
#  define USE_LV_BTNM        0
#endif

/* Keyboard (dependencies: lv_btnm) */

#ifdef CONFIG_USE_LV_KB
#  define USE_LV_KB          CONFIG_USE_LV_KB
#else
#  define USE_LV_KB          0
#endif

/* Check box (dependencies: lv_btn, lv_label) */

#ifdef CONFIG_USE_LV_CB
#  define USE_LV_CB          CONFIG_USE_LV_CB
#else
#  define USE_LV_CB          0
#endif

/* Switch (dependencies: lv_slider) */

#ifdef CONFIG_USE_LV_SW
#  define USE_LV_SW          CONFIG_USE_LV_SW
#else
#  define USE_LV_SW          0
#endif

/* List (dependencies: lv_page, lv_btn, lv_label, (lv_img optionally for
 * icons))
 */

#ifdef CONFIG_USE_LV_LIST
#  define USE_LV_LIST        CONFIG_USE_LV_LIST
#else
#  define USE_LV_LIST        0
#endif

/* Drop down list (dependencies: lv_page, lv_label) */

#ifdef CONFIG_USE_LV_DDLIST
#  define USE_LV_DDLIST      CONFIG_USE_LV_DDLIST
#else
#  define USE_LV_DDLIST      0
#endif

/* Roller (dependencies: lv_ddlist) */

#ifdef CONFIG_USE_LV_ROLLER
#  define USE_LV_ROLLER      CONFIG_USE_LV_ROLLER
#else
#  define USE_LV_ROLLER      0
#endif

/* Slider (dependencies: lv_bar) */

#ifdef CONFIG_USE_LV_SLIDER
#  define USE_LV_SLIDER      CONFIG_USE_LV_SLIDER
#else
#  define USE_LV_SLIDER      0
#endif

#endif /*__APPS_GRAPHICS_LITTLEVGL_LV_CONF_H*/
