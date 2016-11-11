/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_findplane.c
 * This file contains the logic to find the plane at the specified location
 * in the world plane list
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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

/*************************************************************************
 * Included files
 *************************************************************************/

#include <stdio.h>
#include "wld_plane.h"

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_find_plane
 * Description:
 * This function finds the plane at the specified point in the world plane
 * list
 ************************************************************************/

rect_list_t *wld_find_plane(wld_coord_t h, wld_coord_t v, wld_coord_t plane,
                            rect_head_t * list)
{
  rect_list_t *rect;

  /* Search until we find the first occurrence of a rectangle in the specified
   * plane.
   */

  for (rect = list->head;
       ((rect) && (rect->d.plane < plane)); rect = rect->flink);

  /* Then look at every occurrence of rectangles at this plane or until a
   * rectangle containing the specified point is found.
   */

  for (; ((rect) && (rect->d.plane == plane)); rect = rect->flink)
    {
      /* There is another rectangle in this plane.  Check if the point lies
       * within the rectangle.
       */

      if ((h >= rect->d.hstart) && (h <= rect->d.hend)
          && (v >= rect->d.vstart) && (v <= rect->d.vend))
        return rect;
    }

  return NULL;
}
