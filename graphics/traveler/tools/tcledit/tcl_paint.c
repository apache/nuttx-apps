/*******************************************************************************
 * apps/graphics/traveler/tools/tcledit/tcl_paint.c
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

/*******************************************************************************
 * Included files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#ifndef NO_XSHM
#  include <sys/shm.h>
#  include <X11/extensions/XShm.h>
#endif

#include "trv_types.h"
#include "wld_debug.h"
#include "wld_mem.h"
#include "wld_bitmaps.h"
#include "wld_plane.h"
#include "wld_utils.h"
#include "tcl_x11graphics.h"

/*******************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LINE_THICKNESS_DELTA 1

/*******************************************************************************
 * Private Functions
 ****************************************************************************/

/*******************************************************************************
 * Function: tcl_color_index
 * Description:
 ***************************************************************************/

static int tcl_color_index(tcl_window_t *w)
{
  int ndx;

  if (g_edit_mode != EDITMODE_NEW)
    {
      ndx = POS_COLORS * NUM_COLORS;
    }
  else if (g_edit_plane == w->plane)
    {
      ndx = SEL_COLORS * NUM_COLORS;
    }
  else
    {
      ndx = NSEL_COLORS * NUM_COLORS;
    }

  return ndx;
}

/****************************************************************************
 * Function: tcl_paint_rectangle
 * Description:
 ***************************************************************************/

static void tcl_paint_rectangle(tcl_window_t *w, int hleft, int hright,
                                int vtop, int vbottom, dev_pixel_t pixel)
{
  dev_pixel_t *rowstart;
  dev_pixel_t *dest;
  int row;
  int col;

  /* Paint from top to bottom */

  rowstart = w->frameBuffer + vtop * w->width;

  for (row = vtop; row <= vbottom; row++)
    {
      /* Paint from left to right */

      dest = rowstart + hleft;

      for (col = hleft; col <= hright; col++)
        {
          *dest++ = pixel;
        }

      rowstart += w->width;
    }
}

/****************************************************************************
 * Function: tcl_paint_coplanar_rectangle
 * Description:
 ***************************************************************************/

static void tcl_paint_coplanar_rectangle(tcl_window_t *w,
                                         rect_data_t *ptr, int hoffset,
                                         int voffset, dev_pixel_t pixel)
{
  int hleft;
  int hright;
  int vtop;
  int vbottom;

  /* Get the left side offset and verify that some part of the rectangle lies
   * to the left of the right side of the display.
   */

  hleft = ptr->hstart - hoffset;

  if (hleft >= g_view_size)
    {
      return;
    }

  if (hleft < 0)
    {
      hleft = 0;
    }

  /* Get the right side offset and verify that some part of the rectangle lies
   * to the right of the left side of the display.
   */

  hright = ptr->hend - hoffset;

  if (hright < 0)
    {
      return;
    }

  if (hright >= g_view_size)
    {
      hright = g_view_size - 1;
    }

  /* Get the top side offset and verify that some part of the rectangle lies
   * above the bottom of the display.
   */

  vtop = ptr->vstart - voffset;

  if (vtop >= g_view_size)
    {
      return;
    }

  if (vtop < 0)
    {
      vtop = 0;
    }

  /* Get the bottom side offset and verify that some part of the rectangle lies
   * below the top of the display.
   */

  vbottom = ptr->vend - voffset;

  if (vbottom < 0)
    {
      return;
    }

  if (vbottom >= g_view_size)
    {
      vbottom = g_view_size - 1;
    }

  /* If we are viewing the Z plane, then the vpos is "down" the screen from the
   * top.  Otherwise, it is "up" the screen from the bottom.
   */

  if (w->plane != EDITPLANE_Z)
    {
      int vtmp = vbottom;

      vbottom = g_view_size - vtop - 1;
      vtop = g_view_size - vtmp - 1;
    }

  /* We now have some rectangle that we know is to fully on the display.
   * Get the display positions of the rectangle
   */

  hleft = (hleft) * w->width / g_view_size;
  hright = (hright) * w->width / g_view_size;
  vtop = (vtop) * w->height / g_view_size;
  vbottom = (vbottom) * w->height / g_view_size;

  /* Paint it */

  tcl_paint_rectangle(w, hleft, hright, vtop, vbottom, pixel);
}

/****************************************************************************
 * Function: tcl_paint_horizontal_rectangle
 * Description:
 ***************************************************************************/

static int tcl_check_horizontal_rectangle(tcl_window_t *w, rect_data_t *ptr, int pos)
{
  int posstart;
  int posend;

  /* Search the plane list for every place whose "level" component includes the
   * current map level.  This will be the vertical component of the rectangle
   * in every case except for EDITPLANE_X:
   *
   * w->plane vhead level component EDITPLANE_X g_zplane_list H (= X) EDITPLANE_Y
   * g_zplane_list V (= Y) EDITPLANE_Z g_yplane_list V (= Z)
   */

  if (w->plane != EDITPLANE_X)
    {
      posstart = ptr->vstart;
      posend = ptr->vend;
    }
  else
    {
      posstart = ptr->hstart;
      posend = ptr->hend;
    }

  /* Verify the vertical rectangle intersects the viewing plane */

  return ((posstart <= pos) && (posend >= pos));
}

/****************************************************************************
 * Function: tcl_paint_horizontal_rectangle
 * Description:
 ***************************************************************************/

static void tcl_paint_horizontal_rectangle(tcl_window_t *w, rect_data_t *ptr,
                                           int hoffset, int voffset,
                                           dev_pixel_t pixel)
{
  int hleft;
  int hright;
  int vtop;
  int vbottom;

  /* Get the "vertical" position of the horizontal line.  This will always be
   * the plane component of the rectangle, but will have to be "flipped" if the
   * viewing plane is not EDITPLANE_Z:
   *
   * w->plane vhead vertical component EDITPLANE_X g_zplane_list plane (= Z)
   * EDITPLANE_Y g_zplane_list plane (= Z) EDITPLANE_Z g_yplane_list plane (= Y)
   */

  vtop = ptr->plane - voffset;

  if ((vtop < 0) || (vtop >= g_view_size))
    {
      return;
    }

  if (w->plane != EDITPLANE_Z)
    {
      vtop = g_view_size - vtop - 1;
    }

  /* Get the extent of the horizontal line.  This will be the horizontal
   * component of the rectangle in every case except when the viewing plane if
   * EDITPLANE_X
   *
   * w->plane vhead horizontal component EDITPLANE_X g_zplane_list V (= Y) EDITPLANE_X
   * g_zplane_list H (= X) EDITPLANE_X g_yplane_list H (= X)
   */

  if (w->plane != EDITPLANE_X)
    {
      hleft = ptr->hstart;
      hright = ptr->hend;
    }
  else
    {
      hleft = ptr->vstart;
      hright = ptr->vend;
    }

  /* Get the left side offset and verify that some part of the rectangle lies
   * to the left of the right side of the display.
   */

  hleft -= hoffset;

  if (hleft >= g_view_size)
    {
      return;
    }

  if (hleft < 0)
    {
      hleft = 0;
    }

  /* Get the right side offset and verify that some part of the rectangle lies
   * to the right of the left side of the display.
   */

  hright -= hoffset;

  if (hright < 0)
    {
      return;
    }

  if (hright >= g_view_size)
    {
      hright = g_view_size - 1;
    }

  /* We now have some line segment that we know is to fully on the display.
   * Get the display positions of the line segment.
   */

  hleft = hleft * w->width / g_view_size;
  hright = hright * w->width / g_view_size;
  vtop = vtop * w->height / g_view_size;

  /* Apply Line thickness to get a narrow rectangle */

  vbottom = vtop + LINE_THICKNESS_DELTA;

  if (vbottom >= w->height)
    {
      vbottom = w->height - 1;
    }

  vtop -= LINE_THICKNESS_DELTA;

  if (vtop < 0)
    {
      vtop = 0;
    }

  /* Paint it */

  tcl_paint_rectangle(w, hleft, hright, vtop, vbottom, pixel);
}

/****************************************************************************
 * Function: tcl_check_vertical_rectangle
 * Description:
 ***************************************************************************/

static int tcl_check_vertical_rectangle(tcl_window_t *w, rect_data_t *ptr, int pos)
{
  int posstart;
  int posend;

  /* Search the plane list for every place whose "level" component includes the
   * current map level.  This will be the horizontal component of the rectangle
   * in every case except for EDITPLANE_Z
   *
   * x->plane hhead level component EDITPLANE_X g_yplane_list H (= X) EDITPLANE_Y
   * g_xplane_list H (= Y) EDITPLANE_Z g_xplane_list V (= Z)
   */

  if (w->plane != EDITPLANE_Z)
    {
      posstart = ptr->hstart;
      posend = ptr->hend;
    }
  else
    {
      posstart = ptr->vstart;
      posend = ptr->vend;
    }

  /* Verify the vertical rectangle intersects the viewing plane */

  return ((posstart <= pos) && (posend >= pos));
}

/****************************************************************************
 * Function: tcl_paint_vertical_rectangle
 * Description:
 ***************************************************************************/

static void tcl_paint_vertical_rectangle(tcl_window_t *w, rect_data_t *ptr,
                                        int hoffset, int voffset, dev_pixel_t pixel)
{
  int hleft;
  int hright;
  int vtop;
  int vbottom;

  /* Get the "horizontal" position of the vertical line.  This will always be
   * the plane component of the rectangle:
   *
   * w->plane hhead horizontal component EDITPLANE_X g_yplane_list plane (= Y)
   * EDITPLANE_Y g_xplane_list plane (= X) EDITPLANE_Z g_xplane_list plane (= X)
   */

  hleft = ptr->plane - hoffset;

  if ((hleft < 0) || (hleft >= g_view_size))
    {
      return;
    }

  /* Get the extent of the vertical line.  This will be the vertical component
   * of the rectangle in every case except when the viewing plane is
   * EDITPLANE_Z and will have to be "flipped" if the viewing plane is not
   * EDITPLANE_Z:
   *
   * w->plane hhead horizontal component EDITPLANE_X g_yplane_list V (= Z) EDITPLANE_Y
   * g_xplane_list V (= Z) EDITPLANE_Z g_xplane_list H (= Y)
   */

  if (w->plane != EDITPLANE_Z)
    {
      vtop = ptr->vstart;
      vbottom = ptr->vend;
    }
  else
    {
      vtop = ptr->hstart;
      vbottom = ptr->hend;
    }

  /* Get the top side offset and verify that some part of the rectangle lies
   * above the bottom of the display.
   */

  vtop -= voffset;

  if (vtop >= g_view_size)
    {
      return;
    }

  if (vtop < 0)
    {
      vtop = 0;
    }

  /* Get the bottom side offset and verify that some part of the rectangle lies
   * below the top of the display
   */

  vbottom -= voffset;

  if (vbottom < 0)
    {
      return;
    }

  if (vbottom >= g_view_size)
    {
      vbottom = g_view_size - 1;
    }

  /* Flip if necessary */

  if (w->plane != EDITPLANE_Z)
    {
      int vtmp = vbottom;

      vbottom = g_view_size - vtop - 1;
      vtop = g_view_size - vtmp - 1;
    }

  /* We now have some line segment that we know is to fully on the display.
   * Get the display positions of the line segment
   */

  hleft = hleft * w->width / g_view_size;
  vtop = vtop * w->height / g_view_size;
  vbottom = vbottom * w->height / g_view_size;

  /* Apply Line thickness to get a narrow rectangle */

  hright = hleft + LINE_THICKNESS_DELTA;

  if (hright >= w->width)
    {
      hright = w->width;
    }

  hleft -= LINE_THICKNESS_DELTA;

  if (hleft < 0)
    {
      hleft = 0;
    }

  /* Paint it */

  tcl_paint_rectangle(w, hleft, hright, vtop, vbottom, pixel);
}

/*******************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: tcl_paint_background
 * Description:
 ***************************************************************************/

void tcl_paint_background(tcl_window_t *w)
{
  dev_pixel_t *dest;
  dev_pixel_t pixel;
  int ndx = tcl_color_index(w);
  int i;

  info("g_edit_mode=%d g_edit_plane=%d plane=%d ndx=%d\n",
        g_edit_mode, g_edit_plane, w->plane, ndx);

  pixel = w->colorLookup[ndx + BKGD_COLOR];
  dest = w->frameBuffer;

  info("ndx=%d dest=%p pixel=0x%06lx\n", ndx, dest, (unsigned long)pixel);

  for (i = 0; i < w->width * w->height; i++)
    {
      *dest++ = pixel;
    }
}

/****************************************************************************
 * Function: tcl_paint_position
 * Description:
 ***************************************************************************/

void tcl_paint_position(tcl_window_t *w)
{
  /* Only show the position cursor in POS mode */

  if (g_edit_mode == EDITMODE_POS)
    {
      dev_pixel_t *dest;
      dev_pixel_t pixel;
      int ndx = tcl_color_index(w);
      int hpos;
      int vpos;
      int i;

      info("plane=%d ndx=%d\n", w->plane, ndx);

      /* Horizontal and vertical positions will depend on which plane is
       * selected.
       */

      switch (w->plane)
        {
        default:
        case EDITPLANE_X:
          hpos = g_plane_position[EDITPLANE_Y] - g_coord_offset[EDITPLANE_Y];
          vpos = g_plane_position[EDITPLANE_Z] - g_coord_offset[EDITPLANE_Z];
          vpos = g_view_size - vpos - 1;
          break;

        case EDITPLANE_Y:
          hpos = g_plane_position[EDITPLANE_X] - g_coord_offset[EDITPLANE_X];
          vpos = g_plane_position[EDITPLANE_Z] - g_coord_offset[EDITPLANE_Z];
          vpos = g_view_size - vpos - 1;
          break;

        case EDITPLANE_Z:
          hpos = g_plane_position[EDITPLANE_X] - g_coord_offset[EDITPLANE_X];
          vpos = g_plane_position[EDITPLANE_Y] - g_coord_offset[EDITPLANE_Y];
          break;
        }

      /* Scale the position to fit in the display */

      hpos = (hpos * w->width) / g_view_size;
      vpos = (vpos * w->height) / g_view_size;

      /* Paint the horizontal line */

      pixel = w->colorLookup[ndx + HLINE_COLOR];
      dest = w->frameBuffer + vpos * w->width;

      info("dest=%p pixel=0x%06lx\n", dest, (unsigned long)pixel);

      for (i = 0; i < w->width; i++)
        {
          *dest++ = pixel;
        }

      /* Paint the vertical line */

      pixel = w->colorLookup[ndx + HLINE_COLOR];
      dest = w->frameBuffer + hpos;

      info("dest=%p pixel=0x%06lx\n", dest, (unsigned long)pixel);

      for (i = 0; i < w->height; i++)
        {
          *dest = pixel;
          dest += w->width;
        }
    }
}

/****************************************************************************
 * Function: tcl_paint_grid
 * Description:
 ***************************************************************************/

void tcl_paint_grid(tcl_window_t *w)
{
  dev_pixel_t *dest;
  dev_pixel_t pixel;
  int ndx = tcl_color_index(w);
  int hoffset, hpos, haccum;
  int voffset, vpos, vaccum;
  int gridmask;
  int i;

  info("plane=%d ndx=%d\n", w->plane, ndx);

  /* Horizontal and vertical positions will depend on which plane is selected. */

  switch (w->plane)
    {
    default:
    case EDITPLANE_X:
      hoffset = g_coord_offset[EDITPLANE_Y];
      voffset = g_view_size - g_coord_offset[EDITPLANE_Z] - 1;
      break;

    case EDITPLANE_Y:
      hoffset = g_coord_offset[EDITPLANE_X];
      voffset = g_view_size - g_coord_offset[EDITPLANE_Z] - 1;
      break;

    case EDITPLANE_Z:
      hoffset = g_coord_offset[EDITPLANE_X];
      voffset = g_coord_offset[EDITPLANE_Y];
      break;
    }

  /* This is the color of the grid */

  pixel = w->colorLookup[ndx + GRID_COLOR];

  /* Paint the horizontal grid lines */

  gridmask = g_grid_step - 1;
  vaccum = ((voffset + gridmask) & ~gridmask) - voffset;
  vpos = (vaccum * w->height) / g_view_size;

  do
    {
      /* Paint one horizontal grid line */

      dest = w->frameBuffer + vpos * w->width;

      for (i = 0; i < w->width; i++)
        {
          *dest++ = pixel;
        }

      /* Scale the next grid position onto the display */

      vaccum += g_grid_step;
      vpos = (vaccum * w->height) / g_view_size;

    }
  while (vpos < w->height);

  /* Paint the vertical grid lines */
  /* Force the starting offsets to lie on a grid line */

  haccum = ((hoffset + gridmask) & ~gridmask) - hoffset;
  hpos = (haccum * w->width) / g_view_size;

  do
    {
      /* Paint one vertical grid line */

      dest = w->frameBuffer + hpos;

      for (i = 0; i < w->height; i++)
        {
          *dest = pixel;
          dest += w->width;
        }

      /* Scale the next grid position onto the display */

      haccum += g_grid_step;
      hpos = (haccum * w->width) / g_view_size;
    }
  while (hpos < w->width);
}

/****************************************************************************
 * Function: tcl_paint_rectangles
 * Description:
 ***************************************************************************/

void tcl_paint_rectangles(tcl_window_t *w)
{
  rect_list_t *head, *vhead, *hhead;
  rect_list_t *list;
  rect_data_t *ptr;
  dev_pixel_t pixel;
  int ndx = tcl_color_index(w);
  int hoffset, voffset;
  int hplane, vplane;
  int pos;

  /* Select the plane lists and plane position for the current window */

  switch (w->plane)
    {
    default:
    case EDITPLANE_X:
      pos = g_plane_position[EDITPLANE_X];
      hplane = EDITPLANE_Y;
      vplane = EDITPLANE_Z;
      head = g_xplane_list.head;
      hhead = g_yplane_list.head;
      vhead = g_zplane_list.head;
      break;

    case EDITPLANE_Y:
      pos = g_plane_position[EDITPLANE_Y];
      hplane = EDITPLANE_X;
      vplane = EDITPLANE_Z;
      head = g_yplane_list.head;
      hhead = g_xplane_list.head;
      vhead = g_zplane_list.head;
      break;

    case EDITPLANE_Z:
      pos = g_plane_position[EDITPLANE_Z];
      hplane = EDITPLANE_X;
      vplane = EDITPLANE_Y;
      head = g_zplane_list.head;
      hhead = g_xplane_list.head;
      vhead = g_yplane_list.head;
      break;
    }

  hoffset = g_coord_offset[hplane];
  voffset = g_coord_offset[vplane];

  /* Display the rectangles which are in the viewing plane */

  pixel = w->colorLookup[ndx + NRECT_COLOR];

  for (list = head; (list); list = list->flink)
    {
      ptr = &list->d;

      /* We only want the rectangles which lie on the current display level */

      if (ptr->plane == pos)
        {
          tcl_paint_coplanar_rectangle(w, ptr, hoffset, voffset, pixel);
        }
    }

  /* Are we currently editting a new plane that is not yet in the list? It it
   * is the same as the display plane, then we will want to display that as
   * well.
   */

  if ((g_edit_mode == EDITMODE_NEW) &&
      (g_edit_plane == w->plane) && (g_edit_rect.plane == pos))
    {
      pixel = w->colorLookup[ndx + SRECT_COLOR];
      tcl_paint_coplanar_rectangle(w, &g_edit_rect, hoffset, voffset, pixel);
    }

  /* Display the rectangles which are orthogonal and "horizontal" with respect
   * to the viewing plane.
   *
   * Process every plane in the vertical axis (these will be horizontal) This
   * will by the Z plane for every viewing plane except EDITPLANE_Z where it
   * will be the Y plane */

  pixel = w->colorLookup[ndx + BRECT_COLOR];

  for (list = vhead; (list); list = list->flink)
    {
      ptr = &list->d;

      /* Search the plane list for every place whose "level" component includes
       * the current map level.
       */

      if (tcl_check_horizontal_rectangle(w, ptr, pos))
        {
          tcl_paint_horizontal_rectangle(w, ptr, hoffset, voffset, pixel);
        }
    }

  /* Are we currently editting a new plane that is not yet in the list? */

  if ((g_edit_mode == EDITMODE_NEW) && (g_edit_plane == vplane))
    {
      ptr = &g_edit_rect;

      if (tcl_check_horizontal_rectangle(w, ptr, pos))
        {
          pixel = w->colorLookup[ndx + SRECT_COLOR];
          tcl_paint_horizontal_rectangle(w, ptr, hoffset, voffset, pixel);
        }
    }

  /* Display the rectangles which are orthogonal and "vertical" with respect to
   * the viewing plane.
   *
   * Process every plane in the horizontal axis (these will be vertical) This
   * will by the X plane for every viewing plane except EDITPLANE_X where it
   * will be the Y plane.
   */

  pixel = w->colorLookup[ndx + BRECT_COLOR];

  for (list = hhead; (list); list = list->flink)
    {
      ptr = &list->d;

      /* Search the plane list for every place whose "level" component includes
       * the current map level.
       */

      if (tcl_check_vertical_rectangle(w, ptr, pos))
        {
          tcl_paint_vertical_rectangle(w, &list->d, hoffset, voffset, pixel);
        }
    }

  /* Are we currently editting a new plane that is not yet in the list? */

  if ((g_edit_mode == EDITMODE_NEW) && (g_edit_plane == hplane))
    {
      ptr = &g_edit_rect;

      if (tcl_check_vertical_rectangle(w, ptr, pos))
        {
          pixel = w->colorLookup[ndx + SRECT_COLOR];
          tcl_paint_vertical_rectangle(w, ptr, hoffset, voffset, pixel);
        }
    }
}
