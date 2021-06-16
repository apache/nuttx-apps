/****************************************************************************
 * apps/examples/nx/nx_events.c
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
#include <semaphore.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxtk.h>
#include "nx_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxeg_redraw(NXEGWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool morem, FAR void *arg);
static void nxeg_position(NXEGWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void nxeg_mousein(NXEGWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg);
#endif

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
static void nxeg_tbredraw(NXEGWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                          bool morem, FAR void *arg);
static void nxeg_tbposition(NXEGWINDOW hwnd, FAR const struct nxgl_size_s *size,
                            FAR const struct nxgl_point_s *pos,
                            FAR const struct nxgl_rect_s *bounds,
                            FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void nxeg_tbmousein(NXEGWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                            uint8_t buttons, FAR void *arg);
#endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct nx_callback_s g_nxcb =
{
  nxeg_redraw,     /* redraw */
  nxeg_position    /* position */
#ifdef CONFIG_NX_XYINPUT
  , nxeg_mousein   /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxeg_kbdin     /* kbdin */
#endif
  , NULL           /* event */
};

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
const struct nx_callback_s g_tbcb =
{
  nxeg_tbredraw,   /* redraw */
  nxeg_tbposition  /* position */
#ifdef CONFIG_NX_XYINPUT
  , nxeg_tbmousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxeg_tbkbdin   /* my kbdin */
#endif
  , NULL           /* event */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxeg_fillwindow
 ****************************************************************************/

static inline void nxeg_fillwindow(NXEGWINDOW hwnd,
                                   FAR const struct nxgl_rect_s *rect,
                                   FAR struct nxeg_state_s *st)
{
  int ret;

#ifdef CONFIG_EXAMPLES_NX_RAWWINDOWS
  ret = nx_fill(hwnd, rect, st->color);
  if (ret < 0)
    {
      printf("nxeg_fillwindow: nx_fill failed: %d\n", errno);
    }
#else
  ret = nxtk_fillwindow(hwnd, rect, st->color);
  if (ret < 0)
    {
      printf("nxeg_fillwindow: nxtk_fillwindow failed: %d\n", errno);
    }
#endif
#ifdef CONFIG_NX_KBD
  nxeg_filltext(hwnd, rect, st);
#endif
}

/****************************************************************************
 * Name: nxeg_fillwindow
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
static inline void nxeg_filltoolbar(NXTKWINDOW htb,
                                   FAR const struct nxgl_rect_s *rect,
                                   nxgl_mxpixel_t color[CONFIG_NX_NPLANES])
{
  int ret;

  ret = nxtk_filltoolbar(htb, rect, color);
  if (ret < 0)
    {
      printf("nxeg_filltoolbar: nxtk_filltoolbar failed: %d\n", errno);
    }
}
#endif

/****************************************************************************
 * Name: nxeg_redraw
 ****************************************************************************/

static void nxeg_redraw(NXEGWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg)
{
  FAR struct nxeg_state_s *st = (FAR struct nxeg_state_s *)arg;
  printf("nxeg_redraw%d: hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         st->wnum, hwnd,
         rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");

  nxeg_fillwindow(hwnd, rect, st);
}

/****************************************************************************
 * Name: nxeg_position
 ****************************************************************************/

static void nxeg_position(NXEGWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg)
{
  FAR struct nxeg_state_s *st = (FAR struct nxeg_state_s *)arg;

  /* Report the position */

  printf("nxeg_position%d: hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
         st->wnum, hwnd, size->w, size->h, pos->x, pos->y,
         bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Have we picked off the window bounds yet? */

  if (!b_haveresolution)
    {
      /* Save the window limits (these should be the same for all places and all windows */

      g_xres = bounds->pt2.x;
      g_yres = bounds->pt2.y;

      b_haveresolution = true;
      sem_post(&g_semevent);
      printf("nxeg_position2: Have xres=%d yres=%d\n", g_xres, g_yres);
    }
}

/****************************************************************************
 * Name: nxeg_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void nxeg_mousein(NXEGWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg)
{
  FAR struct nxeg_state_s *st = (FAR struct nxeg_state_s *)arg;
  printf("nxeg_mousein%d: hwnd=%p pos=(%d,%d) button=%02x\n",
         st->wnum, hwnd,  pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxeg_tbredraw
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
static void nxeg_tbredraw(NXEGWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                          bool more, FAR void *arg)
{
  FAR struct nxeg_state_s *st = (FAR struct nxeg_state_s *)arg;
  printf("nxeg_tbredraw%d: hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         st->wnum, hwnd,
         rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");
  nxeg_filltoolbar(hwnd, rect, g_tbcolor);
}
#endif

/****************************************************************************
 * Name: nxeg_position
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
static void nxeg_tbposition(NXEGWINDOW hwnd, FAR const struct nxgl_size_s *size,
                           FAR const struct nxgl_point_s *pos,
                           FAR const struct nxgl_rect_s *bounds,
                           FAR void *arg)
{
  FAR struct nxeg_state_s *st = (FAR struct nxeg_state_s *)arg;

  /* Report the position */

  printf("nxeg_ptbosition%d: hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
         st->wnum, hwnd, size->w, size->h, pos->x, pos->y,
         bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);
}
#endif

/****************************************************************************
 * Name: nxeg_tbmousein
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_NX_RAWWINDOWS
#ifdef CONFIG_NX_XYINPUT
static void nxeg_tbmousein(NXEGWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                          uint8_t buttons, FAR void *arg)
{
  FAR struct nxeg_state_s *st = (FAR struct nxeg_state_s *)arg;

  printf("nxeg_tbmousein%d: hwnd=%p pos=(%d,%d) button=%02x\n",
         st->wnum, hwnd,  pos->x, pos->y, buttons);
}
#endif
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nx_listenerthread
 ****************************************************************************/

FAR void *nx_listenerthread(FAR void *arg)
{
  int ret;

  /* Process events forever */

  for (;;)
    {
      /* Handle the next event.  If we were configured blocking, then
       * we will stay right here until the next event is received.  Since
       * we have dedicated a while thread to servicing events, it would
       * be most natural to also select CONFIG_NX_BLOCKING -- if not, the
       * following would be a tight infinite loop (unless we added addition
       * logic with nx_eventnotify and sigwait to pace it).
       */

      ret = nx_eventhandler(g_hnx);
      if (ret < 0)
        {
          /* An error occurred... assume that we have lost connection with
           * the server.
           */

          printf("nx_listenerthread: Lost server connection: %d\n", errno);
          exit(NXEXIT_LOSTSERVERCONN);
        }

      /* If we received a message, we must be connected */

      if (!g_connected)
        {
          g_connected = true;
          sem_post(&g_semevent);
          printf("nx_listenerthread: Connected\n");
        }
    }
}
