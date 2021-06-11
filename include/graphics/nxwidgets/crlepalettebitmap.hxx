/****************************************************************************
 * apps/include/graphics/nxwidgets/crlepalettebitmap.hxx
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CRLEPALETTBITMAP_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CRLEPALETTBITMAP_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/ibitmap.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  /**
   * One Run-Length Encoded (RLE) value
   */

  struct SRlePaletteBitmapEntry
  {
    uint8_t          npixels; /**< Number of pixels */
    uint8_t          lookup;  /**< Pixel RGB lookup index */
  };

  /**
   * Run-Length Encoded (RLE), Paletted Bitmap Structure
   */

  struct SRlePaletteBitmap
  {
    uint8_t          bpp;     /**< Bits per pixel */
    uint8_t          fmt;     /**< Color format */
    uint8_t          nlut;    /**< Number of colors in the Look-Up Table (LUT) */
    nxgl_coord_t     width;   /**< Width in pixels */
    nxgl_coord_t     height;  /**< Height in rows */
    FAR const void  *lut[2];  /**< Pointers to the beginning of the Look-Up Tables (LUTs) */

    /**
     * The pointer to the beginning of the RLE data
     */

    FAR const struct SRlePaletteBitmapEntry *data;
  };

  /**
   * Class providing bitmap accessor for a bitmap represented by SRlePaletteBitmap.
   */

  class CRlePaletteBitmap : public IBitmap
  {
  protected:
    /**
     * The bitmap that is being managed
     */

    FAR const struct SRlePaletteBitmap *m_bitmap;  /**< The bitmap that is being managed */

    /**
     * Accessor state data
     */

    nxgl_coord_t     m_row;       /**< Logical row number */
    nxgl_coord_t     m_col;       /**< Logical column number */
    uint8_t          m_remaining; /**< Number of bytes remaining in current entry */
    FAR const void  *m_lut;       /**< The selected LUT */
    FAR const struct SRlePaletteBitmapEntry *m_rle; /**< RLE entry being processed */

    /**
     * Reset to the beginning of the image
     */

    void startOfImage(void);

    /**
     * Advance position data ahead.  Called after npixels have
     * have been consume.
     *
     * @param npixels The number of pixels to advance
     * @return False if this goes beyond the end of the image
     */

    bool advancePosition(nxgl_coord_t npixels);

    /**
     * Seek ahead the specific number of pixels -- discarding
     * and advancing.
     *
     * @param npixels The number of pixels to skip
     * @return False if this goes beyond the end of the image
     */

    bool skipPixels(nxgl_coord_t npixels);

    /** Seek to the beginning of the next row
     *
     * @return False if this was the last row of the image
     */

    bool nextRow(void);

    /** Seek to the beignning specific row
     *
     * @param row The row number to seek to
     * @return False if this goes beyond the end of the image
     */

    bool seekRow(nxgl_coord_t row);

    /** Copy the pixels from the current RLE entry the specified number of times.
     *
     * @param npixels The number of pixels to copy.  Must be less than or equal
     *  to m_remaining.
     * @param data The memory location provided by the caller
     *   in which to return the data.  This should be at least
     *   (getWidth()*getBitsPerPixl() + 7)/8 bytes in length
     *   and properly aligned for the pixel color format.
     */

    void copyColor(nxgl_coord_t npixels, FAR void *data);

    /** Copy pixels from the current position
     *
     * @param npixels The number of pixels to copy
     * @param data The memory location provided by the caller
     *   in which to return the data.  This should be at least
     *   (getWidth()*getBitsPerPixl() + 7)/8 bytes in length
     *   and properly aligned for the pixel color format.
     * @return False if this goes beyond the end of the image
     */

    bool copyPixels(nxgl_coord_t npixels, FAR void *data);

  public:

    /**
     * Constructor.
     *
     * @param bitmap The bitmap structure being wrapped.
     */

    CRlePaletteBitmap(const struct SRlePaletteBitmap *bitmap);

    /**
     * Destructor.
     */

    inline ~CRlePaletteBitmap(void) {}

    /**
     * Get the bitmap's color format.
     *
     * @return The bitmap's width.
     */

    const uint8_t getColorFormat(void) const;

    /**
     * Get the bitmap's color format.
     *
     * @return The bitmap's color format.
     */

    const uint8_t getBitsPerPixel(void) const;

    /**
     * Get the bitmap's width (in pixels/columns).
     *
     * @return The bitmap's pixel depth.
     */

    const nxgl_coord_t getWidth(void) const;

    /**
     * Get the bitmap's height (in rows).
     *
     * @return The bitmap's height.
     */

    const nxgl_coord_t getHeight(void) const;

    /**
     * Get the bitmap's width (in bytes).
     *
     * @return The bitmap's width.
     */

    const size_t getStride(void) const;

    /**
     * Use the colors associated with a selected image.
     *
     * @param selected.  true: Use colors for a selected widget,
     *   false: Use normal (default) colors.
     */

    void setSelected(bool selected);

    /**
     * Get one row from the bit map image using the selected colors.
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

    bool getRun(nxgl_coord_t x, nxgl_coord_t y, nxgl_coord_t width,
                FAR void *data);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CRLEPALETTBITMAP_HXX
