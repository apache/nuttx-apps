/****************************************************************************
 * examples/touchscreen/tc.h
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

#ifndef __APPS_EXAMPLES_LVGLDEMO_TP_H
#define __APPS_EXAMPLES_LVGLDEMO_TP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <graphics/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* CONFIG_NSH_BUILTIN_APPS - Build the touchscreen test as
 *   an NSH built-in function.  Default: Built as a standalone program
 * CONFIG_EXAMPLES_LGVLDEMO_MINOR - The minor device number.  Minor=N
 *   corresponds to touchscreen device /dev/input0.  Note this value must
 *   with CONFIG_EXAMPLES_LGVLDEMO_DEVPATH.  Default 0.
 * CONFIG_EXAMPLES_LGVLDEMO_DEVPATH - The path to the touchscreen
 *   device.  This must be consistent with CONFIG_EXAMPLES_LGVLDEMO_MINOR.
 *   Default: "/dev/input0"
 * CONFIG_EXAMPLES_LGVLDEMO_MOUSE - The touchscreen test can also be
 *   configured to work with a mouse driver by setting this option.
 */

#ifndef CONFIG_INPUT
#  error "Input device support is not enabled (CONFIG_INPUT)"
#endif

#ifndef CONFIG_EXAMPLES_LGVLDEMO_MINOR
#  undef  CONFIG_EXAMPLES_LGVLDEMO_DEVPATH
#  define CONFIG_EXAMPLES_LGVLDEMO_MINOR 0
#  ifdef CONFIG_EXAMPLES_LGVLDEMO_MOUSE
#    define CONFIG_EXAMPLES_LGVLDEMO_DEVPATH "/dev/mouse0"
#  else
#    define CONFIG_EXAMPLES_LGVLDEMO_DEVPATH "/dev/input0"
#  endif
#endif

#ifndef CONFIG_EXAMPLES_LGVLDEMO_DEVPATH
#  undef  CONFIG_EXAMPLES_LGVLDEMO_MINOR
#  define CONFIG_EXAMPLES_LGVLDEMO_MINOR 0
#  ifdef CONFIG_EXAMPLES_LGVLDEMO_MOUSE
#    define CONFIG_EXAMPLES_LGVLDEMO_DEVPATH "/dev/mouse0"
#  else
#    define CONFIG_EXAMPLES_LGVLDEMO_DEVPATH "/dev/input0"
#  endif
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: tp_init
 *
 * Description:
 *   Initialize The Touch pad
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a positive error code on failure.
 *
 ****************************************************************************/

int tp_init(void);

/****************************************************************************
 * Name: tp_read
 *
 * Description:
 *   Read a TP data and store in 'data' argument
 *
 * Input Parameters:
 *   data - Store the x, y and state information here
 *
 * Returned Value:
 *   false: no more data to read; true: there are more data to read.
 *
 ****************************************************************************/

bool tp_read(FAR lv_indev_data_t *data);

/****************************************************************************
 * Name: tp_read
 *
 * Description:
 *   Set calibration data
 *
 * Input Parameters:
 *   ul - Upper left hand corner TP value
 *   ur - Upper right hand corner TP value
 *   lr - Lower right hand corner TP value
 *   ll - Lower left hand corner TP value
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void tp_set_cal_values(FAR lv_point_t *ul, FAR lv_point_t *ur,
                       FAR lv_point_t *lr, FAR lv_point_t *ll);

#endif /* __APPS_EXAMPLES_LVGLDEMO_TP_H */
