/****************************************************************************
 * apps/examples/lvgldemo/lv_test_theme_1.h
 *
 *   Copyright (C) 2016 2016 Gábor Kiss-Vámosi. All rights reserved.
 *   Author: Gábor Kiss-Vámosi <kisvegabor@gmail.com>
 *
 * Released under the following BSD-compatible MIT license:
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the “Software”), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __APPS_EXAMPLES_LVGLDEMO_LV_TEST_THEME_1_H
#define __APPS_EXAMPLES_LVGLDEMO_LV_TEST_THEME_1_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <graphics/lvgl.h>

#ifdef CONFIG_EXAMPLES_LVGLDEMO_THEME_1

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_THEME_1_HUE
#  define EXAMPLES_LVGLDEMO_THEME_1_HUE CONFIG_EXAMPLES_LVGLDEMO_THEME_1_HUE
#else
#  define EXAMPLES_LVGLDEMO_THEME_1_HUE 30
#endif

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
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a test screen with a lot objects and apply the given theme on them
 * @param th pointer to a theme
 */
void lv_test_theme_1(lv_theme_t *th);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_EXAMPLES_LVGLDEMO_THEME_1 */
#endif /* __APPS_EXAMPLES_LVGLDEMO_LVGL_DEMO */
