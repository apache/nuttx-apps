/****************************************************************************
 * apps/examples/lvgldemo/lv_test_theme_2.h
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

#ifndef __APPS_EXAMPLES_LVGLDEMO_LV_TEST_THEME_2_H
#define __APPS_EXAMPLES_LVGLDEMO_LV_TEST_THEME_2_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <graphics/lvgl.h>

#ifdef CONFIG_EXAMPLES_LVGLDEMO_THEME_2

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
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Test run time theme change */

void lv_test_theme_2(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_EXAMPLES_LVGLDEMO_THEME_2 */
#endif /* __APPS_EXAMPLES_LVGLDEMO_LV_TEST_THEME_2_H */
