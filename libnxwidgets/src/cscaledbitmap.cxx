/****************************************************************************
 * NxWidgets/libnxwidgets/src/cscaledbitmap.hxx
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
 *    me be used to endorse or promote products derived from this software
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

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <cstring>

#include <nuttx/nx/nxglib.h>

#include "cscaledbitmap.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

/**
 * Constructor.
 *
 * @param bitmap The bitmap structure being scaled.
 * @newSize The new, scaled size of the image
 */

CScaledBitmap::CScaledBitmap(IBitmap *bitmap, struct nxgl_size_s &newSize)
: m_bitmap(bitmap), m_size(newSize)
{
  // xScale will be used to convert a request X position to an X position
  // in the contained bitmap:
  //
  // xImage = xRequested * oldWidth / newWidth
  //        = xRequested * xScale

  m_xScale = itob16((uint32_t)m_bitmap->getWidth()) / newSize.w;

  // Similarly, yScale will be used to convert a request Y position to a Y
  // positionin the contained bitmap:
  //
  // yImage = yRequested * oldHeight / newHeight
  //        = yRequested * yScale

  m_yScale = itob16((uint32_t)m_bitmap->getHeight()) / newSize.h;

  // Allocate and initialize the row cache

  size_t stride = bitmap->getStride();
  m_rowCache[0] = new uint8_t[stride];
  m_rowCache[1] = new uint8_t[stride];

  // Read the first two rows into the cache

  m_row = m_bitmap->getWidth(); // Set to an impossible value
  cacheRows(0);
}

/**
 * Destructor.
 */

CScaledBitmap::~CScaledBitmap(void)
{
  // Delete the allocated row cache memory

  if (m_rowCache[0])
    {
      delete m_rowCache[0];
    }

  if (m_rowCache[1])
    {
      delete m_rowCache[1];
   }

  // We are also responsible for deleting the contained IBitmap

  if (m_bitmap)
    {
      delete m_bitmap;
    }
}

/**
 * Get the bitmap's color format.
 *
 * @return The bitmap's width.
 */

const uint8_t CScaledBitmap::getColorFormat(void) const
{
  return m_bitmap->getColorFormat();
}

/**
 * Get the bitmap's color format.
 *
 * @return The bitmap's color format.
 */

const uint8_t CScaledBitmap::getBitsPerPixel(void) const
{
  return m_bitmap->getBitsPerPixel();
}

/**
 * Get the bitmap's width (in pixels/columns).
 *
 * @return The bitmap's pixel depth.
 */

const nxgl_coord_t CScaledBitmap::getWidth(void) const
{
  return m_size.w;
}

/**
 * Get the bitmap's height (in rows).
 *
 * @return The bitmap's height (in rows).
 */

const nxgl_coord_t CScaledBitmap::getHeight(void) const
{
  return m_size.h;
}

/**
 * Get the bitmap's width (in bytes).
 *
 * @return The bitmap's width (in bytes).
 */

const size_t CScaledBitmap::getStride(void) const
{
  return (m_bitmap->getBitsPerPixel() * m_size.w + 7) / 8;
}

/**
 * Get one row from the bit map image.
 *
 *   REVISIT:  This algorithm is really intended to expand images.  Hence,
 *   for example, interpolation is between row and row+1 and column and
 *   column+1 in the original, unscaled image.  You would the interpolation
 *   differently if you really wanted to sub-sample well.
 *
 * @param x The offset into the row to get
 * @param y The row number to get
 * @param width The number of pixels to get from the row
 * @param data The memory location provided by the caller
 *   in which to return the data.  This should be at least
 *   (getWidth()*getBitsPerPixl() + 7)/8 bytes in length
 *   and properly aligned for the pixel color format.
 * @param True if the run was returned successfully.
 */

bool CScaledBitmap::getRun(nxgl_coord_t x, nxgl_coord_t y,
                           nxgl_coord_t width, FAR void *data)
{
#if CONFIG_NXWIDGETS_FMT == FB_FMT_RGB8_332 || CONFIG_NXWIDGETS_FMT == FB_FMT_RGB24
  FAR uint8_t  *dest = (FAR uint8_t *)data;
#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB16_565
  FAR uint16_t *dest = (FAR uint16_t *)data;
#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB32
  FAR uint32_t *dest = (FAR uint32_t *)data;
#else
#  error Unsupported, invalid, or undefined color format
#endif

  // Check ranges.  Casts to unsigned int are ugly but permit one-sided comparisons

  if (((unsigned int)x           >=  (unsigned int)m_size.w) &&
      ((unsigned int)(x + width) > (unsigned int)m_size.w) &&
      ((unsigned int)y           <= (unsigned int)m_size.h))
    {
      return false;
    }

  // Get the row number in the unscaled image corresponding to the
  // requested y position.  This must be either the exact row or the
  // closest row just before the requested position

  b16_t row16      = y * m_yScale;
  nxgl_coord_t row = b16toi(row16);

  // Get that row and the one after it into the row cache. We know that
  // the pixel value that we want is one between the two rows.  This
  // may seem wasteful to read two entire rows.  However, in normal usage
  // we will be traversal each image from top-left to bottom-right in
  // order.  In that case, the caching is most efficient.

  if (!cacheRows(row))
    {
      return false;
    }

  // Now scale and copy the data from the cached row data

  for (int i = 0; i < width; i++, x++)
    {
      // Get the column number in the unscaled row corresponding to the
      // requested x position.  This must be either the exact column or the
      // closest column just before the requested position

      b16_t column = x * m_xScale;

      // Get the color at the position on the first row

      struct rgbcolor_s color1;
      if (!rowColor(m_rowCache[0], column, color1))
        {
          gdbg("ERROR rowColor failed for the first row\n");
          return false;
        }

      // Get the color at the position on the first row

      struct rgbcolor_s color2;
      if (!rowColor(m_rowCache[1], column, color2))
        {
          gdbg("ERROR rowColor failed for the second row\n");
          return false;
        }

      // Check for transparent colors

      bool transparent1;
      bool transparent2;

#if CONFIG_NXWIDGETS_FMT == FB_FMT_RGB8_332
      uint8_t color = RGBTO8(color1.r, color1.g, color1.b);
      transparent1 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

      color = RGBTO8(color2.r, color2.g, color2.b);
      transparent2 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB16_565
      uint16_t color = RGBTO16(color1.r, color1.g, color1.b);
      transparent1 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

      color = RGBTO16(color2.r, color2.g, color2.b);
      transparent2 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB24 || CONFIG_NXWIDGETS_FMT == FB_FMT_RGB32
      uint32_t color = RGBTO24(color1.r, color1.g, color1.b);
      transparent1 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

      color = RGBTO24(color2.r, color2.g, color2.b);
      transparent2 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

#else
#  error Unsupported, invalid, or undefined color format
#endif

      // Is one of the colors transparent?

      struct rgbcolor_s scaledColor;
      b16_t fraction b16frac(row16);

      if (transparent1 || transparent2)
        {
          // Yes.. don't interpolate within transparent regions or
          // between transparent and opaque regions.

          // Get the color closest to the requested position

          if (fraction < b16HALF)
            {
              scaledColor.r = color1.r;
              scaledColor.g = color1.g;
              scaledColor.b = color1.b;
            }
          else
            {
              scaledColor.r = color2.r;
              scaledColor.g = color2.g;
              scaledColor.b = color2.b;
            }
        }
      else
        {
          // No.. both colors are opaque

          if (!scaleColor(color1, color2, fraction, scaledColor))
            {
              return false;
            }
        }

      // Write the interpolated data to the user buffer

#if CONFIG_NXWIDGETS_FMT == FB_FMT_RGB8_332
      color = RGBTO8(scaledColor.r, scaledColor.g, scaledColor.b);
      *dest++ = color;

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB16_565
      color = RGBTO16(scaledColor.r, scaledColor.g, scaledColor.b);
      *dest++ = color;

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB24
      *dest++ = color2.b;
      *dest++ = color2.r;
      *dest++ = color2.g;

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB32
      color = RGBTO24(scaledColor.r, scaledColor.g, scaledColor.b);
      *dest++ = color;

#else
#  error Unsupported, invalid, or undefined color format
#endif
    }

  return true;
}

/**
 * Read two rows into the row cache
 *
 * @param row - The row number of the first row to cache
 */

bool CScaledBitmap::cacheRows(unsigned int row)
{
  nxgl_coord_t bitmapWidth  = m_bitmap->getWidth();
  nxgl_coord_t bitmapHeight = m_bitmap->getHeight();

  // A common case is to advance by one row.  In this case, we only
  // need to read one row

  if (row == m_row + 1)
    {
      // Swap rows

      FAR uint8_t *saveRow = m_rowCache[0];
      m_rowCache[0] = m_rowCache[1];
      m_rowCache[1] = saveRow;

      // Save number of the first row that we have in the cache

      m_row = row;

      // Now read the new row into the second row cache buffer

      if (++row >= (unsigned int)bitmapHeight)
        {
          row = bitmapHeight - 1;
        }

      if (!m_bitmap->getRun(0, row, bitmapWidth, m_rowCache[1]))
        {
          gdbg("Failed to read bitmap row %d\n", row);
          return false;
        }
    }

  // Do we need to read two new rows?  Or do we already have the
  // request row in the cache?

  else if (row != m_row)
    {
      // Read the first row into the cache

      if (row >= (unsigned int)bitmapHeight)
        {
          row = bitmapHeight - 1;
        }

      if (!m_bitmap->getRun(0, row, bitmapWidth, m_rowCache[0]))
        {
          gdbg("Failed to read bitmap row %d\n", row);
          return false;
        }

      // Save number of the first row that we have in the cache

      m_row = row;

      // Read the next row into the cache

      if (++row >= (unsigned int)bitmapHeight)
        {
          row = bitmapHeight - 1;
        }

      if (!m_bitmap->getRun(0, row, bitmapWidth, m_rowCache[1]))
        {
          gdbg("Failed to read bitmap row %d\n", row);
          return false;
        }
    }

  return true;
}

/**
 * Given an two RGB colors and a fractional value, return the scaled
 * value between the two colors.
 *
 * @param incolor1 - The first color to be used
 * @param incolor2 - The second color to be used
 * @param fraction - The fractional value
 * @param outcolor - The returned, scaled color
 */

bool CScaledBitmap::scaleColor(FAR const struct rgbcolor_s &incolor1,
                               FAR const struct rgbcolor_s &incolor2,
                               b16_t fraction, FAR struct rgbcolor_s &outcolor)
{
  uint8_t component;
  b16_t red;
  b16_t green;
  b16_t blue;

  // A fraction of < 0.5 would mean to use use mostly color1; a fraction
  // greater than 0.5 would men to use mostly color2

  b16_t remainder = b16ONE - fraction;

  // Interpolate each color value  (converting to b15)

  red   = (b16_t)incolor1.r * remainder + (b16_t)incolor2.r * fraction;
  green = (b16_t)incolor1.g * remainder + (b16_t)incolor2.g * fraction;
  blue  = (b16_t)incolor1.b * remainder + (b16_t)incolor2.b * fraction;

  // Return the integer, interpolated values, clipping to the range of
  // uint8_t

  component  = b16toi(red);
  outcolor.r = component < 256 ? component : 255;

  component  = b16toi(green);
  outcolor.g = component < 256 ? component : 255;

  component  = b16toi(blue);
  outcolor.b = component < 256 ? component : 255;
  return true;
}

/**
 * Given an image row and a non-integer column offset, return the
 * interpolated RGB color value corresponding to that position
 *
 * @param row - The pointer to the row in the row cache to use
 * @param column - The non-integer column offset
 * @param outcolor - The returned, interpolated color
 *
 */

bool CScaledBitmap::rowColor(FAR uint8_t *row, b16_t column,
                             FAR struct rgbcolor_s &outcolor)
{
  // This is the col at or just before the pixel of interest

  nxgl_coord_t col1 = b16toi(column);
  nxgl_coord_t col2 = col1 + 1;

  nxgl_coord_t bitmapWidth  = m_bitmap->getWidth();
  if (col2 >= bitmapWidth)
    {
      col2 = bitmapWidth - 1;
    }

  b16_t fraction = b16frac(column);

  struct rgbcolor_s color1;
  struct rgbcolor_s color2;

  bool transparent1;
  bool transparent2;

#if CONFIG_NXWIDGETS_FMT == FB_FMT_RGB8_332
  uint8_t color = row[col1];
  color1.r = RBG8RED(color);
  color1.g = RBG8GREEN(color);
  color1.b = RBG8BLUE(color);

  transparent1 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

  color = row[col2];
  color2.r = RBG8RED(color);
  color2.g = RBG8GREEN(color);
  color2.b = RBG8BLUE(color);

  transparent2 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB16_565
  FAR uint16_t *row16 = (FAR uint16_t*)row;
  uint16_t color = row16[col1];
  color1.r = RBG16RED(color);
  color1.g = RBG16GREEN(color);
  color1.b = RBG16BLUE(color);

  transparent1 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

  color = row16[col2];
  color2.r = RBG16RED(color);
  color2.g = RBG16GREEN(color);
  color2.b = RBG16BLUE(color);

  transparent2 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB24
  unsigned int ndx = 3*col1;
  color1.r = row[ndx+2];
  color1.g = row[ndx+1];
  color1.b = row[ndx];

  uint32_t color = RGBTO24(color1.r, color1.g, color1.b);
  transparent1 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

  ndx      = 3*col2;
  color2.r = row[ndx+2];
  color2.g = row[ndx+1];
  color2.b = row[ndx];

  color = RGBTO24(color2.r, color2.g, color2.b);
  transparent2 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

#elif CONFIG_NXWIDGETS_FMT == FB_FMT_RGB32
  FAR uint32_t *row32 = (FAR uint32_t*)row;
  uint32_t color = row32[col1];
  color1.r = RBG24RED(color);
  color1.g = RBG24GREEN(color);
  color1.b = RBG24BLUE(color);

  transparent1 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

  color    = row32[col2];
  color2.r = RBG24RED(color);
  color2.g = RBG24GREEN(color);
  color2.b = RBG24BLUE(color);

  transparent2 = (color == CONFIG_NXWIDGETS_TRANSPARENT_COLOR);

#else
#  error Unsupported, invalid, or undefined color format
#endif

  // Is one of the colors transparent?

  if (transparent1 || transparent2)
    {
      // Yes.. don't interpolate within transparent regions or
      // between transparent and opaque regions.

      // Return the color closest to the requested position
      //
      // A fraction of < 0.5 would mean to use use mostly color1; a fraction
      // greater than 0.5 would men to use mostly color2

      if (fraction < b16HALF)
        {
          outcolor.r = color1.r;
          outcolor.g = color1.b;
          outcolor.g = color1.g;
        }
      else
        {
          outcolor.r = color2.r;
          outcolor.g = color2.b;
          outcolor.g = color2.g;
        }

      return true;
    }
  else
    {
      // No.. both colors are opaque

      return scaleColor(color1, color2, fraction, outcolor);
    }
}
