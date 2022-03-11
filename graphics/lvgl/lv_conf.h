/****************************************************************************
 * apps/graphics/lvgl/lv_conf.h
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

#ifndef __APPS_GRAPHICS_LVGL_LV_CONF_H
#define __APPS_GRAPHICS_LVGL_LV_CONF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/* Type of coordinates. Should be `int16_t`
 * (or `int32_t` for extreme cases)
 */

typedef int16_t lv_coord_t;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Graphical settings
 ****************************************************************************/

/* Maximal horizontal and vertical resolution to support by the library. */

#define LV_HOR_RES_MAX          CONFIG_LV_HOR_RES
#define LV_VER_RES_MAX          CONFIG_LV_VER_RES

/* Color depth:
 * - 1:  1 byte per pixel
 * - 8:  RGB233
 * - 16: RGB565
 * - 32: ARGB8888
 */

#define LV_COLOR_DEPTH     CONFIG_LV_COLOR_DEPTH

/* Swap the 2 bytes of RGB565 color.
 * Useful if the display has a 8 bit interface (e.g. SPI)
 */

#ifdef CONFIG_LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP    CONFIG_LV_COLOR_16_SWAP
#else
#define LV_COLOR_16_SWAP    0
#endif

/* 1: Enable screen transparency.
 * Useful for OSD or other overlapping GUIs.
 * Requires `LV_COLOR_DEPTH = 32` colors and the screen's style
 * should be modified: `style.body.opa = ...`
 */

#ifdef CONFIG_LV_COLOR_SCREEN_TRANSP
#define LV_COLOR_SCREEN_TRANSP    CONFIG_LV_COLOR_SCREEN_TRANSP
#else
#define LV_COLOR_SCREEN_TRANSP    0
#endif

/* Images pixels with this color will not be drawn (with chroma keying) */

/* LV_COLOR_LIME: pure green */

#define LV_COLOR_TRANSP    ((lv_color_t){.full = (CONFIG_LV_COLOR_TRANSP)})

/* Enable chroma keying for indexed images. */

#define LV_INDEXED_CHROMA    1

/* Enable anti-aliasing (lines, and radiuses will be smoothed) */

#ifdef CONFIG_LV_ANTIALIAS
#define LV_ANTIALIAS        CONFIG_LV_ANTIALIAS
#else
#define LV_ANTIALIAS        0
#endif

/* Default display refresh period.
 * Can be changed in the display driver (`lv_disp_drv_t`).
 */

#define LV_DISP_DEF_REFR_PERIOD      CONFIG_LV_DISP_DEF_REFR_PERIOD   /* [ms] */

/* Dot Per Inch: used to initialize default sizes.
 * E.g. a button with width = LV_DPI / 2 -> half inch wide
 * (Not so important, you can adjust it to modify default sizes and spaces)
 */

#define LV_DPI              CONFIG_LV_DPI     /* [px] */

/* The the real width of the display changes some default values:
 * default object sizes, layout of examples, etc.
 * According to the width of the display (hor. res. / dpi)
 * the displays fall in 4 categories.
 * The 4th is extra large which has no upper limit so not listed here
 * The upper limit of the categories are set below in 0.1 inch unit.
 */

#define LV_DISP_SMALL_LIMIT  30
#define LV_DISP_MEDIUM_LIMIT 50
#define LV_DISP_LARGE_LIMIT  70

/****************************************************************************
 * Memory manager settings
 ****************************************************************************/

/* LittelvGL's internal memory manager's settings.
 * The graphical objects and other related data are stored here.
 */

/* 1: use custom malloc/free, 0: use the built-in
 * `lv_mem_alloc` and `lv_mem_free`
 */

#define LV_MEM_CUSTOM      1
#if LV_MEM_CUSTOM == 0

/* Size of the memory used by `lv_mem_alloc` in bytes (>= 2kB) */

#  define LV_MEM_SIZE    CONFIG_LV_MEM_SIZE

/* Compiler prefix for a big array declaration */

#  define LV_MEM_ATTR

/* Set an address for the memory pool instead of allocating it as an array.
 * Can be in external SRAM too.
 */

#  define LV_MEM_ADR          0

/* Automatically defrag. on free. Defrag. means
 * joining the adjacent free cells.
 */

#  define LV_MEM_AUTO_DEFRAG  1
#else       /* LV_MEM_CUSTOM */
#  define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /* Header for the dynamic memory function */
#  define LV_MEM_CUSTOM_ALLOC   malloc       /* Wrapper to malloc */
#  define LV_MEM_CUSTOM_FREE    free         /* Wrapper to free */
#endif     /* LV_MEM_CUSTOM */

/* Garbage Collector settings
 * Used if lvgl is binded to higher level language and the memory is
 * managed by that language
 */

#define LV_ENABLE_GC 0
#if LV_ENABLE_GC != 0
#  define LV_GC_INCLUDE "gc.h"                           /* Include Garbage Collector related things */
#  define LV_MEM_CUSTOM_REALLOC   your_realloc           /* Wrapper to realloc */
#  define LV_MEM_CUSTOM_GET_SIZE  your_mem_get_size      /* Wrapper to lv_mem_get_size */
#endif /* LV_ENABLE_GC */

/****************************************************************************
 * Input device settings
 ****************************************************************************/

/* Input device default settings.
 * Can be changed in the Input device driver (`lv_indev_drv_t`)
 */

/* Input device read period in milliseconds */

#define LV_INDEV_DEF_READ_PERIOD          CONFIG_LV_INDEV_DEF_READ_PERIOD

/* Drag threshold in pixels */

#define LV_INDEV_DEF_DRAG_LIMIT           CONFIG_LV_INDEV_DEF_DRAG_LIMIT

/* Drag throw slow-down in [%]. Greater value -> faster slow-down */

#define LV_INDEV_DEF_DRAG_THROW           CONFIG_LV_INDEV_DEF_DRAG_THROW

/* Long press time in milliseconds.
 * Time to send `LV_EVENT_LONG_PRESSSED`)
 */

#define LV_INDEV_DEF_LONG_PRESS_TIME      CONFIG_LV_INDEV_DEF_LONG_PRESS_TIME

/* Repeated trigger period in long press [ms]
 * Time between `LV_EVENT_LONG_PRESSED_REPEAT
 */

#define LV_INDEV_DEF_LONG_PRESS_REP_TIME \
          CONFIG_LV_INDEV_DEF_LONG_PRESS_REP_TIME

/* Gesture threshold in pixels */

#define LV_INDEV_DEF_GESTURE_LIMIT   CONFIG_LV_INDEV_DEF_GESTURE_LIMIT

/* Gesture min velocity at release before swipe (pixels) */

#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY \
          CONFIG_LV_INDEV_DEF_GESTURE_MIN_VELOCITY

/****************************************************************************
 * Feature usage
 ****************************************************************************/

/* 1: Enable the Animations */

#ifdef CONFIG_USE_LV_ANIMATION
#define LV_USE_ANIMATION        CONFIG_USE_LV_ANIMATION
#else
#define LV_USE_ANIMATION        0
#endif

#if LV_USE_ANIMATION

/* Declare the type of the user data of animations
 * (can be e.g. `void *`, `int`, `struct`)
 */

typedef void * lv_anim_user_data_t;

#endif

/* 1: Enable shadow drawing */

#ifdef CONFIG_USE_LV_SHADOW
#define LV_USE_SHADOW           CONFIG_USE_LV_SHADOW
#else
#define LV_USE_SHADOW           0
#endif

#if LV_USE_SHADOW
/* Allow buffering some shadow calculation
 * LV_SHADOW_CACHE_SIZE is the max. shadow size to buffer,
 * where shadow size is `shadow_width + radius`
 * Caching has LV_SHADOW_CACHE_SIZE^2 RAM cost
 */

# define LV_SHADOW_CACHE_SIZE    0
#endif

/* 1: Use other blend modes than normal (`LV_BLEND_MODE_...`) */

#define LV_USE_BLEND_MODES      1

/* 1: Use the `opa_scale` style property to set the opacity
 * of an object and its children at once
 */

#define LV_USE_OPA_SCALE        1

/* 1: Use image zoom and rotation */

#define LV_USE_IMG_TRANSFORM    1

/* 1: Enable object groups (for keyboard/encoder navigation) */

#ifdef CONFIG_USE_LV_GROUP
#define LV_USE_GROUP            CONFIG_USE_LV_GROUP
#else
#define LV_USE_GROUP            0
#endif
#if LV_USE_GROUP
typedef void * lv_group_user_data_t;
#endif  /* LV_USE_GROUP */

/* 1: Enable GPU interface */

#ifdef CONFIG_USE_LV_GPU
#define LV_USE_GPU              CONFIG_USE_LV_GPU
#else
#define LV_USE_GPU              0
#endif

/* 1: Enable file system (might be required for images */

#ifdef CONFIG_USE_LV_FILESYSTEM
#define LV_USE_FILESYSTEM       CONFIG_USE_LV_FILESYSTEM
#else
#define LV_USE_FILESYSTEM       0
#endif

#if LV_USE_FILESYSTEM
/* Declare the type of the user data of file system drivers
 * (can be e.g. `void *`, `int`, `struct`)
 */

typedef void * lv_fs_drv_user_data_t;
#endif

/* 1: Add a `user_data` to drivers and objects */

#ifdef CONFIG_LV_USE_USER_DATA
#define LV_USE_USER_DATA        CONFIG_LV_USE_USER_DATA
#else
#define LV_USE_USER_DATA        0
#endif

/* 1: Show CPU usage and FPS count in the right bottom corner */

#ifdef CONFIG_LV_USE_PERF_MONITOR
#define LV_USE_PERF_MONITOR        CONFIG_LV_USE_PERF_MONITOR
#else
#define LV_USE_PERF_MONITOR        0
#endif

/* 1: Use the functions and types from the older API if possible */

#define LV_USE_API_EXTENSION_V6  1

/****************************************************************************
 * Image decoder and cache
 ****************************************************************************/

/* 1: Enable indexed (palette) images */

#ifdef CONFIG_LV_IMG_CF_INDEXED
#define LV_IMG_CF_INDEXED       CONFIG_LV_IMG_CF_INDEXED
#else
#define LV_IMG_CF_INDEXED       0
#endif

/* 1: Enable alpha indexed images */

#ifdef  CONFIG_LV_IMG_CF_ALPHA
#define LV_IMG_CF_ALPHA         CONFIG_LV_IMG_CF_ALPHA
#else
#define LV_IMG_CF_ALPHA         0
#endif

/* Default image cache size. Image caching keeps the images opened.
 * If only the built-in image formats are used there is
 * no real advantage of caching.
 * (I.e. no new image decoder is added)
 * With complex image decoders (e.g. PNG or JPG) caching can
 * save the continuous open/decode of images.
 * However the opened images might consume additional RAM.
 * LV_IMG_CACHE_DEF_SIZE must be >= 1
 */

#define LV_IMG_CACHE_DEF_SIZE       1

/* Declare the type of the user data of image decoder
 * (can be e.g. `void *`, `int`, `struct`)
 */

typedef void * lv_img_decoder_user_data_t;

/****************************************************************************
 *  Compiler settings
 ****************************************************************************/

/* Define a custom attribute to `lv_tick_inc` function */

#define LV_ATTRIBUTE_TICK_INC

/* Define a custom attribute to `lv_task_handler` function */

#define LV_ATTRIBUTE_TASK_HANDLER

/* With size optimization (-Os) the compiler might not align data to
 * 4 or 8 byte boundary. This alignment will be explicitly applied
 * where needed.
 * E.g. __attribute__((aligned(4)))
 */

#define LV_ATTRIBUTE_MEM_ALIGN

/* Attribute to mark large constant arrays for example
 * font's bitmaps
 */

#define LV_ATTRIBUTE_LARGE_CONST

/* Prefix performance critical functions to place them into a
 * faster memory (e.g RAM). Uses 15-20 kB extra memory
 */

#define LV_ATTRIBUTE_FAST_MEM

/* Export integer constant to binding.
 * This macro is used with constants in the form of LV_<CONST> that
 * should also appear on lvgl binding API such as Micropython
 *
 * The default value just prevents a GCC warning.
 */

#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning

/****************************************************************************
 *  HAL settings
 ****************************************************************************/

/* 1: use a custom tick source.
 * It removes the need to manually update the tick with `lv_tick_inc`)
 */

#define LV_TICK_CUSTOM     1
#if LV_TICK_CUSTOM == 1
#define LV_TICK_CUSTOM_INCLUDE  "lv_tick_interface.h"       /* Header for the sys time function */
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (lv_tick_interface())  /* Expression evaluating to current systime in ms */
#endif   /* LV_TICK_CUSTOM */

typedef void * lv_disp_drv_user_data_t;             /* Type of user data in the display driver */
typedef void * lv_indev_drv_user_data_t;            /* Type of user data in the input device driver */

/****************************************************************************
 * Log settings
 ****************************************************************************/

/* 1: Enable the log module */

#ifdef CONFIG_LV_USE_LOG
#define LV_USE_LOG      CONFIG_LV_USE_LOG
#else
#define LV_USE_LOG      0
#endif

#if LV_USE_LOG
/* How important log should be added:
 * LV_LOG_LEVEL_TRACE - A lot of logs to give detailed information
 * LV_LOG_LEVEL_INFO  - Log important events
 * LV_LOG_LEVEL_WARN  - Log if something happened but didn't cause a crash
 * LV_LOG_LEVEL_ERROR - Only critical issue, when the system may fail
 * LV_LOG_LEVEL_NONE  - Do not log anything
 */

#ifdef CONFIG_LV_LOG_LEVEL_TRACE
#  define LV_LOG_LEVEL    LV_LOG_LEVEL_TRACE
#endif

#ifdef CONFIG_LV_LOG_LEVEL_INFO
#  define LV_LOG_LEVEL    LV_LOG_LEVEL_INFO
#endif

#ifdef CONFIG_LV_LOG_LEVEL_WARN
#  define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#endif

#ifdef CONFIG_LV_LOG_LEVEL_ERROR
#  define LV_LOG_LEVEL    LV_LOG_LEVEL_ERROR
#endif

#ifdef CONFIG_LV_LOG_LEVEL_NONE
#  define LV_LOG_LEVEL    LV_LOG_LEVEL_NONE
#endif

/* 1: Print the log with 'printf';
 * 0: user need to register a callback with `lv_log_register_print_cb`
 */

#ifdef CONFIG_LV_LOG_PRINTF
#  define LV_LOG_PRINTF   CONFIG_LV_LOG_PRINTF
#else
#  define LV_LOG_PRINTF   0
#endif
#endif  /* LV_USE_LOG */

/****************************************************************************
 * Debug settings
 ****************************************************************************/

/* If Debug is enabled LittelvGL validates the parameters of the functions.
 * If an invalid parameter is found an error log message is printed and
 * the MCU halts at the error. (`LV_USE_LOG` should be enabled)
 * If you are debugging the MCU you can pause
 * the debugger to see exactly where  the issue is.
 *
 * The behavior of asserts can be overwritten by redefining them here.
 * E.g. #define LV_ASSERT_MEM(p)  <my_assert_code>
 */

#ifndef CONFIG_LV_USE_DEBUG
#define CONFIG_LV_USE_DEBUG   0
#endif

#define LV_USE_DEBUG      CONFIG_LV_USE_DEBUG

/* Check if the parameter is NULL. (Quite fast) */

#ifdef CONFIG_LV_USE_ASSERT_NULL
#define LV_USE_ASSERT_NULL      CONFIG_LV_USE_ASSERT_NULL
#else
#define LV_USE_ASSERT_NULL      0
#endif

/* Checks is the memory is successfully allocated or no. (Quite fast) */

#ifdef CONFIG_LV_USE_ASSERT_MEM
#define LV_USE_ASSERT_MEM      CONFIG_LV_USE_ASSERT_MEM
#else
#define LV_USE_ASSERT_MEM      0
#endif

/* Check the integrity of `lv_mem` after critical operations. (Slow) */

#define LV_USE_ASSERT_MEM_INTEGRITY       0

/* Check the strings.
 * Search for NULL, very long strings, invalid characters,
 * and unnatural repetitions. (Slow)
 * If disabled `LV_USE_ASSERT_NULL` will be performed instead
 * (if it's enabled)
 */
#ifdef CONFIG_LV_USE_ASSERT_STR
#define LV_USE_ASSERT_STR      CONFIG_LV_USE_ASSERT_STR
#else
#define LV_USE_ASSERT_STR      0
#endif

/* Check NULL, the object's type and existence
 * (e.g. not deleted). (Quite slow)
 * If disabled `LV_USE_ASSERT_NULL` will be
 * performed instead (if it's enabled)
 */

#ifdef CONFIG_LV_USE_ASSERT_OBJ
#define LV_USE_ASSERT_OBJ      CONFIG_LV_USE_ASSERT_OBJ
#else
#define LV_USE_ASSERT_OBJ      0
#endif

/* Check if the styles are properly initialized. (Fast) */

#ifdef CONFIG_LV_USE_ASSERT_STYLE
#define LV_USE_ASSERT_STYLE      CONFIG_LV_USE_ASSERT_STYLE
#else
#define LV_USE_ASSERT_STYLE      0
#endif

/****************************************************************************
 *  THEME USAGE
 ****************************************************************************/

/* Always enable at least on theme */

/* No theme, you can apply your styles as you need
 * No flags. Set LV_THEME_DEFAULT_FLAG 0
 */

#define LV_USE_THEME_EMPTY       1

/* Simple to the create your theme based on it
 * No flags. Set LV_THEME_DEFAULT_FLAG 0
 */

#define LV_USE_THEME_TEMPLATE    1

/* A fast and impressive theme.
 * Flags:
 * LV_THEME_MATERIAL_FLAG_LIGHT: light theme
 * LV_THEME_MATERIAL_FLAG_DARK: dark theme
 */

#define LV_USE_THEME_MATERIAL    1

/* Mono-color theme for monochrome displays.
 * If LV_THEME_DEFAULT_COLOR_PRIMARY is LV_COLOR_BLACK the
 * texts and borders will be black and the background will be
 * white. Else the colors are inverted.
 * No flags. Set LV_THEME_DEFAULT_FLAG 0
 */

#define LV_USE_THEME_MONO        1

#define LV_THEME_DEFAULT_INCLUDE            <stdint.h>      /* Include a header for the init. function */
#define LV_THEME_DEFAULT_INIT               lv_theme_material_init
#define LV_THEME_DEFAULT_COLOR_PRIMARY      LV_COLOR_RED
#define LV_THEME_DEFAULT_COLOR_SECONDARY    LV_COLOR_BLUE
#define LV_THEME_DEFAULT_FLAG               LV_THEME_MATERIAL_FLAG_LIGHT
#define LV_THEME_DEFAULT_FONT_SMALL         &lv_font_montserrat_16
#define LV_THEME_DEFAULT_FONT_NORMAL        &lv_font_montserrat_16
#define LV_THEME_DEFAULT_FONT_SUBTITLE      &lv_font_montserrat_16
#define LV_THEME_DEFAULT_FONT_TITLE         &lv_font_montserrat_16

/****************************************************************************
 *    FONT USAGE
 ****************************************************************************/

/* The built-in fonts contains the ASCII range and some Symbols
 * with  4 bit-per-pixel.
 * The symbols are available via `LV_SYMBOL_...` defines
 * More info about fonts: https://docs.lvgl.io/v7/en/html/overview/font.html
 * To create a new font go to: https://lvgl.com/ttf-font-to-c-array
 */

/* Montserrat fonts with bpp = 4
 * https://fonts.google.com/specimen/Montserrat
 */

/* They only take up storage space after being used,
 * so we can enable them all by default
 */

#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    1
#define LV_FONT_MONTSERRAT_20    1
#define LV_FONT_MONTSERRAT_22    1
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_26    1
#define LV_FONT_MONTSERRAT_28    1
#define LV_FONT_MONTSERRAT_30    1
#define LV_FONT_MONTSERRAT_32    1
#define LV_FONT_MONTSERRAT_34    1
#define LV_FONT_MONTSERRAT_36    1
#define LV_FONT_MONTSERRAT_38    1
#define LV_FONT_MONTSERRAT_40    1
#define LV_FONT_MONTSERRAT_42    1
#define LV_FONT_MONTSERRAT_44    1
#define LV_FONT_MONTSERRAT_46    1
#define LV_FONT_MONTSERRAT_48    1

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /* bpp = 3 */
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /* Hebrew, Arabic, PErisan letters and all their forms */
#define LV_FONT_SIMSUN_16_CJK            0  /* 1000 most common CJK radicals */

/* Pixel perfect monospace font
 * http://pelulamu.net/unscii/
 */

#ifdef CONFIG_USE_LV_FONT_UNSCII_8
#define LV_FONT_UNSCII_8      CONFIG_USE_LV_FONT_UNSCII_8
#else
#define LV_FONT_UNSCII_8      0
#endif

/* Optionally declare your custom fonts here.
 * You can use these fonts as default font too
 * and they will be available globally. E.g.
 * #define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(my_font_1) \
 *                                LV_FONT_DECLARE(my_font_2)
 */

#define LV_FONT_CUSTOM_DECLARE

/* Enable it if you have fonts with a lot of characters.
 * The limit depends on the font size, font face and bpp
 * but with > 10,000 characters if you see issues probably you need
 * to enable it.
 */

#define LV_FONT_FMT_TXT_LARGE   0

/* Set the pixel order of the display.
 * Important only if "subpx fonts" are used.
 * With "normal" font it doesn't matter.
 */

#define LV_FONT_SUBPX_BGR    0

/* Declare the type of the user data of fonts
 * (can be e.g. `void *`, `int`, `struct`)
 */

typedef void * lv_font_user_data_t;

/****************************************************************************
 *  Text settings
 ****************************************************************************/

/* Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 */

#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* Can break (wrap) texts on these chars */

#define LV_TXT_BREAK_CHARS                  CONFIG_LV_TXT_BREAK_CHARS

/* If a word is at least this long, will break wherever "prettiest"
 * To disable, set to a value <= 0
 */

#define LV_TXT_LINE_BREAK_LONG_LEN          0

/* Minimum number of characters in a long word to put
 * on a line before a break.
 * Depends on LV_TXT_LINE_BREAK_LONG_LEN.
 */

#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3

/* Minimum number of characters in a long word to put on a
 * line after a break.
 * Depends on LV_TXT_LINE_BREAK_LONG_LEN.
 */

#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* The control character to use for signalling text recoloring. */

#define LV_TXT_COLOR_CMD "#"

/* Support bidirectional texts.
 * Allows mixing Left-to-Right and Right-to-Left texts.
 * The direction will be processed according to the
 * Unicode Bidirectioanl Algorithm:
 * https://www.w3.org/International/articles/inline-bidi-markup/uba-basics
 */

#define LV_USE_BIDI     0
#if LV_USE_BIDI
/* Set the default direction. Supported values:
 * `LV_BIDI_DIR_LTR` Left-to-Right
 * `LV_BIDI_DIR_RTL` Right-to-Left
 * `LV_BIDI_DIR_AUTO` detect texts base direction
 */

#define LV_BIDI_BASE_DIR_DEF  LV_BIDI_DIR_AUTO
#endif

/* Enable Arabic/Persian processing
 * In these languages characters should be replaced with
 * an other form based on their position in the text
 */

#define LV_USE_ARABIC_PERSIAN_CHARS 0

/* Change the built in (v)snprintf functions */

#define LV_SPRINTF_CUSTOM   1
#if LV_SPRINTF_CUSTOM
#  define LV_SPRINTF_INCLUDE <stdio.h>
#  define lv_snprintf     snprintf
#  define lv_vsnprintf    vsnprintf
#endif  /* LV_SPRINTF_CUSTOM */

/****************************************************************************
 *  LV_OBJ SETTINGS
 ****************************************************************************/

/* Declare the type of the user data of object
 * (can be e.g. `void *`, `int`, `struct`)
 */

typedef void * lv_obj_user_data_t;

/* 1: enable `lv_obj_realaign()` based on `lv_obj_align()` parameters */

#ifdef CONFIG_LV_OBJ_REALIGN
#define LV_USE_OBJ_REALIGN          CONFIG_LV_OBJ_REALIGN
#else
#define LV_USE_OBJ_REALIGN          0
#endif

/* Enable to make the object clickable on a larger area.
 * LV_EXT_CLICK_AREA_OFF or 0: Disable this feature
 * LV_EXT_CLICK_AREA_TINY: The extra area can be adjusted
 *  horizontally and vertically (0..255 px)
 * LV_EXT_CLICK_AREA_FULL: The extra area can be adjusted
 * in all 4 directions (-32k..+32k px)
 */

#define LV_USE_EXT_CLICK_AREA  LV_EXT_CLICK_AREA_TINY

/****************************************************************************
 *  LV OBJ X USAGE
 ****************************************************************************/

/* Documentation of the object types:
 * https://docs.littlevgl.com/#Object-types
 */

/* Arc (dependencies: -) */

#ifdef CONFIG_USE_LV_ARC
#define LV_USE_ARC      CONFIG_USE_LV_ARC
#else
#define LV_USE_ARC      0
#endif

/* Bar (dependencies: -) */

#ifdef CONFIG_USE_LV_BAR
#define LV_USE_BAR      CONFIG_USE_LV_BAR
#else
#define LV_USE_BAR      0
#endif

/* Button (dependencies: lv_cont) */

#ifdef CONFIG_USE_LV_BTN
#define LV_USE_BTN      CONFIG_USE_LV_BTN
#else
#define LV_USE_BTN      0
#endif

#if LV_USE_BTN != 0

/* Enable button-state animations - draw a
 * circle on click (dependencies: LV_USE_ANIMATION)
 */

#ifdef CONFIG_LV_BTN_INK_EFFECT
#define LV_BTN_INK_EFFECT      CONFIG_LV_BTN_INK_EFFECT
#else
#define LV_BTN_INK_EFFECT      0
#endif

#endif

/* Button matrix (dependencies: -) */

#ifdef CONFIG_USE_LV_BTNM
#define LV_USE_BTNM      CONFIG_USE_LV_BTNM
#else
#define LV_USE_BTNM      0
#endif

/* Calendar (dependencies: -) */

#ifdef CONFIG_USE_LV_CALENDAR
#define LV_USE_CALENDAR      CONFIG_USE_LV_CALENDAR
#else
#define LV_USE_CALENDAR      0
#endif

/* Canvas (dependencies: lv_img) */

#ifdef CONFIG_USE_LV_CANVAS
#define LV_USE_CANVAS      CONFIG_USE_LV_CANVAS
#else
#define LV_USE_CANVAS      0
#endif

/* Check box (dependencies: lv_btn, lv_label) */

#ifdef CONFIG_USE_LV_CB
#define LV_USE_CB      CONFIG_USE_LV_CB
#else
#define LV_USE_CB      0
#endif

/* Chart (dependencies: -) */

#ifdef CONFIG_USE_LV_CHART
#define LV_USE_CHART      CONFIG_USE_LV_CHART
#else
#define LV_USE_CHART      0
#endif

#if LV_USE_CHART
#  define LV_CHART_AXIS_TICK_LABEL_MAX_LEN    CONFIG_LV_CHART_AXIS_TICK_LABEL_MAX_LEN
#endif

/* Container (dependencies: -) */

#define LV_USE_CONT     1

/* Color picker (dependencies: -) */

#define LV_USE_CPICKER   1

/* Drop down list (dependencies: lv_page, lv_label, lv_symbol_def.h) */

#define LV_USE_DDLIST    1
#if LV_USE_DDLIST != 0
/* Open and close default animation time [ms] (0: no animation) */

#  define LV_DDLIST_DEF_ANIM_TIME     200
#endif

/* Gauge (dependencies:lv_bar, lv_lmeter) */

#define LV_USE_GAUGE    1

/* Image (dependencies: lv_label) */

#define LV_USE_IMG      1

/* Image Button (dependencies: lv_btn) */

#define LV_USE_IMGBTN   1
#if LV_USE_IMGBTN
/* 1: The imgbtn requires left, mid and right
 * parts and the width can be set freely
 */

#  define LV_IMGBTN_TILED 0
#endif

/* Keyboard (dependencies: lv_btnm) */

#define LV_USE_KB       1

/* Label (dependencies: -) */

#define LV_USE_LABEL    1
#if LV_USE_LABEL != 0
/* Hor, or ver. scroll speed [px/sec] in
 * 'LV_LABEL_LONG_ROLL/ROLL_CIRC' mode
 */

#  define LV_LABEL_DEF_SCROLL_SPEED       25

/* Waiting period at beginning/end of animation cycle */

#  define LV_LABEL_WAIT_CHAR_COUNT        3

/* Enable selecting text of the label */

#  define LV_LABEL_TEXT_SEL               0

/* Store extra some info in labels (12 bytes)
 * to speed up drawing of very long texts
 */

#  define LV_LABEL_LONG_TXT_HINT          0
#endif

/* LED (dependencies: -) */

#define LV_USE_LED      1

/* Line (dependencies: -) */

#define LV_USE_LINE     1

/* List (dependencies: lv_page, lv_btn, lv_label,
 * (lv_img optionally for icons))
 */

#define LV_USE_LIST     1
#if LV_USE_LIST != 0
/* Default animation time of focusing to a
 * list element [ms] (0: no animation)
 */

#  define LV_LIST_DEF_ANIM_TIME  100
#endif

/* Line meter (dependencies: *) */

#define LV_USE_LMETER   1

/* Message box (dependencies: lv_rect, lv_btnm, lv_label) */

#define LV_USE_MBOX     1

/* Page (dependencies: lv_cont) */

#define LV_USE_PAGE     1
#if LV_USE_PAGE != 0
/* Focus default animation time [ms] (0: no animation) */

#  define LV_PAGE_DEF_ANIM_TIME     400
#endif

/* Preload (dependencies: lv_arc, lv_anim) */

#define LV_USE_PRELOAD      1
#if LV_USE_PRELOAD != 0
#  define LV_PRELOAD_DEF_ARC_LENGTH   60      /* [deg] */
#  define LV_PRELOAD_DEF_SPIN_TIME    1000    /* [ms] */
#  define LV_PRELOAD_DEF_ANIM         LV_PRELOAD_TYPE_SPINNING_ARC
#endif

/* Roller (dependencies: lv_ddlist) */

#define LV_USE_ROLLER    1
#if LV_USE_ROLLER != 0
/* Focus animation time [ms] (0: no animation) */

#  define LV_ROLLER_DEF_ANIM_TIME     200

/* Number of extra "pages" when the roller is infinite */

#  define LV_ROLLER_INF_PAGES         7
#endif

/* Slider (dependencies: lv_bar) */

#define LV_USE_SLIDER    1

/* Spinbox (dependencies: lv_ta) */

#define LV_USE_SPINBOX       1

/* Switch (dependencies: lv_slider) */

#define LV_USE_SW       1

/* Text area (dependencies: lv_label, lv_page) */

#define LV_USE_TA       1
#if LV_USE_TA != 0
#  define LV_TA_DEF_CURSOR_BLINK_TIME 400     /* ms */
#  define LV_TA_DEF_PWD_SHOW_TIME     1500    /* ms */
#endif

/* Table (dependencies: lv_label) */

#define LV_USE_TABLE    1
#if LV_USE_TABLE
#  define LV_TABLE_COL_MAX    12
#endif

/* Tab (dependencies: lv_page, lv_btnm) */

#define LV_USE_TABVIEW      1
#  if LV_USE_TABVIEW != 0
/* Time of slide animation [ms] (0: no animation) */

#  define LV_TABVIEW_DEF_ANIM_TIME    300
#endif

/* Tileview (dependencies: lv_page) */

#define LV_USE_TILEVIEW     1
#if LV_USE_TILEVIEW
/* Time of slide animation [ms] (0: no animation) */

#  define LV_TILEVIEW_DEF_ANIM_TIME   300
#endif

/* Window (dependencies: lv_cont, lv_btn, lv_label, lv_img, lv_page) */

#define LV_USE_WIN      1

/****************************************************************************
 * Non-user section
 ****************************************************************************/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)    /* Disable warnings for Visual Studio*/
#  define _CRT_SECURE_NO_WARNINGS
#endif

#endif /* __APPS_GRAPHICS_LVGL_LV_CONF_H */
