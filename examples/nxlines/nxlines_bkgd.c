/****************************************************************************
 * apps/examples/nxlines/nxlines_bkgd.c
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <debug.h>
#include <fixedmath.h>
#include <inttypes.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

#include "nxlines.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_NX_ANTIALIASING
  /* If anti-aliasing is enabled, then we must clear a slightly
   * larger region to prevent weird edge effects.
   */

#  define CLEAR_WIDTH (CONFIG_EXAMPLES_NXLINES_LINEWIDTH + 2)
#else
#  define CLEAR_WIDTH CONFIG_EXAMPLES_NXLINES_LINEWIDTH
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxlines_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool morem, FAR void *arg);
static void nxlines_position(NXWINDOW hwnd,
                             FAR const struct nxgl_size_s *size,
                             FAR const struct nxgl_point_s *pos,
                             FAR const struct nxgl_rect_s *bounds,
                             FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void nxlines_mousein(NXWINDOW hwnd,
                            FAR const struct nxgl_point_s *pos,
                            uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void nxlines_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Background window call table */

const struct nx_callback_s g_nxlinescb =
{
  nxlines_redraw,   /* redraw */
  nxlines_position  /* position */
#ifdef CONFIG_NX_XYINPUT
  , nxlines_mousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxlines_kbdin   /* kbdin */
#endif
  , NULL            /* event */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxlines_redraw
 ****************************************************************************/

static void nxlines_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg)
{
  ginfo("hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");
}

/****************************************************************************
 * Name: nxlines_position
 ****************************************************************************/

static void nxlines_position(NXWINDOW hwnd,
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

  if (!g_nxlines.havepos)
    {
      /* Save the background window handle */

      g_nxlines.hbkgd = hwnd;

      /* Save the window limits */

      g_nxlines.xres = bounds->pt2.x + 1;
      g_nxlines.yres = bounds->pt2.y + 1;

      g_nxlines.havepos = true;
      sem_post(&g_nxlines.eventsem);
      ginfo("Have xres=%d yres=%d\n", g_nxlines.xres, g_nxlines.yres);
    }
}

/****************************************************************************
 * Name: nxlines_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void nxlines_mousein(NXWINDOW hwnd,
                            FAR const struct nxgl_point_s *pos,
                            uint8_t buttons, FAR void *arg)
{
  printf("nxlines_mousein: hwnd=%p pos=(%d,%d) button=%02x\n",
         hwnd,  pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxlines_kbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void nxlines_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg)
{
  ginfo("hwnd=%p nch=%d\n", hwnd, nch);

  /* In this example, there is no keyboard so a keyboard event is not
   * expected.
   */

  printf("nxlines_kbdin: Unexpected keyboard callback\n");
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxlines_test
 *
 * Description:
 *   Update line motion.
 *
 ****************************************************************************/

void nxlines_test(NXWINDOW hwnd)
{
  struct nxgl_point_s center;
  struct nxgl_vector_s vector;
  struct nxgl_vector_s previous;
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES];
  nxgl_coord_t maxradius;
  nxgl_coord_t radius;
  nxgl_coord_t halfx;
  nxgl_coord_t halfy;
  b16_t angle;
  b16_t sinangle;
  b16_t cosangle;
  int ret;

  /* Get the maximum radius and center of the circle */

  maxradius = MIN(g_nxlines.yres, g_nxlines.xres) >> 1;
  center.x  = g_nxlines.xres >> 1;
  center.y  = g_nxlines.yres >> 1;

  /* Draw a circular background */

  radius = maxradius - ((CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 1) / 2);
  color[0] = CONFIG_EXAMPLES_NXLINES_CIRCLECOLOR;
  ret = nx_fillcircle((NXWINDOW)hwnd, &center, radius, color);
  if (ret < 0)
    {
      printf("nxlines_test: nx_fillcircle failed: %d\n", ret);
    }

  /* Draw the circular border */

  color[0] = CONFIG_EXAMPLES_NXLINES_BORDERCOLOR;
  ret = nx_drawcircle((NXWINDOW)hwnd, &center, radius,
                      CONFIG_EXAMPLES_NXLINES_BORDERWIDTH, color);
  if (ret < 0)
    {
      printf("nxlines_test: nx_fillcircle failed: %d\n", ret);
    }

  /* Back off the radius to account for the thickness of border line
   * and with a big fudge factor that will (hopefully) prevent the corners
   * of the lines from overwriting the border.  This is overly complicated
   * here because we don't assume anything about the screen resolution or
   * the borderwidth or the line thickness (and there are certainly some
   * smarter ways to do this).
   */

  if (maxradius > (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 80))
    {
      radius = maxradius - (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 40);
    }
  else if (maxradius > (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 60))
    {
      radius = maxradius - (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 30);
    }
  else if (maxradius > (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 40))
    {
      radius = maxradius - (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 20);
    }
  else if (maxradius > (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 20))
    {
      radius = maxradius - (CONFIG_EXAMPLES_NXLINES_BORDERWIDTH + 10);
    }
  else
    {
      radius = maxradius - CONFIG_EXAMPLES_NXLINES_BORDERWIDTH;
    }

  /* The loop, showing the line in various orientations */

  angle = 0;
  previous.pt1.x = center.x;
  previous.pt1.y = center.y;
  previous.pt2.x = center.x;
  previous.pt2.y = center.y;

  for (; ; )
    {
      /* Determine the position of the line on this pass */

      sinangle = b16sin(angle);
      halfx = b16toi(b16muli(sinangle, radius));

      cosangle = b16cos(angle);
      halfy = b16toi(b16muli(cosangle, radius));

      vector.pt1.x = center.x + halfx;
      vector.pt1.y = center.y + halfy;
      vector.pt2.x = center.x - halfx;
      vector.pt2.y = center.y - halfy;

      printf("Angle: %08" PRIx32 " vector: (%d,%d)->(%d,%d)\n",
             angle, vector.pt1.x, vector.pt1.y, vector.pt2.x, vector.pt2.y);

      /* Clear the previous line by overwriting it with the circle color */

      color[0] = CONFIG_EXAMPLES_NXLINES_CIRCLECOLOR;
      ret = nx_drawline((NXWINDOW)hwnd, &previous, CLEAR_WIDTH, color,
                        NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("nxlines_test: nx_drawline failed clearing: %d\n", ret);
        }

      /* Draw the new line */

      color[0] = CONFIG_EXAMPLES_NXLINES_LINECOLOR;
      ret = nx_drawline((NXWINDOW)hwnd, &vector,
                        CONFIG_EXAMPLES_NXLINES_LINEWIDTH, color,
                        NX_LINECAP_NONE);
      if (ret < 0)
        {
          printf("nxlines_test: nx_drawline failed clearing: %d\n", ret);
        }

#ifdef CONFIG_NX_ANTIALIASING
      /* If anti-aliasing is enabled, then we must clear a slightly
       * larger region to prevent weird edge effects.
       */

      halfx = b16toi(b16muli(sinangle, radius + 1));
      halfy = b16toi(b16muli(cosangle, radius + 1));

      previous.pt1.x = center.x + halfx;
      previous.pt1.y = center.y + halfy;
      previous.pt2.x = center.x - halfx;
      previous.pt2.y = center.y - halfy;
#else
      memcpy(&previous, &vector, sizeof(struct nxgl_vector_s));
#endif

      /* Set up for the next time through the loop then sleep for a bit. */

      angle += b16PI / 16;  /* 32 angular positions in full circle */

      /* Check if we have gone all the way around */

      if (angle > (31 *  (2 * b16PI) / 32))
        {
          /* Wrap back to zero and continue with the test */

          angle = 0;
        }

      usleep(500 * 1000);
    }
}
