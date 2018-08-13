/****************************************************************************
 * apps/graphics/traveler/src/trv_rayrend.c
 * This file contains the functions needed to render a screen.
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
#include "trv_debug.h"
#include "trv_world.h"
#include "trv_plane.h"
#include "trv_bitmaps.h"
#include "trv_graphics.h"
#include "trv_paltable.h"
#include "trv_trigtbl.h"
#include "trv_raycntl.h"
#include "trv_raycast.h"
#include "trv_rayrend.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* These are sometimes useful during debug */

#define DISABLE_WALL_RENDING  0
#define DISABLE_FLOOR_RENDING 0

/* The following macros perform division (using g_invsize[]) and then a
 * rescaling by the approprate constants so that the texture index is
 * byte aligned
 */

#define TDIV(num,den,s) (((num) * g_invsize[den]) >> (s))

/* This macro just performs the division and leaves the result with eight
 * (more) bits of fraction
 */

#define DIV8(num,den)  ((num) * g_invsize[den])

/* The following macro aligns a SMALL precision number to that the texture
 * index is byte aligned
 */

#define TALIGN(x,s) ((x) << (8-(s)))

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/* This union is used to manage texture indices */

union tex_ndx_u
{
  struct
  {
    uint8_t f;
    uint8_t i;
  } s;
  int16_t w;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void trv_rend_zcell(uint8_t row, uint8_t col, uint8_t height,
                           uint8_t width);
static void trv_rend_zrow(uint8_t row, uint8_t col, uint8_t width);
static void trv_rend_zcol(uint8_t row, uint8_t col, uint8_t height);
static void trv_rend_zpixel(uint8_t row, uint8_t col);

static void trv_rend_wall(uint8_t row, uint8_t col, uint8_t height,
                          uint8_t width);
static void trv_rend_wallrow(uint8_t row, uint8_t col, uint8_t width);
static void trv_rend_wallcol(uint8_t row, uint8_t col, uint8_t height);
static void trv_rend_wallpixel(uint8_t row, uint8_t col);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The following array simply contains the inverted values of the integers
 * from 0...VGULP_SIZE-1.  The values in the table have 8 bits of fraction.
 * The value for 0 is bogus!
 */

static const uint8_t g_invsize[VGULP_SIZE] =
{
  0xff, 0xff, 0x80, 0x55, 0x40, 0x33, 0x2b, 0x25,  /* 0..7 */
  0x20, 0x1c, 0x1a, 0x17, 0x15, 0x14, 0x12, 0x11   /* 8..15 */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_rend_zcell, trv_rend_zrow, trv_rend_zcol, trv_rend_zpixel
 *
 * Description:
 *   After matching hits have been obtained in all four corners of a cell,
 *   this function interpolates to complete the cell then transfers the cell
 *   to the double buffer.  These are the general cases where no assumptions
 *   are made concerning the relative orientation of the cell and the texture.
 *   This general case is only used to texture floors and ceilings.
 *
 ****************************************************************************/

/* This version is for non-degenerate cell, i.e., height>1 and width>1 */

static void trv_rend_zcell(uint8_t row, uint8_t col, uint8_t height, uint8_t width)
{
#if (!DISABLE_FLOOR_RENDING)
  uint8_t i;
  uint8_t j;
  uint8_t endrow;
  uint8_t endcol;
  FAR uint8_t *palptr;
  FAR uint8_t *outpixel;
  uint8_t scale;
  FAR trv_pixel_t *texture;
  FAR struct trv_bitmap_s *bmp;
  union tex_ndx_u xpos;
  union tex_ndx_u ypos;
  uint16_t tmask;
  uint16_t tsize;
  int16_t zone;
  int16_t endzone;
  int16_t zonestep;
  int16_t hstart;
  int16_t hrowstep;
  int16_t hcolstep;
  int16_t hstepincr;
  int16_t vstart;
  int16_t vrowstep;
  int16_t vcolstep;
  int16_t vstepincr;
  int16_t tmpcolstep;

  /* Displace the double buffer pointer */

  outpixel = &g_buffer_row[row][g_cell_column];

  /* Point to the bitmap associated with the upper left pixel.  Since
   * all of the pixels in this cell are the same "hit," we don't have
   * to recalculate this
   */

  if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
      bmp = g_even_bitmaps[g_ray_hit[row][col].rect->texture];
    }
  else
    {
      bmp = g_odd_bitmaps[g_ray_hit[row][col].rect->texture];
    }

  /* Get parameters associated with the size of the bitmap texture */

  texture = bmp->bm;
  tsize = bmp->log2h;
  tmask = TMASK(tsize);

  /* Extract the texture scaling from the rectangle structure */

  scale = g_ray_hit[row][col].rect->scale;

  /* Within this function, all references to height and width are really
   * (height-1) and (width-1)
   */

  height--;
  width--;

  /* Get the indices to the lower, right corner */

  endcol = col + width;
  endrow = row + height;

  /* Calculate the horizontal interpolation values */
  /* This is the H starting position (first row, first column) */

  hstart = TALIGN(g_ray_hit[row][col].xpos, scale);

  /* This is the change in xpos per column in the first row */

  hcolstep =
    TDIV((g_ray_hit[row][endcol].xpos - g_ray_hit[row][col].xpos),
      width, scale);

  /* This is the change in xpos per column in the last row */

  tmpcolstep =
    TDIV((g_ray_hit[endrow][endcol].xpos - g_ray_hit[endrow][col].xpos),
      width, scale);

  /* This is the change in hcolstep per row */

  hstepincr = (TDIV((tmpcolstep - hcolstep), height, scale) >> 8);

  /* This is the change in hstart for each row */

  hrowstep =
    TDIV((g_ray_hit[endrow][col].xpos - g_ray_hit[row][col].xpos),
      height, scale);

  /* Calculate the vertical interpolation values */
  /* This is the V starting position (first row, first column) */

  vstart = TALIGN(g_ray_hit[row][col].ypos, scale);

  /* This is the change in ypos per column in the first row */

  vcolstep =
    TDIV((g_ray_hit[row][endcol].ypos - g_ray_hit[row][col].ypos),
      width, scale);

  /* This is the change in ypos per column in the last row */

  tmpcolstep =
    TDIV((g_ray_hit[endrow][endcol].ypos - g_ray_hit[endrow][col].ypos),
      width, scale);

  /* This is the change in vcolstep per row */

  vstepincr = (TDIV((tmpcolstep - vcolstep), height, scale) >> 8);

  /* This is the change in vstart for each row */

  vrowstep =
    TDIV((g_ray_hit[endrow][col].ypos - g_ray_hit[row][col].ypos),
      height, scale);

  /* Determine the palette mapping table zone for each row */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_FZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist, 8);
      endzone = GET_FZONE(g_ray_hit[endrow][col].xdist, g_ray_hit[endrow][col].ydist, 8);
      zonestep = (DIV8((endzone - zone), height) >> 8);
    }
  else
    {
      zone = 0;
      zonestep = 0;
    }

  /* Now, interpolate to texture each row (vertical component) */

  for (i = row; i <= endrow; i++)
    {
      /* Set the initial horizontal & vertical offset */

      xpos.w = hstart;
      ypos.w = vstart;

      /* Get the palette map to use on this row */

      palptr = GET_PALPTR((zone >> 8));

      /* Interpolate to texture each column in the row */

      for (j = col; j <= endcol; j++)
        {
          /* Transfer the pixel at this interpolated position */

          outpixel[j] = palptr[texture[TNDX(xpos.s.i, ypos.s.i, tsize, tmask)]];

          /* Now Calculate the horizontal position for the next step */

          xpos.w += hcolstep;
          ypos.w += vcolstep;
        }

      /* Point to the next row */

      outpixel += TRV_SCREEN_WIDTH;

      /* Calculate the vertical position for the next step */

      hstart += hrowstep;
      vstart += vrowstep;

      /* Adjust the step sizes to use on the next row */

      hcolstep += hstepincr;
      vcolstep += vstepincr;

      /* Get the zone to use on the next row */

      zone += zonestep;
    }
#endif
}

/* This version is for horizontal lines, i.e., height==1 and width>1 */

static void trv_rend_zrow(uint8_t row, uint8_t col, uint8_t width)
{
#if (!DISABLE_FLOOR_RENDING)
  uint8_t j;
  uint8_t endcol;
  FAR uint8_t *palptr;
  FAR uint8_t *outpixel;
  uint8_t scale;
  FAR trv_pixel_t *texture;
  FAR struct trv_bitmap_s *bmp;
  union tex_ndx_u xpos;
  union tex_ndx_u ypos;
  uint16_t tmask;
  uint16_t tsize;
  int16_t zone;
  int16_t hcolstep;
  int16_t vcolstep;

  /* Displace the double buffer pointer */

  outpixel = &g_buffer_row[row][g_cell_column];

  /* Point to the bitmap associated with the left pixel.  Since
   * all of the pixels in this row are the same "hit," we don't have
   * to recalculate this
   */

   if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
      bmp = g_even_bitmaps[g_ray_hit[row][col].rect->texture];
    }
  else
    {
      bmp = g_odd_bitmaps[g_ray_hit[row][col].rect->texture];
    }

  /* Get parameters associated with the size of the bitmap texture */

  texture = bmp->bm;
  tsize = bmp->log2h;
  tmask = TMASK(tsize);

  /* Extract the texture scaling from the rectangle structure */

  scale = g_ray_hit[row][col].rect->scale;

  /* Get the a pointer to the palette mapping table */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_ZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist);
      palptr = GET_PALPTR(zone);
    }
  else
    {
      palptr = GET_PALPTR(0);
    }

  /* Within this function, all references to width are really
   * (width-1)
   */

  width--;

  /* Get the index to the right side */

  endcol = col + width;

  /* Calculate the horizontal interpolation values */
  /* This is the H starting position (first column) */

  xpos.w = TALIGN(g_ray_hit[row][col].xpos, scale);

  /* This is the change in xpos per column */

  hcolstep =
    TDIV((g_ray_hit[row][endcol].xpos - g_ray_hit[row][col].xpos),
      width, scale);

  /* Calculate the vertical interpolation values */
  /* This is the V starting position (first column) */

  ypos.w = TALIGN(g_ray_hit[row][col].ypos, scale);

  /* This is the change in ypos per column */

  vcolstep =
    TDIV((g_ray_hit[row][endcol].ypos - g_ray_hit[row][col].ypos),
      width, scale);

  /* Interpolate to texture each column in the row */

  for (j = col; j <= endcol; j++)
    {
      /* Transfer the pixel at this interpolated position */

      outpixel[j] = palptr[texture[TNDX(xpos.s.i, ypos.s.i, tsize, tmask)]];

      /* Now Calculate the horizontal position for the next step */

      xpos.w += hcolstep;
      ypos.w += vcolstep;
   }
#endif
}

/* This version is for vertical lines, i.e., height>1 and width==1 */

static void trv_rend_zcol(uint8_t row, uint8_t col, uint8_t height)
{
#if (!DISABLE_FLOOR_RENDING)
  uint8_t i, endrow;
  FAR uint8_t *palptr;
  FAR uint8_t *outpixel;
  uint8_t scale;
  FAR trv_pixel_t *texture;
  FAR struct trv_bitmap_s *bmp;
  union tex_ndx_u xpos;
  union tex_ndx_u ypos;
  uint16_t tmask;
  uint16_t tsize;
  int16_t zone;
  int16_t hrowstep;
  int16_t vrowstep;

  /* Displace the double buffer pointer */

  outpixel = &g_buffer_row[row][g_cell_column+col];

  /* Point to the bitmap associated with the upper pixel.  Since
   * all of the pixels in this column are the same "hit," we don't have
   * to recalculate this
   */

   if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
      bmp = g_even_bitmaps[g_ray_hit[row][col].rect->texture];
    }
  else
    {
      bmp = g_odd_bitmaps[g_ray_hit[row][col].rect->texture];
    }

  /* Get parameters associated with the size of the bitmap texture */

  texture = bmp->bm;
  tsize = bmp->log2h;
  tmask = TMASK(tsize);

  /* Extract the texture scaling from the rectangle structure */

  scale = g_ray_hit[row][col].rect->scale;

  /* Get the a pointer to the palette mapping table */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_ZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist);
      palptr = GET_PALPTR(zone);
    }
  else
    {
      palptr = GET_PALPTR(0);
    }

  /* Within this function, all references to height are really
   * (height-1)
   */

   height--;

  /* Get the indices to the lower end */

  endrow = row + height;

  /* Calculate the horizontal interpolation values */
  /* This is the H starting position (first row) */

  xpos.w = TALIGN(g_ray_hit[row][col].xpos, scale);

  /* This is the change in xpos for each row */

  hrowstep =
    TDIV((g_ray_hit[endrow][col].xpos - g_ray_hit[row][col].xpos),
      height, scale);

  /* Calculate the vertical interpolation values */
  /* This is the V starting position (first row) */

  ypos.w = TALIGN(g_ray_hit[row][col].ypos, scale);

  /* This is the change in ypos for each row */

  vrowstep =
    TDIV((g_ray_hit[endrow][col].ypos - g_ray_hit[row][col].ypos),
      height, scale);

  /* Now, interpolate to texture each row (vertical component) */

  for (i = row; i <= endrow; i++)
    {
      /* Transfer the pixel at this interpolated position */

      *outpixel = palptr[texture[TNDX(xpos.s.i, ypos.s.i, tsize, tmask)]];

      /* Point to the next row */

      outpixel += TRV_SCREEN_WIDTH;

      /* Calculate the vertical position for the next step */

      xpos.w += hrowstep;
      ypos.w += vrowstep;
    }
#endif
}

/* This version is for a single pixel, i.e., height==1 and width==1 */

static void trv_rend_zpixel(uint8_t row, uint8_t col)
{
#if (!DISABLE_FLOOR_RENDING)
  FAR uint8_t *palptr;
  FAR trv_pixel_t *texture;
  FAR struct trv_bitmap_s *bmp;
  uint16_t tmask;
  uint16_t tsize;
  int16_t zone;

  /* Get the a pointer to the palette mapping table */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_ZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist);
      palptr = GET_PALPTR(zone);
    }
  else
    {
      palptr = GET_PALPTR(0);
    }

  /* Point to the bitmap associated with the upper left pixel. */

  if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
    bmp = g_even_bitmaps[g_ray_hit[row][col].rect->texture];
    }
  else
    {
    bmp = g_odd_bitmaps[g_ray_hit[row][col].rect->texture];
    }

  /* Get parameters associated with the size of the bitmap texture */

  texture = bmp->bm;
  tsize = bmp->log2h;
  tmask = TMASK(tsize);

  g_buffer_row[row][g_cell_column+col] =
    palptr[texture[TNDX(g_ray_hit[row][col].xpos, g_ray_hit[row][col].ypos,
                        tsize, tmask)]];
#endif
}

/****************************************************************************
 * Name: trv_rend_wall, trv_rend_wallrow, trv_rend_wallcol, diplayWallPixel
 *
 * Description:
 *   After matching hits have been obtained in all four corners of a cell,
 *   this function interpolates to complete the cell then transfers the cell
 *   to the double buffer.  These special simplifications for use on on
 *   vertical (X or Y) walls.  In this case, we can assume that:
 *
 *     g_ray_hit[row][col].xpos == g_ray_hit[row+height-1][col]
 *     g_ray_hit[row][col+width-1].xpos == g_ray_hit[row+height-1][col+width-1]
 *
 *   In addition to these simplifications, these functions include the
 *   added complications of handling internal INVISIBLE_PIXELs which may
 *   occur within TRANSPARENT_WALLs.
 *
 ****************************************************************************/

/* This version is for non-degenerate cell, i.e., height>1 and width>1 */

static void trv_rend_wall(uint8_t row, uint8_t col,
                          uint8_t height, uint8_t width)
{
#if (!DISABLE_WALL_RENDING)
  uint8_t i, j;
  uint8_t endrow;
  uint8_t endcol;
  uint8_t inpixel;
  FAR uint8_t *palptr;
  FAR uint8_t *outpixel;
  uint8_t scale;
  FAR trv_pixel_t *texture;
  FAR struct trv_bitmap_s *bmp;
  union tex_ndx_u xpos;
  union tex_ndx_u ypos;
  uint16_t tmask;
  uint16_t tsize;
  int16_t zone;
  int16_t hstart;
  int16_t hcolstep;
  int16_t vstart;
  int16_t vrowstep;
  int16_t vcolstep;
  int16_t vstepincr;
  int16_t tmpcolstep;

  /* Displace the double buffer pointer */

  outpixel = &g_buffer_row[row][g_cell_column];

  /* Point to the bitmap associated with the upper left pixel.  Since
   * all of the pixels in this cell are the same "hit," we don't have
   * to recalculate this
   */

  if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
      bmp = g_even_bitmaps[g_ray_hit[row][col].rect->texture];
    }
  else
    {
      bmp = g_odd_bitmaps[g_ray_hit[row][col].rect->texture];
    }

  /* Get parameters associated with the size of the bitmap texture */

  texture = bmp->bm;
  tsize = bmp->log2h;
  tmask = TMASK(tsize);

  /* Extract the texture scaling from the rectangle structure */

  scale = g_ray_hit[row][col].rect->scale;

  /* Get the a pointer to the palette mapping table */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_ZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist);
      palptr = GET_PALPTR(zone);
    }
  else
    {
      palptr = GET_PALPTR(0);
    }

  /* Within this function, all references to height and width are really
   * (height-1) and (width-1)
   */

  height--;
  width--;

  /* Get the indices to the lower, right corner */

  endcol = col + width;
  endrow = row + height;

  /* Calculate the horizontal interpolation values */
  /* This is the H starting position (first row, first column) */

  hstart = TALIGN(g_ray_hit[row][col].xpos, scale);

  /* This is the change in xpos per column in the first row */

  hcolstep =
    TDIV((g_ray_hit[row][endcol].xpos - g_ray_hit[row][col].xpos),
      width, scale);

  /* Calculate the vertical interpolation values */
  /* This is the V starting position (first row, first column) */

  vstart = TALIGN(g_ray_hit[row][col].ypos, scale);

  /* This is the change in ypos per column in the first row */

  vcolstep =
    TDIV((g_ray_hit[row][endcol].ypos - g_ray_hit[row][col].ypos),
      width, scale);

  /* This is the change in ypos per column in the last row */

  tmpcolstep =
    TDIV((g_ray_hit[endrow][endcol].ypos - g_ray_hit[endrow][col].ypos),
      width, scale);

  /* This is the change in vcolstep per row */

  vstepincr = (TDIV((tmpcolstep - vcolstep), height, scale) >> 8);

  /* This is the change in vstart for each row */

  vrowstep =
    TDIV((g_ray_hit[endrow][col].ypos - g_ray_hit[row][col].ypos),
      height, scale);

  /* Now, interpolate to texture each row (vertical component) */

  for (i = row; i <= endrow; i++)
    {
      /* Set the initial horizontal & vertical offset */

      xpos.w = hstart;
      ypos.w = vstart;

      /* Interpolate to texture each column in the row */

      for (j = col; j <= endcol; j++)
        {
          /* Extract the pixel from the texture */

          inpixel = texture[TNDX(xpos.s.i, ypos.s.i, tsize, tmask)];

          /* If this is an INVISIBLE_PIXEL in a TRANSPARENT_WALL, then
           * we will have to take some pretty extreme measures to get the
           * correct value of the pixel
           */

          if ((inpixel == INVISIBLE_PIXEL) &&
              (IS_TRANSPARENT(g_ray_hit[row][col].rect)))
            {
              /* Check if we hit anything */

              if ((inpixel = trv_get_texture(i, j)) != INVISIBLE_PIXEL)
                {
                  /* Map the normal pixel and transfer the pixel at this
                   * interpolated position
                   */

                  outpixel[j] = inpixel;
                }
            }
          else
            {
              /* Map the normal pixel and transfer the pixel at this
               * interpolated position
               */

              outpixel[j] = palptr[inpixel];
            }

          /* Now Calculate the horizontal position for the next step */

          xpos.w += hcolstep;
          ypos.w += vcolstep;
        }

      /* Point to the next row */

      outpixel += TRV_SCREEN_WIDTH;

      /* Calculate the vertical position for the next step */

      vstart += vrowstep;

      /* Adjust the step sizes to use on the next row */

      vcolstep += vstepincr;
    }
#endif
}

/* This version is for horizontal lines, i.e., height==1 and width>1 */

static void trv_rend_wallrow(uint8_t row, uint8_t col, uint8_t width)
{
#if (!DISABLE_WALL_RENDING)
  uint8_t j;
  uint8_t endcol;
  uint8_t inpixel;
  FAR uint8_t *palptr;
  FAR uint8_t *outpixel;
  uint8_t scale;
  FAR trv_pixel_t *texture;
  FAR struct trv_bitmap_s *bmp;
  union tex_ndx_u xpos;
  union tex_ndx_u ypos;
  uint16_t tmask;
  uint16_t tsize;
  int16_t zone;
  int16_t hcolstep;
  int16_t vcolstep;

  /* Displace the double buffer pointer */

  outpixel = &g_buffer_row[row][g_cell_column];

  /* Point to the bitmap associated with the left pixel.  Since
   * all of the pixels in this row are the same "hit," we don't have
   * to recalculate this
   */

  if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
      bmp = g_even_bitmaps[g_ray_hit[row][col].rect->texture];
    }
  else
    {
      bmp = g_odd_bitmaps[g_ray_hit[row][col].rect->texture];
    }

  /* Get parameters associated with the size of the bitmap texture */

  texture = bmp->bm;
  tsize = bmp->log2h;
  tmask = TMASK(tsize);

  /* Extract the texture scaling from the rectangle structure */

  scale = g_ray_hit[row][col].rect->scale;

  /* Get the a pointer to the palette mapping table */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_ZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist);
      palptr = GET_PALPTR(zone);
    }
  else
    {
      palptr = GET_PALPTR(0);
    }

  /* Within this function, all references to width are really
   * (width-1)
   */

  width--;

  /* Get the index to the right side */

  endcol = col + width;

  /* Calculate the horizontal interpolation values */
  /* This is the H starting position (first column) */

  xpos.w = TALIGN(g_ray_hit[row][col].xpos, scale);

  /* This is the change in xpos per column */

  hcolstep =
    TDIV((g_ray_hit[row][endcol].xpos - g_ray_hit[row][col].xpos),
      width, scale);

  /* Calculate the vertical interpolation values */
  /* This is the V starting position (first column) */

  ypos.w = TALIGN(g_ray_hit[row][col].ypos, scale);

  /* This is the change in ypos per column */

  vcolstep =
    TDIV((g_ray_hit[row][endcol].ypos - g_ray_hit[row][col].ypos),
      width, scale);

  /* Interpolate to texture each column in the row */

  for (j = col; j <= endcol; j++)
    {
      /* Extract the pixel from the texture */

      inpixel = texture[TNDX(xpos.s.i, ypos.s.i, tsize, tmask)];

      /* If this is an INVISIBLE_PIXEL in a TRANSPARENT_WALL, then
       * we will have to take some pretty extreme measures to get the
       * correct value of the pixel
       */

      if ((inpixel == INVISIBLE_PIXEL) &&
          (IS_TRANSPARENT(g_ray_hit[row][col].rect)))
        {
          /* Cast another ray and see if we hit anything */

          if ((inpixel = trv_get_texture(row, j)) != INVISIBLE_PIXEL)
            {
              /* Map the normal pixel and transfer the pixel at this
               * interpolated position
               */

              outpixel[j] = inpixel;
            }
        }
      else
        {
          /* Map the normal pixel and transfer the pixel at this
           * interpolated position
           */

          outpixel[j] = palptr[inpixel];
        }

      /* Now Calculate the horizontal position for the next step */

      xpos.w += hcolstep;
      ypos.w += vcolstep;
    }
#endif
}

/* This version is for vertical line, i.e., height>1 and width==1 */

static void trv_rend_wallcol(uint8_t row, uint8_t col, uint8_t height)
{
#if (!DISABLE_WALL_RENDING)
  uint8_t i;
  uint8_t endrow;
  uint8_t inpixel;
  FAR uint8_t *palptr;
  FAR uint8_t *outpixel;
  uint8_t scale;
  FAR trv_pixel_t *texture;
  FAR struct trv_bitmap_s *bmp;
  union tex_ndx_u ypos;
  uint16_t tmask;
  uint16_t tsize;
  int16_t zone;
  int16_t xpos;
  int16_t vrowstep;

  /* Displace the double buffer pointer */

  outpixel = &g_buffer_row[row][g_cell_column+col];

  /* Point to the bitmap associated with the upper pixel.  Since
   * all of the pixels in this cell are the same "hit," we don't have
   * to recalculate this
   */

  if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
      bmp = g_even_bitmaps[g_ray_hit[row][col].rect->texture];
    }
  else
    {
      bmp = g_odd_bitmaps[g_ray_hit[row][col].rect->texture];
    }

  /* Get parameters associated with the size of the bitmap texture */

  texture = bmp->bm;
  tsize = bmp->log2h;
  tmask = TMASK(tsize);

  /* Extract the texture scaling from the rectangle structure */

  scale = g_ray_hit[row][col].rect->scale;

  /* Get the a pointer to the palette mapping table */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_ZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist);
      palptr = GET_PALPTR(zone);
    }
  else
    {
      palptr = GET_PALPTR(0);
    }

  /* Within this function, all references to height are really
   * (height-1)
   */

  height--;

  /* Get the indices to the lower end */

  endrow = row + height;

  /* Calculate the horizontal interpolation values */

  xpos = sFRAC(g_ray_hit[row][col].xpos >> scale);

  /* Calculate the vertical interpolation values */
  /* This is the V starting position (first row, first column) */

  ypos.w = TALIGN(g_ray_hit[row][col].ypos, scale);

  /* This is the change in ypos for each row */

  vrowstep =
    TDIV((g_ray_hit[endrow][col].ypos - g_ray_hit[row][col].ypos),
      height, scale);

  /* Now, interpolate to texture the vertical line */

  for (i = row; i <= endrow; i++)
    {
      /* Extract the pixel from the texture */

      inpixel = texture[TNDX(xpos, ypos.s.i, tsize, tmask)];

      /* If this is an INVISIBLE_PIXEL in a TRANSPARENT_WALL, then
       * we will have to take some pretty extreme measures to get the
       * correct value of the pixel
       */

      if ((inpixel == INVISIBLE_PIXEL) &&
          (IS_TRANSPARENT(g_ray_hit[row][col].rect)))
        {
          /* Check if we hit anything */

          if ((inpixel = trv_get_texture(i, col)) != INVISIBLE_PIXEL)
            {
              /* Map the normal pixel and transfer the pixel at this
               * interpolated position
               */

              *outpixel = inpixel;
            }
        }
      else
        {
          /* Map the normal pixel and transfer the pixel at this
           * interpolated position
           */

          *outpixel = palptr[inpixel];
        }

      /* Point to the next row */

      outpixel += TRV_SCREEN_WIDTH;

      /* Calculate the vertical position for the next step */

      ypos.w += vrowstep;
    }
#endif
}

/* This version is for a single pixel, i.e., height==1 and width==1 */

static void trv_rend_wallpixel(uint8_t row, uint8_t col)
{
#if (!DISABLE_WALL_RENDING)
  uint8_t *palptr;
  int16_t zone;

  /* Get the a pointer to the palette mapping table */

  if (IS_SHADED(g_ray_hit[row][col].rect))
    {
      zone = GET_ZONE(g_ray_hit[row][col].xdist, g_ray_hit[row][col].ydist);
      palptr = GET_PALPTR(zone);
    }
  else
    {
      palptr = GET_PALPTR(0);
    }

  /* The map and transfer the pixel to the display buffer */

  if (IS_FRONT_HIT(&g_ray_hit[row][col]))
    {
      g_buffer_row[row][g_cell_column+col] =
        palptr[GET_FRONT_PIXEL(g_ray_hit[row][col].rect,
                               g_ray_hit[row][col].xpos,
                               g_ray_hit[row][col].ypos)];
    }
  else
    {
      g_buffer_row[row][g_cell_column+col] =
        palptr[GET_BACK_PIXEL(g_ray_hit[row][col].rect,
                              g_ray_hit[row][col].xpos,
                              g_ray_hit[row][col].ypos)];
    }
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_rend_backdrop
 * Description:
 *
 *  Clear the screen and draw the sky and ground using 32 bit transfers.
 *
 ****************************************************************************/

void trv_rend_backdrop(FAR struct trv_camera_s *camera,
                       FAR struct trv_graphics_info_s *ginfo)
{
#ifdef ENABLE_VIDEO
  FAR uint32_t *dest;
  uint32_t plotpixels;
  int32_t skyrows;
  uint16_t buffersize;
  uint16_t n;
  int16_t pitchoffset;

  /* The destination of the transfer is the screen buffer */

  dest = (uint32_t*)ginfo->swbuffer;

  /* Convert the pitch angle into signed screen offset */

  if ((pitchoffset = camera->pitch) > ANGLE_90)
    {
      pitchoffset -= ANGLE_360;
    }

  /* Determine the size of the "sky" buffer in rows.  Positive pitchoffset
   * means we are looking up.  In this case the sky gets bigger.
   */

  skyrows = WINDOW_MIDDLE + pitchoffset;

  /* Handle the case where we are looking down and do not see any of the
   * sky
   */

  if (skyrows <= 0)
    {
      /* No sky rows -- No sky buffersize */

      skyrows = buffersize = 0;
    }

  /* Copy the sky color into the top half of the screen */

  else
    {
      /* Handle the case where we are looking up and see only the sky */

      if (skyrows > WINDOW_HEIGHT)
        {
          skyrows = WINDOW_HEIGHT;
        }

      /* Determine the size of the "sky" buffer in 32-bit units */

      buffersize = (TRV_SCREEN_WIDTH/4) * skyrows;

      /* Combine the sky color into 32-bit "quadruple pixels" */

      plotpixels = (((uint32_t)g_sky_color << 24) |
                    ((uint32_t)g_sky_color << 16) |
                    ((uint32_t)g_sky_color << 8) |
                     (uint32_t)g_sky_color);

      /* Then transfer the "sky" */

      for (n = 0; n < buffersize; n++)
        {
          *dest++ = plotpixels;
        }
    }

  /* Copy the ground color into the bottom half of the screen */

  if (skyrows < WINDOW_HEIGHT)
    {
      /* Determine the size of the "ground" buffer in 32-bit units */

      buffersize = (TRV_SCREEN_WIDTH/4) * (WINDOW_HEIGHT - skyrows);

      /* Combine the ground color into 32-bit "quadruple pixels" */

      plotpixels = (((uint32_t)g_ground_color << 24) |
                    ((uint32_t)g_ground_color << 16) |
                    ((uint32_t)g_ground_color << 8) |
                     (uint32_t)g_ground_color);

      /* Then transfer the "ground" */

      for (n = 0; n < buffersize; n++)
        {
          *dest++ = plotpixels;
        }
    }
#endif
}

/****************************************************************************
 * Name: trv_rend_cell, trv_rend_row, trv_rend_column, trv_rend_pixel
 *
 * Description:
 *   After matching hits have been obtained in all four corners of a cell,
 *   this function interpolates to complete the cell then transfers the cell
 *   to the double buffer.
 *
 ****************************************************************************/

/* This version is for non-degenerate cell, i.e., height>1 and width>1 */

void trv_rend_cell(uint8_t row, uint8_t col, uint8_t height, uint8_t width)
{
  /* If the cell is visible, then put it in the off-screen buffer.
   * Otherwise, just drop it on the floor
   */

  if (g_ray_hit[row][col].rect)
    {
      /* Apply texturing... special case for hits on floor or ceiling */

      if (IS_ZRAY_HIT(&g_ray_hit[row][col]))
        {
          trv_rend_zcell(row, col, height, width);
        }
      else
        {
          trv_rend_wall(row, col, height, width);
        }
    }
}

/* This version is for horizontal lines, i.e., height==1 and width>1 */

void trv_rend_row(uint8_t row, uint8_t col, uint8_t width)
{
  /* If the cell is visible, then put it in the off-screen buffer.
   * Otherwise, just drop it on the floor
   */

  if (g_ray_hit[row][col].rect)
    {
      /* Apply texturing... special case for hits on floor or ceiling */

      if (IS_ZRAY_HIT(&g_ray_hit[row][col]))
        {
          trv_rend_zrow(row, col, width);
        }
      else
        {
          trv_rend_wallrow(row, col, width);
        }
    }
}

/* This version is for vertical lines, i.e., height>1 and width==1 */

void trv_rend_column(uint8_t row, uint8_t col, uint8_t height)
{
  /* If the cell is visible, then put it in the off-screen buffer.
   * Otherwise, just drop it on the floor
   */

  if (g_ray_hit[row][col].rect)
    {
      /* Apply texturing... special case for hits on floor or ceiling */

      if (IS_ZRAY_HIT(&g_ray_hit[row][col]))
        {
          trv_rend_zcol(row, col, height);
        }
      else
        {
          trv_rend_wallcol(row, col, height);
        }
    }
}

/* This version is for a single pixel, i.e., height==1 and width==1 */

void trv_rend_pixel(uint8_t row, uint8_t col)
{
  /* If the cell is visible, then put it in the off-screen buffer.
   * Otherwise, just drop it on the floor
   */

  if (g_ray_hit[row][col].rect)
    {
      /* Apply texturing... special case for hits on floor or ceiling */

      if (IS_ZRAY_HIT(&g_ray_hit[row][col]))
        {
          trv_rend_zpixel(row, col);
        }
      else
        {
          trv_rend_wallpixel(row, col);
        }
    }
}

/****************************************************************************
 * Name: trv_get_rectpixel
 *
 * Description:
 *   Returns the pixel for a hit at (xpos, ypos) on rect.
 *
 ****************************************************************************/

trv_pixel_t trv_get_rectpixel(int16_t xpos, int16_t ypos,
                              FAR struct trv_bitmap_s *bmp, uint8_t scale)
{
   uint16_t tmask;
   uint16_t tsize;

   /* Get parameters associated with the size of the bitmap texture */

   tsize = bmp->log2h;
   tmask = TMASK(tsize);

   /* Return the texture code at this position */

   return(bmp->bm[TNDX((xpos >> scale), (ypos >> scale), tsize, tmask)]);
}

