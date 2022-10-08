/****************************************************************************
 * apps/graphics/lvgl/port/lv_port.c
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

#include "lv_port.h"
#include "lv_port_button.h"
#include "lv_port_encoder.h"
#include "lv_port_fbdev.h"
#include "lv_port_lcddev.h"
#include "lv_port_mem.h"
#include "lv_port_keypad.h"
#include "lv_port_syslog.h"
#include "lv_port_touchpad.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_init
 *
 * Description:
 *   Initialize all porting.
 *
 ****************************************************************************/

void lv_port_init(void)
{
#if defined(CONFIG_LV_USE_LOG)
  lv_port_syslog_init();
#endif

#if defined(CONFIG_LV_PORT_USE_LCDDEV)
  lv_port_lcddev_init(NULL, 0);
#endif

#if defined(CONFIG_LV_PORT_USE_FBDEV)
  lv_port_fbdev_init(NULL);
#endif

#if defined(CONFIG_LV_PORT_USE_BUTTON)
  lv_port_button_init(NULL);

#if defined(CONFIG_UINPUT_BUTTON)
  lv_port_button_init("/dev/ubutton");
#endif

#endif

#if defined(CONFIG_LV_PORT_USE_KEYPAD)
  lv_port_keypad_init(NULL);

#if defined(CONFIG_UINPUT_BUTTON)
  lv_port_keypad_init("/dev/ubutton");
#endif

#endif

#if defined(CONFIG_LV_PORT_USE_TOUCHPAD)
  lv_port_touchpad_init(NULL);

#if defined(CONFIG_UINPUT_TOUCH)
  lv_port_touchpad_init("/dev/utouch");
#endif

#endif

#if defined(CONFIG_LV_PORT_USE_ENCODER)
  lv_port_encoder_init(NULL);
#endif
}
