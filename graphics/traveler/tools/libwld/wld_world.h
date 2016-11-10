/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_world.h
 * This file contains definitions for the world model
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

#ifndef __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_WORLD_H
#define __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_WORLD_H

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*************************************************************************
 * Included Files
 *************************************************************************/

#include "wld_bitmaps.h"
#include "wld_plane.h"

/*************************************************************************
 * Pre-processor Definitions
 *************************************************************************/

/* This defines the maximum length of a file name */

#define FILE_NAME_SIZE 16

/*************************************************************************
 * Public Type Definitions
 *************************************************************************/

/* World file return codes */

enum {
  WORLD_SUCCESS = 0,
  WORLD_FILE_OPEN_ERROR = 100,
  WORLD_INTEGER_OUT_OF_RANGE,
  WORLD_INTEGER_NOT_FOUND,
  WORLD_FILENAME_NOT_FOUND,
  WORLD_PLANE_FILE_NAME_ERROR,
  WORLD_PALR_FILE_NAME_ERROR,
  WORLD_BITMAP_FILE_NAME_ERROR
};


/* The following structure contains all information necessary to define
 * a point-of-view
 */

typedef struct {

  wld_coord_t x, y, z;       /* Camera position */
  int16_t  yaw, pitch;    /* Camera orientation */

} wld_camera_t;

/*************************************************************************
 * Global Data
 *************************************************************************/

/* This is the starting position and orientation of the camera in the world */

extern wld_camera_t g_initial_camera;

/* This is the height of player (distance from the camera Z position to
 * the position of the player's "feet"
 */

extern wld_coord_t playerHeight;

/* This is size of something that the player can step over when "walking" */

extern wld_coord_t walkStepHeight;

/* This is size of something that the player can step over when "running" */

extern wld_coord_t runStepHeight;

/*************************************************************************
 * Global Function Prototypes
 *************************************************************************/

uint8_t wld_create_world(char *mapfile);
void    wld_deallocate_world(void);

#ifdef __cplusplus
}
#endif

#endif

