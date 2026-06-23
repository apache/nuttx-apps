/****************************************************************************
 * apps/examples/baromonitor/baromonitor_main.c
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

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <uORB/uORB.h>

#include <lvgl/lvgl.h>

#include <nuttx/sensors/sensor.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PRESSURE_COLOUR 0xef4444
#define TEMPERATURE_COLOUR 0x6366f1

#define LINE_WIDTH (8)
#define GAUGE_SIZE (600)

/* Axis limits */

#define PRESS_MIN (960)
#define PRESS_MAX (1010)

#define TEMP_MIN (-30)
#define TEMP_MAX (60)
#define TEMP_THRESH (TEMP_MAX - 10)

#define PRESS_MAJOR_TICKS_EVERY (5)
#define TEMP_MAJOR_TICKS_EVERY (5)

#define NUM_TEMP_LABELS ((TEMP_MAX - TEMP_MIN) / TEMP_MAJOR_TICKS_EVERY)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_temp_labels[NUM_TEMP_LABELS + 1];
static char g_label_buf[NUM_TEMP_LABELS][8];

/****************************************************************************
 * Private Functions
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

static void set_column_flex(lv_obj_t *obj)
{
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_CENTER, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * baromonitor_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  lv_obj_t *screen;
  lv_obj_t *gauge_cont;
  lv_obj_t *press_gauge;
  lv_obj_t *press_line;
  lv_obj_t *press_cont;
  lv_obj_t *press_label;
  lv_obj_t *temp_gauge;
  lv_obj_t *temp_line;
  lv_obj_t *temp_cont;
  lv_obj_t *temp_label;
  lv_obj_t *title_cont;
  lv_obj_t *title;
  lv_scale_section_t *section;
  lv_style_t indicator_style;
  lv_style_t minor_ticks_style;
  lv_style_t main_line_style;
  lv_style_t section_label_style;
  lv_style_t section_minor_ticks_style;
  lv_style_t section_main_line_style;

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t ui_loop;
#endif

  int err;
  int fd;
  bool got_data;
  struct sensor_baro sensor_data;
  unsigned instance = 0;
  char topic[32];

  if (argc == 2)
    {
      instance = atoi(argv[1]);
    }

  snprintf(topic, sizeof(topic), "sensor_baro%u", instance);

  fd = orb_open("sensor_baro", instance, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      fprintf(stderr, "Could not open %s: %d\n", topic, errno);
      return EXIT_FAILURE;
    }

  printf("Subscribed to %s\n", topic);

  /* LVGL setup */

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_memzero(&ui_loop, sizeof(ui_loop));
#endif

  if (!lv_is_initialized())
    {
      printf("Initializing LVGL...\n");
      lv_init();
      printf("LVGL initialized...\n");
    }

  lv_nuttx_dsc_init(&info);
  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      fprintf(stderr, "Failed to initialize LVGL.\n");
      close(fd);
      return EXIT_FAILURE;
    }

  /* Set up stuff to render */

  screen = lv_screen_active();
  set_column_flex(screen);

  /* Create containers for layout. We want a spanning title, followed by two
   * side-by-side containers for the gauges.
   *
   * Gauge container is therefore a ROW flex, while its inner containers are
   * column flex (scale followed by a label below)
   */

  title_cont = lv_obj_create(screen);
  lv_obj_set_size(title_cont, LV_PCT(100), LV_PCT(10));
  lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, 0);
  title = lv_label_create(title_cont);
  lv_label_set_text(title, "Barometer monitor");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  gauge_cont = lv_obj_create(screen);
  lv_obj_set_size(gauge_cont, LV_PCT(100), LV_PCT(90));
  lv_obj_set_flex_flow(gauge_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_flex_main_place(gauge_cont, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_flex_cross_place(gauge_cont, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_flex_track_place(gauge_cont, LV_FLEX_ALIGN_CENTER, 0);

  press_cont = lv_obj_create(gauge_cont);
  temp_cont = lv_obj_create(gauge_cont);
  set_column_flex(press_cont);
  set_column_flex(temp_cont);
  lv_obj_set_size(press_cont, LV_PCT(50), LV_PCT(100));
  lv_obj_set_size(temp_cont, LV_PCT(50), LV_PCT(100));

  /* TODO: Get the text labels on the gauges to be padded better */

  /* Pressure gauge */

  press_gauge = lv_scale_create(press_cont);

  lv_obj_set_size(press_gauge, GAUGE_SIZE, GAUGE_SIZE);
  lv_scale_set_mode(press_gauge, LV_SCALE_MODE_ROUND_INNER);
  lv_obj_set_style_bg_opa(press_gauge, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(press_gauge,
                            lv_palette_lighten(LV_PALETTE_RED, 4), 0);
  lv_obj_set_style_radius(press_gauge, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(press_gauge, true, 0);
  lv_obj_set_style_text_font(press_gauge, &lv_font_montserrat_24, 0);
  lv_obj_align(press_gauge, LV_ALIGN_LEFT_MID, LV_PCT(5), 0);

  lv_scale_set_label_show(press_gauge, true);

  lv_scale_set_total_tick_count(press_gauge, (PRESS_MAX - PRESS_MIN));
  lv_scale_set_major_tick_every(press_gauge, PRESS_MAJOR_TICKS_EVERY);

  lv_obj_set_style_length(press_gauge, 5, LV_PART_ITEMS);
  lv_obj_set_style_length(press_gauge, 10, LV_PART_INDICATOR);
  lv_scale_set_range(press_gauge, PRESS_MIN, PRESS_MAX);

  lv_scale_set_angle_range(press_gauge, 270);
  lv_scale_set_rotation(press_gauge, 135);

  press_line = lv_line_create(press_gauge);

  lv_obj_set_style_line_width(press_line, LINE_WIDTH, LV_PART_MAIN);
  lv_obj_set_style_line_rounded(press_line, true, LV_PART_MAIN);
  lv_obj_set_style_line_color(press_line, lv_color_black(), LV_PART_MAIN);
  lv_obj_center(press_line);

  /* Temperature gauge */

  temp_gauge = lv_scale_create(temp_cont);
  lv_obj_set_size(temp_gauge, GAUGE_SIZE, GAUGE_SIZE);
  lv_scale_set_label_show(temp_gauge, true);
  lv_scale_set_mode(temp_gauge, LV_SCALE_MODE_ROUND_OUTER);
  lv_obj_align(temp_gauge, LV_ALIGN_RIGHT_MID, LV_PCT(-5), 0);

  lv_scale_set_total_tick_count(temp_gauge, (TEMP_MAX - TEMP_MIN));
  lv_scale_set_major_tick_every(temp_gauge, TEMP_MAJOR_TICKS_EVERY);
  lv_obj_set_style_length(temp_gauge, 5, LV_PART_ITEMS);
  lv_obj_set_style_length(temp_gauge, 10, LV_PART_INDICATOR);
  lv_scale_set_range(temp_gauge, TEMP_MIN, TEMP_MAX);

  lv_scale_set_angle_range(temp_gauge, 270);
  lv_scale_set_rotation(temp_gauge, 135);

  temp_line = lv_line_create(temp_gauge);

  lv_obj_set_style_line_width(temp_line, LINE_WIDTH, LV_PART_MAIN);
  lv_obj_set_style_line_rounded(temp_line, true, LV_PART_MAIN);
  lv_obj_center(temp_line);

  /* Generate labels */

  for (unsigned i = 0; i < NUM_TEMP_LABELS; i++)
    {
      snprintf(g_label_buf[i], sizeof(g_label_buf[i]), "%d °C",
               TEMP_MIN + i * (TEMP_MAX - TEMP_MIN) / NUM_TEMP_LABELS);
      g_temp_labels[i] = (const char *)g_label_buf[i];
    }

  g_temp_labels[NUM_TEMP_LABELS] = NULL;

  lv_scale_set_text_src(temp_gauge, g_temp_labels);

  /* Temp gauge styles */

  lv_style_init(&indicator_style);

  lv_style_set_text_font(&indicator_style, &lv_font_montserrat_24);
  lv_style_set_text_color(&indicator_style,
                          lv_palette_darken(LV_PALETTE_BLUE, 3));

  lv_style_set_line_color(&indicator_style,
                          lv_palette_darken(LV_PALETTE_BLUE, 3));
  lv_style_set_width(&indicator_style, 10U);     /* Tick length */
  lv_style_set_line_width(&indicator_style, 2U); /* Tick width */
  lv_obj_add_style(temp_gauge, &indicator_style, LV_PART_INDICATOR);

  lv_style_init(&minor_ticks_style);
  lv_style_set_line_color(&minor_ticks_style,
                          lv_palette_lighten(LV_PALETTE_BLUE, 2));
  lv_style_set_width(&minor_ticks_style, 5U);      /* Tick length */
  lv_style_set_line_width(&minor_ticks_style, 2U); /* Tick width */
  lv_obj_add_style(temp_gauge, &minor_ticks_style, LV_PART_ITEMS);

  lv_style_init(&main_line_style);
  lv_style_set_arc_color(&main_line_style,
                         lv_palette_darken(LV_PALETTE_BLUE, 3));
  lv_style_set_arc_width(&main_line_style, 2U); /* Tick width */

  /* Section on temperature gauge */

  lv_style_init(&section_label_style);
  lv_style_init(&section_minor_ticks_style);
  lv_style_init(&section_main_line_style);

  lv_style_set_text_font(&section_label_style, &lv_font_montserrat_24);
  lv_style_set_text_color(&section_label_style,
                          lv_palette_darken(LV_PALETTE_RED, 3));

  lv_style_set_line_color(&section_label_style,
                          lv_palette_darken(LV_PALETTE_RED, 3));
  lv_style_set_line_width(&section_label_style, 5U); /* Tick width */

  lv_style_set_line_color(&section_minor_ticks_style,
                          lv_palette_lighten(LV_PALETTE_RED, 2));
  lv_style_set_line_width(&section_minor_ticks_style, 4U); /* Tick width */

  lv_style_set_arc_color(&section_main_line_style,
                         lv_palette_darken(LV_PALETTE_RED, 3));
  lv_style_set_arc_width(&section_main_line_style, 4U); /* Tick width */

  section = lv_scale_add_section(temp_gauge);
  lv_scale_section_set_range(section, TEMP_THRESH, TEMP_MAX);
  lv_scale_section_set_style(section, LV_PART_INDICATOR,
                             &section_label_style);
  lv_scale_section_set_style(section, LV_PART_ITEMS,
                             &section_minor_ticks_style);
  lv_scale_section_set_style(section, LV_PART_MAIN,
                             &section_main_line_style);

  /* Data labels */

  press_label = lv_label_create(press_cont);
  lv_obj_align(press_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_font(press_label, &lv_font_montserrat_24, 0);
  temp_label = lv_label_create(temp_cont);
  lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);

  /* Main loop */

#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  lv_nuttx_uv_loop(&ui_loop, &result);
#else
  for (; ; )
    {
      err = read(fd, &sensor_data, sizeof(sensor_data));
      if (err < 0)
        {
          fprintf(stderr, "Failed read data from %s: %d\n", topic, errno);
        }
      else if (err == 0)
        {
          fprintf(stderr, "No data from %s\n", topic);
        }
      else
        {
          got_data = true;
        }

      if (got_data)
        {
          /* Update display if we obtained it successfully */

          lv_label_set_text_fmt(temp_label, "Temperature: %2.f °C",
                                sensor_data.temperature);
          lv_label_set_text_fmt(press_label, "Pressure: %2.f mbar",
                                sensor_data.pressure);

          lv_scale_set_line_needle_value(press_gauge, press_line, -10,
                                         (int32_t)sensor_data.pressure);
          lv_scale_set_line_needle_value(temp_gauge, temp_line, -10,
                                         (int32_t)sensor_data.temperature);

          if (sensor_data.temperature < TEMP_THRESH)
            {
              lv_obj_set_style_line_color(
                  temp_line, lv_palette_darken(LV_PALETTE_BLUE, 3),
                  LV_PART_MAIN);
            }
          else
            {
              lv_obj_set_style_line_color(
                  temp_line, lv_palette_darken(LV_PALETTE_RED, 3),
                  LV_PART_MAIN);
            }

          got_data = false; /* Reset for next loop */
        }

      lv_timer_handler();
      usleep(CONFIG_EXAMPLES_BAROMONITOR_SAMPLERATE * 1000);

      /* TODO: make this equivalent in LIBUV loop */
    }
#endif

  orb_close(fd);
  lv_nuttx_deinit(&result);
  lv_deinit();
  return EXIT_SUCCESS;
}
