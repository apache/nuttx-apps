/****************************************************************************
 * apps/examples/lvgltouch/lvgltouch_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lvgl/lvgl.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR lv_indev_t *g_touch;
static FAR lv_obj_t *g_status;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void lvgltouch_update(FAR lv_timer_t *timer)
{
  lv_indev_state_t state;
  lv_point_t point;
  char text[64];

  UNUSED(timer);

  state = lv_indev_get_state(g_touch);
  lv_indev_get_point(g_touch, &point);
  snprintf(text, sizeof(text), "state: %s\nx: %d  y: %d",
           state == LV_INDEV_STATE_PRESSED ? "pressed" : "released",
           point.x, point.y);
  lv_label_set_text(g_status, text);
}

static void lvgltouch_create(FAR lv_indev_t *indev)
{
  FAR lv_obj_t *screen;
  FAR lv_obj_t *title;

  screen = lv_obj_create(NULL);
  title = lv_label_create(screen);
  lv_label_set_text(title, "Touchscreen diagnostic");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

  g_status = lv_label_create(screen);
  lv_obj_align(g_status, LV_ALIGN_CENTER, 0, 0);

  if (indev == NULL)
    {
      lv_label_set_text(g_status, "Touchscreen device could not be opened");
    }
  else
    {
      g_touch = indev;
      lv_label_set_text(g_status, "Touch the display");
      lv_timer_create(lvgltouch_update, 50, NULL);
    }

  lv_screen_load(screen);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

  UNUSED(argc);
  UNUSED(argv);

  if (lv_is_initialized())
    {
      LV_LOG_ERROR("LVGL already initialized");
      return EXIT_FAILURE;
    }

  lv_init();
  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#else
  info.fb_path = CONFIG_EXAMPLES_LVGLTOUCH_FB_DEVPATH;
#endif
  info.input_path = CONFIG_EXAMPLES_LVGLTOUCH_INPUT_DEVPATH;
  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("display initialization failed");
      lv_nuttx_deinit(&result);
      lv_deinit();
      return EXIT_FAILURE;
    }

  lvgltouch_create(result.indev);

  for (; ; )
    {
      uint32_t idle = lv_timer_handler();

      usleep((idle ? idle : 1) * 1000);
    }

  return EXIT_SUCCESS;
}
