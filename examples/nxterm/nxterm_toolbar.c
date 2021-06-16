/****************************************************************************
 * apps/examples/nxterm/nxterm_toolbar.c
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
#include <nuttx/nx/nxfonts.h>

#include "nxterm_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxtool_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                          bool morem, FAR void *arg);
static void nxtool_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                            FAR const struct nxgl_point_s *pos,
                            FAR const struct nxgl_rect_s *bounds,
                            FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void nxtool_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                           uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void nxtool_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                         FAR void *arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Background window call table */

const struct nx_callback_s g_nxtoolcb =
{
  nxtool_redraw,   /* redraw */
  nxtool_position  /* position */
#ifdef CONFIG_NX_XYINPUT
  , nxtool_mousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxtool_kbdin   /* kbdin */
#endif
  , NULL           /* event */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxtool_redraw
 ****************************************************************************/

static void nxtool_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                          bool more, FAR void *arg)
{
  nxgl_mxpixel_t color[CONFIG_NX_NPLANES];
  int ret;

  ginfo("hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");

  color[0] = CONFIG_EXAMPLES_NXTERM_TBCOLOR;
  ret = nxtk_filltoolbar(hwnd, rect, color);
  if (ret < 0)
    {
      gerr("ERROR: nxtk_filltoolbar failed: %d\n", errno);
    }
}

/****************************************************************************
 * Name: nxtool_position
 ****************************************************************************/

static void nxtool_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                            FAR const struct nxgl_point_s *pos,
                            FAR const struct nxgl_rect_s *bounds,
                            FAR void *arg)
{
  ginfo("hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
        hwnd, size->w, size->h, pos->x, pos->y,
        bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);
}

/****************************************************************************
 * Name: nxtool_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void nxtool_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                           uint8_t buttons, FAR void *arg)
{
  ginfo("hwnd=%p pos=(%d,%d) button=%02x\n", hwnd,  pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxtool_kbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void nxtool_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                         FAR void *arg)
{
  ginfo("hwnd=%p nch=%d\n", hwnd, nch);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
