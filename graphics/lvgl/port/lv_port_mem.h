/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_mem.h
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

#ifndef __APPS_GRAPHICS_LVGL_PORT_LV_PORT_MEM_H
#define __APPS_GRAPHICS_LVGL_PORT_LV_PORT_MEM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <lvgl/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_LV_PORT_MEM_CUSTOM_SIZE
#  define CONFIG_LV_PORT_MEM_CUSTOM_SIZE 0
#endif

#if CONFIG_LV_PORT_MEM_CUSTOM_SIZE

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
 * Name: lv_port_mem_alloc
 ****************************************************************************/

FAR void *lv_port_mem_alloc(size_t size);

/****************************************************************************
 * Name: lv_port_mem_free
 ****************************************************************************/

void lv_port_mem_free(FAR void *mem);

/****************************************************************************
 * Name: lv_port_mem_realloc
 ****************************************************************************/

FAR void *lv_port_mem_realloc(FAR void *oldmem, size_t size);

/****************************************************************************
 * Name: lv_port_mem_memalign
 ****************************************************************************/

FAR void *lv_port_mem_memalign(size_t alignment, size_t size);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LV_PORT_MEM_CUSTOM_SIZE */

#endif /* __APPS_GRAPHICS_LVGL_PORT_LV_PORT_MEM_H */
