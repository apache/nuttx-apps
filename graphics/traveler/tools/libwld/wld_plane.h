/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_plane.h
 * This file contains definitions for the world planes
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

/*************************************************************************
 * FILE: wld_plane.h
 *************************************************************************/

#ifndef __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PLANE_H
#define __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PLANE_H 1

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*************************************************************************
 * Included Files
 *************************************************************************/

#include <stdint.h>
#include <stdio.h>

/*************************************************************************
 * Pre-processor Definitions
 *************************************************************************/

/* Return codes */

enum
{
  PLANE_SUCCESS = 0,
  PLANE_ALLOCATION_FAILURE = 1,
  PLANE_READ_OPEN_ERROR,
  PLANE_WRITE_OPEN_ERROR,
  PLANE_HEADER_READ_ERROR,
  PLANE_DATA_READ_ERROR,
  PLANE_HEADER_WRITE_ERROR,
  PLANE_DATA_WRITE_ERROR
};

/* These are bit-field definitions for the rect_data_t attribute field
 * STATIC settings
 */

#define SHADED_PLANE       0x01
#define TRANSPARENT_PLANE  0x02
#define DOOR_PLANE         0x04

/* DYNAMIC settings */

#define OPEN_DOOR_PLANE    0x08
#define MOVING_DOOR_PLANE  0x10

/* These macros are used to test the various texture attributes */

#define IS_NORMAL(r)       ( ((r)->attribute & ~SHADED_PLANE) == 0 )
#define IS_SHADED(r)       ( ((r)->attribute & SHADED_PLANE) != 0 )
#define IS_TRANSPARENT(r)  ( ((r)->attribute & TRANSPARENT_PLANE) != 0 )
#define IS_DOOR(r)         ( ((r)->attribute & DOOR_PLANE) != 0 )
#define IS_OPEN_DOOR(r)    ( ((r)->attribute & OPEN_DOOR_PLANE) != 0 )
#define IS_CLOSED_DOOR(r)  ( (IS_DOOR(R)) && (!IS_OPEN_DOOR(r)) )
#define IS_MOVING_DOOR(r)  ( ((r)->attribute & MOVING_DOOR_PLANE) != 0 )
#define IS_PASSABLE(r)     IS_OPEN_DOOR(r)

/* Legal values of texture scaling in rect_data_t */

#define ONEX_SCALING  0
#define TW0X_SCALING  1
#define FOURX_SCALING 2
#define MAXX_SCALING  2

/*************************************************************************
 * Public Type Definitions
 *************************************************************************/

typedef short wld_coord_t;  /* Max world size is +/- 65536/64 = 1024 */
typedef uint8_t attrib_t; /* Max attributes = 8 */

struct rect_data_s
{
  wld_coord_t plane;      /* defines the plane that the rect lies in */
  wld_coord_t hstart;     /* defines the starting "horizontal" position */
  wld_coord_t hend;       /* defines the ending "horizontal" position */
  wld_coord_t vstart;     /* defines the starting "vertical" position */
  wld_coord_t vend;       /* defines the ending "vertical" position */
  attrib_t attribute; /* bit-encoded attributes of the plane */
  uint8_t texture;      /* defines the texture that should be applied */
  uint8_t scale;              /* defines the scaling of the texture */
};

typedef struct rect_data_s rect_data_t;

#define SIZEOF_RECTDATATYPE 13

struct rect_list_s
{
  struct rect_list_s *flink; /* points at next rectangle in a list */
  struct rect_list_s *blink; /* points at previous rectangle in a list */
  rect_data_t d;      /* the data which defines the rectangle */
};

typedef struct rect_list_s rect_list_t;

struct rect_head_s
{
  rect_list_t *head;  /* points to the start of the list */
  rect_list_t *tail;  /* points to the end of the list */
};

typedef struct rect_head_s rect_head_t;

struct plane_file_header_s
{
  uint16_t num_xrects;
  uint16_t num_yrects;
  uint16_t num_zrects;
};

typedef struct plane_file_header_s plane_file_header_t;

#define SIZEOF_PLANEFILEHEADERTYPE 6

/*************************************************************************
 * Public Data
 *************************************************************************/

/* The is the world!!! The world is described by lists of rectangles, one
 * for each of the X, Y, and Z planes.
 */

extern rect_head_t g_xplane_list;  /* list of X=plane rectangles */
extern rect_head_t g_yplane_list;  /* list of Y=plane rectangles */
extern rect_head_t g_zplane_list;  /* list of Z=plane rectangles */

/* This is the maximum value of a texture code */

extern uint8_t maxTextureCode;

/* "Deallocated" planes are retained in a free list */

extern rect_list_t *g_free_planes;

/*************************************************************************
 * Public Function Prototypes
 *************************************************************************/

uint8_t       wld_initialize_planes(void);
void          wld_discard_planes(void);
uint8_t       wld_load_planefile(const char *wldfile);
uint8_t       wld_load_planes(FILE *fp);
uint8_t       wld_save_planes(const char *wldfile);
rect_list_t *wld_new_plane(void);
void          wld_add_plane(rect_list_t *rect,
                            rect_head_t *list);
void          wld_merge_planelists(rect_head_t *outList,
                                   rect_head_t *inList);
void          wld_remove_plane(rect_list_t *rect,
                               rect_head_t *list);
void          wld_move_plane(rect_list_t *rect,
                             rect_head_t *destList,
                             rect_head_t *srcList);
rect_list_t  *wld_find_plane(wld_coord_t h, wld_coord_t v, wld_coord_t plane,
                             rect_head_t *list);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PLANE_H */
