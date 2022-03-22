/****************************************************************************
 * apps/examples/nxdemo/nxdemo_bkgd.c
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "nxdemo.h"
#include "images.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Select renderer -- Some additional logic would be required to support
 * pixel depths that are not directly addressable (1,2,4, and 24).
 */

#if CONFIG_EXAMPLES_NXDEMO_BPP == 1
#  define RENDERER nxf_convert_1bpp
#elif CONFIG_EXAMPLES_NXDEMO_BPP == 2
#  define RENDERER nxf_convert_2bpp
#elif CONFIG_EXAMPLES_NXDEMO_BPP == 4
#  define RENDERER nxf_convert_4bpp
#elif CONFIG_EXAMPLES_NXDEMO_BPP == 8
#  define RENDERER nxf_convert_8bpp
#elif CONFIG_EXAMPLES_NXDEMO_BPP == 16
#  define RENDERER nxf_convert_16bpp
#elif CONFIG_EXAMPLES_NXDEMO_BPP == 24
#  define RENDERER nxf_convert_24bpp
#elif CONFIG_EXAMPLES_NXDEMO_BPP == 32
#  define RENDERER nxf_convert_32bpp
#else
#  error "Unsupported CONFIG_EXAMPLES_NXDEMO_BPP"
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxdemo_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                          bool morem, FAR void *arg);
static void nxdemo_position(NXWINDOW hwnd,
                            FAR const struct nxgl_size_s *size,
                            FAR const struct nxgl_point_s *pos,
                            FAR const struct nxgl_rect_s *bounds,
                            FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void nxdemo_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                           uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void nxdemo_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                         FAR void *arg);
#endif

static void nxdemo_demo_1(NXWINDOW hwnd);
static void nxdemo_demo_2(NXWINDOW hwnd);
static void nxdemo_demo_3(NXWINDOW hwnd);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Background window call table */

const struct nx_callback_s g_nxdemocb =
{
  nxdemo_redraw,    /* redraw */
  nxdemo_position   /* position */
#ifdef CONFIG_NX_XYINPUT
  , nxdemo_mousein  /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxdemo_kbdin    /* kbdin */
#endif
  , NULL            /* event */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxdemo_redraw
 ****************************************************************************/

static void nxdemo_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                              bool more, FAR void *arg)
{
  ginfo("hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
        hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
        more ? "true" : "false");
}

/****************************************************************************
 * Name: nxdemo_position
 ****************************************************************************/

static void nxdemo_position(NXWINDOW hwnd,
                            FAR const struct nxgl_size_s *size,
                            FAR const struct nxgl_point_s *pos,
                            FAR const struct nxgl_rect_s *bounds,
                            FAR void *arg)
{
  /* Report the position */

  ginfo("hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
        hwnd, size->w, size->h, pos->x, pos->y,
        bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Have we picked off the window bounds yet? */

  if (!g_nxdemo.havepos)
    {
      /* Save the background window handle */

      g_nxdemo.hbkgd = hwnd;

      /* Save the window limits */

      g_nxdemo.xres = bounds->pt2.x + 1;
      g_nxdemo.yres = bounds->pt2.y + 1;

      g_nxdemo.havepos = true;
      sem_post(&g_nxdemo.eventsem);
      ginfo("Have xres=%d yres=%d\n", g_nxdemo.xres, g_nxdemo.yres);
    }
}

/****************************************************************************
 * Name: nxdemo_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void nxdemo_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                               uint8_t buttons, FAR void *arg)
{
  printf("nxdemo_mousein: hwnd=%p pos=(%d,%d) button=%02x\n",
         hwnd, pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxdemo_kbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void nxdemo_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                             FAR void *arg)
{
  ginfo("hwnd=%p nch=%d\n", hwnd, nch);

  /* In this example, there is no keyboard so a keyboard event is not
   * expected.
   */

  printf("nxdemo_kbdin: Unexpected keyboard callback\n");
}
#endif

/****************************************************************************
 * Name: nxdemo_demo_1
 ****************************************************************************/

static void nxdemo_demo_1(NXWINDOW hwnd)
{
  struct nxgl_point_s center;
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES];
  nxgl_coord_t circle_radius = 0;
  bool direction_x = false;
  bool direction_y = false;
  int ret;
  int i;

  circle_radius = 3;
  i = 0;
  center.x = center.y = circle_radius * 2;

  while (i < 270)
    {
      if (direction_x == false)
        {
          center.x++;
        }
      else
        {
          center.x--;
        }

      if (center.x >= g_nxdemo.xres - circle_radius)
        {
          direction_x = true;
        }
      else if (center.x <= circle_radius)
        {
          direction_x = false;
        }

      if (direction_y == false)
        {
          center.y++;
        }
      else
        {
          center.y--;
        }

      if (center.y >= g_nxdemo.yres - circle_radius)
        {
          direction_y = true;
        }
      else if (center.y <= circle_radius)
        {
          direction_y = false;
        }

      color[0] = CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR;
      ret = nx_drawcircle((NXWINDOW)hwnd, &center, circle_radius, 1, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }

      usleep(30000);

      color[0] = CONFIG_EXAMPLES_NXDEMO_BGCOLOR;
      ret = nx_drawcircle((NXWINDOW)hwnd, &center, circle_radius, 1, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }

      i++;
    }

  center.x = g_nxdemo.xres >> 1;
  center.y = g_nxdemo.yres >> 1;

  for (i = 0; i < (MIN(g_nxdemo.xres, g_nxdemo.yres) >> 1); i++)
    {
      circle_radius = i;

      color[0] = CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR;
      ret = nx_fillcircle((NXWINDOW)hwnd, &center, circle_radius, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }

      usleep(100000);

      color[0] = CONFIG_EXAMPLES_NXDEMO_BGCOLOR;
      ret = nx_fillcircle((NXWINDOW)hwnd, &center, circle_radius, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }
    }

  for (i = MIN(g_nxdemo.xres, g_nxdemo.yres) >> 1; i > 1; i--)
    {
      circle_radius = i;

      color[0] = CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR;
      ret = nx_fillcircle((NXWINDOW)hwnd, &center, circle_radius, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }

      usleep(100000);

      color[0] = CONFIG_EXAMPLES_NXDEMO_BGCOLOR;
      ret = nx_fillcircle((NXWINDOW)hwnd, &center, circle_radius, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }
    }
}

/****************************************************************************
 * Name: nxdemo_demo_2
 ****************************************************************************/

static void nxdemo_demo_2(NXWINDOW hwnd)
{
  struct nxgl_point_s center;
  struct nxgl_rect_s rect;
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES];
  int ret;
  int i;

  center.x = g_nxdemo.xres >> 1;
  center.y = g_nxdemo.yres >> 1;

  for (i = 0; i < (MIN(g_nxdemo.xres, g_nxdemo.yres) >> 1); i++)
    {
      rect.pt1.x = center.x - i;
      rect.pt1.y = center.y - i;
      rect.pt2.x = center.x + i;
      rect.pt2.y = center.y + i;

      color[0] = CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR;
      ret = nx_fill((NXWINDOW)hwnd, &rect, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }

      usleep(100000);

      color[0] = CONFIG_EXAMPLES_NXDEMO_BGCOLOR;
      ret = nx_fill((NXWINDOW)hwnd, &rect, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }
    }

  for (i = MIN(g_nxdemo.xres, g_nxdemo.yres) >> 1; i > 1; i--)
    {
      rect.pt1.x = center.x - i;
      rect.pt1.y = center.y - i;
      rect.pt2.x = center.x + i;
      rect.pt2.y = center.y + i;

      color[0] = CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR;
      ret = nx_fill((NXWINDOW)hwnd, &rect, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }

      usleep(100000);

      color[0] = CONFIG_EXAMPLES_NXDEMO_BGCOLOR;
      ret = nx_fill((NXWINDOW)hwnd, &rect, color);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawcircle failed: %d\n", ret);
        }
    }
}

/****************************************************************************
 * Name: nxdemo_demo_3
 ****************************************************************************/

static void nxdemo_demo_3(NXWINDOW hwnd)
{
  struct nxgl_vector_s vect;
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES];
  nxgl_coord_t line_width = 2;
  int ret;
  int i;

  for (i = 0; i < g_nxdemo.yres; i++)
    {
      vect.pt1.x = 0;
      vect.pt1.y = i;
      vect.pt2.x = g_nxdemo.xres;
      vect.pt2.y = i;

      color[0] = CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR;
      ret = nx_drawline((NXWINDOW)hwnd,
                        &vect,
                        line_width,
                        color, NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawline failed: %d\n", ret);
        }

      usleep(50000);

      color[0] = CONFIG_EXAMPLES_NXDEMO_BGCOLOR;
      ret = nx_drawline((NXWINDOW)hwnd,
                        &vect,
                        line_width,
                        color, NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawline failed: %d\n", ret);
        }
    }

  for (i = 0; i < g_nxdemo.xres; i++)
    {
      vect.pt1.x = i;
      vect.pt1.y = 0;
      vect.pt2.x = i;
      vect.pt2.y = g_nxdemo.yres;

      color[0] = CONFIG_EXAMPLES_NXDEMO_DRAWCOLOR;
      ret = nx_drawline((NXWINDOW)hwnd,
                        &vect,
                        line_width,
                        color, NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawline failed: %d\n", ret);
        }

      usleep(50000);

      color[0] = CONFIG_EXAMPLES_NXDEMO_BGCOLOR;
      ret = nx_drawline((NXWINDOW)hwnd,
                        &vect,
                        line_width,
                        color, NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("nxnxdemo: nx_drawline failed: %d\n", ret);
        }
    }
}

/****************************************************************************
 * Name: nxdemo_demo_4
 ****************************************************************************/

static void nxdemo_demo_4(NXWINDOW hwnd)
{
  struct nxgl_point_s origin;
  struct nxgl_rect_s rect;
  FAR const void *src[CONFIG_NX_NPLANES];
  int ret;

  rect.pt1.x = 0;
  rect.pt1.y = 0;
  rect.pt2.x = 23;
  rect.pt2.y = 23;

  origin.x   = 0;
  origin.y   = 0;

  src[0] = (FAR const void *)g_battery;

  ret = nx_bitmap((NXWINDOW)hwnd,
                  &rect,
                  src,
                  &origin,
                  3);
  if (ret < 0)
    {
      printf("nxnxdemo: nx_bitmap failed: %d\n", ret);
    }

  sleep(5);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxdemo_hello
 *
 * Description:
 *   Demo displaying graphic on the screen
 *
 ****************************************************************************/

void nxdemo_hello(NXWINDOW hwnd)
{
  nxdemo_demo_1((NXWINDOW)hwnd);
  nxdemo_demo_2((NXWINDOW)hwnd);
  nxdemo_demo_3((NXWINDOW)hwnd);
  nxdemo_demo_4((NXWINDOW)hwnd);
}
