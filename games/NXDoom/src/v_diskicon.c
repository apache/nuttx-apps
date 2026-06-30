/****************************************************************************
 * apps/games/NXDoom/src/v_diskicon.c
 *
 * SPDX-License-Identifier: GPLv2
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
 *  Disk load indicator.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "deh_str.h"
#include "doomtype.h"
#include "i_swap.h"
#include "i_video.h"
#include "m_argv.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "v_diskicon.h"

#include <string.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Only display the disk icon if more then this much bytes have been read
 * during the previous tic.
 */

static const int diskicon_threshold = 20 * 1024;

/* Two buffers: disk_data contains the data representing the disk icon
 * (raw, not a patch_t) while saved_background is an equivalently-sized
 * buffer where we save the background data while the disk is on screen.
 */

static pixel_t *disk_data;
static pixel_t *saved_background;

static int loading_disk_xoffs = 0;
static int loading_disk_yoffs = 0;

/* Number of bytes read since the last call to v_draw_disk_icon(). */

static size_t recent_bytes_read = 0;
static boolean disk_drawn;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void copy_region(pixel_t *dest, int dest_pitch, pixel_t *src,
                        int src_pitch, int w, int h)
{
  pixel_t *s, *d;
  int y;

  s = src;
  d = dest;
  for (y = 0; y < h; ++y)
    {
      memcpy(d, s, w * sizeof(*d));
      s += src_pitch;
      d += dest_pitch;
    }
}

static void save_disk_data(const char *disk_lump, int xoffs, int yoffs)
{
  pixel_t *tmpscreen;
  patch_t *disk;

  /* Allocate a complete temporary screen where we'll draw the patch. */

  tmpscreen = z_malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(*tmpscreen),
                       PU_STATIC, NULL);
  memset(tmpscreen, 0, SCREENWIDTH * SCREENHEIGHT * sizeof(*tmpscreen));
  v_use_buffer(tmpscreen);

  /* Buffer where we'll save the disk data. */

  if (disk_data != NULL)
    {
      z_free(disk_data);
      disk_data = NULL;
    }

  disk_data = z_malloc(LOADING_DISK_W * LOADING_DISK_H * sizeof(*disk_data),
                       PU_STATIC, NULL);

  /* Draw the patch and save the result to disk_data. */

  disk = w_cache_lump_name(disk_lump, PU_STATIC);
  v_draw_patch(loading_disk_xoffs, loading_disk_yoffs, disk);
  copy_region(disk_data, LOADING_DISK_W,
              tmpscreen + yoffs * SCREENWIDTH + xoffs, SCREENWIDTH,
              LOADING_DISK_W, LOADING_DISK_H);
  w_release_lump_name(disk_lump);

  v_restore_buffer();
  z_free(tmpscreen);
}

static pixel_t *disk_region_pointer(void)
{
  return i_video_buffer + loading_disk_yoffs * SCREENWIDTH +
         loading_disk_xoffs;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void v_enable_loading_disk(const char *lump_name, int xoffs, int yoffs)
{
  loading_disk_xoffs = xoffs;
  loading_disk_yoffs = yoffs;

  if (saved_background != NULL)
    {
      z_free(saved_background);
      saved_background = NULL;
    }

  saved_background =
      z_malloc(LOADING_DISK_W * LOADING_DISK_H * sizeof(*saved_background),
               PU_STATIC, NULL);
  save_disk_data(lump_name, xoffs, yoffs);
}

void v_begin_read(size_t nbytes)
{
  recent_bytes_read += nbytes;
}

void v_draw_disk_icon(void)
{
  if (disk_data != NULL && recent_bytes_read > diskicon_threshold)
    {
      /* Save the background behind the disk before we draw it. */

      copy_region(saved_background, LOADING_DISK_W, disk_region_pointer(),
                  SCREENWIDTH, LOADING_DISK_W, LOADING_DISK_H);

      /* Write the disk to the screen buffer. */

      copy_region(disk_region_pointer(), SCREENWIDTH, disk_data,
                  LOADING_DISK_W, LOADING_DISK_W, LOADING_DISK_H);
      disk_drawn = true;
    }

  recent_bytes_read = 0;
}

void v_restore_disk_background(void)
{
  if (disk_drawn)
    {
      /* Restore the background. */

      copy_region(disk_region_pointer(), SCREENWIDTH, saved_background,
                  LOADING_DISK_W, LOADING_DISK_W, LOADING_DISK_H);

      disk_drawn = false;
    }
}
