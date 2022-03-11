/****************************************************************************
 * apps/examples/lvgldemo/tp.h
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

#ifndef __APPS_EXAMPLES_LVGLDEMO_TP_H
#define __APPS_EXAMPLES_LVGLDEMO_TP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <lvgl/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* CONFIG_EXAMPLES_LVGLDEMO_MINOR - The minor device number.  Minor=N
 *   corresponds to touchscreen device /dev/input0.  Note this value must
 *   with CONFIG_EXAMPLES_LVGLDEMO_DEVPATH.  Default 0.
 * CONFIG_EXAMPLES_LVGLDEMO_DEVPATH - The path to the touchscreen
 *   device.  This must be consistent with CONFIG_EXAMPLES_LVGLDEMO_MINOR.
 *   Default: "/dev/input0"
 * CONFIG_EXAMPLES_LVGLDEMO_MOUSE - The touchscreen test can also be
 *   configured to work with a mouse driver by setting this option.
 */

#if !defined(CONFIG_INPUT_TOUCHSCREEN) && !defined(CONFIG_INPUT_MOUSE)
#  error "Input device support is not enabled (CONFIG_INPUT_TOUCHSCREEN || CONFIG_INPUT_MOUSE)"
#endif

#ifndef CONFIG_EXAMPLES_LVGLDEMO_MINOR
#  undef  CONFIG_EXAMPLES_LVGLDEMO_DEVPATH
#  define CONFIG_EXAMPLES_LVGLDEMO_MINOR 0
#  ifdef CONFIG_EXAMPLES_LVGLDEMO_MOUSE
#    define CONFIG_EXAMPLES_LVGLDEMO_DEVPATH "/dev/mouse0"
#  else
#    define CONFIG_EXAMPLES_LVGLDEMO_DEVPATH "/dev/input0"
#  endif
#endif

#ifndef CONFIG_EXAMPLES_LVGLDEMO_DEVPATH
#  undef  CONFIG_EXAMPLES_LVGLDEMO_MINOR
#  define CONFIG_EXAMPLES_LVGLDEMO_MINOR 0
#  ifdef CONFIG_EXAMPLES_LVGLDEMO_MOUSE
#    define CONFIG_EXAMPLES_LVGLDEMO_DEVPATH "/dev/mouse0"
#  else
#    define CONFIG_EXAMPLES_LVGLDEMO_DEVPATH "/dev/input0"
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
 *   indev_drv - Input device handler
 *   data - Store the x, y and state information here
 *
 * Returned Value:
 *   false: no more data to read; true: there are more data to read.
 *
 ****************************************************************************/

bool tp_read(FAR struct _lv_indev_drv_t *indev_drv,
             FAR lv_indev_data_t *data);

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
