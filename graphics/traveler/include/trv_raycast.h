/****************************************************************************
 * apps/graphics/traveler/include/trv_raycast.h
 * This is the header file associated with trv_raycast.c
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYCAST_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYCAST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The following definitions define the world view window in terms of the
 * 320x200 display coordinates.
 */

#define WINDOW_LEFT             0  /* Offset (in bytes) to the left side
                                    * of the world window */
#define WINDOW_WIDTH          320  /* Width of the world window */
#define WINDOW_TOP              0  /* Offset (in multiples of TRV_SCREEN_WIDTH)
                                    * to the top of world window */
#define WINDOW_HEIGHT         200  /* height of the world window */
#define WINDOW_MIDDLE         100  /* Center or horizon of the world window */

/* This calculation tends to overflow unless it is cast carefully */

#define WINDOW_SIZE (((int32_t)WINDOW_WIDTH)*((int32_t)WINDOW_HEIGHT))

/* This is the biggest distance that can be represented with int16_t */

#define TRV_INFINITY 0x7fff

/* The actual size of the image will be determined by the
 * both the WINDOW size as well as the GULP size
 */

#define NUMBER_HGULPS (WINDOW_WIDTH/HGULP_SIZE)
#define IMAGE_WIDTH   (HGULP_SIZE*NUMBER_HGULPS)
#define IMAGE_LEFT    (((WINDOW_WIDTH-IMAGE_WIDTH) >> 1) + WINDOW_LEFT)
#define IMAGE_HEIGHT  WINDOW_HEIGHT
#define IMAGE_TOP     WINDOW_TOP

/* This defines the number of pixels (in each direction) with will be
 * processed on each pass through the innermost ray casting loop
 */

#define HGULP_SIZE  7
#define VGULP_SIZE 16

/* The following define the various meanings of hit.type */

#define NO_HIT    0x00
#define FRONT_HIT 0x00
#define BACK_HIT  0x01
#define FB_MASK   0x01
#define X_HIT     0x00
#define Y_HIT     0x02
#define Z_HIT     0x04
#define XYZ_MASK  0x06

#define IS_FRONT_HIT(h) (((h)->type & FB_MASK) == FRONT_HIT)
#define IS_BACK_HIT(h)  (((h)->type & FB_MASK) == BACK_HIT)
#define IS_XRAY_HIT(h)  (((h)->type & XYZ_MASK) == X_HIT)
#define IS_YRAY_HIT(h)  (((h)->type & XYZ_MASK) == Y_HIT)
#define IS_ZRAY_HIT(h)  (((h)->type & XYZ_MASK) == Z_HIT)

#define MK_HIT_TYPE(fb,xyz) ((fb)|(xyz))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure provides the return values for each ray caster.  For
 * performance reasons, the size of this structure should be an even
 * power of two (makes index calculation faster).
 */

struct trv_rect_data_s;
struct trv_raycast_s
{
  /* 'rect' points to the rectangle that was hit (if any) */

  FAR struct trv_rect_data_s *rect;

  uint8_t type;     /* Type of hit:  X/Y/Z cast, front/back */
  uint8_t unused2;  /* Padding to force power of two size */
  int16_t xpos;     /* Horizontal position on surface of the hit */
  int16_t ypos;     /* Vertical position on surface of the hit */
  int16_t xdist;    /* X distance to the hit */
  int16_t ydist;    /* Y distance to the hit */
  int16_t zdist;    /* Z distance to the hit (not used) */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This structure holds pre-calculated pitch data for all ray casts.  NOTE:
 * would be a performance improvement if this structure is an even power of
 *two in size (don't have to multiply to calculate indices)
 */

/* The following array describes the hits from X/Y/Z-ray casting for the
 * current HGULP_SIZE x VGULP_SIZE cell
 */

extern struct trv_raycast_s g_ray_hit[VGULP_SIZE][HGULP_SIZE+1];

/* This structure points to the double buffer row corresponding to the
 * pitch angle
 */

extern uint8_t *g_buffer_row[VGULP_SIZE];

/* The is the "column" offset in g_buffer_row for the current cell being
 * operated on.  This value is updated in a loop by trv_raycaster.
 */

extern int16_t g_cell_column;

/* This structure holds the parameters used in the current ray cast */

extern struct trv_camera_s g_camera;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void trv_raycast(int16_t pitch, int16_t yaw, int16_t screenyaw,
                 FAR struct trv_raycast_s *result);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYCAST_H */
