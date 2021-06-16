/****************************************************************************
 * apps/examples/pwlines/pwlines_events.c
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
#include "pwlines_internal.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void pwlines_wndo_redraw(NXTKWINDOW hwnd,
              FAR const struct nxgl_rect_s *rect, bool morem, FAR void *arg);
static void pwlines_wndo_position(NXTKWINDOW hwnd, FAR
              FAR const struct nxgl_size_s *size,
              FAR const struct nxgl_point_s *pos,
              FAR const struct nxgl_rect_s *bounds,
              FAR void *arg);

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct nx_callback_s g_pwlines_wncb =
{
  pwlines_wndo_redraw,   /* redraw */
  pwlines_wndo_position  /* position */
#ifdef CONFIG_NX_XYINPUT
  , NULL                 /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , NULL                 /* kbdin */
#endif
  , NULL                 /* event */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwlines_wndo_redraw
 ****************************************************************************/

static void pwlines_wndo_redraw(NXTKWINDOW hwnd,
                             FAR const struct nxgl_rect_s *rect,
                             bool more, FAR void *arg)
{
  /* There should be no redraw requests when using per-window framebuffers */

  printf("pwlines_wndo_redraw: hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd,
         rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");
}

/****************************************************************************
 * Name: pwlines_wndo_position
 ****************************************************************************/

static void pwlines_wndo_position(NXTKWINDOW hwnd,
                               FAR const struct nxgl_size_s *size,
                               FAR const struct nxgl_point_s *pos,
                               FAR const struct nxgl_rect_s *bounds,
                               FAR void *arg)
{
  FAR struct pwlines_state_s *st = (FAR struct pwlines_state_s *)arg;

#ifdef CONFIG_EXAMPLES_PWLINES_VERBOSE
  /* Report the position */

  printf("pwlines_wndo_position: hwnd=%p size=(%d,%d) pos=(%d,%d) "
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

      printf("pwlines_wndo_position: Have xres=%d yres=%d\n",
             st->xres, st->yres);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwlines_listener
 ****************************************************************************/

FAR void *pwlines_listener(FAR void *arg)
{
  FAR struct pwlines_state_s *st = (FAR struct pwlines_state_s *)arg;
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

          printf("pwlines_listener: Lost server connection: %d\n", errno);
          pthread_exit(NULL);
        }

      /* If we received a message, we must be connected */

      if (!st->connected)
        {
          st->connected = true;
          sem_post(&st->semevent);
          printf("pwlines_listener: Connected\n");
        }
    }
}
