/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_framebuffer.c
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
#include <poll.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FB_DEFAULT_DEVPATH    "/dev/fb0"
#define FB_TEST_CASE_NUM      4

#define COLOR_888_TO_565(color) (((((color) >> 19) & 0x1f) << 11) \
                                |((((color) >> 10) & 0x3f) << 5) \
                                |(((color) >> 3) & 0x1f))

#define OPTARG_TO_VALUE(value, type, base)                            \
  do                                                                  \
    {                                                                 \
      FAR char *ptr;                                                  \
      value = (type)strtoul(optarg, &ptr, base);                      \
      if (*ptr != '\0')                                               \
        {                                                             \
          printf("Parameter error: -%c %s\n", ch, optarg);            \
          fb_help(argv[0], fb_state, EXIT_FAILURE);                   \
        }                                                             \
    } while (0)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

struct fb_info_s
{
  FAR void *fb_mem;
  FAR void *fb_mem2;
  FAR void *act_fbmem;
  uint32_t mem2_yoffset;
  struct fb_planeinfo_s plane_info;
  struct fb_videoinfo_s video_info;
};

struct fb_state_s
{
  char devpath[PATH_MAX];
  int test_case_id;
  int fb_device;
  struct fb_info_s fb_info;
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
 * Name: fb_help
 ****************************************************************************/

static void fb_help(FAR const char *progname,
                    FAR struct fb_state_s *fb_state, int exitcode)
{
  printf("Usage: %s"
         " -p <devpath> -t <testcase>\n",
         progname);
  printf("  [-p devpath] selects the framebuffer device.  "
         "Default: %s Current: %s\n", FB_DEFAULT_DEVPATH,
         fb_state->devpath);
  printf("  [-t testcase] selects the testcase to show framebuffer.\n"
         "  Case 0: show black color on FB\n"
         "  Case 1: show white color on FB\n"
         "  Case 2: show black|red|green|blue|white rectangle on FB\n"
         "  Case 3: show grayscale rectangle on FB\n"
         );

  printf("  [-h] shows this message and exits\n");

  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct fb_state_s *fb_state, int argc,
                              FAR char **argv)
{
  int ch;
  int converted;

  while ((ch = getopt(argc, argv, "p:d:n:f:t:h")) != ERROR)
    {
      switch (ch)
        {
          case 'p':
            snprintf(fb_state->devpath, sizeof(fb_state->devpath), "%s",
                      optarg);
            break;
          case 't':
            OPTARG_TO_VALUE(converted, uint8_t, 10);
            if (converted < 0 || converted >= FB_TEST_CASE_NUM)
              {
                printf("Testcase out of range: %d\n", converted);
                fb_help(argv[0], fb_state, EXIT_FAILURE);
              }

            fb_state->test_case_id = converted;
            break;
          case 'h':
            fb_help(argv[0], fb_state, EXIT_FAILURE);
            break;
          case '?':
            printf("Unsupported option: %s\n", optarg);
            fb_help(argv[0], fb_state, EXIT_FAILURE);
            break;
        }
    }

  printf("devname = %s\n"
         "testcase = %d\n",
         fb_state->devpath,
         fb_state->test_case_id);
}

/****************************************************************************
 * fb_init_mem2
 ****************************************************************************/

static int fb_init_mem2(FAR struct fb_state_s *state,
                        FAR struct fb_planeinfo_s *pinfo)
{
  int ret;
  uintptr_t buf_offset;

  pinfo->display = state->fb_info.plane_info.display + 1;

  ret = ioctl(state->fb_device, FBIOGET_PLANEINFO,
              (unsigned long)(uintptr_t)pinfo);
  if (ret < 0)
    {
      int errcode = errno;
      printf("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n", errcode);
      return EXIT_FAILURE;
    }

  /* Check bpp */

  if (pinfo->bpp != state->fb_info.plane_info.bpp)
    {
      printf("ERROR: fbmem2 is incorrect");
      return -EINVAL;
    }

  /* Check the buffer address offset,
   * It needs to be divisible by pinfo.stride
   */

  buf_offset = pinfo->fbmem - state->fb_info.fb_mem;

  if ((buf_offset % state->fb_info.plane_info.stride) != 0)
    {
      printf("ERROR: It is detected that buf_offset(%" PRIuPTR ") "
             "and stride(%d) are not divisible, please ensure "
             "that the driver handles the address offset by itself.",
             buf_offset, state->fb_info.plane_info.stride);
    }

  /* Calculate the address and yoffset of fbmem2 */

  if (buf_offset == 0)
    {
      /* Use consecutive fbmem2. */

      state->fb_info.mem2_yoffset = state->fb_info.video_info.yres;
      state->fb_info.fb_mem2 =
          pinfo->fbmem + state->fb_info.mem2_yoffset * pinfo->stride;
    }
  else
    {
      /* Use non-consecutive fbmem2. */

      state->fb_info.mem2_yoffset =
          buf_offset / state->fb_info.plane_info.stride;
      state->fb_info.fb_mem2 = pinfo->fbmem;
    }

  return 0;
}

/****************************************************************************
 * Name: fb_setup
 ****************************************************************************/

static int fb_setup(FAR void **state)
{
  FAR struct fb_state_s * fb_state;
  struct fb_planeinfo_s pinfo;
  int ret = 0;

  memset(&pinfo, 0, sizeof(struct fb_planeinfo_s));
  fb_state = (struct fb_state_s *)*state;
  fb_state->fb_device = open(fb_state->devpath, O_RDWR);
  assert_true(fb_state->fb_device > 0);

  ret = ioctl(fb_state->fb_device, FBIOGET_PLANEINFO,
               &fb_state->fb_info.plane_info);
  assert_int_equal(ret, 0);

  ret = ioctl(fb_state->fb_device, FBIOGET_VIDEOINFO,
               &fb_state->fb_info.video_info);
  assert_int_equal(ret, 0);

  fb_state->fb_info.fb_mem = mmap(NULL, fb_state->fb_info.plane_info.fblen,
      PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fb_state->fb_device, 0);

  assert_ptr_not_equal(fb_state->fb_info.fb_mem, MAP_FAILED);

  /* double buffer mode */

  if (fb_state->fb_info.plane_info.yres_virtual ==
      (fb_state->fb_info.video_info.yres * 2))
    {
      if ((ret = fb_init_mem2(fb_state, &pinfo)) < 0)
        {
          return 0;
        }
    }

  fb_state->fb_info.act_fbmem = fb_state->fb_info.fb_mem;

  return 0;
}

/****************************************************************************
 * pan_display
 ****************************************************************************/

static void pan_display(FAR struct fb_state_s *state)
{
  struct pollfd pfd;
  int ret;
  pfd.fd = state->fb_device;
  pfd.events = POLLOUT;

  ret = poll(&pfd, 1, 0);

  if (ret > 0)
    {
      if (state->fb_info.fb_mem2 == NULL)
        {
          return;
        }

      if (state->fb_info.act_fbmem == state->fb_info.fb_mem)
        {
          state->fb_info.plane_info.yoffset = 0;
        }
      else
        {
          state->fb_info.plane_info.yoffset = state->fb_info.mem2_yoffset;
        }

      ioctl(state->fb_device, FBIOPAN_DISPLAY,
            (unsigned long)(uintptr_t)&state->fb_info.plane_info);

      state->fb_info.act_fbmem =
          state->fb_info.act_fbmem == state->fb_info.fb_mem ?
          state->fb_info.fb_mem2 : state->fb_info.fb_mem;
    }
}

/****************************************************************************
 * sync_area
 ****************************************************************************/

static void sync_area(FAR struct fb_state_s *state)
{
  if (state->fb_info.fb_mem2 == NULL)
    {
      return;
    }

  if (state->fb_info.act_fbmem == state->fb_info.fb_mem)
    {
      memcpy(state->fb_info.fb_mem, state->fb_info.fb_mem2,
             state->fb_info.video_info.yres *
             state->fb_info.plane_info.stride);
    }
  else
    {
      memcpy(state->fb_info.fb_mem2, state->fb_info.fb_mem,
             state->fb_info.video_info.yres *
             state->fb_info.plane_info.stride);
    }
}

/****************************************************************************
 * Name: draw_rect
 ****************************************************************************/

static void draw_rect(FAR struct fb_state_s *fb_state, int x, int y,
                      int w, int h, uint32_t color)
{
  int i = 0;
  int j = 0;
  uint8_t *fb_tmp;
  uint32_t *fb_bpp32;
  uint16_t *fb_bpp16;
  struct fb_info_s *fb_info = &fb_state->fb_info;
  const uint8_t bpp = fb_info->plane_info.bpp;
  const uint32_t xres = fb_info->video_info.xres;
  const uint32_t yres = fb_info->video_info.yres;

#ifdef CONFIG_FB_UPDATE
  int ret;
  struct fb_area_s area;
  area.x = 0;
  area.y = 0;
  area.w = xres;
  area.h = yres;
#endif

  for (j = y; j <= (y + h - 1) && j < yres; j++)
    {
      fb_tmp = fb_info->act_fbmem + (j * fb_info->plane_info.stride);
      for (i = x; i <= (x + w - 1) && x < xres; i++)
        {
          if (bpp == 32)
            {
              fb_bpp32 = (uint32_t *)fb_tmp;
              *(fb_bpp32 + i) = color;
            }
          else if (bpp == 16)
            {
              fb_bpp16 = (uint16_t *)fb_tmp;
              *(fb_bpp16 + i) = COLOR_888_TO_565(color);
            }
        }
    }

#ifdef CONFIG_FB_UPDATE
  ret = ioctl(fb_state->fb_device, FBIO_UPDATE,
              (unsigned long)((uintptr_t)&area));
  assert_int_equal(ret, 0);
#endif

  if (fb_info->plane_info.yres_virtual == (fb_info->video_info.yres * 2))
    {
      pan_display(fb_state);
      usleep(50 * 1000);
      sync_area(fb_state);
    }
}

/****************************************************************************
 * Name: drivertest_framebuffer_black
 ****************************************************************************/

static void drivertest_framebuffer_black(FAR void **state)
{
  FAR struct fb_state_s * fb_state;
  fb_state = (struct fb_state_s *)*state;

  const uint32_t xres = fb_state->fb_info.video_info.xres;
  const uint32_t yres = fb_state->fb_info.video_info.yres;

  draw_rect(fb_state, 0, 0, xres, yres, 0xff000000);
}

/****************************************************************************
 * Name: drivertest_framebuffer_white
 ****************************************************************************/

static void drivertest_framebuffer_white(FAR void **state)
{
  FAR struct fb_state_s * fb_state;
  fb_state = (struct fb_state_s *)*state;

  const uint32_t xres = fb_state->fb_info.video_info.xres;
  const uint32_t yres = fb_state->fb_info.video_info.yres;

  draw_rect(fb_state, 0, 0, xres, yres, 0xffffffff);
}

/****************************************************************************
 * Name: drivertest_framebuffer_cross
 ****************************************************************************/

static void drivertest_framebuffer_cross(FAR void **state)
{
  FAR struct fb_state_s * fb_state;
  fb_state = (struct fb_state_s *)*state;
  int i = 0;
  int start_x = 0;
  int step_width = 0;
  int step_num = 0;
  uint32_t colors_to_show[] =
    {
      0xff000000, 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff
    };

  const uint32_t xres = fb_state->fb_info.video_info.xres;
  const uint32_t yres = fb_state->fb_info.video_info.yres;

  step_num = nitems(colors_to_show);
  step_width = xres / step_num;
  for (i = 0; i < step_num; i++)
    {
      start_x = step_width * i;
      if (i == step_num - 1)
        {
          step_width = xres - start_x;
        }

      draw_rect(fb_state, start_x, 0, step_width, yres, colors_to_show[i]);
    }
}

/****************************************************************************
 * Name: drivertest_framebuffer_vertical
 ****************************************************************************/

static void drivertest_framebuffer_vertical(FAR void **state)
{
  FAR struct fb_state_s * fb_state;
  fb_state = (struct fb_state_s *)*state;
  int i = 0;
  int start_y = 0;
  int step_height = 0;
  int step_num = 8;
  uint32_t gray_color = 0;

  const uint32_t xres = fb_state->fb_info.video_info.xres;
  const uint32_t yres = fb_state->fb_info.video_info.yres;

  step_height = yres / step_num;
  for (i = 0; i < step_num; i++)
    {
      start_y = step_height * i;
      if (i == step_num - 1)
        {
          step_height = yres - start_y;
        }

      gray_color = (0xff / step_num) * i;
      draw_rect(
        fb_state, 0, start_y, xres, step_height,
        0xff000000 | gray_color << 16 | gray_color << 8 | gray_color);
    }
}

/****************************************************************************
 * Name: fb_teardown
 ****************************************************************************/

static int fb_teardown(FAR void **state)
{
  FAR struct fb_state_s * fb_state;

  fb_state = (struct fb_state_s *)*state;

  munmap(fb_state->fb_info.fb_mem, fb_state->fb_info.plane_info.fblen);

  close(fb_state->fb_device);

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

  struct fb_state_s fb_state;

  memset(&fb_state, 0, sizeof(struct fb_state_s));
  snprintf(fb_state.devpath, sizeof(fb_state.devpath), "%s",
           FB_DEFAULT_DEVPATH);

  parse_commandline(&fb_state, argc, argv);

  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_prestate_setup_teardown(drivertest_framebuffer_black,
                                             fb_setup, fb_teardown,
                                             &fb_state),
    cmocka_unit_test_prestate_setup_teardown(drivertest_framebuffer_white,
                                             fb_setup, fb_teardown,
                                             &fb_state),
    cmocka_unit_test_prestate_setup_teardown(drivertest_framebuffer_cross,
                                             fb_setup, fb_teardown,
                                             &fb_state),
    cmocka_unit_test_prestate_setup_teardown(drivertest_framebuffer_vertical,
                                             fb_setup, fb_teardown,
                                             &fb_state),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
