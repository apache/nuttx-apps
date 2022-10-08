/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_syslog.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <lvgl/lvgl.h>
#include <syslog.h>
#include "lv_port_syslog.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: syslog_print_cb
 ****************************************************************************/

static void syslog_print_cb(FAR const char *buf)
{
  syslog(LOG_INFO, "[LVGL] %s", buf);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_syslog_init
 *
 * Description:
 *   Syslog interface initialization.
 *
 ****************************************************************************/

void lv_port_syslog_init(void)
{
  lv_log_register_print_cb(syslog_print_cb);
}
