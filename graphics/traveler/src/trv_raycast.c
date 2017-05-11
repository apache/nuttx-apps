/****************************************************************************
 * apps/graphics/traveler/src/trv_raycast.c
 * This file contains the low-level ray casting logic
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
#include "trv_doors.h"
#include "trv_plane.h"
#include "trv_bitmaps.h"
#include "trv_trigtbl.h"
#include "trv_rayrend.h"
#include "trv_rayprune.h"
#include "trv_raycast.h"

/****************************************************************************
 * Compilation switches
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The following switch enables view correction logic. */

#define ENABLE_VIEW_CORRECTION 1

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void trv_ray_xcaster14(FAR struct trv_raycast_s *result);
static void trv_ray_xcaster23(FAR struct trv_raycast_s *result);
static void trv_ray_ycaster12(FAR struct trv_raycast_s *result);
static void trv_ray_ycaster34(FAR struct trv_raycast_s *result);
static void trv_ray_zcasteru(FAR struct trv_raycast_s *result);
static void trv_ray_zcasterl(FAR struct trv_raycast_s *result);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The following are the tangent and the cotangent of the pitch angle
 * adjusted for the viewing yaw angle so that the view is correct for the
 * "fish eye" effect which results from the projection of the polar ray cast
 * onto the flat display
 */

static int32_t g_adj_tanpitch;
static int32_t g_adj_cotpitch;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_ray_xcaster14
 *
 * Description:
 *   This function casts a ray along the Y-Axis looking at points of
 *   intersection on the X-Axis.  If a block is defined at this intersection
 *   then a "hit" is found and the distance to this hit is determined.
 *
 *   If we are in the "right" half of the view, then the X ray cast will
 *   proceed in a positive along the X axis and that all possible X-axis
 *   intersections will occur to at the "left" of a cell.
 *
 *   NOTE:  The X-Ray caster must run first because it initializes a
 *   data structure needed by both the Y and Z ray casters.
 *
 ****************************************************************************/

static void trv_ray_xcaster14(FAR struct trv_raycast_s *result)
{
  struct trv_rect_list_s *list; /* Points to the current X plane rectangle */
  struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t relx;             /* Relative position of the X plane */
  trv_coord_t absy;             /* Absolute Y position at relx given yaw */
  trv_coord_t absz;             /* Absolute Z position at relx given pitch */
  trv_coord_t lastrelx1 = -1;   /* Last relative X position processed */
  trv_coord_t lastrelx2 = -1;   /* Last relative X position processed */
  int32_t dydx;                 /* Rate of change of Y wrt X (double) */
  int32_t dzdx;                 /* Rate of change of Z wrt X (double) */

  /* At a viewing angle of 270 degrees, no intersections with the g_ray_xplanes
   * are possible!
   */

  if (g_camera.yaw == ANGLE_270)
    {
      return;
    }

  /* Pre-calculate the rate of change of Y and Z with respect to X */
  /* The tangent is equal to the rate of change of Y with respect to the
   * X-axis.  The tangent is stored at double the "normal" scaling.
   */

  dydx = TAN(g_camera.yaw);

  /* Determine the rate of change of the Z with respect to X. The tangent is
   * "double" precision; the secant is "double" precision.  dzdx will be
   * retained as "double" precision.
   */

  dzdx = qTOd(g_adj_tanpitch * ABS(g_sec_table[g_camera.yaw]));

  /* Look at every rectangle lying in the X plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (list = g_ray_xplane.head; list; list = list->flink)
    {
      rect = &list->d;

      /* Search for a rectangle which lies "beyond" the current camera
       * position
       */

      if (rect->plane > g_camera.x)
        {
          /* get the X distance to the plane */

          relx = rect->plane - g_camera.x;

#if 0
          /* g_ray_xplane is an ordered list, if we have already hit something
           * closer, then we can abort the casting now.
           */

          if (relx > result->xdist)
            {
              return;
            }
#endif

          /* Calculate the Y position at this relative X position.  We can skip
           * this step if we are processing another rectangle at the same relx
           * distance.
           */

          if (relx != lastrelx1)
            {
              int32_t deltay;  /* Scale == "triple" */

              /* The dydx is stored at double the"normal" scaling -- so
               * deltay is "triple" precision
               */

              deltay    = dydx * ((int32_t) relx);
              absy      = tTOs(deltay) + g_camera.y; /* back to "single" */
              lastrelx1 = relx;
            }

          /* Check if this Y position intersects the rectangle */

          if (absy >= rect->hstart && absy <= rect->hend)
            {
              /* The Y position lies in the rectangle.  Now, calculate the
               * theZ position at this relative X position.  We can skip
               * this step if we are processing another rectangle at the
               * same relx distance.
               */

              if (relx != lastrelx2)
                {
                  int32_t deltaz;      /* Scale == TRIPLE */

                  /* The dzdx is stored at double the"normal" scaling -- so
                   * deltaz is "triple" precision
                   */

                  deltaz    = dzdx * ((int32_t) relx);
                  absz      = tTOs(deltaz) + g_camera.z; /* Back to single */
                  lastrelx2 = relx;
                }

              /* Check if this Z position intersects the rectangle */

              if (absz >= rect->vstart && absz <= rect->vend)
                {
                  /* We've got a potential hit, let's see what it is */
                  /* Check if we just hit an ordinary opaque wall */

                  if (IS_NORMAL(rect))
                    {
                      /* Yes..Save the parameters associated with the normal
                       * wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(FRONT_HIT, X_HIT);
                      result->xpos = absy;
                      result->ypos = absz;

                      result->xdist = relx;
                      result->ydist = ABS(absy - g_camera.y);
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate X casting */

                      return;
                    }
                  else if (IS_DOOR(rect))
                    {
                      /* Check if the door is in motion. */

                      if (!IS_MOVING_DOOR(rect))
                        {
                          /* Save the parameters associated with the normal
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(FRONT_HIT, X_HIT);
                          result->xpos = absy;
                          result->ypos = absz;

                          result->xdist = relx;
                          result->ydist = ABS(absy - g_camera.y);
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate X casting */

                          return;
                        }

                      /* The door is in motion, the Z-position to see if we can
                       * see under the door
                       */

                      else if (absz > g_opendoor.zbottom)
                        {
                          /* Save the parameters associated with the moving
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(FRONT_HIT, X_HIT);
                          result->xpos = absy;
                          result->ypos = absz - g_opendoor.zdist;

                          result->xdist = relx;
                          result->ydist = ABS(absy - g_camera.y);
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate X casting */

                          return;
                        }
                    }

                  /* Otherwise, it must be a transparent wall.  We'll need to
                   * make our decision based upon the pixel that we hit
                   */

                  /* Check if the pixel at this location is visible */

                  else if (GET_FRONT_PIXEL(rect, absy, absz) != INVISIBLE_PIXEL)
                    {
                      /* Its visible, save the parameters associated with the
                       * transparent wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(FRONT_HIT, X_HIT);
                      result->xpos = absy;
                      result->ypos = absz;

                      result->xdist = relx;
                      result->ydist = ABS(absy - g_camera.y);
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate X casting */

                      return;
                    }
                }
            }
        }
    }
}

/****************************************************************************
 * Name: trv_ray_xcaster23
 *
 * Description:
 *   This function casts a ray along the Y-Axis looking at points of
 *   intersection on the X-Axis.  If a block is defined at this intersection
 *   then a "hit" is found and the distance to this hit is determined.
 *
 *   If we are in the "left" half of the view, then the X ray cast will
 *   proceed in a negative along the X axis and that all possible X-axis
 *   intersections will occur on the "right" of a cell.
 *
 *   NOTE:  The X-Ray caster must run first because it initializes a
 *   data structure needed by both the Y and Z ray casters.
 *
 ****************************************************************************/

static void trv_ray_xcaster23(FAR struct trv_raycast_s *result)
{
  struct trv_rect_list_s *list; /* Points to the current X plane rectangle */
  struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t relx;             /* Relative position of the X plane */
  trv_coord_t absy;             /* Absolute Y position at relx given yaw */
  trv_coord_t absz;             /* Absolute Z position at relx given pitch */
  trv_coord_t lastrelx1 = -1;   /* Last relative X position processed */
  trv_coord_t lastrelx2 = -1;   /* Last relative X position processed */
  int32_t dydx;                 /* Rate of change of Y wrt X (double) */
  int32_t dzdx;                 /* Rate of change of Z wrt X (double) */

  /* At a view angle of 90 degrees, no intersections with the g_ray_xplanes are
   * possible!
   */

  if (g_camera.yaw == ANGLE_90)
    {
      return;
    }

  /* Pre-calculate the rate of change of Y and Z with respect to X */
  /* The negative tangent is equal to the rate of change of Y with respect
   * to the X-axis.  The tangent is stored at double the "normal" scaling.
   */

  dydx = -TAN(g_camera.yaw);

  /* Determine the rate of change of the Z with respect to X. dydx is
   * "double" precision; the secant is "double" precision.  dzdx will be
   * retained as "double" precision.
   */

  dzdx = qTOd(g_adj_tanpitch * ABS(g_sec_table[g_camera.yaw]));

  /* Look at every rectangle lying in the X plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (list = g_ray_xplane.tail; list; list = list->blink)
    {
      rect = &list->d;

      /* Search for a rectangle which lies "before" the current camera
       * position
       */

      if (rect->plane < g_camera.x)
        {
          /* get the X distance to the plane */

          relx = g_camera.x - rect->plane;
#if 0
          /* g_ray_xplane is an ordered list, if we have already hit something
           * closer, then we can abort the casting now.
           */

          if (relx > result->xdist)
            {
              return;
            }
#endif

          /* Calculate the Y position at this relative X position.  We can skip
           * this step if we are processing another rectangle at the same relx
           * distance.
           */

          if (relx != lastrelx1)
            {
              int32_t deltay;  /* Scale == "triple" */

              /* The dydx is stored at double the"normal" scaling -- so deltay
               * is "triple" precision
               */

              deltay    = dydx * ((int32_t) relx);
              absy      = tTOs(deltay) + g_camera.y; /* back to "single" */
              lastrelx1 = relx;
            }

          /* Check if this Y position intersects the rectangle */

          if (absy >= rect->hstart && absy <= rect->hend)
            {
              /* The Y position lies in the rectangle.  Now, calculate the
               * Z position at this relative X position.  We can skip this
               * step if we are processing another rectangle at the same
               * relx distance.
               */

              if (relx != lastrelx2)
                {
                  int32_t deltaz;      /* Scale == TRIPLE */

                  /* The dzdx is stored at double the"normal" scaling -- so
                   * deltaz is "triple" precision
                   */

                  deltaz    = dzdx * ((int32_t) relx);
                  absz      = tTOs(deltaz) + g_camera.z; /* Back to single */
                  lastrelx2 = relx;
                }

              /* Check if this Z position intersects the rectangle */

              if (absz >= rect->vstart && absz <= rect->vend)
                {
                  /* We've got a potential hit, let's see what it is */
                  /* Check if we just hit an ordinary opaque wall */

                  if (IS_NORMAL(rect))
                    {
                      /* Yes..Save the parameters associated with the normal
                       * wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(BACK_HIT, X_HIT);
                      result->xpos = absy;
                      result->ypos = absz;

                      result->xdist = relx;
                      result->ydist = ABS(absy - g_camera.y);
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate X casting */

                      return;
                    }
                  else if (IS_DOOR(rect))
                    {
                      /* Check if the door is in motion. */

                      if (!IS_MOVING_DOOR(rect))
                        {
                          /* Save the parameters associated with the normal
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(BACK_HIT, X_HIT);
                          result->xpos = absy;
                          result->ypos = absz;

                          result->xdist = relx;
                          result->ydist = ABS(absy - g_camera.y);
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate X casting */

                          return;
                        }

                      /* The door is in motion, the Z-position to see if we can
                       * see under the door
                       */

                      else if (absz > g_opendoor.zbottom)
                        {
                          /* Save the parameters associated with the moving
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(BACK_HIT, X_HIT);
                          result->xpos = absy;
                          result->ypos = absz - g_opendoor.zdist;

                          result->xdist = relx;
                          result->ydist = ABS(absy - g_camera.y);
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate X casting */

                          return;
                        }
                    }

                  /* Otherwise, it must be a transparent wall.  We'll need to
                   * make our decision based upon the pixel that we hit
                   */

                  /* Check if the pixel at this location is visible */

                  else if (GET_BACK_PIXEL(rect, absy, absz) != INVISIBLE_PIXEL)
                    {
                      /* Its visible, save the parameters associated with the
                       * transparent wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(BACK_HIT, X_HIT);
                      result->xpos = absy;
                      result->ypos = absz;

                      result->xdist = relx;
                      result->ydist = ABS(absy - g_camera.y);
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate X casting */

                      return;
                    }
                }
            }
        }
    }
}

/****************************************************************************
 * Name: trv_ray_ycaster12
 *
 * Description:
 *   This function casts a ray along the X-Axis looking at points of
 *   intersection on the Y-Axis.  If a block is defined at this intersection
 *   then a "hit" is found and the distance to this hit is determined.
 *
 *   If we are in the "forward" half of the view, then the Y ray cast will
 *   proceed in a positive along the Y axis and that all possible Y-axis
 *   intersections will occur on the "front" of a cell.
 *
 *   NOTE:  The X-Ray is assumed to have been performed first!
 *
 ****************************************************************************/

static void trv_ray_ycaster12(FAR struct trv_raycast_s *result)
{
  struct trv_rect_list_s *list; /* Points to the current P plane rectangle */
  struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t rely;             /* Relative position of the Y plane */
  trv_coord_t absx;             /* Absolute X position at rely given yaw */
  trv_coord_t absz;             /* Absolute Z position at rely given pitch */
  trv_coord_t lastrely1 = -1;   /* Last relative Y position processed */
  trv_coord_t lastrely2 = -1;   /* Last relative Y position processed */
  int32_t dxdy;                 /* Rate of change of X wrt Y (double) */
  int32_t dzdy;                 /* Rate of change of Z wrt Y (double) */

  /* At a viewing angle of 0 degrees, no intersections with the g_ray_yplane is
   * possible!
   */

  if (g_camera.yaw == ANGLE_0)
    {
      return;
    }

  /* Pre-calculate the rate of change of X and Z with respect to Y */
  /* The inverted tangent is equal to the rate of change of X with respect to
   * the Y-axis.  The cotangent is stored at double the "normal" scaling.
   */

  dxdy = g_cot_table(g_camera.yaw);

  /* Determine the rate of change of the Z with respect to Y.  The tangent
   * is "double" precision; the cosecant is "double" precision.  dzdy will
   * be retained as "double" precision.
   */

  dzdy = qTOd(g_adj_tanpitch * ABS(g_csc_table[g_camera.yaw]));

  /* Look at every rectangle lying in a Y plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (list = g_ray_yplane.head; list; list = list->flink)
    {
      rect = &list->d;

      /* Search for a rectangle which lies "beyond" the current camera
       * position
       */

      if (rect->plane > g_camera.y)
        {
          /* get the Y distance to the plane */

          rely = rect->plane - g_camera.y;

          /* g_ray_yplane is an ordered list, if we have already hit something
           * closer, then we can abort the casting now.
           */

          if (rely > result->ydist)
            {
              return;
            }

          /* Calculate the Y position at this relative X position.  We can skip
           * this step if we are processing another rectangle at the same relx
           * distance.
           */

          if (rely != lastrely1)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdy is stored at double the"normal" scaling -- so deltax
               * is "triple" precision
               */

              deltax    = dxdy * ((int32_t) rely);
              absx      = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrely1 = rely;
            }

          /* Check if this X position intersects the rectangle */

          if (absx >= rect->hstart && absx <= rect->hend)
            {
              /* The X position lies in the rectangle.  Now, calculate the
               * Z position at this relative X position.  We can skip this step
               * if we are processing another rectangle at the same relx
               * distance.
               */

              if (rely != lastrely2)
                {
                  int32_t deltaz;      /* Scale == TRIPLE */

                  /* The dzdy is stored at double the"normal" scaling -- so
                   * deltaz is "triple" precision
                   */

                  deltaz    = dzdy * ((int32_t) rely);
                  absz      = tTOs(deltaz) + g_camera.z; /* Back to single */
                  lastrely2 = rely;
                }

              /* Check if this Z position intersects the rectangle */

              if (absz >= rect->vstart && absz <= rect->vend)
                {
                  /* We've got a potential hit, let's see what it is */
                  /* Check if we just hit an ordinary opaque wall */

                  if (IS_NORMAL(rect))
                    {
                      /* Yes..Save the parameters associated with the normal
                       * wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(FRONT_HIT, Y_HIT);
                      result->xpos = absx;
                      result->ypos = absz;

                      result->xdist = ABS(absx - g_camera.x);
                      result->ydist = rely;
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate Y casting */

                      return;
                    }
                  else if (IS_DOOR(rect))
                    {
                      /* Check if the door is in motion. */

                      if (!IS_MOVING_DOOR(rect))
                        {
                          /* Save the parameters associated with the normal
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(FRONT_HIT, Y_HIT);
                          result->xpos = absx;
                          result->ypos = absz;

                          result->xdist = ABS(absx - g_camera.x);
                          result->ydist = rely;
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate Y casting */

                          return;
                        }

                      /* The door is in motion, the Z-position to see if we can
                       * see under the door
                       */

                      else if (absz > g_opendoor.zbottom)
                        {
                          /* Save the parameters associated with the moving
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(FRONT_HIT, Y_HIT);
                          result->xpos = absx;
                          result->ypos = absz - g_opendoor.zdist;

                          result->xdist = ABS(absx - g_camera.x);
                          result->ydist = rely;
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate Y casting */

                          return;
                        }
                    }

                  /* Otherwise, it must be a transparent wall.  We'll need to
                   * make our decision based upon the pixel that we hit
                   */

                  /* Check if the pixel at this location is visible */

                  else if (GET_FRONT_PIXEL(rect, absx, absz) != INVISIBLE_PIXEL)
                    {
                      /* Its visible, save the parameters associated with the
                       * transparent wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(FRONT_HIT, Y_HIT);
                      result->xpos = absx;
                      result->ypos = absz;

                      result->xdist = ABS(absx - g_camera.x);
                      result->ydist = rely;
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate Y casting */

                      return;
                    }
                }
            }
        }
    }
}

/****************************************************************************
 * Name: trv_ray_ycaster34
 *
 * Description:
 *   This function casts a ray along the X-Axis looking at points of
 *   intersection on the Y-Axis.  If a block is defined at this intersection
 *   then a "hit" is found and the distance to this hit is determined.
 *
 *   If we are in the "back" half of the view, then the Y ray cast will
 *   proceed in a negative along the Y axis and that all possible Y-axis
 *   intersections will occur on the "back" of a cell.
 *
 *   NOTE:  The X-Ray is assumed to have been performed first!
 *
 ****************************************************************************/

static void trv_ray_ycaster34(FAR struct trv_raycast_s *result)
{
  struct trv_rect_list_s *list; /* Points to the current P plane rectangle */
  struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t rely;             /* Relative position of the Y plane */
  trv_coord_t absx;             /* Absolute X position at rely given yaw */
  trv_coord_t absz;             /* Absolute Z position at rely given pitch */
  trv_coord_t lastrely1 = -1;   /* Last relative Y position processed */
  trv_coord_t lastrely2 = -1;   /* Last relative Y position processed */
  int32_t dxdy;                 /* Rate of change of X wrt Y (double) */
  int32_t dzdy;                 /* Rate of change of Z wrt Y (double) */

  /* At a viewing angle of 180 degrees, no intersections with the line x = bXi
   * are possible!
   */

  if (g_camera.yaw == ANGLE_180)
    {
      return;
    }

  /* Pre-calculate the rate of change of X and Z with respect to Y */
  /* The negative inverted tangent is equal to the rate of change of X with
   * respect to the Y-axis.  The cotangent is stored at double the
   * "normal" scaling.
   */

  dxdy = -g_cot_table(g_camera.yaw - ANGLE_180);

  /* Determine the rate of change of the Z with respect to Y.  The tangent
   * is "double" precision; the cosecant is "double" precision.  dzdy will
   * be retained as "double" precision.
   */

  dzdy = qTOd(g_adj_tanpitch * ABS(g_csc_table[g_camera.yaw]));

  /* Look at every rectangle lying in a Y plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (list = g_ray_yplane.tail; list; list = list->blink)
    {
      rect = &list->d;

      /* Search for a rectangle which lies "before" the current camera
       * position
       */

      if (rect->plane < g_camera.y)
        {
          /* get the Y distance to the plane */

          rely = g_camera.y - rect->plane;

          /* g_ray_yplane is an ordered list, if we have already hit something
           * closer, then we can abort the casting now.
           */

          if (rely > result->ydist)
            {
              return;
            }

          /* Calculate the Y position at this relative X position.  We can skip
           * this step if we are processing another rectangle at the same relx
           * distance.
           */

           if (rely != lastrely1)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdy is stored at double the"normal" scaling -- so deltax
               * is "triple" precision
               */

              deltax    = dxdy * ((int32_t) rely);
              absx      = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrely1 = rely;
            }

          /* Check if this X position intersects the rectangle */

          if (absx >= rect->hstart && absx <= rect->hend)
            {

              /* The X position lies in the rectangle.  Now, calculate the
               * Z position at this relative X position.  We can skip this
               * step if we are processing another rectangle at the same
               * relx distance.
               */

              if (rely != lastrely2)
                {
                  int32_t deltaz;      /* Scale == TRIPLE */

                  /* The dzdy is stored at double the"normal" scaling -- so
                   * deltaz is "triple" precision
                   */

                  deltaz    = dzdy * ((int32_t) rely);
                  absz      = tTOs(deltaz) + g_camera.z; /* Back to single */
                  lastrely2 = rely;
                }

              /* Check if this Z position intersects the rectangle */

              if (absz >= rect->vstart && absz <= rect->vend)
                {

                  /* We've got a potential hit, let's see what it is */
                  /* Check if we just hit an ordinary opaque wall */

                  if (IS_NORMAL(rect))
                    {
                      /* Yes..Save the parameters associated with the normal
                       * wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(BACK_HIT, Y_HIT);
                      result->xpos = absx;
                      result->ypos = absz;

                      result->xdist = ABS(absx - g_camera.x);
                      result->ydist = rely;
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate Y casting */

                      return;
                    }
                  else if (IS_DOOR(rect))
                    {
                      /* Check if the door is in motion. */

                      if (!IS_MOVING_DOOR(rect))
                        {
                          /* Save the parameters associated with the normal
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(BACK_HIT, Y_HIT);
                          result->xpos = absx;
                          result->ypos = absz;

                          result->xdist = ABS(absx - g_camera.x);
                          result->ydist = rely;
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate Y casting */

                          return;
                        }

                      /* The door is in motion, the Z-position to see if we can
                       * see under the door
                       */

                      else if (absz > g_opendoor.zbottom)
                        {
                          /* Save the parameters associated with the moving
                           * door hit
                           */

                          result->rect = rect;
                          result->type = MK_HIT_TYPE(BACK_HIT, Y_HIT);
                          result->xpos = absx;
                          result->ypos = absz - g_opendoor.zdist;

                          result->xdist = ABS(absx - g_camera.x);
                          result->ydist = rely;
                          result->zdist = ABS(absz - g_camera.z);

                          /* Terminate Y casting */

                          return;
                        }
                    }

                  /* Otherwise, it must be a transparent wall.  We'll need to
                   * make our decision based upon the pixel that we hit
                   */

                  /* Check if the pixel at this location is visible */

                  else if (GET_BACK_PIXEL(rect, absx, absz) != INVISIBLE_PIXEL)
                    {
                      /* Its visible, save the parameters associated with the
                       * transparent wall hit
                       */

                      result->rect = rect;
                      result->type = MK_HIT_TYPE(BACK_HIT, Y_HIT);
                      result->xpos = absx;
                      result->ypos = absz;

                      result->xdist = ABS(absx - g_camera.x);
                      result->ydist = rely;
                      result->zdist = ABS(absz - g_camera.z);

                      /* Terminate Y casting */

                      return;
                    }
                }
            }
        }
    }
}

/****************************************************************************
 * Name: trv_ray_zcasteru
 *
 * Description:
 *   This function casts a ray along the rotated Y-Axis looking at points of
 *   intersection on the Z-Axis.  If a block is defined at this intersection
 *   then a "hit" is found and the distance to this hit is determined.
 *
 *   If we are in the "upper" half of the view, then the Z ray cast will
 *   proceed along the positive Z axis and that all possible Z-axis
 *   intersections will occur on the "bottom" of a cell.
 *
 *   NOTE: It is assumed that both the X and Y ray casters have already
 *   ran!
 ****************************************************************************/

static void trv_ray_zcasteru(FAR struct trv_raycast_s *result)
{
  struct trv_rect_list_s *list; /* Points to the current Z plane rectangle */
  struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t relz;             /* Relative position of the Z plane */
  trv_coord_t absx;             /* Absolute X position at relz given yaw */
  trv_coord_t absy;             /* Absolute Y position at relz given yaw */
  trv_coord_t lastrelz1 = -1;   /* Last relative Z position processed */
  trv_coord_t lastrelz2 = -1;   /* Last relative Z position processed */
  int32_t dxdz;                 /* Rate of change of X wrt Z (double) */
  int32_t dydz;                 /* Rate of change of Y wrt Z (double) */

  /* At a viewing angle of 0 degrees, no intersections with the g_ray_zplanes are
   * possible!
   */

  if (g_camera.pitch == ANGLE_0)
    {
      return;
    }

  /* Pre-calculate the rate of change of X and Y with respect to Z */
  /* Calculate the rate of change of X with respect to the Z-axis. The
   * cotangent is stored at double the "normal" scaling and the cosine is
   * also at double scaling.  dxdz will be also be stored at double
   * precision.
   */

  dxdz = qTOd(g_adj_cotpitch * ((int32_t) g_cos_table[g_camera.yaw]));

  /* Calculate the rate of change of Y with respect to the Z-axis. The
   * cotangent is stored at double the "normal" scaling and the sine is also
   * at double scaling.  dxdz will be also be stored at double precision.
   */

  dydz = qTOd(g_adj_cotpitch * ((int32_t) g_sin_table[g_camera.yaw]));

  /* Look at every rectangle lying in the Z plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (list = g_ray_zplane.head; list; list = list->flink)
    {
      rect = &list->d;

      /* Search for a rectangle which lies "beyond" the current camera
       * position
       */

      if (rect->plane > g_camera.z)
        {
          /* get the Z distance to the plane */

          relz = rect->plane - g_camera.z;

          /* g_ray_zplane is an ordered list, if we have already hit something
           * closer, then we can abort the casting now.
           */

          if (relz > result->zdist)
            {
              return;
            }

          /* Calculate the X position at this relative Z position.  We can skip
           * this step if we are processing another rectangle at the same relx
           * distance.
           */

          if (relz != lastrelz1)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdz is stored at double the"normal" scaling -- so deltax
               * is "triple" precision
               */

              deltax    = dxdz * ((int32_t) relz);
              absx      = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrelz1 = relz;
            }

          /* Check if this X position intersects the rectangle */

          if (absx >= rect->hstart && absx <= rect->hend)
            {
              /* The X position lies in the rectangle.  Now, calculate the
               * Y position at this relative Z position.  We can skip this step
               * if we are processing another rectangle at the same relx
               * distance.
               */

              if (relz != lastrelz2)
                {
                  int32_t deltay;      /* Scale == TRIPLE */

                  /* The dydz is stored at double the"normal" scaling -- so
                   * deltay is "triple" precision
                   */

                  deltay    = dydz * ((int32_t) relz);
                  absy      = tTOs(deltay) + g_camera.y; /* back to "single" */
                  lastrelz2 = relz;
                }

              /* Check if this Y position intersects the rectangle */

              if (absy >= rect->vstart && absy <= rect->vend)
                {
                  /* We've got a hit, ..Save the parameters associated with the
                   * ceiling hit
                   */

                  result->rect = rect;
                  result->type = MK_HIT_TYPE(BACK_HIT, Z_HIT);
                  result->xpos = absx;
                  result->ypos = absy;

                  result->xdist = ABS(absx - g_camera.x);
                  result->ydist = ABS(absy - g_camera.y);
                  result->zdist = relz;

                  /* Terminate Z casting */

                  return;
                }
            }
        }
    }
}

/****************************************************************************
 * Name: trv_ray_zcasterl
 *
 * Description:
 *   This function casts a ray along the rotated Y-Axis looking at points of
 *   intersection on the Z-Axis.  If a block is defined at this intersection
 *   then a "hit" is found and the distance to this hit is determined.
 *
 *   If we are in the "lower" half of the view, then the Z ray cast will
 *   proceed along the negative Z axis and that all possible Z-axis
 *   intersections will occur on the "top" of a cell.
 *
 *   NOTE: It is assumed that both the X and Y ray casters have already
 *   ran!
 *
 ****************************************************************************/

static void trv_ray_zcasterl(FAR struct trv_raycast_s *result)
{
  struct trv_rect_list_s *list; /* Points to the current Z plane rectangle */
  struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t relz;             /* Relative position of the Z plane */
  trv_coord_t absx;             /* Absolute X position at relz given yaw */
  trv_coord_t absy;             /* Absolute Y position at relz given yaw */
  trv_coord_t lastrelz1 = -1;   /* Last relative Z position processed */
  trv_coord_t lastrelz2 = -1;   /* Last relative Z position processed */
  int32_t dxdz;                 /* Rate of change of X wrt Z (double) */
  int32_t dydz;                 /* Rate of change of Y wrt Z (double) */

  /* At a viewing angle of 0 degrees, no intersections with the g_ray_zplanes are
   * possible!
   */

  if (g_camera.pitch == ANGLE_0)
    {
      return;
    }

  /* Pre-calculate the rate of change of X and Y with respect to Z */
  /* Calculate the rate of change of X with respect to the Z-axis. The
   * cotangent is stored at double the "normal" scaling and the cosine is
   * also at double scaling.  dxdz will be also be stored at double
   * precision.
   */

  dxdz = qTOd(g_adj_cotpitch * ((int32_t) g_cos_table[g_camera.yaw]));

  /* Calculate the rate of change of Y with respect to the Z-axis. The
   * cotangent is stored at double the "normal" scaling and the sine is
   * also at double scaling.  dxdz will be also be stored at double
   * precision.
   */

  dydz = qTOd(g_adj_cotpitch * ((int32_t) g_sin_table[g_camera.yaw]));

  /* Look at every rectangle lying in the Z plane */
  /* This logic should be improved at some point so that non-visible planes
   * are "pruned" from the list prior to ray casting!
   */

  for (list = g_ray_zplane.tail; list; list = list->blink)
    {
      rect = &list->d;

      /* Search for a rectangle which lies "before" the current camera
       * position
       */

      if (rect->plane < g_camera.z)
        {
          /* get the Z distance to the plane */

          relz = g_camera.z - rect->plane;

          /* g_ray_zplane is an ordered list, if we have already hit something
           * closer, then we can abort the casting now.
           */

          if (relz > result->zdist)
            {
              return;
            }

          /* Calculate the X position at this relative Z position.  We can skip
           * this step if we are processing another rectangle at the same relx
           * distance.
           */

          if (relz != lastrelz1)
            {
              int32_t deltax;  /* Scale == "triple" */

              /* The dxdz is stored at double the"normal" scaling -- so deltax
               * is "triple" precision
               */

              deltax    = dxdz * ((int32_t) relz);
              absx      = tTOs(deltax) + g_camera.x; /* back to "single" */
              lastrelz1 = relz;
            }

          /* Check if this X position intersects the rectangle */

          if (absx >= rect->hstart && absx <= rect->hend)
            {
              /* The X position lies in the rectangle.  Now, calculate the
               * Y position at this relative Z position.  We can skip this step
               * if we are processing another rectangle at the same relx
               * distance.
               */

              if (relz != lastrelz2)
                {
                  int32_t deltay;      /* Scale == TRIPLE */

                  /* The dydz is stored at double the"normal" scaling -- so
                   * deltay is "triple" precision
                   */

                  deltay    = dydz * ((int32_t) relz);
                  absy      = tTOs(deltay) + g_camera.y; /* back to "single" */
                  lastrelz2 = relz;
                }

              /* Check if this Y position intersects the rectangle */

              if (absy >= rect->vstart && absy <= rect->vend)
                {
                  /* We've got a hit, ..Save the parameters associated with the
                   * floor hit
                   */

                  result->rect = rect;
                  result->type = MK_HIT_TYPE(FRONT_HIT, Z_HIT);
                  result->xpos = absx;
                  result->ypos = absy;

                  result->xdist = ABS(absx - g_camera.x);
                  result->ydist = ABS(absy - g_camera.y);
                  result->zdist = relz;

                  /* Terminate Z casting */

                  return;
                }
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_raycast
 *
 * Description:
 *   This function performs a single ray cast.  It decomposes the cast by
 *   quadrants so that simpler casting algorithms can be used.  It also
 *   enforces the order of casting:  X first, then Y, and finally Z.
 *
 ****************************************************************************/

void trv_raycast(int16_t pitch, int16_t yaw, int16_t screenyaw,
                FAR struct trv_raycast_s *result)
{
  /* Set the camera pitch and yaw angles for this cast */

  g_camera.pitch = pitch;
  g_camera.yaw = yaw;

  /* Initialize the result structure, assuming that there will be no hit */

  result->rect  = NULL;
  result->type  = NO_HIT;
  result->xpos  = 0;
  result->ypos  = 0;
  result->xdist = TRV_INFINITY;
  result->ydist = TRV_INFINITY;
  result->zdist = TRV_INFINITY;

  /* Prepare for X and Y ray casts.  These casts will need the adjusted tangent
   * of the pitch angle in order to correct for "fish eye" distortion.  This
   * correction consists of multiplying by the cosine of the relative screen
   * yaw position. The tangent is double precision, the cosine is double
   * precision, the result will be retained as double precision.
   */

  screenyaw = ABS(screenyaw);
#if ENABLE_VIEW_CORRECTION
  g_adj_tanpitch = qTOd(TAN(pitch) * ((int32_t) g_cos_table[screenyaw]));
#else
  g_adj_tanpitch = TAN(pitch);
#endif

  /* Perform X & Y raycasting based on the quadrant of the yaw angle */

  if (g_camera.yaw < ANGLE_90)
    {
      trv_ray_xcaster14(result);
      trv_ray_ycaster12(result);
    }
  else if (g_camera.yaw < ANGLE_180)
    {
      trv_ray_xcaster23(result);
      trv_ray_ycaster12(result);
    }
  else if (g_camera.yaw < ANGLE_270)
    {
      trv_ray_xcaster23(result);
      trv_ray_ycaster34(result);
    }
  else
    {
      trv_ray_xcaster14(result);
      trv_ray_ycaster34(result);
    }

  /* Perform Z ray casting based upon if we are looking up or down */

  if (g_camera.pitch < ANGLE_90)
    {
      /* Get the adjusted cotangent of the pitch angle which is used to correct
       * for the "fish eye" distortion.  This correction consists of
       * multiplying by the inverted cosine of the relative screen yaw
       * position.  The cotangent is double precision, the secant is double
       * precision, the result will be retained as double precision.
       */

#if ENABLE_VIEW_CORRECTION
      g_adj_cotpitch = qTOd(g_cot_table(pitch) * g_sec_table[screenyaw]);
#else
      g_adj_cotpitch = g_cot_table(pitch);
#endif
      trv_ray_zcasteru(result);
    }
  else
    {
      /* Get the adjusted cotangent of the pitch angle which is used to correct
       * for the "fish eye" distortion.  This correction consists of
       * multiplying by the inverted cosine of the relative screen yaw
       * position. The cotangent is double precision, the secant is double
       * precision, the result will be retained as double precision.
       */

#if ENABLE_VIEW_CORRECTION
      g_adj_cotpitch =
        qTOd(g_cot_table(ANGLE_360 - pitch) * g_sec_table[screenyaw]);
#else
      g_adj_cotpitch = g_cot_table(ANGLE_360 - pitch);
#endif
      trv_ray_zcasterl(result);
    }
}
