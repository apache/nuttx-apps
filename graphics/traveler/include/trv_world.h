/****************************************************************************
 * apps/graphics/traveler/include/trv_world.h
 * This file contains definitions for the world model
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_WORLD_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_WORLD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The following structure contains all information necessary to define
 * a point-of-view.
 */

struct trv_camera_s
{
  trv_coord_t x;      /* Camera X position */
  trv_coord_t y;      /* Camera Y position */
  trv_coord_t z;      /* Camera Z position */
  int16_t     yaw;    /* Camera yaw orientation */
  int16_t     pitch;  /* Camera pitch orientation */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/
/* This is the starting position and orientation of the camera in the world */

extern struct trv_camera_s g_initial_camera;

/* This is the height of player (distance from the camera Z position to
 * the position of the player's "feet"
 */

extern trv_coord_t g_player_height;

/* This is size of something that the player can step over when "walking" */

extern trv_coord_t g_walk_stepheight;

/* This is size of something that the player can step over when "running" */

extern trv_coord_t g_run_stepheight;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int  trv_world_create(FAR const char *wldpath, FAR const char *wldfile);
void trv_world_destroy(void);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_WORLD_H */
