/****************************************************************************
 * apps/examples/pwfb/pwfb_motion.c
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

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxcursor.h>
#include <nuttx/nx/nxtk.h>

#include "pwfb_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwfb_move_window
 ****************************************************************************/

#if CONFIG_EXAMPLES_PWFB_NWINDOWS > 0
static inline bool pwfb_move_window(FAR struct pwfb_state_s *st, int wndx)
{
  FAR struct pwfb_window_s *wndo = &st->wndo[wndx];
  FAR struct nxgl_point_s pos;
  b16_t newx;
  b16_t newy;
  bool hit = false;
  int ret;

#ifdef CONFIG_EXAMPLES_PWFB_VERBOSE
  printf("pwfb_move_window: Velocity: (%lx.%04lx,%lx.%04lx)\n",
         (unsigned long)wndo->deltax >> 16,
         (unsigned long)wndo->deltax & 0xffff,
         (unsigned long)wndo->deltay >> 16,
         (unsigned long)wndo->deltay & 0xffff);
  printf("pwfb_move_window: Max: (%lx.%04lx,%lx.%04lx)\n",
         (unsigned long)wndo->xmax >> 16,
         (unsigned long)wndo->xmax & 0xffff,
         (unsigned long)wndo->ymax >> 16,
         (unsigned long)wndo->ymax & 0xffff);
#endif

  /* Update X position */

  newx             = wndo->xpos + wndo->deltax;

  /* Check for collision with left or right side */

  if (newx <= 0)
    {
      newx         = 0;
      wndo->deltax = -wndo->deltax;
      hit          = true;
    }
  else if (newx >= wndo->xmax)
    {
      newx         = wndo->xmax;
      wndo->deltax = -wndo->deltax;
      hit          = true;
    }

  /* Update Y position */

  newy             = wndo->ypos + wndo->deltay;

  /* Check for collision with top or bottom side */

  if (newy <= 0)
    {
      newy         = 0;
      wndo->deltay = -wndo->deltay;
      hit          = true;
    }
  else if (newy >= wndo->ymax)
    {
      newy         = wndo->ymax;
      wndo->deltay = -wndo->deltay;
      hit          = true;
    }

#ifdef CONFIG_EXAMPLES_PWFB_VERBOSE
  printf("pwfb_move_window: Old pos: (%lx.%04lx,%lx.%04lx) "
         "New pos: (%lx.%04lx,%lx.%04lx)\n",
         (unsigned long)wndo->xpos >> 16,
         (unsigned long)wndo->xpos & 0xffff,
         (unsigned long)wndo->ypos >> 16,
         (unsigned long)wndo->ypos & 0xffff,
         (unsigned long)newx >> 16,
         (unsigned long)newx & 0xffff,
         (unsigned long)newy >> 16,
         (unsigned long)newy & 0xffff);
#endif

  /* Set the new window position */

  wndo->xpos       = newx;
  wndo->ypos       = newy;

  pos.x            = b16toi(newx);
  pos.y            = b16toi(newy);

  printf("pwfb_move_window: Set position (%d,%d)\n", pos.x, pos.y);

  ret              = nxtk_setposition(wndo->hwnd, &pos);
  if (ret < 0)
    {
      printf("pwfb_move_window: ERROR:"
             "nxtk_setposition failed: %d\n",
             errno);
      return false;
    }

  /* If we hit an edge, the raise the window */

  if (hit)
    {
#ifdef CONFIG_EXAMPLES_PWFB_VERBOSE
      printf("pwfb_move_window: New velocity: (%lx.%04lx,%lx.%04lx)\n",
             (unsigned long)wndo->deltax >> 16,
             (unsigned long)wndo->deltax & 0xffff,
             (unsigned long)wndo->deltay >> 16,
             (unsigned long)wndo->deltay & 0xffff);
      printf("pwfb_move_window: Raising window\n");
#endif

      ret          = nxtk_raise(wndo->hwnd);
      if (ret < 0)
        {
          printf("pwfb_move_window: ERROR:"
                 "nxtk_raise failed: %d\n",
                 errno);
          return false;
        }
    }

  return true;
}
#endif

/****************************************************************************
 * Name: pwfb_move_cursor
 ****************************************************************************/

#ifdef CONFIG_NX_SWCURSOR
static inline bool pwfb_move_cursor(FAR struct pwfb_state_s *st)
{
  FAR struct nxgl_point_s pos;
  b16_t newx;
  b16_t newy;
  int ret;

#ifdef CONFIG_EXAMPLES_PWFB_VERBOSE
  printf("pwfb_move_cursor: State: %u countdown: %u blinktime: %u\n",
         (unsigned int)st->cursor.state, (unsigned int)st->cursor.countdown,
         (unsigned int)st->cursor.blinktime;
  printf("pwfb_move_cursor: Velocity: (%lx.%04lx,%lx.%04lx)\n",
         (unsigned long)st->cursor.deltax >> 16,
         (unsigned long)st->cursor.deltax & 0xffff,
         (unsigned long)st->cursor.deltay >> 16,
         (unsigned long)st->cursor.deltay & 0xffff);
#endif

  /* Handle the update based on cursor state */
  /* If the state is not PFWB_CURSOR_STATIONARY, then update the cursor
   * position.
   */

 if (st->cursor.state != PFWB_CURSOR_STATIONARY)
   {
      /* Update X position */

      newx                  = st->cursor.xpos + st->cursor.deltax;

      /* Check for collision with left or right side */

      if (newx <= 0)
        {
          newx              = 0;
          st->cursor.deltax = -st->cursor.deltax;
        }
      else if (newx >= st->cursor.xmax)
        {
          newx              = st->cursor.xmax;
          st->cursor.deltax = -st->cursor.deltax;
        }

      /* Update Y position */

      newy                  = st->cursor.ypos + st->cursor.deltay;

      /* Check for collision with top or bottom side */

      if (newy <= 0)
        {
          newy              = 0;
          st->cursor.deltay = -st->cursor.deltay;
        }
      else if (newy >= st->cursor.ymax)
        {
          newy              = st->cursor.ymax;
          st->cursor.deltay = -st->cursor.deltay;
        }

#ifdef CONFIG_EXAMPLES_PWFB_VERBOSE
      printf("pwfb_move_cursor: Old pos: (%lx.%04lx,%lx.%04lx) "
             "New pos: (%lx.%04lx,%lx.%04lx)\n",
             (unsigned long)st->cursor.xpos >> 16,
             (unsigned long)st->cursor.xpos & 0xffff,
             (unsigned long)st->cursor.ypos >> 16,
             (unsigned long)st->cursor.ypos & 0xffff,
             (unsigned long)newx >> 16,
             (unsigned long)newx & 0xffff,
             (unsigned long)newy >> 16,
             (unsigned long)newy & 0xffff);
#endif

      /* Set the new cursor position */

      st->cursor.xpos       = newx;
      st->cursor.ypos       = newy;

      pos.x                 = b16toi(newx);
      pos.y                 = b16toi(newy);

      printf("pwfb_move_cursor: Set position (%d,%d)\n", pos.x, pos.y);

      ret                   = nxcursor_setposition(st->hnx, &pos);
      if (ret < 0)
        {
          printf("nxcursor_setposition: ERROR:"
                 "nxcursor_setposition failed: %d\n",
                 errno);

          return false;
        }
   }

  /* Check for state changes */

  switch (st->cursor.state)
    {
      case PFWB_CURSOR_MOVING:
        if (--st->cursor.countdown <= 0)
          {
            /* Set up for the the STATIONARY state next */

            st->cursor.state     = PFWB_CURSOR_STATIONARY;
            st->cursor.countdown = CURSOR_STATIONARY_DELAY;
          }

        break;

      case PFWB_CURSOR_STATIONARY:
        if (--st->cursor.countdown <= 0)
          {
            /* Set up for the the BLINKING state next */

            st->cursor.state     = PFWB_CURSOR_BLINKING;
            st->cursor.countdown = CURSOR_BLINKING_DELAY;
            st->cursor.blinktime = CURSOR_BLINK_DELAY;
          }

        break;

      case PFWB_CURSOR_BLINKING:
        if (--st->cursor.countdown <= 0)
          {
            /* Set up for the the MOVING state next */

            st->cursor.state     = PFWB_CURSOR_MOVING;
            st->cursor.countdown = CURSOR_MOVING_DELAY;
            st->cursor.visible   = true;
            nxcursor_enable(st->hnx, true);
          }
        else if (--st->cursor.blinktime <= 0)
          {
            /* Toggle visibility */

            st->cursor.blinktime = CURSOR_BLINK_DELAY;
            st->cursor.visible   = !st->cursor.visible;
            nxcursor_enable(st->hnx, st->cursor.visible);
          }
    }

  return true;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwfb_motion
 ****************************************************************************/

bool pwfb_motion(FAR struct pwfb_state_s *st)
{
#if CONFIG_EXAMPLES_PWFB_NWINDOWS > 0
  int wndx;

  /* Move each window */

  for (wndx = 0; wndx < CONFIG_EXAMPLES_PWFB_NWINDOWS; wndx++)
    {
      if (!pwfb_move_window(st, wndx))
        {
          printf("pwfb_motion: ERROR:"
                 "pwfb_move_window failed for window %d\n",
                 wndx + 1);
        }
    }
#endif

#ifdef CONFIG_NX_SWCURSOR
  /* Move the cursor */

  if (!pwfb_move_cursor(st))
    {
      printf("pwfb_motion: ERROR: pwfb_move_cursor failed\n");
    }
#endif

  return true;
}
