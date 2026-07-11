/****************************************************************************
 * apps/examples/lvgldemo/lvgldemo.c
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
#include <unistd.h>
#include <sys/boardctl.h>

#include <lvgl/lvgl.h>
#include <lvgl/demos/lv_demos.h>
#ifdef CONFIG_LV_USE_NUTTX_LIBUV
#  include <uv.h>
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_LVGLDEMO_TOUCH_DIAGNOSTIC
static FAR lv_indev_t *g_touch_indev;
static FAR lv_obj_t *g_touch_label;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
static void lv_nuttx_uv_loop(FAR uv_loop_t *loop,
                             FAR lv_nuttx_result_t *result)
{
  lv_nuttx_uv_t uv_info;
  FAR void *data;

  uv_loop_init(loop);

  lv_memset(&uv_info, 0, sizeof(uv_info));
  uv_info.loop  = loop;
  uv_info.disp  = result->disp;
  uv_info.indev = result->indev;
#ifdef CONFIG_UINPUT_TOUCH
  uv_info.uindev = result->utouch_indev;
#endif

  data = lv_nuttx_uv_init(&uv_info);
  uv_run(loop, UV_RUN_DEFAULT);
  lv_nuttx_uv_deinit(&data);
}
#endif

#ifdef CONFIG_EXAMPLES_LVGLDEMO_TOUCH_DIAGNOSTIC
static void touch_diag_update(FAR lv_timer_t *timer)
{
  char text[64];
  lv_point_t point;
  lv_indev_state_t state;

  if (g_touch_indev == NULL)
    {
      return;
    }

  state = lv_indev_get_state(g_touch_indev);
  lv_indev_get_point(g_touch_indev, &point);
  snprintf(text, sizeof(text), "state: %s\nx: %d  y: %d",
           state == LV_INDEV_STATE_PRESSED ? "pressed" : "released",
           (int)point.x, (int)point.y);
  lv_label_set_text(g_touch_label, text);

  (void)timer;
}

static void touch_diag_create(FAR lv_indev_t *indev)
{
  FAR lv_obj_t *screen;
  FAR lv_obj_t *title;

  screen = lv_obj_create(NULL);
  title  = lv_label_create(screen);
  lv_label_set_text(title, "Touchscreen diagnostic");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

  g_touch_label = lv_label_create(screen);
  lv_obj_align(g_touch_label, LV_ALIGN_CENTER, 0, 0);
  g_touch_indev = indev;

  if (indev == NULL)
    {
      lv_label_set_text(g_touch_label,
                        "Touchscreen device could not be opened");
    }
  else
    {
      lv_label_set_text(g_touch_label,
                        "Touch the display to inspect coordinates");
      lv_timer_create(touch_diag_update, 50, NULL);
    }

  lv_scr_load(screen);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main or lv_demos_main
 *
 * Description:
 *
 * Input Parameters:
 *   Standard argc and argv
 *
 * Returned Value:
 *   Zero on success; a positive, non-zero value on failure.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
  lv_memzero(&ui_loop, sizeof(ui_loop));
#endif

  if (lv_is_initialized())
    {
      LV_LOG_ERROR("LVGL already initialized! aborting.");
      return -1;
    }

  lv_init();

  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#else
  info.fb_path = CONFIG_EXAMPLES_LVGLDEMO_FBDEVPATH;
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN
  info.input_path = CONFIG_EXAMPLES_LVGLDEMO_INPUT_DEVPATH;
#endif

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("lv_demos initialization failure!");
      lv_nuttx_deinit(&result);
      lv_deinit();
      return 1;
    }

#ifdef CONFIG_EXAMPLES_LVGLDEMO_TOUCH_DIAGNOSTIC
  touch_diag_create(result.indev);
#else
  if (!lv_demos_create(&argv[1], argc - 1))
    {
      lv_demos_show_help();

      /* we can add custom demos here */

      goto demo_end;
    }
#endif

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_nuttx_uv_loop(&ui_loop, &result);
#else
  while (1)
    {
      uint32_t idle;
      idle = lv_timer_handler();

      /* Minimum sleep of 1ms */

      idle = idle ? idle : 1;
      usleep(idle * 1000);
    }
#endif

#ifndef CONFIG_EXAMPLES_LVGLDEMO_TOUCH_DIAGNOSTIC
demo_end:
#endif
  lv_nuttx_deinit(&result);
  lv_deinit();

  return 0;
}
