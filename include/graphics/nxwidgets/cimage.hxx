/****************************************************************************
 * apps/include/graphics/nxwidgets/cimage.hxx
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
 *
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CIMAGE_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CIMAGE_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cnxwidget.hxx"

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
   * Forward references
   */

  class CWidgetControl;
  class IBitmap;

  /**
   * Simple image widget for present static images in the widget framework.
   */

  class CImage : public CNxWidget
  {
  protected:
    FAR IBitmap        *m_bitmap;      /**< Source bitmap image */
    struct nxgl_point_s m_origin;      /**< Origin for offset image display position */
    bool                m_highlighted; /**< Image is highlighted */

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the drawContents(port) and by classes that inherit from
     * CImage.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    void drawContents(CGraphicsPort *port, bool selected);

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the redraw() function to draw all visible regions.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawContents(CGraphicsPort *port);

    /**
     * Draw the border of this widget.  Called by the indirectly via
     * drawBoard(port) and also by classes that inherit from CImage.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    void drawBorder(CGraphicsPort *port, bool selected);

    /**
     * Draw the border of this widget. Called by the redraw() function to draw
     * all visible regions.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawBorder(CGraphicsPort *port);

    /**
     * Redraws the button.
     *
     * @param x The x coordinate of the click.
     * @param y The y coordinate of the click.
     */

    virtual void onClick(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Raises an action.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onPreRelease(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Redraws the image.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onRelease(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Redraws the image.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onReleaseOutside(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CImage(const CImage &label) : CNxWidget(label) { };

  public:

    /**
     * Constructor for an image.
     *
     * @param pWidgetControl The controlling widget for the display
     * @param x The x coordinate of the image box, relative to its parent.
     * @param y The y coordinate of the image box, relative to its parent.
     * @param width The width of the textbox.
     * @param height The height of the textbox.
     * @param bitmap The source bitmap image.
     * @param style The style that the widget should use.  If this is not
     *        specified, the button will use the global default widget
     *        style.
     */

    CImage(CWidgetControl *pWidgetControl, nxgl_coord_t x, nxgl_coord_t y,
           nxgl_coord_t width, nxgl_coord_t height, FAR IBitmap *bitmap,
           CWidgetStyle *style = NULL);

    /**
     * Destructor.
     *
     * NOTE: That the contained bitmap image is not destroyed when the image
     * container is destroyed.
     */

    virtual inline ~CImage() { }

    /**
     * Get pointer to the bitmap that this image contains.
     */

    inline FAR IBitmap *getBitmap() const { return m_bitmap; }

    /**
     * Set the bitmap that this image contains.
     */

    inline void setBitmap(FAR IBitmap *bitmap) { m_bitmap = bitmap; }

    /**
     * Insert the dimensions that this widget wants to have into the rect
     * passed in as a parameter.  All coordinates are relative to the
     * widget's parent.
     *
     * @param rect Reference to a rect to populate with data.
     */

    void getPreferredDimensions(CRect &rect) const;

    /**
     * Set the horizontal position of the bitmap.  Zero is the left edge
     * of the bitmap and values >0 will move the bit map to the right.
     * This method is useful for horizontal scrolling a large bitmap
     * within a smaller window
     */

    void setImageLeft(nxgl_coord_t column);

    /**
     * Align the image at the left of the widget region.
     *
     * NOTE: The CImage widget does not support any persistent alignment
     * attribute (at least not at the moment).  As a result, this alignment
     * can be lost if the image is changed or if the widget is resized.
     */

    inline void alignHorizontalLeft(void)
    {
      setImageLeft(0);
    }

    /**
     * Align the image at the left of the widget region.
     *
     * NOTE: The CImage widget does not support any persistent alignment
     * attribute (at least not at the moment).  As a result, this alignment
     * can be lost if the image is changed or if the widget is resized.
     */

    void alignHorizontalCenter(void);

    /**
     * Align the image at the left of the widget region.
     *
     * NOTE: The CImage widget does not support any persistent alignment
     * attribute (at least not at the moment).  As a result, this alignment
     * can be lost if the image is changed or if the widget is resized.
     */

    void alignHorizontalRight(void);

    /**
     * Set the vertical position of the bitmap.  Zero is the top edge
     * of the bitmap and values >0 will move the bit map down.
     * This method is useful for vertical scrolling a large bitmap
     * within a smaller window
     */

    void setImageTop(nxgl_coord_t row);

    /**
     * Align the image at the top of the widget region.
     *
     * NOTE: The CImage widget does not support any persistent alignment
     * attribute (at least not at the moment).  As a result, this alignment
     * can be lost if the image is changed or if the widget is resized.
     */

    inline void alignVerticalTop(void)
    {
      setImageTop(0);
    }

    /**
     * Align the image at the middle of the widget region.
     *
     * NOTE: The CImage widget does not support any persistent alignment
     * attribute (at least not at the moment).  As a result, this alignment
     * can be lost if the image is changed or if the widget is resized.
     */

    void alignVerticalCenter(void);

    /**
     * Align the image at the left of the widget region.
     *
     * NOTE: The CImage widget does not support any persistent alignment
     * attribute (at least not at the moment).  As a result, this alignment
     * can be lost if the image is changed or if the widget is resized.
     */

    void alignVerticalBottom(void);

    /**
     * Control the highlight state.
     *
     * @param highlightOn True(1), the image will be highlighted
     */

    void highlight(bool highlightOn);
 };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CIMAGE_HXX
