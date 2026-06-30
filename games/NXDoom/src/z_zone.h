/****************************************************************************
 * apps/games/NXDoom/src/z_zone.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *  Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
 *
 *  Remark: this was the only stuff that, according to John Carmack, might
 *  have been useful for Quake.
 *
 ****************************************************************************/

#ifndef __Z_ZONE__
#define __Z_ZONE__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* This is used to get the local FILE:LINE info from CPP prior to really call
 * the function in question.
 */

#define z_change_tag(p, t) z_change_tag2((p), (t), __FILE__, __LINE__)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* ZONE MEMORY
 * PU - purge tags.
 */

enum
{
  PU_STATIC = 1, /* static entire execution time */
  PU_SOUND,      /* static while playing */
  PU_MUSIC,      /* static while playing */
  PU_FREE,       /* a free block */
  PU_LEVEL,      /* static until level exited */
  PU_LEVSPEC,    /* a special thinker in a level */

  /* Tags >= PU_PURGELEVEL are purgeable whenever needed. */

  PU_PURGELEVEL,
  PU_CACHE,

  /* Total number of different tag types */

  PU_NUM_TAGS
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void z_init(void);
void *z_malloc(int size, int tag, void *ptr);
void z_free(void *ptr);
void z_free_tags(int lowtag, int hightag);
void z_dump_heap(int lowtag, int hightag);
void z_file_dump_heap(FILE *f);
void z_check_heap(void);
void z_change_tag2(void *ptr, int tag, const char *file, int line);
void z_change_user(void *ptr, void **user);
int z_free_memory(void);
unsigned int z_zone_size(void);

#endif /* __Z_ZONE__ */
