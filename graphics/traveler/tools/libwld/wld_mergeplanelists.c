/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_mergeplanelists.c
 * This file contains the logic  that concatenates two world plane lists.
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
 * Name: wld_merge_planelists
 * Description:
 * This function concatenates two world plane lists
 ************************************************************************/

void wld_merge_planelists(rect_head_t * outList, rect_head_t * inList)
{
  rect_list_t *inRect, *nextInRect;
  rect_list_t *outRect, *prevRect;

  /* Initialize the inner plane search loop */

  outRect = outList->head;

  /* Process every plane in the input plane list */

  for (inRect = inList->head; (inRect); inRect = nextInRect)
    {
      nextInRect = inRect->flink;

      /* Search the output plane list to find the location to insert the input
       * rectangle. Each is list is maintained in ascending plane order.
       */

      for (;
           ((outRect) && (outRect->d.plane < inRect->d.plane));
           outRect = outRect->flink);

      /* Add the inRect to the spot found in the list.  Check if the inRect
       * goes at the one of the ends of the list. */

      if (!outRect)
        {
          /* No rectangle with plane larger than the one to be added was found
           * in the list.  The inRect goes at the end of the list.
           */

          prevRect = outList->tail;
          if (!prevRect)
            {
              /* Special case: The list is empty */

              inRect->flink = NULL;
              inRect->blink = NULL;
              outList->head = inRect;
              outList->tail = inRect;
            }
          else
            {
              inRect->flink = NULL;
              inRect->blink = prevRect;
              prevRect->flink = inRect;
              outList->tail = inRect;
            }
        }
      else
        {
          /* The inRect goes just before outRect */

          prevRect = outRect->blink;
          if (!prevRect)
            {
              /* Special case: Insert at the head of the list */

              inRect->flink = outRect;
              inRect->blink = NULL;
              outRect->blink = inRect;
              outList->head = inRect;
            }
          else
            {
              /* Insert in the middle of the list */

              inRect->flink = outRect;
              inRect->blink = prevRect;
              prevRect->flink = inRect;
              outRect->blink = inRect;
            }
        }

      /* Set up for the next time through */

      outRect = inRect;
    }

  /* Mark the input list empty */

  inList->head = NULL;
  inList->tail = NULL;
}
