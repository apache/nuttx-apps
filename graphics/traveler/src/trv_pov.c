/****************************************************************************
 * apps/graphics/traveler/trv_pov.c
 * This file contains the logic which manages player's point-of-view (POV)
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
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_input.h"
#include "trv_trigtbl.h"
#include "trv_rayavoid.h"
#include "trv_pov.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This structure defines the current camera position of the player's eyes */

struct trv_camera_s g_player;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  trv_new_viewangle
 * Description:
 * This function determines the new POV to use on this cycle
 ****************************************************************************/

static void trv_new_viewangle(void)
{
  /* Are we turning to the left or right (yaw)? */

  if (g_trv_input.yawrate != 0)
    {
      /* Determine the amount to rotate on this cycle */

      g_player.yaw += g_trv_input.yawrate;

      /* Make sure that yaw is still within range */

      if (g_player.yaw >= ANGLE_360)
        {
          g_player.yaw -= ANGLE_360;
        }
      else if (g_player.yaw < ANGLE_0)
        {
          g_player.yaw += ANGLE_360;
        }
    }

  /* Are we "nodding" up or down (pitch) */

  if (g_trv_input.pitchrate != 0)
    {
      g_player.pitch += g_trv_input.pitchrate;

      /* Make sure that pitch is still within range */

      if (g_player.pitch >= ANGLE_360)
        {
          g_player.pitch -= ANGLE_360;
        }
      else if (g_player.pitch < ANGLE_0)
        {
          g_player.pitch += ANGLE_360;
        }

     /* Don't let the player look up more than thirty degrees */

     if (g_player.pitch > ANGLE_30 &&
         g_player.pitch < ANGLE_180)
       {
          g_player.pitch = ANGLE_30;
        }

      /* Don't let the player look down more than thirty degrees */

      else if (g_player.pitch < (ANGLE_360 - ANGLE_30) &&
               g_player.pitch > ANGLE_180)
        {
          g_player.pitch = (ANGLE_360 - ANGLE_30);
        }
    }
}

/****************************************************************************
 * Name:  trv_new_playerposition
 *
 * Description:
 *   This function determines the new POV to use on this cycle
 *
 ****************************************************************************/

static void trv_new_playerposition(void)
{
  int16_t     move_angle;
  int16_t     left_angle;
  trv_coord_t fwd_distance;
  trv_coord_t left_distance;
  trv_coord_t move_distance;

  /* Assume that we are moving forward */

  move_angle    = g_player.yaw;
  left_angle    = ANGLE_90;
  fwd_distance  = g_trv_input.fwdrate;
  left_distance = g_trv_input.leftrate;

  /* Are we are moving "backward" */

  if (fwd_distance < 0)
    {
      /* Yes.. switch to moving backward at 180 degrees.  Moving left now
       * becomes moving right.
       */

      fwd_distance  = -fwd_distance;
      left_distance = -left_distance;

      move_angle   += ANGLE_180;
      if (move_angle >= ANGLE_360)
        {
          move_angle -= (int16_t)ANGLE_360;
        }

      /* If we also doing horizontal movement, it the angle should actually
       * be based on the relative magnitude forward and lateral movement
       * distance.  But we will just use 45 degrees which should be good
       * enough for out purposes.
       */

     left_angle = ANGLE_45;
    }

  /* Are we moving left or right? */

  if (left_distance > 0)
    {
      move_angle = g_player.yaw - left_angle;
    }
  else if (left_distance < 0)
    {
      left_distance = -left_distance;
      move_angle = g_player.yaw + left_angle;
    }

  if (move_angle < ANGLE_0)
    {
      move_angle += (int16_t)ANGLE_360;
    }
  else if (move_angle >= ANGLE_360)
    {
      move_angle -= (int16_t)ANGLE_360;
    }

  /* The move distance is approximated as follows */

  if (left_distance > fwd_distance)
    {
      move_distance = left_distance + (fwd_distance >> 1);
    }
  else
    {
      move_distance = fwd_distance + (left_distance >> 1);
    }

  /* Decompose the desired move distance into X and Y components
   * and clip these components to avoid collisions with walls and objects
   */

  g_player.x +=
    trv_rayclip_player_xmotion(&g_player, move_distance,
                               move_angle, g_trv_input.stepheight);
  g_player.y +=
    trv_rayclip_player_ymotion(&g_player, move_distance,
                               move_angle, g_trv_input.stepheight);

  /* Adjust the player's vertical position (he may have fallen down or
   * stepped up something.
   */

  g_player.z += trv_ray_adjust_zpos(&g_player, g_player_height);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  trv_pov_reset
 *
 * Description:
 *   This function returns the player to the starting position
 *
 ****************************************************************************/

void trv_pov_reset(void)
{
  g_player = g_initial_camera;
}

/****************************************************************************
 * Name:  trv_pov_new
 *
 * Description:
 *   This function determines the new POV to use on this cycle based on the
 *   current inputs
 ****************************************************************************/

void trv_pov_new(void)
{
  /* Get angular changes first */

  trv_new_viewangle();

  /* The get the player motion along the new angles */

  trv_new_playerposition();
}
