/****************************************************************************
 * examples/pwlines/pwlines_motion.c
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

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxtk.h>

#include "pwlines_internal.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwlines_move_window
 ****************************************************************************/

static inline bool pwlines_move_window(FAR struct pwlines_state_s *st, int wndx)
{
  FAR struct pwlines_window_s *wndo = &st->wndo[wndx];
  FAR struct nxgl_point_s pos;
  b16_t newx;
  b16_t newy;
  bool hit = false;
  int ret;

#ifdef CONFIG_EXAMPLES_PWLINES_VERBOSE
  printf("pwlines_move_window: Velocity: (%lx.%04lx,%lx.%04lx)\n",
         (unsigned long)wndo->deltax >> 16,
         (unsigned long)wndo->deltax & 0xffff,
         (unsigned long)wndo->deltay >> 16,
         (unsigned long)wndo->deltay & 0xffff);
  printf("pwlines_move_window: Max: (%lx.%04lx,%lx.%04lx)\n",
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

#ifdef CONFIG_EXAMPLES_PWLINES_VERBOSE
  printf("pwlines_move_window: Old pos: (%lx.%04lx,%lx.%04lx) "
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

  printf("pwlines_move_window: Set position (%d,%d)\n", pos.x, pos.y);

  ret              = nxtk_setposition(wndo->hwnd, &pos);
  if (ret < 0)
    {
      printf("pwlines_move_window: ERROR:"
             "nxtk_setposition failed: %d\n",
             errno);
      return false;
    }

  /* If we hit an edge, the raise the window */

  if (hit)
    {
#ifdef CONFIG_EXAMPLES_PWLINES_VERBOSE
      printf("pwlines_move_window: New velocity: (%lx.%04lx,%lx.%04lx)\n",
             (unsigned long)wndo->deltax >> 16,
             (unsigned long)wndo->deltax & 0xffff,
             (unsigned long)wndo->deltay >> 16,
             (unsigned long)wndo->deltay & 0xffff);
      printf("pwlines_move_window: Raising window\n");
#endif

      ret          = nxtk_raise(wndo->hwnd);
      if (ret < 0)
        {
          printf("pwlines_move_window: ERROR:"
                 "nxtk_raise failed: %d\n",
                 errno);
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pwlines_motion
 ****************************************************************************/

bool pwlines_motion(FAR struct pwlines_state_s *st)
{
  int wndx;

  /* Move each window */

  for (wndx = 0; wndx < 3; wndx++)
    {
      if (!pwlines_move_window(st, wndx))
        {
          printf("pwlines_motion: ERROR:"
                 "pwlines_move_window failed for window %d\n",
                 wndx + 1);
        }
    }

  return true;
}
