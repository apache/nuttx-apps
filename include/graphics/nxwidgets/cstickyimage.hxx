/****************************************************************************
 * apps/include/graphics/nxwidgets/cstickyimage.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSTICKIMAGE_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSTICKIMAGE_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cimage.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class CWidgetControl;

  /**
   * CImage that sticks in the selected selected state when clicked.
   */

  class CStickyImage : public CImage
  {
  protected:
    bool m_stuckSelection;  /**< True if the image is stuck in the selected */

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the redraw() function to draw all visible regions.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawContents(CGraphicsPort *port);

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the redraw() function to draw all visible regions.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawBorder(CGraphicsPort *port);

    /**
     * Don't redraw on click events.  The image state is completely controlled
     * by the m_stuckSelection state.
     *
     * @param x The x coordinate of the click.
     * @param y The y coordinate of the click.
     */

    virtual void onClick(nxgl_coord_t x, nxgl_coord_t y) {}

    /**
     * Don't redraw on release events.  The image state is completely controlled
     * by the m_stuckSelection state.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onRelease(nxgl_coord_t x, nxgl_coord_t y) { }

    /**
     * Don't redraw on release events.  The image state is completely controlled
     * by the m_stuckSelection state.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onReleaseOutside(nxgl_coord_t x, nxgl_coord_t y) { }

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CStickyImage(const CStickyImage &image) : CImage(image) { }

  public:

    /**
     * Constructor for an image.
     *
     * @param pWidgetControl The controlling widget for the display
     * @param x The x coordinate of the image box, relative to its parent.
     * @param y The y coordinate of the image box, relative to its parent.
     * @param width The width of the image box.
     * @param height The height of the image box.
     * @param bitmap The source bitmap image.
     * @param style The style that the widget should use.  If this is not
     *        specified, the image will use the global default widget
     *        style.
     */

    CStickyImage(CWidgetControl *pWidgetControl, nxgl_coord_t x, nxgl_coord_t y,
                 nxgl_coord_t width, nxgl_coord_t height, FAR IBitmap *bitmap,
                 CWidgetStyle *style = (CWidgetStyle *)NULL);

    /**
     * Destructor.
     */

    virtual inline ~CStickyImage(void) { }

    /**
     * Sets the image's stuck selection state.
     *
     * @param selection The new stuck selection state.
     */

    void setStuckSelection(bool selection);

    /**
     * Toggles the images stuck selection state.
     */

    inline void toggleStuckSelection(void)
      {
        setStuckSelection(!m_stuckSelection);
      }

   /**
     * Returns the stuck selection state.
     *
     * @return True if the image is in the stuck selection state.
     */

    inline const bool isStuckSelection(void) const
      {
        return m_stuckSelection;
      }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSTICKIMAGE_HXX
