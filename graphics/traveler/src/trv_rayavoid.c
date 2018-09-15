/****************************************************************************
 * apps/graphics/traveler/src/trv_rayavoid.c
 * This file contains the logic which determines if the desired player motion
 * would cause a collision with various walls or if the motion would cause
 * the player to change his vertical position in the world.  This collision
 * avoidance logic is also used to determine if there is a door in front of
 * the player.
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
#include "trv_trigtbl.h"
#include "trv_plane.h"
#include "trv_world.h"
#include "trv_rayavoid.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* This is the closest that we will allow the player to approach a wall */

#define MIN_APPROACH_DISTANCE (64/4) /* One quarter cell */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The rayClipPlayX/YMotion functions have the side effect of saving the
 * pointer to the wall with which the player collided in the following
 * private variable.  This gives a "back door" mechanism which is used
 * to handle door processing.
 */

static struct trv_rect_data_s *g_clip_rect;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  trv_ray_test_xplane
 *
 * Description:
 *
 *   This function tests if there is an X plane within the supplied distance
 *   from the player at the supplied angle.  This function is used to
 *   determine in there is a door in front of the player.  NOTE:  This
 *   function exploits the normal collision detection logic in
 *   trv_rayclip_player_xmotion and depends on the side-effect setting of
 *   g_clip_rect.
 *
 ****************************************************************************/

FAR struct trv_rect_data_s *trv_ray_test_xplane(FAR struct trv_camera_s *pov,
                                              trv_coord_t dist, int16_t yaw,
                                              trv_coord_t height)
{
  (void) trv_rayclip_player_xmotion(pov, dist, yaw, height);
  return g_clip_rect;
}

/****************************************************************************
 * Name:  trv_rayclip_player_xmotion
 *
 * Description:
 *   This function calculates the acceptable change in the player's position
 *   along the X component of the specified yaw angle which would not cause
 *   a collision with an X plane.  This logic is essentially a modified X
 *   ray cast.
 *
 ****************************************************************************/

trv_coord_t trv_rayclip_player_xmotion(FAR struct trv_camera_s *pov,
                                     trv_coord_t dist, int16_t yaw,
                                     trv_coord_t height)
{
  FAR struct trv_rect_list_s *list; /* Points to the current X plane rectangle */
  FAR struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t reqdeltax;
  trv_coord_t standoff;
  trv_coord_t footlevel;
  trv_coord_t relx;                 /* Relative position of the X plane */
  trv_coord_t absy;                 /* Absolute Y position at relx given yaw */
  trv_coord_t lastrelx = -1;        /* Last relative X position processed */

  /* Decompose the desired move distance into its X component */

  reqdeltax = tTOs(g_cos_table[ yaw ] * dist);

  /* Assume that no clipping will be performed */

  g_clip_rect = NULL;

  /* Perform X raycasting based on the quadrant of the yaw angle */
  /* The first and fourth quadrants correspond to the positive X
   * direction (excluding angles 90 and 270).
   */

  if (yaw < ANGLE_90 || yaw > ANGLE_270)
    {
      /* Calculate the requested distance along the (positive) X axis (adding
       * a little to how close the play can come to a wall
       */

      standoff = reqdeltax + MIN_APPROACH_DISTANCE;

      /* This is the position of the player's feet */

      footlevel = pov->z - height;

      /* Look at every rectangle lying in the X plane */

      for (list = g_xplane.head; list; list = list->flink)
        {
          rect = &list->d;

          /* Search for a rectangle which lies "beyond" the current camera
           * position
           */

          if (rect->plane > pov->x)
            {
              /* Get the X distance to the plane.  This is an order list:  if
               * the distance to the plane is larger then the requested step
               * then the requested step is acceptable.
               */

              relx = rect->plane - pov->x;
              if (relx >= standoff)
                {
                  return reqdeltax;
                }

              /* The distance to the plane is smaller that the requested step.
               * It may be possible to collide with the plane.  Check if a
               * collision due to the height of the player is possible first
               * (its easier to compute)
               */

              if (footlevel >= rect->vstart && pov->z <= rect->vend)
                {
                  /* A collision is possible based on the players height.
                   * Now, we'll have to check if a collision is possible
                   * due to the player's Y position.  We can skip this
                   * step if we are processing another rectangle at the
                   * same relx distance.
                   */

                  if (relx != lastrelx)
                    {
                      int32_t deltay; /* Scale == "triple" */

                      /* The tangent is equal to the rate of change of Y with
                       * respect to the X-axis.  The tangent is stored at
                       * double the "normal" scaling -- so deltay is
                       * "triple" precision
                       */

                      deltay   = TAN(yaw) * ((int32_t)relx);
                      absy     = tTOs(deltay) + pov->y; /* back to "single" */
                      lastrelx = relx;
                    }

                  /* Check if this Y position intersects the rectangle */

                  if (absy >= rect->hstart && absy <= rect->hend)
                    {
                      /* It collides -- Check, maybe we can walk through
                       * this wall
                       */

                      if (!IS_PASSABLE(rect))
                        {
                          /* Nope... return a clipped value for the new
                           * player position (which includes the a stand-off
                           * constant)
                           * NOTE:  This returned value could be negative!
                           */

                          g_clip_rect = rect;
                          return relx - MIN_APPROACH_DISTANCE;
                        }
                    }
                }
            }
        }
    }

  /* The second and third quadrants correspond to the negative X
   * direction (excluding angles 90 and 270).
   */

  else if (yaw > ANGLE_90 && yaw < ANGLE_270)
    {
      /* Calculate the requested distance along the (negative) X axis
       * (adding a little to how close the play can come to a wall)
       */

      standoff = MIN_APPROACH_DISTANCE - reqdeltax;

      /* This is the position of the player's feet */

      footlevel = pov->z - height;

      /* Look at every rectangle lying in the X plane */

      for (list = g_xplane.tail; list; list = list->blink)
        {
          rect = &list->d;

          /* Search for a rectangle which lies "before" the current camera
           * position
           */

          if (rect->plane < pov->x)
            {
              /* Get the X distance to the plane.  This is an order list:
               * if the distance to the plane is larger then the requested
               * step then the requested step is acceptable.
               */

              relx = pov->x - rect->plane;
              if (relx >= standoff)
                {
                  return reqdeltax;
                }

              /* The distance to the plane is smaller that the requested
               * step.  It may be possible to collide with the plane.  Check
               * if a collision due to the height of the player is possible
               * first (its easier to compute)
               */

              if (footlevel >= rect->vstart && pov->z <= rect->vend)
                {
                  /* A collision is possible based on the players height.
                   * Now, we'll have to check if a collision is possible due
                   * to the player's Y position.  We can skip this step if
                   * we are processing another rectangle at the same relx
                   * distance.
                   */

                  if (relx != lastrelx)
                    {
                      int32_t deltay; /* Scale == "triple" */

                      /* The negative tangent is equal to the rate of change
                       * of Y with respect to the X-axis.The tangent is
                       * stored at double the "normal" scaling -- so
                       * deltay is "triple" precision
                       */

                      deltay   = -TAN(yaw) * ((int32_t)relx);
                      absy     = tTOs(deltay) + pov->y; /* back to "single" */
                      lastrelx = relx;
                    }

                  /* Check if this Y position intersects the rectangle */

                  if (absy >= rect->hstart && absy <= rect->hend)
                    {
                      /* It collides -- Check, maybe we can walk through
                       * this wall?
                       */

                      if (!IS_PASSABLE(rect))
                        {
                          /* Nope -- return a clipped value for the new
                           * player position (which includes the a stand-off
                           * constant)
                           * NOTE:  This returned value could be positive!
                           */

                          g_clip_rect = rect;
                          return MIN_APPROACH_DISTANCE - relx;
                        }
                    }
                }
            }
        }
    }

  /* If we got there, then no collisions were found.  Just return the
   * requested step value
   */

  return reqdeltax;
}

/****************************************************************************
 * Name:  trv_ray_test_yplane
 *
 * Description:
 *   This function tests if there is an Y plane within the supplied distance
 *   from the player at the supplied angle.  This function is used to
 *   determine in there is a door in front of the player.  NOTE:  This
 *   function exploits the normal collision detection logic in
 *   trv_rayclip_player_ymotion and depends on the side-effect setting of
 *   g_clip_rect.
 *
 ****************************************************************************/

FAR struct trv_rect_data_s *trv_ray_test_yplane(FAR struct trv_camera_s *pov,
                                                trv_coord_t dist, int16_t yaw,
                                                trv_coord_t height)
{
  (void)trv_rayclip_player_ymotion(pov, dist, yaw, height);
  return g_clip_rect;
}

/****************************************************************************
 * Name:  trv_rayclip_player_ymotion
 *
 * Description:
 *   This function calculates the acceptable change in the player's position
 *   along the Y component of the specified yaw angle which would not cause
 *   a collision with a Y plane.  This logic is essentially a modified X
 *   ray cast.
 *
 ****************************************************************************/

trv_coord_t trv_rayclip_player_ymotion(FAR struct trv_camera_s *pov,
                                       trv_coord_t dist, int16_t yaw,
                                       trv_coord_t height)
{
  FAR struct trv_rect_list_s *list; /* Points to the current Y plane rectangle */
  FAR struct trv_rect_data_s *rect; /* Points to the rectangle data */
  trv_coord_t reqdeltay;
  trv_coord_t standoff;
  trv_coord_t footlevel;
  trv_coord_t rely;                 /* Relative position of the Y plane */
  trv_coord_t absx;                 /* Absolute X position at rely given yaw */
  trv_coord_t lastrely = -1;        /* Last relative Y position processed */

  /* Decompose the desired move distance into its Y component */

  reqdeltay = tTOs(g_sin_table[ yaw ] * dist);

  /* Assume that no clipping will be performed */

  g_clip_rect = NULL;

  /* Perform Y raycasting based on the quadrant of the yaw angle */
  /* The first and second quadrants correspond to the positive Y
   * direction (excluding angles 0 and 180).
   */

   if (yaw > ANGLE_0 && yaw < ANGLE_180)
     {
       /* Calculate the requested distance along the (positive) X axis
        * (adding a little to how close the play can come to a wall)
        */

        standoff = reqdeltay + MIN_APPROACH_DISTANCE;

        /* This is the position of the player's feet */

        footlevel = pov->z - height;

        /* Look at every rectangle lying in a Y plane */

        for (list = g_yplane.head; list; list = list->flink)
          {
            rect = &list->d;

           /* Search for a rectangle which lies "beyond" the current camera
            * position
            */

           if (rect->plane > pov->y)
             {
               /* Get the Y distance to the plane.  This is an order list:
                * If the distance to the plane is larger then the requested
                * step then the requested step is acceptable.
                */

              rely = rect->plane - pov->y;
              if (rely >= standoff)
                {
                  return reqdeltay;
                }

              /* The distance to the plane is smaller that the requested
               * step.  It may be possible to collide with the plane.
               * Check if a collision due to the height of the player is
               * possible first (its easier to compute)
               */

              if (footlevel >= rect->vstart && pov->z <= rect->vend)
                {
                  /* A collision is possible based on the players height.
                   * Now, we'll have to check if a collision is possible
                   * due to the player's X position.  We can skip this
                   * step if we are processing another rectangle at the
                   * same relx distance.
                   */

                  if (rely != lastrely)
                    {
                      int32_t deltax; /* Scale == "triple" */

                      /* The inverted tangent is equal to the rate of change
                       * of X with respect to the Y-axis.  The cotangent is
                       * stored at double the "normal" scaling -- so
                       * deltax is "triple" precision
                       */

                      deltax   = g_cot_table(yaw) * ((int32_t)rely);
                      absx     = tTOs(deltax) + pov->x; /* back to "single" */
                      lastrely = rely;
                    }

                  /* Check if this X position intersects the rectangle */

                  if (absx >= rect->hstart && absx <= rect->hend)
                    {
                      /* It collides -- Check, maybe we can walk through
                       * this wall?
                       */

                      if (!IS_PASSABLE(rect))
                        {
                          /* Nope -- return a clipped value for the new
                           * player position (which includes the a stand-off
                           * constant)
                           * NOTE:  This returned value could be negative!
                           */

                          g_clip_rect = rect;
                          return rely - MIN_APPROACH_DISTANCE;
                        }
                    }
                }
            }
        }
    }

  /* The third and fourth quadrants correspond to the negative Y
   * direction (excluding angles 0 and 180).
   */

  else if (yaw > ANGLE_180)
    {
      /* Calculate the requested distance along the (negative) X axis
       * (adding a little to how close the play can come to a wall)
       */

      standoff = MIN_APPROACH_DISTANCE - reqdeltay;

      /* This is the position of the player's feet */

      footlevel = pov->z - height;

      /* Look at every rectangle lying in the X plane */

      for (list = g_yplane.tail; list; list = list->blink)
        {
          rect = &list->d;

          /* Search for a rectangle which lies "before" the current camera
           * position
           */

          if (rect->plane < pov->y)
            {
              /* Get the X distance to the plane.  This is an order list:  if
               * the distance to the plane is larger then the requested step
               * then the requested step is acceptable.
               */

              rely = pov->y - rect->plane;
              if (rely >= standoff)
                {
                  return reqdeltay;
                }

              /* The distance to the plane is smaller that the requested
               * step.  It may be possible to collide with the plane.  Check
               * if a collision due to the height of the player is possible
               * first (its easier to compute)
               */

              if (footlevel >= rect->vstart && pov->z <= rect->vend)
                {
                  /* A collision is possible based on the players height.
                   * Now, we'll have to check if a collision is possible due
                   * to the player's X position.  We can skip this step if
                   * we are processing another rectangle at the same relx
                   * distance.
                   */

                  if (rely != lastrely)
                    {
                      int32_t deltax; /* Scale == "triple" */

                      /* The negative inverted tangent is equal to the rate
                       * of change of X with respect to the Y-axis.  The
                       * cotangent is stored at double the "normal"
                       * scaling -- so deltax is "triple" precision
                       */

                      deltax   = -g_cot_table(yaw - ANGLE_180) * ((int32_t)rely);
                      absx     = tTOs(deltax) + pov->x; /* back to "single" */
                      lastrely = rely;
                    }

                  /* Check if this X position intersects the rectangle */

                  if (absx >= rect->hstart && absx <= rect->hend)
                    {
                      /* It collides -- Check, maybe we can walk through
                       * this wall
                       */

                      if (!IS_PASSABLE(rect))
                        {
                          /* It collides -- return a clipped value for the
                           * new player position (which includes the a
                           * stand-off constant)
                           * NOTE:  This returned value could be positive!
                           */

                          g_clip_rect = rect;
                          return MIN_APPROACH_DISTANCE - rely;
                        }
                    }
                }
            }
        }
    }

  /* Return the clipped value */

  return reqdeltay;
}

/****************************************************************************
 * Name:  trv_ray_adjust_zpos
 *
 * Description:
 *   Make sure that the player is standing on something!
 *
 ****************************************************************************/

trv_coord_t trv_ray_adjust_zpos(FAR struct trv_camera_s *pov,
                                trv_coord_t height)
{
  struct trv_rect_list_s *list; /* Points to the current Z plane rectangle */
  struct trv_rect_data_s *rect; /* Points to the rectangle data */

  /* We will place the player's feet at the largest Z plane
   * which is lower the player's eye level.  We traverse
   * the g_zplane list in order of increase Z values
   */

  for (list = g_zplane.head; list; list = list->flink)
    {
      rect = &list->d;

      /* The Z plane list is an ordered list.  If the next
        * plane is over the player's head, then the player
        * must be standing at "ground zero."
        */

      if (rect->plane >= pov->z)
        {
          return height - pov->z;
        }

      /* Check if this plane if under the player */

      if (pov->x >= rect->hstart &&  pov->x <= rect->hend &&
          pov->y >= rect->vstart &&  pov->y <= rect->vend)
        {
          /* We have the smallest Z plane under the player
           * which is below the player's eye level (pov->z).
           * Determine the approach delta Z value to return
           */

          return height + rect->plane - pov->z;
        }
    }

  /* No plane was found under the player.  Set him at his
   * height above the "bottom" of the world
   */

  return height - pov->z;
}
