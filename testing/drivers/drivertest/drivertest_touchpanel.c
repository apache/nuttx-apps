/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_touchpanel.c
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
#include <nuttx/input/touchscreen.h>

#include <sys/mount.h>
#include <sys/ioctl.h>

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <cmocka.h>
#include <nuttx/crc32.h>

#include <lvgl/lvgl.h>

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
#include <uv.h>
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SQUARE_X 40
#define SQUARE_Y 80
#define CIRCULAR_PIX 25
#define SQUARE_PIX 20
#define DISTANCE_PIX 15

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  int fd;
  lv_indev_t *indev;
  bool is_square;

  union
  {
    struct
    {
      lv_obj_t *canvas;
      lv_layer_t layer;
      bool left_top;
      bool right_top;
      bool left_bottom;
      bool right_bottom;

      float line_k;

      /* left_top to right_bottom */

      float line_b_up_1;
      float line_b_down_1;

      /* right_top to left_bottom */

      float line_b_up_2;
      float line_b_down_2;
    };

    struct
    {
      lv_obj_t *slider_h;
      lv_obj_t *slider_v;
      lv_obj_t *arc_slider;
      int16_t diameter;
      lv_point_t arc_indic_pos;
      lv_obj_t *label_pass;
    };
  };
}touchpad_s;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: canvas_frame
 ****************************************************************************/

void darw_canvas_frame(touchpad_s *touchpad)
{
  lv_draw_rect_dsc_t rect_dsc;
  int i_hor;
  int j_ver;
  lv_draw_line_dsc_t line_dsc;
  lv_area_t rect_area;

  /* Draw squares */

  int sum_x = LV_HOR_RES / SQUARE_X;
  int sum_y = LV_VER_RES / SQUARE_Y;

  lv_draw_rect_dsc_init(&rect_dsc);
  rect_dsc.bg_opa = LV_OPA_COVER;
  rect_dsc.bg_color = lv_palette_main(LV_PALETTE_GREY);
  rect_dsc.border_width = 1;

  /* Draw HOR top and middle and bottom */

  for (i_hor = 0; i_hor < sum_x; i_hor++)
    {
      rect_area.x1 = i_hor * SQUARE_X;
      rect_area.x2 = SQUARE_X;
      rect_area.y1 = 0;
      rect_area.y2 = SQUARE_Y;
      lv_draw_rect(&touchpad->layer, &rect_dsc, &rect_area);
      rect_area.y1 = LV_VER_RES - SQUARE_Y;
      lv_draw_rect(&touchpad->layer, &rect_dsc, &rect_area);
    }

  /* Draw VER left and middle and right */

  for (j_ver = 0; j_ver < sum_y; j_ver++)
    {
      rect_area.x1 = 0;
      rect_area.y1 = j_ver * SQUARE_Y;
      lv_draw_rect(&touchpad->layer, &rect_dsc, &rect_area);
      rect_area.x1 = LV_HOR_RES - SQUARE_X;
      lv_draw_rect(&touchpad->layer, &rect_dsc, &rect_area);
    }

  /* Draw line diagonal */

  lv_draw_line_dsc_init(&line_dsc);
  line_dsc.color = lv_palette_main(LV_PALETTE_RED);
  line_dsc.width = 4;
  line_dsc.round_end = 1;
  line_dsc.round_start = 1;

  /* left_top to right_bottom */

  line_dsc.p1.x = SQUARE_Y / 2;
  line_dsc.p1.y = 0;
  line_dsc.p2.x = LV_HOR_RES;
  line_dsc.p2.y = LV_VER_RES - SQUARE_Y / 2;
  lv_draw_line(&touchpad->layer, &line_dsc);

  line_dsc.p1.x = 0;
  line_dsc.p1.y = SQUARE_Y / 2;
  line_dsc.p2.x = LV_HOR_RES - SQUARE_Y / 2;
  line_dsc.p2.y = LV_VER_RES;
  lv_draw_line(&touchpad->layer, &line_dsc);

  /* right_top to left_bottom */

  line_dsc.p1.x = LV_HOR_RES - SQUARE_Y / 2;
  line_dsc.p1.y = 0;
  line_dsc.p2.x = 0;
  line_dsc.p2.y = LV_VER_RES - SQUARE_Y / 2;
  lv_draw_line(&touchpad->layer, &line_dsc);

  line_dsc.p1.x = LV_HOR_RES;
  line_dsc.p1.y = SQUARE_Y / 2;
  line_dsc.p2.x = SQUARE_Y / 2;
  line_dsc.p2.y = LV_VER_RES;
  lv_draw_line(&touchpad->layer, &line_dsc);

  /* left_top to right_bottom */

  touchpad->line_k = (LV_VER_RES - SQUARE_Y / 2) /
    (float)(LV_HOR_RES - SQUARE_X / 2);
  touchpad->line_b_up_1 = (SQUARE_Y / 2 - LV_VER_RES) /
    (float)(LV_HOR_RES - SQUARE_X / 2) * (SQUARE_X / 2);
  touchpad->line_b_down_1 = SQUARE_Y / 2;

  /* right_top to left_bottom */

  touchpad->line_b_up_2 = LV_VER_RES - SQUARE_Y / 2;
  touchpad->line_b_down_2 =
    (LV_HOR_RES * LV_VER_RES - (SQUARE_Y * SQUARE_Y / 4)) /
    (float)(LV_HOR_RES - SQUARE_X / 2);
}

/****************************************************************************
 * Name: draw_track_line
 ****************************************************************************/

void draw_track_line(lv_obj_t *canvas, lv_point_t pos)
{
  int index_x;
  int index_y;
  for (index_x = pos.x - 1; index_x < pos.x + 2; index_x++)
    {
      for (index_y = pos.y - 1; index_y < pos.y + 2; index_y++)
        {
          lv_canvas_set_px(canvas, index_x, index_y,
            lv_palette_main(LV_PALETTE_RED), LV_OPA_COVER);
        }
    }
}

/****************************************************************************
 * Name: draw_touch_fill
 ****************************************************************************/

void draw_touch_fill(touchpad_s *touchpad, lv_point_t pos)
{
  lv_draw_rect_dsc_t rect_dsc;
  lv_draw_line_dsc_t line_dsc;
  lv_area_t rect_area;

  if (pos.x < 1 || pos.x > LV_HOR_RES || pos.y < 1 || pos.y > LV_VER_RES)
    {
      return;
    }

  /* Calculate square index */

  int x_index = pos.x / SQUARE_X;
  int y_index = pos.y / SQUARE_Y;

  lv_draw_rect_dsc_init(&rect_dsc);
  rect_dsc.bg_opa = LV_OPA_COVER;
  rect_dsc.bg_color = lv_palette_main(LV_PALETTE_BLUE);
  rect_dsc.border_width = 1;

  if (x_index == 0 || x_index == LV_HOR_RES / SQUARE_X - 1
    || y_index == 0 || y_index == LV_VER_RES / SQUARE_Y - 1)
    {
      rect_area.x1 = x_index * SQUARE_X;
      rect_area.x2 = y_index * SQUARE_Y;
      rect_area.y1 = SQUARE_X;
      rect_area.y2 = SQUARE_Y;
      lv_draw_rect(&touchpad->layer, &rect_dsc, &rect_area);
      draw_track_line(touchpad->canvas, pos);
    }

  /* left_top */

  if (pos.x < SQUARE_X && pos.y < SQUARE_Y)
    {
      touchpad->left_top = true;
    }

  /* right_bottom */

  if (pos.x > (LV_HOR_RES - SQUARE_X) && pos.y > (LV_VER_RES - SQUARE_Y))
    {
      touchpad->right_bottom = true;
    }

  /* right_top */

  if (pos.x > (LV_HOR_RES - SQUARE_X) && pos.y < SQUARE_Y)
    {
      touchpad->right_top = true;
    }

  /* left_bottom */

  if (pos.x < SQUARE_X && pos.y > (LV_VER_RES - SQUARE_Y))
    {
      touchpad->left_bottom = true;
    }

  if (touchpad->left_top || touchpad->right_bottom)
    {
      if (pos.y < (touchpad->line_k * pos.x +
        touchpad->line_b_up_1) ||
        pos.y > (touchpad->line_k * pos.x +
        touchpad->line_b_down_1))
        {
          touchpad->left_top = false;
          touchpad->right_bottom = false;
        }
      else
        {
          draw_track_line(touchpad->canvas, pos);
        }
    }

  if (touchpad->right_top || touchpad->left_bottom)
    {
      if (pos.y < (-touchpad->line_k * pos.x +
        touchpad->line_b_up_2) ||
        pos.y > (-touchpad->line_k * pos.x +
        touchpad->line_b_down_2))
        {
          touchpad->right_top = false;
          touchpad->left_bottom = false;
        }
      else
        {
          draw_track_line(touchpad->canvas, pos);
        }
    }

  /* fill blue color: left_top to right_bottom */

  if (touchpad->left_top && touchpad->right_bottom)
    {
      lv_draw_line_dsc_init(&line_dsc);
      line_dsc.opa = LV_OPA_COVER;
      line_dsc.color = lv_palette_main(LV_PALETTE_BLUE);
      line_dsc.width = SQUARE_Y;
      line_dsc.p1.x = 0;
      line_dsc.p1.y = 0;
      line_dsc.p2.x = LV_HOR_RES;
      line_dsc.p2.y = LV_VER_RES;
      lv_draw_line(&touchpad->layer, &line_dsc);
    }

  /* fill blue color: right_top to left_bottom */

  if (touchpad->left_bottom && touchpad->right_top)
    {
      lv_draw_line_dsc_init(&line_dsc);
      line_dsc.opa = LV_OPA_COVER;
      line_dsc.color = lv_palette_main(LV_PALETTE_BLUE);
      line_dsc.width = SQUARE_Y;
      line_dsc.p1.x = LV_HOR_RES;
      line_dsc.p1.y = 0;
      line_dsc.p2.x = 0;
      line_dsc.p2.y = LV_VER_RES;
      lv_draw_line(&touchpad->layer, &line_dsc);
    }
}

/****************************************************************************
 * Name: create_slider
 ****************************************************************************/

lv_obj_t *create_slider(bool is_horizontal)
{
  lv_obj_t *slider = lv_slider_create(lv_scr_act());
  if (is_horizontal)
    {
      lv_obj_set_size(slider, LV_HOR_RES, lv_pct(3));
      lv_obj_set_pos(slider, 0, LV_VER_RES / 2);
      lv_slider_set_range(slider, 0, LV_HOR_RES);
    }
  else
    {
      lv_obj_set_size(slider, lv_pct(3), LV_VER_RES);
      lv_obj_set_pos(slider, LV_HOR_RES / 2, 0);
      lv_slider_set_range(slider, 0, LV_VER_RES);
    }

  lv_slider_set_value(slider, SQUARE_PIX, LV_ANIM_OFF);
  return slider;
}

/****************************************************************************
 * Name: create_arc
 ****************************************************************************/

lv_obj_t *create_arc(int16_t diameter)
{
  lv_obj_t *arc_slider = lv_arc_create(lv_scr_act());
  lv_obj_set_size(arc_slider, diameter, diameter);
  lv_arc_set_rotation(arc_slider, 180);
  lv_arc_set_bg_angles(arc_slider, 0, 360);
  lv_arc_set_range(arc_slider, 0, diameter * 2);
  lv_arc_set_value(arc_slider, 0);
  lv_obj_center(arc_slider);
  return arc_slider;
}

/****************************************************************************
 * Name: circle_hit_test
 ****************************************************************************/

bool circle_hit_test(touchpad_s *touchpad, lv_point_t pos)
{
  int16_t distance;
  if (abs(pos.x - touchpad->arc_indic_pos.x) < CIRCULAR_PIX &&
      abs(pos.y - touchpad->arc_indic_pos.y) < CIRCULAR_PIX)
    {
      distance = sqrt((pos.x - LV_HOR_RES / 2) * (pos.x - LV_HOR_RES / 2) +
        (pos.y - LV_VER_RES / 2) * (pos.y - LV_VER_RES / 2));
      return abs(distance - touchpad->diameter / 2) <= SQUARE_PIX;
    }

  return false;
}

/****************************************************************************
 * Name: touchpad_read
 ****************************************************************************/

void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  touchpad_s *touchpad = lv_indev_get_driver_data(indev);
  LV_ASSERT_NULL(touchpad);
  struct touch_sample_s sample;
  lv_point_t pos;
  int nbytes;
  uint8_t touch_flags;

  /* Read one sample */

  nbytes = read(touchpad->fd, &sample,
      sizeof(struct touch_sample_s));

  /* Handle unexpected return values */

  if (nbytes != sizeof(struct touch_sample_s))
    {
      return;
    }

  touch_flags = sample.point[0].flags;
  if (touch_flags & TOUCH_DOWN || touch_flags & TOUCH_MOVE)
    {
      pos.x = sample.point[0].x;
      pos.y = sample.point[0].y;
      if (touchpad->is_square == true)
        {
          draw_touch_fill(touchpad, pos);
        }
      else
        {
          if (touchpad->slider_v == NULL &&
              pos.x - lv_slider_get_value(touchpad->slider_h) > 0 &&
              pos.x - lv_slider_get_value(touchpad->slider_h)
              < DISTANCE_PIX &&
              abs(pos.y - LV_VER_RES / 2 - DISTANCE_PIX) < SQUARE_PIX)
            {
              lv_slider_set_value(touchpad->slider_h, pos.x, LV_ANIM_ON);
            }
          else if (touchpad->arc_slider == NULL &&
              LV_HOR_RES - lv_slider_get_value(touchpad->slider_h)
              < SQUARE_PIX)
            {
              if (touchpad->slider_v == NULL)
                {
                  touchpad->slider_v = create_slider(false);
                }

              if (LV_VER_RES - pos.y -
                  lv_slider_get_value(touchpad->slider_v) > 0 &&
                  LV_VER_RES - pos.y -
                  lv_slider_get_value(touchpad->slider_v) < DISTANCE_PIX &&
                  abs(pos.x - LV_HOR_RES / 2 - DISTANCE_PIX) < SQUARE_PIX)
                {
                  lv_slider_set_value(touchpad->slider_v,
                    LV_VER_RES - pos.y, LV_ANIM_ON);
                }
            }

          if (touchpad->slider_v &&
            LV_VER_RES - lv_slider_get_value(touchpad->slider_v)
            < SQUARE_PIX)
            {
              if (touchpad->arc_slider == NULL)
                {
                  touchpad->arc_slider = create_arc(touchpad->diameter);
                }

              if (circle_hit_test(touchpad, pos))
                {
                  int16_t diff = pos.x - touchpad->arc_indic_pos.x;
                  if (pos.y > LV_VER_RES / 2)
                    {
                      diff = -diff;
                    }

                  lv_arc_set_value(touchpad->arc_slider,
                    lv_arc_get_value(touchpad->arc_slider) + diff);
                  touchpad->arc_indic_pos.x = pos.x;
                  touchpad->arc_indic_pos.y = pos.y;
                }
            }

          if (touchpad->arc_slider &&
            touchpad->diameter * 2 -
            lv_arc_get_value(touchpad->arc_slider) < DISTANCE_PIX)
            {
              if (touchpad->label_pass == NULL)
                {
                  static lv_style_t style;
                  lv_style_init(&style);
                  lv_style_set_bg_color(&style,
                    lv_palette_main(LV_PALETTE_GREEN));
                  lv_obj_add_style(lv_scr_act(), &style, 0);

                  touchpad->label_pass = lv_label_create(lv_scr_act());
                  lv_obj_set_size(touchpad->label_pass,
                    LV_HOR_RES, LV_VER_RES);
                  lv_label_set_text(touchpad->label_pass, "PASS");
                  lv_obj_set_style_text_align(touchpad->label_pass,
                    LV_TEXT_ALIGN_CENTER, 0);
                  lv_obj_center(touchpad->label_pass);
                }
            }
        }
    }
  else if (touch_flags & TOUCH_UP)
    {
      if (touchpad->is_square == true)
        {
          touchpad->left_top = false;
          touchpad->right_top = false;
          touchpad->left_bottom = false;
          touchpad->right_bottom = false;
        }
    }
}

/****************************************************************************
 * Name: touchpad_init
 ****************************************************************************/

bool touchpad_init(touchpad_s **touchpad,
                   const char * input_path,
                   uint32_t screen_shape)
{
  touchpad_s *tmp_touchpad;

  if (touchpad == NULL)
    {
      LV_LOG_ERROR("touchpad is NULL");
      return false;
    }

  tmp_touchpad = malloc(sizeof(touchpad_s));
  if (tmp_touchpad == NULL)
    {
      LV_LOG_ERROR("touchpad_s malloc failed");
      return false;
    }

  *touchpad = tmp_touchpad;

  tmp_touchpad->fd = open(input_path, O_RDONLY | O_NONBLOCK);
  if (tmp_touchpad->fd < 0)
    {
      LV_LOG_ERROR("touchpad %s open failed: %d",
        input_path, errno);
      return false;
    }

  tmp_touchpad->is_square = (bool)screen_shape;

  tmp_touchpad->indev = lv_indev_create();
  if (tmp_touchpad->indev == NULL)
    {
      LV_LOG_ERROR("touchpad indev is NULL");
      return false;
    }

  lv_indev_set_type(tmp_touchpad->indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(tmp_touchpad->indev, touchpad_read);
  lv_indev_set_driver_data(tmp_touchpad->indev, tmp_touchpad);

  if (tmp_touchpad->is_square == true)
    {
      tmp_touchpad->canvas = lv_canvas_create(lv_scr_act());
      tmp_touchpad->left_top = false;
      tmp_touchpad->right_top = false;
      tmp_touchpad->left_bottom = false;
      tmp_touchpad->right_bottom = false;

      lv_obj_set_size(tmp_touchpad->canvas, LV_HOR_RES, LV_VER_RES);

      /* Create a buffer for the canvas */

      lv_draw_buf_t * draw_buf = lv_draw_buf_create(LV_HOR_RES, LV_VER_RES,
        LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
      if (draw_buf == NULL)
        {
          LV_LOG_ERROR("malloc failed for canvas buffer");
          return false;
        }

      lv_canvas_set_draw_buf(tmp_touchpad->canvas, draw_buf);
      lv_canvas_init_layer(tmp_touchpad->canvas, &tmp_touchpad->layer);

      lv_canvas_fill_bg(tmp_touchpad->canvas,
        lv_palette_lighten(LV_PALETTE_GREY, 3),
        LV_OPA_COVER);
      lv_obj_center(tmp_touchpad->canvas);
      darw_canvas_frame(tmp_touchpad);
    }
  else
    {
      /* Create and configure a horizontal slider */

      tmp_touchpad->slider_h = create_slider(true);
      tmp_touchpad->slider_v = NULL;
      tmp_touchpad->arc_slider = NULL;
      tmp_touchpad->label_pass = NULL;
      tmp_touchpad->diameter =
        LV_HOR_RES > LV_VER_RES ? LV_VER_RES : LV_HOR_RES;
      tmp_touchpad->diameter = tmp_touchpad->diameter * 3 / 4;
      tmp_touchpad->arc_indic_pos.x =
        (LV_HOR_RES - tmp_touchpad->diameter) / 2;
      tmp_touchpad->arc_indic_pos.y = LV_VER_RES / 2;
    }

  return true;
}

void touchpad_deinit(touchpad_s *touchpad)
{
  if (touchpad == NULL)
    {
      return;
    }

  if (touchpad->indev)
    {
      lv_indev_delete(touchpad->indev);
    }

  if (touchpad->fd > 0)
    {
      close(touchpad->fd);
    }

  free(touchpad);
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(void)
{
    LV_LOG_USER("Usage: -s 0 -- circular screen\n");
    LV_LOG_USER("Usage: -s 1 -- square screen\n");
    exit(EXIT_FAILURE);
}

/****************************************************************************
 * Name: lv_nuttx_uv_loop
 ****************************************************************************/

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
static void lv_nuttx_uv_loop(uv_loop_t *loop, lv_nuttx_result_t *result)
{
  lv_nuttx_uv_t uv_info;
  void *data;

  uv_loop_init(loop);

  lv_memset(&uv_info, 0, sizeof(uv_info));
  uv_info.loop = loop;
  uv_info.disp = result->disp;
  uv_info.indev = result->indev;
#ifdef CONFIG_UINPUT_TOUCH
  uv_info.uindev = result->utouch_indev;
#endif

  data = lv_nuttx_uv_init(&uv_info);
  uv_run(loop, UV_RUN_DEFAULT);
  lv_nuttx_uv_deinit(&data);
}
#endif

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(uint32_t *screen_shape,
                              int argc, FAR char **argv)
{
  int ch;

  while ((ch = getopt(argc, argv, "hs::")) != -1)
    {
      switch (ch)
        {
        case 'h':
          show_usage();
          break;
        case 's':
          *screen_shape = atoi(optarg);
          break;
        }
    }

  if (*screen_shape > 1)
    {
      show_usage();
    }
}

/****************************************************************************
 * Name: drivertest_touchpanel
 ****************************************************************************/

static void drivertest_touchpanel(FAR void **state)
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  touchpad_s *touchpad = NULL;
  FAR uint32_t *screen_shape = (FAR uint32_t *)*state;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
#endif

  /* LVGL initialization */

  lv_init();
  lv_nuttx_dsc_init(&info);
  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      LV_LOG_ERROR("touchpad_init initialization failure!");
      return;
    }

  if (!touchpad_init(&touchpad, info.input_path, *screen_shape))
    {
      LV_LOG_ERROR("touchpad_init init failed");
      goto errout;
    }

  /* Handle LVGL tasks */

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_nuttx_uv_loop(&ui_loop, &result);
#else
  while (1)
    {
      lv_timer_handler();
      usleep(10 * 1000);
    }
#endif

errout:
  touchpad_deinit(touchpad);
  lv_disp_remove(result.disp);
  lv_deinit();

  LV_LOG_USER("Terminating!\n");
  return;
}

/****************************************************************************
 * Name: touchpanel_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uint32_t screen_shape = 0;

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate(drivertest_touchpanel, &screen_shape)
  };

  parse_commandline(&screen_shape, argc, argv);

  return cmocka_run_group_tests(tests, NULL, NULL);
}
