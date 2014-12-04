/****************************************************************************
 * apps/graphics/traveler/trv_nxbkgd.c
 * NX background window callback handlers
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#ifdef CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT
#  include "trv_input.h"
#endif

#include <string.h>
#include <semaphore.h>
#include <errno.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxfonts.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void trv_nxredraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool morem, FAR void *arg);
static void trv_nxposition(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void trv_nxmousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void trv_nxkbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Background window call table */

const struct nx_callback_s g_trv_nxcallback =
{
  trv_nxredraw,   /* redraw */
  trv_nxposition  /* position */
#ifdef CONFIG_NX_XYINPUT
  , trv_nxmousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , trv_nxkbdin   /* my kbdin */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_nxredraw
 ****************************************************************************/

static void trv_nxredraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg)
{
}

/****************************************************************************
 * Name: trv_nxposition
 ****************************************************************************/

static void trv_nxposition(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg)
{
  FAR struct trv_graphics_info_s *ginfo = (FAR struct trv_graphics_info_s *)arg;

  /* Report the position */

  trv_vdebug("hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
        hwnd, size->w, size->h, pos->x, pos->y,
        bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Have we picked off the window bounds yet? */

  if (!g_trv_nxresolution)
    {
      /* Save the background window handle */

      ginnfo->bgwnd = hwnd;

      /* Save the background window size */

      ginfo->width  = size->w;
      ginfo->height = size->h;

      g_trv_nxresolution = true;
      sem_post(&g_trv_nxevent);
      trv_vdebug("Have width=%d height=%d\n", ginfo->width, ginfo->height);
    }
}

/****************************************************************************
 * Name: trv_nxmousein
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void trv_nxmousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg)
{
  trv_vdebug("trv_nxmousein: hwnd=%p pos=(%d,%d) button=%02x\n",
         hwnd,  pos->x, pos->y, buttons);

#ifdef CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT
  trv_input_xyinput((trv_coord_t)pos->x,(trv_coord_t) pos->y, buttons);
#endif
}
#endif

/****************************************************************************
 * Name: trv_nxkbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void trv_nxkbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg)
{
  trv_vdebug("hwnd=%p nch=%d\n", hwnd, nch);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
