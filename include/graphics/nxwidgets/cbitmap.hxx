/****************************************************************************
 * apps/include/graphics/nxwidgets/cbitmap.hxx
 *
 * SPDX-License-Identifier: Apache-2.0
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

/****************************************************************************
 * Portions of this package derive from Woopsi (http://woopsi.org/) and
 * portions are original efforts.  It is difficult to determine at this
 * point what parts are original efforts and which parts derive from Woopsi.
 * However, in any event, the work of  Antony Dzeryn will be acknowledged
 * in all NxWidget files.  Thanks Antony!
 *
 *   Copyright (c) 2007-2011, Antony Dzeryn
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the names "Woopsi", "Simian Zombie" nor the
 *   names of its contributors may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Antony Dzeryn ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Antony Dzeryn BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBITMAP_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBITMAP_HXX

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
   * Bitmap Structure
   */

  struct SBitmap
  {
    uint8_t               bpp;    /**< Bits per pixel */
    uint8_t               fmt;    /**< Color format */
    nxgl_coord_t          width;  /**< Width in pixels */
    nxgl_coord_t          height; /**< Height in rows */
    uint16_t              stride; /**< Width in bytes */
    FAR const void       *data;   /**< Pointer to the beginning of pixel data */
  };

  /**
   * Class providing bitmap accessor for a bitmap represented by SBitmap.
   */

  class CBitmap : public IBitmap
  {
  protected:
    const struct SBitmap *m_bitmap;  /**< The bitmap that is being managed */

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CBitmap(const CBitmap &bitmap) { }

  public:

    /**
     * Constructor.
     *
     * @param bitmap The bitmap structure being wrapped.
     */

    CBitmap(const struct SBitmap *bitmap);

    /**
     * Destructor.
     */

    inline ~CBitmap(void) {}

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

    inline void setSelected(bool selected) {}

    /**
     * Get one row from the bit map image.
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

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBITMAP_HXX
