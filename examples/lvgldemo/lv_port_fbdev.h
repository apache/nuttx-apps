/****************************************************************************
 * apps/examples/lvgldemo/lv_port_fbdev.h
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

#ifndef __APPS_EXAMPLES_LVGLDEMO_LV_PORT_FBDEV_H
#define __APPS_EXAMPLES_LVGLDEMO_LV_PORT_FBDEV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <lvgl/lvgl.h>

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
 * Name: lv_port_disp_init
 *
 * Description:
 *   Initialize and register the framebuffer display driver using the
 *   LVGL v9 display API. Implements Safeguard Architecture:
 *     1. Color probe test (behind CONFIG_EXAMPLES_LVGLDEMO_COLOR_PROBE)
 *     2. Smart flush callback with byte-order adaptation & forced Alpha
 *     3. Partial draw buffering (40 rows) to prevent heap exhaustion.
 *
 * Returned Value:
 *   Pointer to registered lv_display_t on success; NULL on failure.
 *
 ****************************************************************************/

lv_display_t *lv_port_disp_init(void);

/****************************************************************************
 * Name: lv_port_disp_deinit
 *
 * Description:
 *   Removes the LVGL display, unmaps memory, and closes framebuffer device.
 *
 ****************************************************************************/

void lv_port_disp_deinit(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_EXAMPLES_LVGLDEMO_LV_PORT_FBDEV_H */
