/****************************************************************************
 * apps/graphics/traveler/src/trv_doors.c
 * This file contains the logic which manages world door logic.
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
#include "trv_pov.h"
#include "trv_plane.h"
#include "trv_world.h"
#include "trv_rayavoid.h"
#include "trv_doors.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* This is the door opening rate */

#define DOOR_ZSTEP     15
#define DOOR_OPEN_WAIT 30

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/* These are possible values for the g_opendoor state variable */

enum trv_door_state_e
{
  DOOR_IDLE = 0,    /* No door action in progress */
  DOOR_OPENING,     /* A door is opening */
  DOOR_OPEN,        /* A door is open */
  DOOR_CLOSING,     /* A door is closing */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void trv_door_startopen(void);
static void trv_door_animation(void);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Variables to track status of a door */

struct trv_opendoor_s g_opendoor;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_door_startopen
 *
 * Description:
 *
 ****************************************************************************/

static void trv_door_startopen (void)
{
  FAR struct trv_rect_data_s *rect;

  /* Check if a door is already opening */

  if (g_opendoor.state == DOOR_IDLE)
    {
      /* Test if there is a door within three steps in front of the player */
      /* Try the X planes first */

      rect = trv_ray_test_xplane(&g_player, 3*STEP_DISTANCE,
                                 g_player.yaw, g_player_height);

      /* If there is no X door in front of the player, then try the Y Planes
       * (it is assumed that there would not be doors this close in both
       * planes!)
       */

      if (!rect || !IS_DOOR(rect))
        {
          rect = trv_ray_test_yplane(&g_player, 3*STEP_DISTANCE,
                                     g_player.yaw, g_player_height);
        }

      /* Check if we found a door in either the X or Y plane. */

      if (rect && IS_DOOR(rect))
        {
          /* We now have found a door to open.  Set the door open sequence
           * in motion
           */

          g_opendoor.rect    = rect;
          g_opendoor.state   = DOOR_OPENING;
          g_opendoor.zbottom = rect->vstart;
          g_opendoor.zdist   = 0;

          /* Mark the door's attribute to indicate that it is in motion */

          rect->attribute |= MOVING_DOOR_PLANE;
        }
    }
}

/****************************************************************************
 * Name: trv_door_animation
 * Description:
 * This function is called every frame when a door is in motion.
 ****************************************************************************/

static void trv_door_animation(void)
{
  /* What we do next depends upon the state of the door state machine */

  switch (g_opendoor.state)
    {
    /* A door is opening */

    case DOOR_OPENING :

      /* Raise the door a little */

      g_opendoor.zbottom += DOOR_ZSTEP;
      g_opendoor.zdist   += DOOR_ZSTEP;

      /* When the bottom of the door is above the player's head, we will
       * say that the door is open
       */

      if (g_opendoor.zbottom > g_player.z)
        {
          g_opendoor.rect->attribute |= OPEN_DOOR_PLANE;
        }

      /* Check if the door is fully open */

      if (g_opendoor.zbottom >= g_opendoor.rect->vend)
        {
          /* Make sure that the door does not open wider than it is tall */

          g_opendoor.zbottom = g_opendoor.rect->vend;
          g_opendoor.zdist   = g_opendoor.rect->vend - g_opendoor.rect->vstart;

          /* The door is done opening, the next state is the DOOR_OPEN state
           * where we will hold the door open a while
           */

          g_opendoor.state = DOOR_OPEN;

          /* Initialize the countdown timer which will determine how long
           * the door is held open
           */

           g_opendoor.clock = DOOR_OPEN_WAIT;
        }
      break;

    /* The door is open */

    case DOOR_OPEN :
      /* Decrement the door open clock.  When it goes to zero, it is time
       * to begin closing the door
       */

      if (--g_opendoor.clock <= 0)
        {
          /* The door is done opening, the next state is the DOOR_CLOSING
           * states
           */

          g_opendoor.state = DOOR_CLOSING;

          /* Lower the door a little */

          g_opendoor.zbottom -= DOOR_ZSTEP;
          g_opendoor.zdist   -= DOOR_ZSTEP;

          /* When the bottom of the door is below the player's head, we
           * will say that the door is closed
           */

          if (g_opendoor.zbottom <= g_player.z)
            {
              g_opendoor.rect->attribute &= ~OPEN_DOOR_PLANE;
            }
        }
      break;

    /* The door is closing */

    case DOOR_CLOSING :
      /* Lower the door a little */

      g_opendoor.zbottom -= DOOR_ZSTEP;
      g_opendoor.zdist   -= DOOR_ZSTEP;

      /* When the bottom of the door is below the player's head, we will
       * say that the door is closed
       */

      if (g_opendoor.zbottom <= g_player.z)
        {
          g_opendoor.rect->attribute &= ~OPEN_DOOR_PLANE;
        }

      /* Check if the door is fully closed */

      if (g_opendoor.zdist <= 0)
        {
          /* Indicate that the door is no longer in motion */

          g_opendoor.rect->attribute &= ~(OPEN_DOOR_PLANE|MOVING_DOOR_PLANE);

          /* Indicate that the entire door movement sequence is done */

          g_opendoor.rect = NULL;
          g_opendoor.state = DOOR_IDLE;
        }
      break;

    /* There is nothing going on! */

    case DOOR_IDLE :
    default :
      break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_door_initialize
 *
 * Description:
 *   This function performs initialization of the door animation logic
 *
 ****************************************************************************/

void trv_door_initialize(void)
{
  /* Initialize the g_opendoor structure */

  g_opendoor.rect = NULL;
  g_opendoor.state = DOOR_IDLE;
}

/****************************************************************************
 * Name: trv_door_animate
 *
 * Description:
 *   This function must be called on each cycle.  It checks if the player
 *   is attempting to open a door.  If so, the appropriate door animation
 *   is started.  This function then calls trv_door_animation which must be
 *   called on each cycle to perform the door movement.
 *
 ****************************************************************************/

void trv_door_animate(void)
{
  /* Check if the user is trying to open a door. */

  if (g_trv_input.dooropen && g_opendoor.state == DOOR_IDLE)
    {

      /* Start the door opening action */

      trv_door_startopen();
    }

  /* Process any necessary door movement animation on this cycle */

  if (g_opendoor.state != DOOR_IDLE)
    {
      /* Perform the animation */

      trv_door_animation();
   }
}
