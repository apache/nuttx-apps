/****************************************************************************
 * apps/graphics/traveler/include/trv_plane.h
 * This file contains definitions for the world planes
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_PLANE_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_PLANE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* These are bit-field definitions for the struct trv_rect_data_s attribute field
 * STATIC settings
 */

#define SHADED_PLANE       0x01
#define TRANSPARENT_PLANE  0x02
#define DOOR_PLANE         0x04

/* DYNAMIC settings */

#define OPEN_DOOR_PLANE    0x08
#define MOVING_DOOR_PLANE  0x10

/* Legal values of texture scaling in struct trv_rect_data_s */

#define ONEX_SCALING  0
#define TW0X_SCALING  1
#define FOURX_SCALING 2
#define MAXX_SCALING  2

/* These macros are used to test the various texture attributes */

#define IS_NORMAL(r)       (((r)->attribute & ~SHADED_PLANE) == 0)
#define IS_SHADED(r)       (((r)->attribute & SHADED_PLANE) != 0)
#define IS_TRANSPARENT(r)  (((r)->attribute & TRANSPARENT_PLANE) != 0)
#define IS_DOOR(r)         (((r)->attribute & DOOR_PLANE) != 0)
#define IS_OPEN_DOOR(r)    (((r)->attribute & OPEN_DOOR_PLANE) != 0)
#define IS_CLOSED_DOOR(r)  ((IS_DOOR(R)) && (!IS_OPEN_DOOR(r)))
#define IS_MOVING_DOOR(r)  (((r)->attribute & MOVING_DOOR_PLANE) != 0)
#define IS_PASSABLE(r)     IS_OPEN_DOOR(r)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef uint8_t attrib_t; /* Max attributes = 8 */

struct trv_rect_data_s
{
  trv_coord_t plane;      /* defines the plane that the rect lies in */
  trv_coord_t hstart;     /* defines the starting "horizontal" position */
  trv_coord_t hend;       /* defines the ending "horizontal" position */
  trv_coord_t vstart;     /* defines the starting "vertical" position */
  trv_coord_t vend;       /* defines the ending "vertical" position */
  attrib_t attribute;     /* bit-encoded attributes of the plane */
  uint8_t texture;        /* defines the texture that should be applied */
  uint8_t scale;          /* defines the scaling of the texture */
};
#define RESIZEOF_TRVRECTDATA_T 13

struct trv_rect_list_s
{
  struct trv_rect_list_s *flink; /* points at next rectangle in a list */
  struct trv_rect_list_s *blink; /* points at previous rectangle in a list */
  struct trv_rect_data_s d;      /* the data which defines the rectangle */
};

struct trv_rect_head_s
{
  struct trv_rect_list_s *head;  /* points to the start of the list */
  struct trv_rect_list_s *tail;  /* points to the end of the list */
};

struct trv_planefile_header_s
{
  uint16_t nxrects;
  uint16_t nyrects;
  uint16_t nzrects;
};
#define SIZEOF_TRVPLANEFILEHEADER_T 6

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The is the world!!! The world is described by lists of rectangles, one
 * for each of the X, Y, and Z planes.
 */

extern struct trv_rect_head_s g_xplane;  /* list of X=plane rectangles */
extern struct trv_rect_head_s g_yplane;  /* list of Y=plane rectangles */
extern struct trv_rect_head_s g_zplane;  /* list of Z=plane rectangles */

/* "Deallocated" planes are retained in a free list */

extern struct trv_rect_list_s *g_rect_freelist;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Plane list management */

int trv_initialize_planes(void);
void trv_add_plane(FAR struct trv_rect_list_s *rect,
                   FAR struct trv_rect_head_s *list);
void trv_move_plane(FAR struct trv_rect_list_s *rect,
                    FAR struct trv_rect_head_s *destlist,
                    FAR struct trv_rect_head_s *srclist);
void trv_merge_planelists(FAR struct trv_rect_head_s *outlist,
                          FAR struct trv_rect_head_s *inlist);
void trv_release_planes(void);

/* Plane memory management */

FAR struct trv_rect_list_s *trv_new_plane(void);

/* Plane file management */

int  trv_load_planefile(FAR const char *wldfile);
int  trv_save_planes(const char *wldfile);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_PLANE_H */
