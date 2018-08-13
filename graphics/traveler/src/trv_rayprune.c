/****************************************************************************
 * apps/graphics/traveler/src/trv_rayprune.c
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
#include "trv_world.h"
#include "trv_plane.h"
#include "trv_trigtbl.h"
#include "trv_raycast.h"
#include "trv_rayprune.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* These definitions are used to decompose a range of angles into quadrants */

#define START0   0x0001
#define STARTQ1  0x0002
#define START90  0x0004
#define STARTQ2  0x0008
#define START180 0x0010
#define STARTQ3  0x0020
#define START270 0x0040
#define STARTQ4  0x0080
#define END0     0x0100
#define ENDQ1    0x0200
#define END90    0x0400
#define ENDQ2    0x0800
#define END180   0x1000
#define ENDQ3    0x2000
#define END270   0x4000
#define ENDQ4    0x8000

#define S0EQ1   (START0|ENDQ1)
#define SQ1EQ1  (STARTQ1|ENDQ1)
#define SQ1E90  (STARTQ1|END90)
#define SQ1EQ2  (STARTQ1|ENDQ2)
#define S90EQ2  (START90|ENDQ2)
#define SQ2EQ2  (STARTQ2|ENDQ2)
#define SQ2E180 (STARTQ2|END180)
#define SQ2EQ3  (STARTQ2|ENDQ3)
#define S180EQ3 (START180|ENDQ3)
#define SQ3EQ3  (STARTQ3|ENDQ3)
#define SQ3E270 (STARTQ3|END270)
#define SQ3EQ4  (STARTQ3|ENDQ4)
#define S270EQ4 (START270|ENDQ4)
#define SQ4EQ4  (STARTQ4|ENDQ4)
#define SQ4E0   (STARTQ4|END0)
#define SQ4EQ1  (STARTQ4|ENDQ1)

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

enum working_plane_state_e
{
  WEMPTY = 0,
  WTOP,
  WBOTTOM
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void trv_ray_yawxprune_14cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw);
static void trv_ray_yawxprune_14ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw);
static void trv_ray_yawxprune_23cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw);
static void trv_ray_yawxprune_23ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw);
static void trv_ray_yawyprune_12cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw);
static void trv_ray_yawyprune_12ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw);
static void trv_ray_yawyprune_34cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw);
static void trv_ray_yawyprune_34ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw);
static void trv_ray_mkplanelist(FAR struct trv_rect_head_s *outlist,
                                FAR struct trv_rect_head_s *inlist,
                                int16_t yaw);
static void trv_ray_selectupper(FAR struct trv_rect_head_s *outlist,
                                FAR struct trv_rect_head_s *inlist);
static void trv_ray_selectlower(FAR struct trv_rect_head_s *outlist,
                                FAR struct trv_rect_head_s *inlist);

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct trv_rect_head_s g_ray_xplane;    /* List of X=plane rectangles */
struct trv_rect_head_s g_ray_yplane;    /* List of Y=plane rectangles */
struct trv_rect_head_s g_ray_zplane;    /* List of Z=plane rectangles */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static enum working_plane_state_e g_working_planestate;
static struct trv_rect_head_s g_working_xplane;   /* List of X=plane rectangles */
static struct trv_rect_head_s g_working_yplane;   /* List of Y=plane rectangles */

static struct trv_rect_head_s g_candidate_xplane; /* List of X=plane rectangles */
static struct trv_rect_head_s g_candidate_yplane; /* List of Y=plane rectangles */
static struct trv_rect_head_s g_candidate_zplane; /* List of Z=plane rectangles */

static struct trv_rect_head_s g_discard_xplane;   /* List of discarded X=plane
                                                   * rectangles */
static struct trv_rect_head_s g_discard_yplane;   /* List of discarded Y=plane
                                                   * rectangles */
static struct trv_rect_head_s g_discard_zplane;   /* List of discarded Z=plane
                                                   * rectangles */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  trv_ray_yawxprune_14cw
 *
 * Description:
 *   This function casts a ray along the Y-Axis looking at points of
 *   intersection on the X-Axis.  We are pointed toward in the "right" half
 *   of the view.  The X ray cast will proceed positively along the X axis.
 *   Every plane in the input plane list that lies to the "right" of the
 *   camera position and "clockwise" of the current yaw is moved into
 *   the output X plane list.
 *
 ****************************************************************************/

static void trv_ray_yawxprune_14cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current X plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next X plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t relx;                  /* Relative position of the X plane */
  trv_coord_t absy;                  /* Absolute Y position at relx given yaw */
  trv_coord_t lastrelx = -1;         /* Last relative X position processed */
  int32_t dydx;                      /* Rate of change of Y wrt X (double) */

  /* Pre-calculate the rate of change of Y with respect to X */
  /* The tangent is equal to the rate of change of Y with respect to the
   * X-axis.  The tangent is stored at double the "normal" scaling.
   */

  dydx = TAN(yaw);

  /* Look at every rectangle lying in the X plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->head; (entry); entry = next)
    {
      next = entry->flink;
      rect = &entry->d;

      /* Search for a rectangle which lies "beyond" the current camera
       * position
       */

      if (rect->plane > g_camera.x)
        {
          /* Get the X distance to the plane */

          relx = rect->plane - g_camera.x;

          /* Calculate the Y position at this relative X position.  We can
           * skip this step if we are processing another rectangle at the
           * same relx distance.
           */

          if (relx != lastrelx)
            {
              int32_t deltay;  /* Scale == "triple" */

              /* The dydx is stored at double the"normal" scaling -- so deltay
               * is "triple" precision
               */

              deltay   = dydx * ((int32_t) relx);
              absy     = tTOs(deltay) + g_camera.y; /* back to "single" */
              lastrelx = relx;
            }

          /* Check if any part of this plane lies clockwise from this Y
           * position.  In quadrants 1 & 4, clockwise corresponds to smaller Y
           * values
           */

          if (absy >= rect->hstart)
            {
              /* Remove the plane from the input plane list and add it to the
               * output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_yawxprune_14ccw
 *
 * Description:
 *   This function casts a ray along the Y-Axis looking at points of
 *   intersection on the X-Axis.  We are pointed toward in the "right" half
 *   of the view.  The X ray cast will proceed positively along the X axis.
 *   Every plane in the input plane list that lies to the "right" of the
 *   camera position and "counterclockwise" of the current yaw is moved into
 *   the output X plane list.
 *
 ****************************************************************************/

static void trv_ray_yawxprune_14ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current X plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next X plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t relx;                  /* Relative position of the X plane */
  trv_coord_t absy;                  /* Absolute Y position at relx given yaw */
  trv_coord_t lastrelx = -1;         /* Last relative X position processed */
  int32_t dydx;                      /* Rate of change of Y wrt X (double) */

  /* Pre-calculate the rate of change of Y with respect to X */
  /* The tangent is equal to the rate of change of Y with respect to the
   * X-axis.  The tangent is stored at double the "normal" scaling.
   */

  dydx = TAN(yaw);

  /* Look at every rectangle lying in the X plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->head; (entry); entry = next)
    {
      next = entry->flink;
      rect = &entry->d;

      /* Search for a rectangle which lies "beyond" the current camera
       * position
       */

      if (rect->plane > g_camera.x)
        {
          /* Get the X distance to the plane */

          relx = rect->plane - g_camera.x;

          /* Calculate the Y position at this relative X position.  We can
           * skip this step if we are processing another rectangle at the
           * same relx distance.
           */

          if (relx != lastrelx)
            {
              int32_t deltay;  /* Scale == "triple" */

              /* The dydx is stored at double the"normal" scaling -- so deltay
               * is "triple" precision
               */

              deltay    = dydx * ((int32_t) relx);
              absy     = tTOs(deltay) + g_camera.y; /* back to "single" */
              lastrelx = relx;
            }

          /* Check if any part of this plane lies counterclockwise from this Y
           * position.  In quadrants 1 & 4, counterclockwise corresponds to
           * larger Y values
           */

          if (absy <= rect->hend)
            {
              /* Remove the plane from the input plane list and add it to the
               * output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  rayYawYPrune23CW
 *
 * Description:
 *   This function casts a ray along the Y-Axis looking at points of
 *   intersection on the X-Axis.  We are pointed toward in the "left" half
 *   of the view.  The X ray cast will proceed negatively along the X axis.
 *   Every plane in the input X plane list that lies to the "left" of the
 *   camera position and "clockwise" of the current yaw is moved into
 *   the output X plane list.
 *
 ****************************************************************************/

static void trv_ray_yawxprune_23cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current X plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next X plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t relx;                  /* Relative position of the X plane */
  trv_coord_t absy;                  /* Absolute Y position at relx given yaw */
  trv_coord_t lastrelx = -1;         /* Last relative X position processed */
  int32_t dydx;                      /* Rate of change of Y wrt X (double) */

  /* Pre-calculate the rate of change of Y with respect to X */
  /* The negative tangent is equal to the rate of change of Y with respect
   * to the X-axis.  The tangent is stored at double the "normal" scaling.
   */

  dydx = -TAN(yaw);

  /* Look at every rectangle lying in the X plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->tail; (entry); entry = next)
    {
      next = entry->blink;
      rect = &entry->d;

      /* Search for a rectangle which lies "before" the current camera
       * position
       */

      if (rect->plane < g_camera.x)
        {
          /* Get the X distance to the plane */

          relx = g_camera.x - rect->plane;

          /* Calculate the Y position at this relative X position.  We can
           * skip this step if we are processing another rectangle at the
           * same relx distance.
           */

          if (relx != lastrelx)
            {
              int32_t deltay;  /* Scale == "triple" */

              /* The dydx is stored at double the"normal" scaling -- so deltay
               * is "triple" precision
               */

              deltay   = dydx * ((int32_t) relx);
              absy     = tTOs(deltay) + g_camera.y; /* back to "single" */
              lastrelx = relx;
            }

          /* Check if any part of this plane lies clockwise from this Y
           * position.  In quadrants 2 & 3, clockwise corresponds to larger Y
           * values
           */

          if (absy <= rect->hend)
            {
              /* Remove the plane from the input plane list and add it to the
               * output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  rayYawYPrune23CCW
 *
 * Description:
 *   This function casts a ray along the Y-Axis looking at points of
 *   intersection on the X-Axis.  We are pointed toward in the "left" half
 *   of the view.  The X ray cast will proceed negatively along the X axis.
 *   Every plane in the input X plane list that lies to the "left" of the
 *   camera position and "counterclockwise" of the current yaw is moved
 *   into the output X plane list.
 *
 ****************************************************************************/

static void trv_ray_yawxprune_23ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current X plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next X plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t relx;                  /* Relative position of the X plane */
  trv_coord_t absy;                  /* Absolute Y position at relx given yaw */
  trv_coord_t lastrelx = -1;         /* Last relative X position processed */
  int32_t dydx;                      /* Rate of change of Y wrt X (double) */

  /* Pre-calculate the rate of change of Y with respect to X */
  /* The negative tangent is equal to the rate of change of Y with respect
   * to the X-axis.  The tangent is stored at double the "normal" scaling.
   */

  dydx = -TAN(yaw);

  /* Look at every rectangle lying in the X plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->tail; (entry); entry = next)
    {
      next = entry->blink;
      rect = &entry->d;

      /* Search for a rectangle which lies "before" the current camera
       * position
       */

      if (rect->plane < g_camera.x)
        {
          /* Get the X distance to the plane */

          relx = g_camera.x - rect->plane;

          /* Calculate the Y position at this relative X position.  We can
           * skip this step if we are processing another rectangle at the
           * same relx distance.
           */

          if (relx != lastrelx)
            {
              int32_t deltay;  /* Scale == "triple" */

              /* The dydx is stored at double the"normal" scaling -- so deltay
               * is "triple" precision
               */

              deltay = dydx * ((int32_t) relx);
              absy = tTOs(deltay) + g_camera.y; /* back to "single" */
              lastrelx = relx;
            }

          /* Check if any part of this plane lies counterclockwise from this Y
           * position.  In quadrants 2 & 3, counterclockwise corresponds to
           * smaller Y values
           */

          if (absy >= rect->hstart)
            {
              /* Remove the plane from the input plane list and add it to the
               * output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_yawyprune_12cw
 *
 * Description:
 *   This function casts a ray along the X-Axis looking at points of
 *   intersection on the Y-Axis.  We are pointed toward in the "forward" half
 *   of the view.  The Y ray cast will proceed positively along the Y axis.
 *   Every plane in the input plane list that lies to the "forward" of the
 *   camera position and "clockwise" of the current yaw is moved into
 *   the output Y plane list.
 *
 ****************************************************************************/

static void trv_ray_yawyprune_12cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current Y plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next Y plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t rely;                  /* Relative position of the Y plane */
  trv_coord_t absx;                  /* Absolute X position at rely given yaw */
  trv_coord_t lastrely = -1;         /* Last relative Y position processed */
  int32_t dxdy;                      /* Rate of change of X wrt Y (double) */

  /* Pre-calculate the rate of change of X with respect to Y */
  /* The inverted tangent is equal to the rate of change of X with respect
   * to the Y-axis.  The cotangent is stored at double the "normal"
   * scaling.
   */

  dxdy = g_cot_table(yaw);

  /* Look at every rectangle lying in the Y plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->head; (entry); entry = next)
    {
      next = entry->flink;
      rect = &entry->d;

      /* Search for a rectangle which lies "beyond" the current camera
       * position
       */

      if (rect->plane > g_camera.y)
        {
          /* Get the Y distance to the plane */

          rely = rect->plane - g_camera.y;

          /* Calculate the X position at this relative Y position.  We can
           * skip this step if we are processing another rectangle at the
           * same rely distance.
           */

          if (rely != lastrely)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdy is stored at double the"normal" scaling -- so deltax
               * is "triple" precision
               */

              deltax   = dxdy * ((int32_t) rely);
              absx     = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrely = rely;
            }

          /* Check if any part of this plane lies clockwise from this X
           * position.  In quadrants 1 & 2, clockwise corresponds to larger
           * X values
           */

          if (absx <= rect->hend)
            {

              /* Remove the plane from the input plane list and add it to the
               * output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_yawyprune_12ccw
 *
 * Description:
 *   This function casts a ray along the X-Axis looking at points of
 *   intersection on the Y-Axis.  We are pointed toward in the "forward" half
 *   of the view.  The Y ray cast will proceed positively along the Y axis.
 *   Every plane in the input plane list that lies to the "forward" of the
 *   camera position and "counterclockwise" of the current yaw is moved into
 *   the output Y plane list.
 *
 ****************************************************************************/

static void trv_ray_yawyprune_12ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current Y plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next Y plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t rely;                  /* Relative position of the Y plane */
  trv_coord_t absx;                  /* Absolute X position at rely given yaw */
  trv_coord_t lastrely = -1;         /* Last relative Y position processed */
  int32_t dxdy;                      /* Rate of change of X wrt Y (double) */

  /* Pre-calculate the rate of change of X with respect to Y */
  /* The inverted tangent is equal to the rate of change of X with respect
   * to the Y-axis.  The cotangent is stored at double the "normal"
   * scaling.
   */

  dxdy = g_cot_table(yaw);

  /* Look at every rectangle lying in the Y plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->head; (entry); entry = next)
    {
      next = entry->flink;
      rect = &entry->d;

      /* Search for a rectangle which lies "beyond" the current camera
       * position
       */

      if (rect->plane > g_camera.y)
        {
          /* Get the Y distance to the plane */

          rely = rect->plane - g_camera.y;

          /* Calculate the X position at this relative Y position.  We can
           * skip this step if we are processing another rectangle at the
           * same rely distance.
           */

          if (rely != lastrely)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdy is stored at double the"normal" scaling -- so
               * deltax is "triple" precision
               */

              deltax = dxdy * ((int32_t) rely);
              absx = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrely = rely;
            }

          /* Check if any part of this plane lies counterclockwise from this
           * X position.  In quadrants 1 & 2, counterclockwise corresponds
           * to smaller X values
           */

          if (absx >= rect->hstart)
            {

              /* Remove the plane from the input plane list and add it to the
               * output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_yawyprune_34cw
 *
 * Description:
 *   This function casts a ray along the X-Axis looking at points of
 *   intersection on the Y-Axis.  We are pointed toward in the "back" half
 *   of the view.  The Y ray cast will proceed negatively along the Y axis.
 *   Every plane in the input plane list that lies to the "back" of the
 *   camera position and "clockwise" of the current yaw is moved into
 *   the output Y plane list.
 *
 ****************************************************************************/

static void trv_ray_yawyprune_34cw(FAR struct trv_rect_head_s *outlist,
                                   FAR struct trv_rect_head_s *inlist,
                                   int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current Y plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next Y plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t rely;                  /* Relative position of the Y plane */
  trv_coord_t absx;                  /* Absolute X position at rely given yaw */
  trv_coord_t lastrely = -1;         /* Last relative Y position processed */
  int32_t dxdy;                      /* Rate of change of X wrt Y (double) */

  /* Pre-calculate the rate of change of X with respect to Y */
  /* The negative inverted tangent is equal to the rate of change of X with
   * respect to the Y-axis.  The cotangent is stored at double the
   * "normal" scaling.
   */

  dxdy = -g_cot_table(yaw - ANGLE_180);

  /* Look at every rectangle lying in the Y plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->tail; (entry); entry = next)
    {
      next = entry->blink;
      rect = &entry->d;

      /* Search for a rectangle which lies "before" the current camera
       * position
       */

      if (rect->plane < g_camera.y)
        {
          /* Get the Y distance to the plane */

          rely = g_camera.y - rect->plane;

          /* Calculate the X position at this relative Y position.  We can skip
           * this step if we are processing another rectangle at the same rely
           * distance.
           */

          if (rely != lastrely)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdy is stored at double the"normal" scaling -- so
               * deltax is "triple" precision
               */

              deltax   = dxdy * ((int32_t) rely);
              absx     = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrely = rely;
            }

          /* Check if any part of this plane lies clockwise from this X
           * position.  In quadrants 3 & 4, clockwise corresponds to
           * smaller X values
           */

          if (absx >= rect->hstart)
            {

              /* Remove the plane from the input plane list and add it to
               * the output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_yawyprune_34ccw
 *
 * Description:
 *   This function casts a ray along the X-Axis looking at points of
 *   intersection on the Y-Axis.  We are pointed toward in the "back" half
 *   of the view.  The Y ray cast will proceed negatively along the Y axis.
 *   Every plane in the input plane list that lies to the "back" of the
 *   camera position and "counterclockwise" of the current yaw is moved
 *   into the output Y plane list.
 *
 ****************************************************************************/

static void trv_ray_yawyprune_34ccw(FAR struct trv_rect_head_s *outlist,
                                    FAR struct trv_rect_head_s *inlist,
                                    int16_t yaw)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current Y plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next Y plane
                                      * rectangle */
  FAR struct trv_rect_data_s *rect;  /* Points to the rectangle data */
  trv_coord_t rely;                  /* Relative position of the Y plane */
  trv_coord_t absx;                  /* Absolute X position at rely given yaw */
  trv_coord_t lastrely = -1;         /* Last relative Y position processed */
  int32_t dxdy;                      /* Rate of change of X wrt Y (double) */

  /* Pre-calculate the rate of change of X with respect to Y */
  /* The negative inverted tangent is equal to the rate of change of X with
   * respect to the Y-axis.  The cotangent is stored at double the
   * "normal" scaling.
   */

  dxdy = -g_cot_table(yaw - ANGLE_180);

  /* Look at every rectangle lying in the Y plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (entry = inlist->tail; (entry); entry = next)
    {
      next = entry->blink;
      rect = &entry->d;

      /* Search for a rectangle which lies "before" the current camera
       * position
       */

      if (rect->plane < g_camera.y)
        {
          /* Get the Y distance to the plane */

          rely = g_camera.y - rect->plane;

          /* Calculate the X position at this relative Y position.  We can
           * skip this step if we are processing another rectangle at the
           * same rely distance.
           */

          if (rely != lastrely)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdy is stored at double the"normal" scaling -- so
               * deltax is "triple" precision
               */

              deltax   = dxdy * ((int32_t) rely);
              absx     = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrely = rely;
            }

          /* Check if any part of this plane lies counterclockwise from this X
           * position.  In quadrants 3 & 4, counterclockwise corresponds to
           * larger X values
           */

          if (absx <= rect->hend)
            {
              /* Remove the plane from the input plane list and add it to the
               * output plane list
               */

              trv_move_plane(entry, outlist, inlist);
            }
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_mkplanelist
 *
 * Description:
 *   This function creates a "fake" plane list based on the current working
 *   plane list.  The "head" of the "fake" plane list points to the first
 *   entry "beyond" the current camera position; the "tail" point to the
 *   first plane "before" the current position.
 *
 ****************************************************************************/

static void trv_ray_mkplanelist(FAR struct trv_rect_head_s *outlist,
                                FAR struct trv_rect_head_s *inlist,
                                int16_t pos)
{
  FAR struct trv_rect_list_s *list; /* Points to the current plane rectangle */

  /* Look at every rectangle lying in the specified "in" plane starting at
   * the beginning of the list.
   */

  outlist->head = NULL;
  for (list = inlist->head; list; list = list->flink)
    {
      /* Search for the first rectangle which lies "beyond" the current
       * camera position
       */

      if (list->d.plane > pos)
        {
          outlist->head = list;
          break;
        }
    }

  /* Look at every rectangle lying in the specified "in" plane starting at
   * the end of the list
   */

  outlist->tail = NULL;
  for (list = inlist->tail; list; list = list->blink)
    {
      /* Search for the first rectangle which lies "before" the current
       * camera position
       */

      if (list->d.plane < pos)
        {
          outlist->tail = list;
          break;
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_selectupper
 *
 * Description:
 *   Transfer each plane that lies (at least in part) above the level viewing
 *   angle from the inlist to the outlist).
 *
 ****************************************************************************/

static void trv_ray_selectupper(FAR struct trv_rect_head_s *outlist,
                                FAR struct trv_rect_head_s *inlist)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current Y plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next Y plane
                                      * rectangle */

  /* Look at every rectangle lying in the inlist */

  for (entry = inlist->head; (entry); entry = next)
    {
      next = entry->flink;

      /* If the "top" of the rectangle lies above the camera Z level, then
       * some (or all) of the rectangle is above the level viewing angle.
       */

      if (entry->d.vend >= g_camera.z)
        {
          /* Remove the plane from the input plane list and add it to the
           * output plane list
           */

          trv_move_plane(entry, outlist, inlist);
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_selectlower
 *
 * Description:
 *   Transfer each plane that lies (at least in part) above the level viewing
 *   angle from the inlist to the outlist).
 *
 ****************************************************************************/

static void trv_ray_selectlower(FAR struct trv_rect_head_s *outlist,
                                FAR struct trv_rect_head_s *inlist)
{
  FAR struct trv_rect_list_s *entry; /* Points to the current Y plane
                                      * rectangle */
  FAR struct trv_rect_list_s *next;  /* Points to the next Y plane
                                      * rectangle */

  /* Look at every rectangle lying in the inlist */

  for (entry = inlist->head; (entry); entry = next)
    {
      next = entry->flink;

      /* If the "bottom" of the rectangle lies below the camera Z level, then
       * some (or all) of the rectangle is above the level viewing angle.
       */

      if (entry->d.vstart < g_camera.z)
        {
          /* Remove the plane from the input plane list and add it to the
           * output plane list
           */

          trv_move_plane(entry, outlist, inlist);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  trv_ray_yawprune
 *
 * Description:
 *   This function may be called at the beginning of each rending cycle to
 *   prepare ray casting data.  This function "prunes" out all planes that
 *   may be visible in the specified yaw range.
 *
 ****************************************************************************/

void trv_ray_yawprune(int16_t yawstart, int16_t yawend)
{
  uint16_t quadrants;

  /* Nullify the working plane lists */

  g_working_xplane.head = g_working_xplane.tail = NULL;
  g_working_xplane.head = g_working_xplane.tail = NULL;
  g_working_planestate = WEMPTY;

  /* Nullify the candidate plane lists */

  g_candidate_xplane.head = g_candidate_xplane.tail = NULL;
  g_candidate_yplane.head = g_candidate_yplane.tail = NULL;
  g_candidate_zplane.head = g_candidate_zplane.tail = NULL;

  /* Nullify the discard plane lists */

  g_discard_xplane.head = g_discard_xplane.tail = NULL;
  g_discard_yplane.head = g_discard_yplane.tail = NULL;
  g_discard_zplane.head = g_discard_zplane.tail = NULL;

  /* Determine the quadrant(s) spanned by yawstart and yawend */

  if (yawstart == ANGLE_0)
    {
      quadrants = START0;
    }
  else if (yawstart < ANGLE_90)
    {
      quadrants = STARTQ1;
    }
  else if (yawstart == ANGLE_90)
    {
      quadrants = START90;
    }
  else if (yawstart < ANGLE_180)
    {
      quadrants = STARTQ2;
    }
  else if (yawstart == ANGLE_180)
    {
      quadrants = START180;
    }
  else if (yawstart < ANGLE_270)
    {
      quadrants = STARTQ3;
    }
  else if (yawstart == ANGLE_270)
    {
      quadrants = START270;
    }
  else
    {
      quadrants = STARTQ4;
    }

  if (yawend == ANGLE_0)
    {
      quadrants |= END0;
    }
  else if (yawend < ANGLE_90)
    {
      quadrants |= ENDQ1;
    }
  else if (yawend == ANGLE_90)
    {
      quadrants |= END90;
    }
  else if (yawend < ANGLE_180)
    {
      quadrants |= ENDQ2;
    }
  else if (yawend == ANGLE_180)
    {
      quadrants |= END180;
    }
  else if (yawend < ANGLE_270)
    {
      quadrants |= ENDQ3;
    }
  else if (yawend == ANGLE_270)
    {
      quadrants |= END270;
    }
  else
    {
      quadrants |= ENDQ4;
    }

  /* Now, perform the pruning based upon the quadrant(s) spanned by yawstart
   * and yawend
   */

  switch (quadrants)
    {
    case S0EQ1:
      trv_ray_yawxprune_14ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_14cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_12cw(&g_candidate_yplane, &g_yplane, yawend);
      break;

    case SQ1EQ1:
      trv_ray_yawxprune_14ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_14cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_12ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_12cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ1E90:
      trv_ray_yawxprune_14ccw(&g_candidate_xplane, &g_xplane, yawstart);
      trv_ray_yawyprune_12ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_12cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ1EQ2:
      trv_ray_yawxprune_14ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_23cw(&g_candidate_xplane, &g_xplane, yawend);
      trv_merge_planelists(&g_candidate_xplane, &g_discard_xplane);
      trv_ray_yawyprune_12ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_12cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case S90EQ2:
      trv_ray_yawxprune_23cw(&g_candidate_xplane, &g_xplane, yawend);
      trv_ray_yawyprune_12ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_12cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ2EQ2:
      trv_ray_yawxprune_23ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_23cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_12ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_12cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ2E180:
      trv_ray_yawxprune_23ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_23cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_12ccw(&g_candidate_yplane, &g_yplane, yawstart);
      break;

    case SQ2EQ3:
      trv_ray_yawxprune_23ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_23cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_12ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_34cw(&g_candidate_yplane, &g_yplane, yawend);
      trv_merge_planelists(&g_candidate_yplane, &g_discard_yplane);
      break;

    case S180EQ3:
      trv_ray_yawxprune_23ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_23cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_34cw(&g_candidate_yplane, &g_yplane, yawend);
      break;

    case SQ3EQ3:
      trv_ray_yawxprune_23ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_23cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_34ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_34cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ3E270:
      trv_ray_yawxprune_23ccw(&g_candidate_xplane, &g_xplane, yawend);
      trv_ray_yawyprune_34ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_34cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ3EQ4:
      trv_ray_yawxprune_23ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_14cw(&g_candidate_xplane, &g_xplane, yawend);
      trv_merge_planelists(&g_candidate_xplane, &g_discard_xplane);
      trv_ray_yawyprune_34ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_34cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case S270EQ4:
      trv_ray_yawxprune_14cw(&g_candidate_xplane, &g_xplane, yawend);
      trv_ray_yawyprune_34ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_34cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ4EQ4:
      trv_ray_yawxprune_14ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_14cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_34ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_34cw(&g_candidate_yplane, &g_discard_yplane, yawend);
      break;

    case SQ4E0:
      trv_ray_yawxprune_14ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_14cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_34ccw(&g_candidate_yplane, &g_yplane, yawstart);
      break;

    case SQ4EQ1:
      trv_ray_yawxprune_14ccw(&g_discard_xplane, &g_xplane, yawstart);
      trv_ray_yawxprune_14cw(&g_candidate_xplane, &g_discard_xplane, yawend);
      trv_ray_yawyprune_34ccw(&g_discard_yplane, &g_yplane, yawstart);
      trv_ray_yawyprune_12cw(&g_candidate_yplane, &g_yplane, yawend);
      trv_merge_planelists(&g_candidate_yplane, &g_discard_yplane);
      break;

    default: /* No other combination should occur if the range is < 90 deg */
      break;
    }

  /* Create the special plane lists for the raycaster */

  trv_ray_mkplanelist(&g_ray_xplane, &g_candidate_xplane, g_camera.x);
  trv_ray_mkplanelist(&g_ray_yplane, &g_candidate_yplane, g_camera.y);
  trv_ray_mkplanelist(&g_ray_zplane, &g_zplane, g_camera.z);
}

/****************************************************************************
 * Name:  trv_ray_pitchprune
 *
 * Description:
 *   This function may be called at each "horizontal" pitch swathe to
 *   prepare ray casting data.  This function "prunes" out all planes that
 *   may be visible in the specified pitch range.
 *
 ****************************************************************************/

void trv_ray_pitchprune(int16_t pitchstart, int16_t pitchend)
{
  /* Check the current pitch range */

  if (pitchend <= ANGLE_180)
    {
      /* The entire view is in the upper hemisphere */

      if (g_working_planestate != WTOP)
        {
          if (g_working_planestate == WBOTTOM)
            {
              /* Merge the workingPlane lists with the candidate plane lists */

              trv_merge_planelists(&g_candidate_xplane, &g_working_xplane);
              trv_merge_planelists(&g_candidate_yplane, &g_working_yplane);
            }

          /* Select the rectangles which lie above the level line of sight */

          trv_ray_selectupper(&g_working_xplane, &g_candidate_xplane);
          trv_ray_selectupper(&g_working_yplane, &g_candidate_yplane);

          /* Create the special plane lists for the raycaster */

          trv_ray_mkplanelist(&g_ray_xplane, &g_working_xplane, g_camera.x);
          trv_ray_mkplanelist(&g_ray_yplane, &g_working_yplane, g_camera.y);

          g_working_planestate = WTOP;
        }
    }
  else if (pitchstart <= ANGLE_180)
    {
      /* The view is split between the upper and lower hemispheres */

      if (g_working_planestate != WEMPTY)
        {
          /* Merge the workingPlane lists with the candidate plane lists */

          trv_merge_planelists(&g_candidate_xplane, &g_working_xplane);
          trv_merge_planelists(&g_candidate_yplane, &g_working_yplane);

          /* Create the special plane lists for the raycaster */

          trv_ray_mkplanelist(&g_ray_xplane, &g_candidate_xplane, g_camera.x);
          trv_ray_mkplanelist(&g_ray_yplane, &g_candidate_yplane, g_camera.y);

          g_working_planestate = WEMPTY;
        }
    }
  else
    {
      /* The entire view is in the lower hemisphere */

      if (g_working_planestate != WBOTTOM)
        {
          if (g_working_planestate == WTOP)
            {
              /* Merge the workingPlane lists with the candidate plane lists */

              trv_merge_planelists(&g_candidate_xplane, &g_working_xplane);
              trv_merge_planelists(&g_candidate_yplane, &g_working_yplane);
            }

          /* Select the rectangles which lie below the level line of sight */

          trv_ray_selectlower(&g_working_xplane, &g_candidate_xplane);
          trv_ray_selectlower(&g_working_yplane, &g_candidate_yplane);

          /* Create the special plane lists for the raycaster */

          trv_ray_mkplanelist(&g_ray_xplane, &g_working_xplane, g_camera.x);
          trv_ray_mkplanelist(&g_ray_yplane, &g_working_yplane, g_camera.y);

          g_working_planestate = WBOTTOM;
        }
    }
}

/****************************************************************************
 * Name:  trv_ray_pitchunprune
 *
 * Description:
 *   This function may be called at the end of each rending cycle to
 *   restore the environment which was perturbed by trv_ray_pitchprune.
 *
 ****************************************************************************/

void trv_ray_pitchunprune(void)
{
}

/****************************************************************************
 * Name:  trv_ray_yawunprune
 *
 * Description:
 *   This function may be called at the end of each rending cycle to
 *   restore the environment which was perturbed by trv_ray_yawprune.
 *
 ****************************************************************************/

void trv_ray_yawunprune(void)
{
  /* Merge the working X plane list with the discarded X plane list */

  trv_merge_planelists(&g_discard_xplane, &g_working_xplane);

  /* Merge the candidate X plane list with the discarded X plane list */

  trv_merge_planelists(&g_discard_xplane, &g_candidate_xplane);

  /* Merge the discarded X plane list with the original "raw" X plane list */

  trv_merge_planelists(&g_xplane, &g_discard_xplane);

  /* Merge the working Y plane list with the discarded Y plane list */

  trv_merge_planelists(&g_discard_yplane, &g_working_yplane);

  /* Merge the candidate X plane list with the discarded X plane list */

  trv_merge_planelists(&g_discard_yplane, &g_candidate_yplane);

  /* Merge the discarded Y plane list with the original "raw" Y plane list */

  trv_merge_planelists(&g_yplane, &g_discard_yplane);
}
