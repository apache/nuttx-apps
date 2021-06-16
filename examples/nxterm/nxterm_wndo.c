/****************************************************************************
 * apps/examples/nxterm/nxterm_wndo.c
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

#include <sys/boardctl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "nxterm_internal.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxwndo_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                          bool morem, FAR void *arg);
static void nxwndo_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                            FAR const struct nxgl_point_s *pos,
                            FAR const struct nxgl_rect_s *bounds,
                            FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void nxwndo_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                           uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void nxwndo_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                         FAR void *arg);
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Background window call table */

const struct nx_callback_s g_nxtermcb =
{
  nxwndo_redraw,   /* redraw */
  nxwndo_position  /* position */
#ifdef CONFIG_NX_XYINPUT
  , nxwndo_mousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxwndo_kbdin   /* kbdin */
#endif
  , NULL           /* event */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxwndo_redraw
 ****************************************************************************/

static void nxwndo_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                          bool more, FAR void *arg)
{
  nxgl_mxpixel_t wcolor[CONFIG_NX_NPLANES];

  ginfo("hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");

  /* Don't attempt to redraw if the driver has not yet been opened */

  if (g_nxterm_vars.hdrvr)
    {
      struct boardioc_nxterm_ioctl_s iocargs;
      struct nxtermioc_redraw_s redraw;

      /* Inform the NX console of the redraw request */

      redraw.handle = g_nxterm_vars.hdrvr;
      redraw.more   = more;
      nxgl_rectcopy(&redraw.rect, rect);

      iocargs.cmd = NXTERMIOC_NXTERM_REDRAW;
      iocargs.arg = (uintptr_t)&redraw;

      boardctl(BOARDIOC_NXTERM_IOCTL, (uintptr_t)&iocargs);
    }
  else
    {
      /* If the driver has not been opened, then just redraw the window color */

      wcolor[0] = CONFIG_EXAMPLES_NXTERM_WCOLOR;
      nxtk_fillwindow(hwnd, rect, wcolor);
    }
}

/****************************************************************************
 * Name: nxwndo_position
 ****************************************************************************/

static void nxwndo_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                            FAR const struct nxgl_point_s *pos,
                            FAR const struct nxgl_rect_s *bounds,
                            FAR void *arg)
{
  /* Report the position */

  ginfo("hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
        hwnd, size->w, size->h, pos->x, pos->y,
        bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Have we picked off the window bounds yet? */

  if (!g_nxterm_vars.haveres)
    {
      /* Save the background window handle */

      g_nxterm_vars.hwnd = hwnd;

      /* Save the background window size */

      g_nxterm_vars.wndo.wsize.w = size->w;
      g_nxterm_vars.wndo.wsize.h = size->h;

      /* Save the window limits (these should be the same for all places and
       * all windows)
       */

      g_nxterm_vars.xres = bounds->pt2.x + 1;
      g_nxterm_vars.yres = bounds->pt2.y + 1;

      g_nxterm_vars.haveres = true;
      sem_post(&g_nxterm_vars.eventsem);
      ginfo("Have xres=%d yres=%d\n", g_nxterm_vars.xres, g_nxterm_vars.yres);
    }
}

/****************************************************************************
 * Name: nxwndo_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void nxwndo_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                           uint8_t buttons, FAR void *arg)
{
  ginfo("hwnd=%p pos=(%d,%d) button=%02x\n",
        hwnd,  pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxwndo_kbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void nxwndo_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                         FAR void *arg)
{
  ginfo("hwnd=%p nch=%d\n", hwnd, nch);
  write(1, ch, nch);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
