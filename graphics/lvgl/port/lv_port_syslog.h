/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_syslog.h
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

#ifndef __APPS_GRAPHICS_LVGL_PORT_LV_PORT_SYSLOG_H
#define __APPS_GRAPHICS_LVGL_PORT_LV_PORT_SYSLOG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_LV_USE_LOG)

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
 * Name: lv_port_syslog_init
 *
 * Description:
 *   Syslog interface initialization.
 *
 ****************************************************************************/

void lv_port_syslog_init(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LV_USE_LOG */

#endif /* __APPS_GRAPHICS_LVGL_PORT_LV_PORT_SYSLOG_H */
