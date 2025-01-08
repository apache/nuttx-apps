/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_lcd.c
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
#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>
#include <nuttx/lcd/lcd_dev.h>
#include <nuttx/lcd/lcd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <inttypes.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LCD_DEFAULT_DEVPATH    "/dev/lcd0"
#define LCD_TEST_CASE_NUM      4

#define COLOR_888_TO_565(color) (((((color) >> 19) & 0x1f) << 11) \
                                |((((color) >> 10) & 0x3f) << 5)  \
                                |(((color) >> 3) & 0x1f))

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: -%c %s\n", ch, optarg);            \
          lcd_help(argv[0], lcd_state, EXIT_FAILURE);                 \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

struct lcd_info_s
{
  int fd;
  struct lcd_planeinfo_s plane_info;
  struct fb_videoinfo_s video_info;
  struct lcddev_area_align_s align_info;
};

struct lcd_state_s
{
  char devpath[PATH_MAX];
  int test_case_id;
  struct lcd_info_s lcd_info;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lcd_help
 ****************************************************************************/

static void lcd_help(FAR const char *progname,
                     FAR struct lcd_state_s *lcd_state, int exitcode)
{
  printf("Usage: %s"
         " -p <devpath> -t <testcase>\n",
         progname);
  printf("  [-p devpath] selects the lcd device.  "
         "Default: %s Current: %s\n", LCD_DEFAULT_DEVPATH,
         lcd_state->devpath);
  printf("  [-t testcase] selects the testcase to show color on LCD.\n"
         "  Case 0: show black color on LCD\n"
         "  Case 1: show white color on LCD\n"
         "  Case 2: show black|red|green|blue|white rectangle on LCD\n"
         "  Case 3: show grayscale rectangle on LCD\n"
         );

  printf("  [-h] shows this message and exits\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct lcd_state_s *lcd_state, int argc,
                              FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "p:d:n:f:t:h")) != ERROR)
    {
      switch (ch)
        {
          case 'p':
            snprintf(lcd_state->devpath, sizeof(lcd_state->devpath), "%s",
                      optarg);
            break;
          case 't':
            OPTARG_TO_VALUE(converted, uint8_t, 10);
            if (converted < 0 || converted >= LCD_TEST_CASE_NUM)
              {
                printf("Testcase out of range: %d\n", converted);
                lcd_help(argv[0], lcd_state, EXIT_FAILURE);
              }

            lcd_state->test_case_id = converted;
            break;
          case 'h':
            lcd_help(argv[0], lcd_state, EXIT_FAILURE);
            break;
          case '?':
            printf("Unsupported option: %s\n", optarg);
            lcd_help(argv[0], lcd_state, EXIT_FAILURE);
            break;
        }
    }

  printf("devname = %s\n"
         "testcase = %d\n",
         lcd_state->devpath,
         lcd_state->test_case_id);
}

/****************************************************************************
 * Name: lcd_setup
 ****************************************************************************/

static int lcd_setup(FAR void **state)
{
  FAR struct lcd_state_s * lcd_state;
  int ret = 0;

  lcd_state = (struct lcd_state_s *)*state;
  lcd_state->lcd_info.fd = open(lcd_state->devpath, 0);
  assert_true(lcd_state->lcd_info.fd > 0);

  ret = ioctl(lcd_state->lcd_info.fd, LCDDEVIO_GETPLANEINFO,
               &lcd_state->lcd_info.plane_info);
  assert_int_equal(ret, 0);

  ret = ioctl(lcd_state->lcd_info.fd, LCDDEVIO_GETVIDEOINFO,
               &lcd_state->lcd_info.video_info);
  assert_int_equal(ret, 0);

  ret = ioctl(lcd_state->lcd_info.fd, LCDDEVIO_GETAREAALIGN,
               &lcd_state->lcd_info.align_info);
  assert_int_equal(ret, 0);

  return 0;
}

static uint16_t align_round_up(uint16_t v, uint16_t align)
{
  return ((v + align - 1) / align) * align;
}

/****************************************************************************
 * Name: draw_rect_buf_alloc
 ****************************************************************************/

static void draw_rect_buf_alloc(FAR struct lcd_info_s *lcd_info,
                                FAR struct lcddev_area_s *area)
{
  uint16_t x = area->col_start;
  uint16_t y = area->row_start;
  uint16_t w = area->col_end - area->col_start + 1;
  uint16_t h = area->row_end - area->row_start + 1;

  const uint8_t bpp = lcd_info->plane_info.bpp;
  FAR struct lcddev_area_align_s *align_info = &lcd_info->align_info;

  y = align_round_up(y, align_info->row_start_align);
  x = align_round_up(x, align_info->col_start_align);
  h = align_round_up(h, align_info->height_align);
  w = align_round_up(w, align_info->width_align);

  area->col_start = x;
  area->row_start = y;
  area->col_end = x + w - 1;
  area->row_end = y + h - 1;

  area->data = aligned_alloc(align_info->buf_align, w * h * (bpp >> 3));
}

/****************************************************************************
 * Name: draw_rect
 ****************************************************************************/

static void draw_rect(FAR struct lcd_info_s *lcd_info, int x, int y,
                     int w, int h, uint32_t color)
{
  int i = 0;
  int j = 0;
  int ret = 0;
  int offset = 0;
  int valid_w = w;
  int valid_h = h;
  uint32_t *fb_bpp32 = NULL;
  uint16_t *fb_bpp16 = NULL;
  const uint8_t bpp = lcd_info->plane_info.bpp;
  const uint32_t xres = lcd_info->video_info.xres;
  const uint32_t yres = lcd_info->video_info.yres;
  struct lcddev_area_s draw_area;

  memset(&draw_area, 0, sizeof(draw_area));
  if (x + w > xres)
    {
      valid_w = xres - x;
    }

  if (y + h > yres)
    {
      valid_h = yres - y;
    }

  draw_area.row_start = y;
  draw_area.col_start = x;
  draw_area.row_end = y + valid_h - 1;
  draw_area.col_end = x + valid_w - 1;
  draw_rect_buf_alloc(lcd_info, &draw_area);
  assert_ptr_not_equal(draw_area.data, NULL);

  fb_bpp16 = (uint16_t *)draw_area.data;
  fb_bpp32 = (uint32_t *)draw_area.data;
  for (j = 0; j <= (draw_area.row_end - draw_area.row_start); j++)
    {
      offset = j  * valid_w;
      for (i = 0; i <= (draw_area.col_end - draw_area.col_start); i++)
        {
          if (bpp == 32)
            {
              *(fb_bpp32 + offset + i) = color;
            }
          else if (bpp == 16)
            {
              *(fb_bpp16 + offset + i) = COLOR_888_TO_565(color);
            }
        }
    }

  ret = ioctl(lcd_info->fd, LCDDEVIO_PUTAREA, (unsigned long)&draw_area);
  assert_int_equal(ret, 0);
  free(draw_area.data);
}

/****************************************************************************
 * Name: drivertest_lcd_black
 ****************************************************************************/

static void drivertest_lcd_black(FAR void **state)
{
  FAR struct lcd_state_s * lcd_state;
  lcd_state = (struct lcd_state_s *)*state;

  const uint32_t xres = lcd_state->lcd_info.video_info.xres;
  const uint32_t yres = lcd_state->lcd_info.video_info.yres;

  draw_rect(&lcd_state->lcd_info, 0, 0, xres, yres, 0xff000000);
}

/****************************************************************************
 * Name: drivertest_lcd_white
 ****************************************************************************/

static void drivertest_lcd_white(FAR void **state)
{
  FAR struct lcd_state_s * lcd_state;
  lcd_state = (struct lcd_state_s *)*state;

  const uint32_t xres = lcd_state->lcd_info.video_info.xres;
  const uint32_t yres = lcd_state->lcd_info.video_info.yres;

  draw_rect(&lcd_state->lcd_info, 0, 0, xres, yres, 0xffffffff);
}

/****************************************************************************
 * Name: drivertest_lcd_cross
 ****************************************************************************/

static void drivertest_lcd_cross(FAR void **state)
{
  FAR struct lcd_state_s * lcd_state = (struct lcd_state_s *)*state;
  int i = 0;
  int start_x = 0;
  int step_width = 0;
  int step_num = 0;
  uint32_t colors_to_show[] =
    {
      0x00000000, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00ffffff
    };

  const uint32_t xres = lcd_state->lcd_info.video_info.xres;
  const uint32_t yres = lcd_state->lcd_info.video_info.yres;

  step_num = nitems(colors_to_show);
  step_width = xres / step_num;
  for (i = 0; i < step_num; i++)
    {
      start_x = step_width * i;
      if (i == step_num - 1)
        {
          step_width = xres - start_x;
        }

      draw_rect(&lcd_state->lcd_info, start_x, 0, step_width, yres,
                  colors_to_show[i]);
    }
}

/****************************************************************************
 * Name: drivertest_lcd_vertical
 ****************************************************************************/

static void drivertest_lcd_vertical(FAR void **state)
{
  FAR struct lcd_state_s * lcd_state = (struct lcd_state_s *)*state;
  int i = 0;
  int start_y = 0;
  int step_height = 0;
  int step_num = 8;
  uint32_t gray_color = 0;

  const uint32_t xres = lcd_state->lcd_info.video_info.xres;
  const uint32_t yres = lcd_state->lcd_info.video_info.yres;

  step_height = yres / step_num;
  for (i = 0; i < step_num; i++)
    {
      start_y = step_height * i;
      if (i == step_num - 1)
        {
          step_height = yres - start_y;
        }

      gray_color = (0xff / step_num) * i;
      draw_rect(&lcd_state->lcd_info, 0, start_y, xres, step_height,
                  gray_color << 16 | gray_color << 8 | gray_color);
    }
}

/****************************************************************************
 * Name: lcd_teardown
 ****************************************************************************/

static int lcd_teardown(FAR void **state)
{
  FAR struct lcd_state_s * lcd_state;

  lcd_state = (struct lcd_state_s *)*state;

  close(lcd_state->lcd_info.fd);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Initialize the state data */

  struct lcd_state_s lcd_state;

  memset(&lcd_state, 0, sizeof(struct lcd_state_s));
  snprintf(lcd_state.devpath, sizeof(lcd_state.devpath), "%s",
                                       LCD_DEFAULT_DEVPATH);

  parse_commandline(&lcd_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate_setup_teardown(drivertest_lcd_black,
                                             lcd_setup, lcd_teardown,
                                             &lcd_state),
    cmocka_unit_test_prestate_setup_teardown(drivertest_lcd_white,
                                             lcd_setup, lcd_teardown,
                                             &lcd_state),
    cmocka_unit_test_prestate_setup_teardown(drivertest_lcd_cross,
                                             lcd_setup, lcd_teardown,
                                             &lcd_state),
    cmocka_unit_test_prestate_setup_teardown(drivertest_lcd_vertical,
                                             lcd_setup, lcd_teardown,
                                             &lcd_state),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
