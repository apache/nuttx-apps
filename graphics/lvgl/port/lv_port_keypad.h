/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_keypad.h
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

#ifndef __APPS_GRAPHICS_LVGL_PORT_LV_PORT_KEYPAD_H
#define __APPS_GRAPHICS_LVGL_PORT_LV_PORT_KEYPAD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <lvgl/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_LV_PORT_USE_KEYPAD)

/****************************************************************************
 * Type Definitions
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
 * Name: lv_port_keypad_init
 *
 * Description:
 *   Keypad interface initialization.
 *
 * Input Parameters:
 *   dev_path - input device path, set to NULL to use the default path
 *
 * Returned Value:
 *   lv_indev object address on success; NULL on failure.
 *
 ****************************************************************************/

FAR lv_indev_t *lv_port_keypad_init(FAR const char *dev_path);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LV_PORT_USE_KEYPAD */

#endif /* __APPS_GRAPHICS_LVGL_PORT_LV_PORT_KEYPAD_H */
