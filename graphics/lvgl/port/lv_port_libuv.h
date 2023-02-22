/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_libuv.h
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

#ifndef __APPS_GRAPHICS_LVGL_PORT_LV_PORT_LIBUV_H
#define __APPS_GRAPHICS_LVGL_PORT_LV_PORT_LIBUV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_LIBUV)

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
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_libuv_init
 *
 * Description:
 *   Add the UI event loop to the uv_loop.
 *
 * Input Parameters:
 *   loop - Pointer to uv_loop.
 *
 * Returned Value:
 *   Pointer to UI event context.
 *
 ****************************************************************************/

FAR void *lv_port_libuv_init(FAR void *loop);

/****************************************************************************
 * Name: lv_port_libuv_uninit
 *
 * Description:
 *   Remove the UI event loop.
 *
 * Input Parameters:
 *   Pointer to UI event context.
 *
 ****************************************************************************/

void lv_port_libuv_uninit(FAR void *ctx);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LIBUV */

#endif /* __APPS_GRAPHICS_LVGL_PORT_LV_PORT_LIBUV_H */
