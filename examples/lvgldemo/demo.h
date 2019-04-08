/****************************************************************************
 * apps/examples/lvgldemo/demo.h
 *
 *   Copyright (C) 2019 Gábor Kiss-Vámosi. All rights reserved.
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

#ifndef __APPS_EXAMPLES_LVGLDEMO_DEMO_H
#define __APPS_EXAMPLES_LVGLDEMO_DEMO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <graphics/lvgl.h>

#ifdef CONFIG_EXAMPLES_LVGLDEMO_SIMPLE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_WALLPAPER
#  define LV_DEMO_WALLPAPER 1
#else
#  define LV_DEMO_WALLPAPER 0
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: demo_create
 *
 * Description:
 *   Initialize the LVGL demo screen
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *
 ****************************************************************************/

void demo_create(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_EXAMPLES_LVGLDEMO_SIMPLE */
#endif /* __APPS_EXAMPLES_LVGLDEMO_DEMO_H */
