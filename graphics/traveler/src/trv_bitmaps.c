/****************************************************************************
 * apps/graphics/traveler/src/trv_bitmaps.c
 * This file contains low-level texture bitmap logic
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
#include "trv_mem.h"
#include "trv_bitmaps.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* These point to the (allocated) bit map buffers for the even and odd
 * bitmaps
 */

FAR struct trv_bitmap_s *g_even_bitmaps[MAX_BITMAPS];
#ifndef WEDIT
FAR struct trv_bitmap_s *g_odd_bitmaps[MAX_BITMAPS];
#endif

/* This is the maximum value + 1 of a texture code. */

uint16_t g_trv_nbitmaps;

/* These are the colors from the world palette which should used to rend
 * the sky and ground
 */

trv_pixel_t g_sky_color;
trv_pixel_t g_ground_color;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_free_texture
 *
 * Description:
 *   Free both the bitmap and the bitmap container
 *
 ****************************************************************************/

static void trv_free_texture(FAR struct trv_bitmap_s *bitmap)
{
  if (bitmap->bm)
    {
      trv_free(bitmap->bm);
    }

  trv_free(bitmap);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_initialize_bitmaps
 *
 * Description:
 *
 ****************************************************************************/

int trv_initialize_bitmaps(void)
{
  int i;

  for (i = 0; i < MAX_BITMAPS; i++)
    {
      g_even_bitmaps[i] = NULL;
#ifndef WEDIT
      g_odd_bitmaps[i]  = NULL;
#endif
    }

  g_trv_nbitmaps = 0;
  return OK;
}

/****************************************************************************
 * Name: trv_release_bitmaps
 *
 * Description:
 *   This function deallocates all bitmaps.
 *
 ****************************************************************************/

void trv_release_bitmaps(void)
{
  int i;

  for (i = 0; i < MAX_BITMAPS; i++)
    {
      if (g_even_bitmaps[i])
        {
          trv_free_texture(g_even_bitmaps[i]);
          g_even_bitmaps[i] = NULL;
        }

#ifndef WEDIT
      if (g_odd_bitmaps[i])
        {
          trv_free_texture(g_odd_bitmaps[i]);
          g_odd_bitmaps[i] = NULL;
        }
#endif
    }

  g_trv_nbitmaps = 0;
}
