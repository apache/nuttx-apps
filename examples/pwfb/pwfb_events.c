/****************************************************************************
 * examples/pwfb/pwfb_events.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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
#include "pwfb_internal.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void pwfb_wndo_redraw(NXTKWINDOW hwnd,
              FAR const struct nxgl_rect_s *rect, bool morem, FAR void *arg);
static void pwfb_wndo_position(NXTKWINDOW hwnd, FAR
              FAR const struct nxgl_size_s *size,
              FAR const struct nxgl_point_s *pos,
              FAR const struct nxgl_rect_s *bounds,
              FAR void *arg);
static void pwfb_tb_redraw(NXTKWINDOW hwnd,
              FAR const struct nxgl_rect_s *rect, bool morem, FAR void *arg);
static void pwfb_tb_position(NXTKWINDOW hwnd,
              FAR const struct nxgl_size_s *size,
              FAR const struct nxgl_point_s *pos,
              FAR const struct nxgl_rect_s *bounds,
              FAR void *arg);

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct nx_callback_s g_pwfb_wncb =
{
  pwfb_wndo_redraw,   /* redraw */
  pwfb_wndo_position  /* position */
#ifdef CONFIG_NX_XYINPUT
  , NULL              /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , NULL              /* kbdin */
#endif
  , NULL              /* event */
};

const struct nx_callback_s g_pwfb_tbcb =
{
  pwfb_tb_redraw,     /* redraw */
  pwfb_tb_position    /* position */
#ifdef CONFIG_NX_XYINPUT
  , NULL              /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , NULL              /* kbdin */
#endif
  , NULL              /* event */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwfb_wndo_redraw
 ****************************************************************************/

static void pwfb_wndo_redraw(NXTKWINDOW hwnd,
                             FAR const struct nxgl_rect_s *rect,
                             bool more, FAR void *arg)
{
  /* There should be no redraw requests when using per-window framebuffers */

  printf("pwfb_wndo_redraw: hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd,
         rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");
}

/****************************************************************************
 * Name: pwfb_wndo_position
 ****************************************************************************/

static void pwfb_wndo_position(NXTKWINDOW hwnd,
                               FAR const struct nxgl_size_s *size,
                               FAR const struct nxgl_point_s *pos,
                               FAR const struct nxgl_rect_s *bounds,
                               FAR void *arg)
{
  FAR struct pwfb_state_s *st = (FAR struct pwfb_state_s *)arg;

#ifdef CONFIG_EXAMPLES_PWFB_VERBOSE
  /* Report the position */

  printf("pwfb_wndo_position: hwnd=%p size=(%d,%d) pos=(%d,%d) "
         "bounds={(%d,%d),(%d,%d)}\n",
         hwnd, size->w, size->h, pos->x, pos->y,
         bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);
#endif

  /* Have we picked off the window bounds yet? */

  if (!st->haveres)
    {
      /* Save the window limits (these should be the same for all places and
       * all windows.
       */

      st->xres = bounds->pt2.x + 1;
      st->yres = bounds->pt2.y + 1;

      st->haveres = true;
      sem_post(&st->semevent);

      printf("pwfb_wndo_position: Have xres=%d yres=%d\n",
             st->xres, st->yres);
    }
}

/****************************************************************************
 * Name: pwfb_tb_redraw
 ****************************************************************************/

static void pwfb_tb_redraw(NXTKWINDOW hwnd,
                           FAR const struct nxgl_rect_s *rect,
                           bool more, FAR void *arg)
{
  printf("pwfb_tb_redraw: hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd,
         rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");
}

/****************************************************************************
 * Name: pwfb_tb_position
 ****************************************************************************/

static void pwfb_tb_position(NXTKWINDOW hwnd,
                             FAR const struct nxgl_size_s *size,
                             FAR const struct nxgl_point_s *pos,
                             FAR const struct nxgl_rect_s *bounds,
                             FAR void *arg)
{
#ifdef CONFIG_EXAMPLES_PWFB_VERBOSE
  FAR struct nxeg_state_s *st = (FAR struct nxeg_state_s *)arg;

  /* Report the position */

  printf("pwfb_tb_position: hwnd=%p size=(%d,%d) pos=(%d,%d) "
         "bounds={(%d,%d),(%d,%d)}\n",
         hwnd, size->w, size->h, pos->x, pos->y,
         bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwfb_listener
 ****************************************************************************/

FAR void *pwfb_listener(FAR void *arg)
{
  FAR struct pwfb_state_s *st = (FAR struct pwfb_state_s *)arg;
  int ret;

  /* Process events forever */

  for (; ; )
    {
      /* Handle the next event.  If we were configured blocking, then
       * we will stay right here until the next event is received.  Since
       * we have dedicated a while thread to servicing events, it would
       * be most natural to also select CONFIG_NX_BLOCKING -- if not, the
       * following would be a tight infinite loop (unless we added addition
       * logic with nx_eventnotify and sigwait to pace it).
       */

      ret = nx_eventhandler(st->hnx);
      if (ret < 0)
        {
          /* An error occurred... assume that we have lost connection with
           * the server.
           */

          printf("pwfb_listener: Lost server connection: %d\n", errno);
          pthread_exit(NULL);
        }

      /* If we received a message, we must be connected */

      if (!st->connected)
        {
          st->connected = true;
          sem_post(&st->semevent);
          printf("pwfb_listener: Connected\n");
        }
    }
}
