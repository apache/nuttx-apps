/****************************************************************************
 * apps/graphics/traveler/src/trv_planelist.c
 * This file contains the logic to manage world plane lists.
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
 * Included files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_mem.h"
#include "trv_plane.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The is the world!!! The world is described by lists of rectangles, one
 * for each of the X, Y, and Z planes.
 */

struct trv_rect_head_s  g_xplane;  /* list of X=plane rectangles */
struct trv_rect_head_s  g_yplane;  /* list of Y=plane rectangles */
struct trv_rect_head_s  g_zplane;  /* list of Z=plane rectangles */

/* "Deallocated" planes are retained in a free list */

FAR struct trv_rect_list_s *g_rect_freelist;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_release_worldplane
 *
 * Description:
 *   This function deallocates one plane of the world
 *
 ****************************************************************************/

static void trv_release_worldplane(FAR struct trv_rect_list_s *rect)
{
  FAR struct trv_rect_list_s *next;

  while (rect)
    {
      next = rect->flink;
      trv_free((void *) rect);
      rect = next;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_initialize_planes
 *
 * Description:
 *
 ****************************************************************************/

int trv_initialize_planes(void)
{
  g_xplane.head   = NULL;
  g_xplane.tail   = NULL;
  g_yplane.head   = NULL;
  g_yplane.tail   = NULL;
  g_zplane.head   = NULL;
  g_zplane.tail   = NULL;
  g_rect_freelist = NULL;

  return OK;
}

/****************************************************************************
 * Name: trv_add_plane
 *
 * Description:
 *   This function adds a plane to a world plane list
 *
 ****************************************************************************/

void trv_add_plane(FAR struct trv_rect_list_s *rect,
                   FAR struct trv_rect_head_s *list)
{
  struct trv_rect_list_s *next;
  struct trv_rect_list_s *prev;

  /* Search the list to find the location to insert the new rectangle. Each
   * is list is maintained in ascending plane order.
   */

  for (next = list->head;
       ((next) && (next->d.plane < rect->d.plane));
       next = next->flink);

  /* Add the 'rect' to the spot found in the list.  Check if the 'rect' lies
   * at the end of the list.
   */

  if (!next)
    {
      /* No rectangle with plane larger than the one to be added was found
       * in the list.  The 'rect' goes at the end of the list.
       */

      prev = list->tail;
      if (!prev)
        {
          /* Special case:  The list is empty */

          rect->flink = NULL;
          rect->blink = NULL;
          list->head  = rect;
          list->tail  = rect;
        }
      else
        {
          rect->flink = NULL;
          rect->blink = prev;
          prev->flink = rect;
          list->tail  = rect;
        }
    }
  else
    {
      /* The 'rect' goes just before 'next' */

      prev = next->blink;
      if (!prev)
        {
          /* Special case:  Insert at the head of the list */

          rect->flink = next;
          rect->blink = NULL;
          next->blink = rect;
          list->head  = rect;
        }
      else
        {
          /* Insert in the middle of the list */

          rect->flink = next;
          rect->blink = prev;
          prev->flink = rect;
          next->blink = rect;
        }
    }
}

/****************************************************************************
 * Name: trv_move_plane
 *
 * Description:
 *
 *   This function removes the specified plane from the world plane srclist
 *   then adds it to the world plane destlist
 *
 ****************************************************************************/

void trv_move_plane(FAR struct trv_rect_list_s *rect,
                    FAR struct trv_rect_head_s *destlist,
                    FAR struct trv_rect_head_s *srclist)
{
  /* Un-hook the backward link to the rect */

  if (rect->flink)
    {
      rect->flink->blink = rect->blink;
    }
  else
    {
      srclist->tail = rect->blink;
    }

  /* Un-hook the forward link to the rect */

  if (rect->blink)
    {
      rect->blink->flink = rect->flink;
    }
  else
    {
      srclist->head = rect->flink;
    }

  /* Then add the rect to the specified list */

  trv_add_plane(rect, destlist);
}

/****************************************************************************
 * Name: trv_merge_planelists
 *
 * Description:
 *   This function concatenates two world plane lists
 *
 ****************************************************************************/

void trv_merge_planelists(FAR struct trv_rect_head_s *outlist,
                          FAR struct trv_rect_head_s *inlist)
{
  struct trv_rect_list_s *inrect;
  struct trv_rect_list_s *nextin;
  struct trv_rect_list_s *outrect;
  struct trv_rect_list_s *prevout;

  /* Initialize the inner plane search loop */

  outrect = outlist->head;

  /* Process every plane in the input plane list */

  for (inrect = inlist->head; (inrect); inrect = nextin)
    {
      nextin = inrect->flink;

      /* Search the output plane list to find the location to insert the
       * input rectangle. Each is list is maintained in ascending plane
       * order.
       */

      for (;
           outrect && outrect->d.plane < inrect->d.plane;
           outrect = outrect->flink);

      /* Add the 'inrect' to the spot found in the list.  Check if the
       * 'inrect' goes at the one of the ends of the list.
       */

      if (!outrect)
        {
          /* No rectangle with plane larger than the one to be added
           * was found in the list.  The 'inrect' goes at the end of
           * the list.
           */

          prevout = outlist->tail;
          if (!prevout)
            {
              /* Special case:  The list is empty */

              inrect->flink  = NULL;
              inrect->blink  = NULL;
              outlist->head  = inrect;
              outlist->tail  = inrect;
            }
          else
            {
              inrect->flink  = NULL;
              inrect->blink  = prevout;
              prevout->flink = inrect;
              outlist->tail  = inrect;
            }
        }
      else
        {
          /* The 'inrect' goes just before 'outrect' */

          prevout = outrect->blink;
          if (!prevout)
            {
              /* Special case:  Insert at the head of the list */

              inrect->flink  = outrect;
              inrect->blink  = NULL;
              outrect->blink = inrect;
              outlist->head  = inrect;
            }
          else
            {
              /* Insert in the middle of the list */

              inrect->flink  = outrect;
              inrect->blink  = prevout;
              prevout->flink = inrect;
              outrect->blink = inrect;
            }
        }

      /* Set up for the next time through */

      outrect = inrect;
    }

  /* Mark the input list empty */

  inlist->head = NULL;
  inlist->tail = NULL;
}

/****************************************************************************
 * Function: trv_new_plane
 *
 * Description:
 *   This function allocates memory for a new plane rectangle.
 *
 ****************************************************************************/

FAR struct trv_rect_list_s *trv_new_plane(void)
{
  FAR struct trv_rect_list_s *rect;

  /* Try to get the new structure from the free list */

  rect = g_rect_freelist;
  if (rect)
    {
      /* Got get... remove it from the g_rect_freelist; */

      g_rect_freelist = rect->flink;
    }
  else
    {
      /* Nothing on the free list.  Allocate a new one */

      rect = (FAR struct trv_rect_list_s*)trv_malloc(sizeof(struct trv_rect_list_s));
    }

  return rect;
}

/****************************************************************************
 * Name: trv_release_planes
 *
 * Description:
 *
 *   This function deallocates the entire world.
 *
 ****************************************************************************/

void trv_release_planes(void)
{
  /* Release all world planes */

  trv_release_worldplane(g_xplane.head);
  trv_release_worldplane(g_yplane.head);
  trv_release_worldplane(g_zplane.head);
  trv_release_worldplane(g_rect_freelist);

  /* Re-initialize the world plane lists */

  trv_initialize_planes();
}
