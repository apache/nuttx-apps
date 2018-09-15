/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_addplane.c
 * This file contains the logic to add a plane to a world plane list
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
 * Name: wld_add_plane
 * Description:
 * This function adds a plane to a world plane list
 ************************************************************************/

void wld_add_plane(rect_list_t * newRect, rect_head_t * list)
{
  rect_list_t *next;
  rect_list_t *prev;

  /* Search the list to find the location to insert the new rectangle. Each
   * list is maintained in ascending plane order.
   */

  for (next = list->head;
       ((next) && (next->d.plane < newRect->d.plane));
       next = next->flink);

  /* Add the newRect to the spot found in the list.  Check if the newRect goes
   * at the end of the list.
   */

  if (!next)
    {
      /* No rectangle with plane larger than the one to be added was found in
       * the list.  The newRect goes at the end of the list.
       */

      prev = list->tail;
      if (!prev)
        {
          /* Special case: The list is empty */

          newRect->flink = NULL;
          newRect->blink = NULL;
          list->head = newRect;
          list->tail = newRect;
        }
      else
        {
          newRect->flink = NULL;
          newRect->blink = prev;
          prev->flink = newRect;
          list->tail = newRect;
        }
    }
  else
    {
      /* The newRect goes just before next */

      prev = next->blink;
      if (!prev)
        {
          /* Special case: Insert at the head of the list */

          newRect->flink = next;
          newRect->blink = NULL;
          next->blink = newRect;
          list->head = newRect;
        }
      else
        {
          /* Insert in the middle of the list */

          newRect->flink = next;
          newRect->blink = prev;
          prev->flink = newRect;
          next->blink = newRect;
        }
    }
}
