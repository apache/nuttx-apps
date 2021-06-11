/****************************************************************************
 * apps/include/graphics/nxwidgets/cstickybuttonarray.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSTICKYBUTTONARRAY_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSTICKYBUTTONARRAY_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cbuttonarray.hxx"
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
  /**
   * Forward references
   */

  class CWidgetControl;
  class CNxString;

  /**
   * Manages a two-dimensional array of buttons as one widget.  A two-
   * dimensional array of buttons might also be called a "keyboard".
   */

  class CStickyButtonArray : public CButtonArray
  {
  protected:
    bool    m_isStuckDown;  /**< True if one key in the array stuck down */
    bool    m_stickDown;    /**< True there is a change in a sticky button state */
    uint8_t m_stickyRow;    /**< The row index of the stuck button */
    uint8_t m_stickyColumn; /**< The column index of the stuck button */

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the redraw() function to draw all visible regions.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawContents(CGraphicsPort *port);

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CStickyButtonArray(const CStickyButtonArray &button) : CButtonArray(button) { }

  public:

    /**
     * Constructor for an array of sticky buttons.
     *
     * @param pWidgetControl The widget control for the display.
     * @param x The x coordinate of the button array, relative to its parent.
     * @param y The y coordinate of the button array, relative to its parent.
     * @param buttonColumns The number of buttons in one row of the button array
     * @param buttonRows The number of buttons in one column of the button array
     * @param buttonWidth The width of one button
     * @param buttonHeight The height of one button
     * @param style The style that the button should use.  If this is not
     *        specified, the button will use the global default widget
     *        style.
     */

    CStickyButtonArray(CWidgetControl *pWidgetControl,
                       nxgl_coord_t x, nxgl_coord_t y,
                       uint8_t buttonColumns, uint8_t buttonRows,
                       nxgl_coord_t buttonWidth, nxgl_coord_t buttonHeight,
                       CWidgetStyle *style = (CWidgetStyle *)NULL);

    /**
     * CStickyButtonArray Destructor.
     */

    virtual inline ~CStickyButtonArray(void) { }

    /**
     * Return the position of the last stuck down button (0,0 will be returned
     * the no button has every been stuck down).  The button at this position
     * is currently stuck down then, in addition, return true.
     *
     * @param column The location to return the column index of the button
     *    of interest
     * @param row The location to return the row index of the button of
     *    interest
     * @return True if a button in the array is clicked
     */

    virtual bool isAnyButtonStuckDown(int &column, int &row) const;

    /**
     * Check if this specific button in the array is stuck down
     *
     * @param column The column of the button to check.
     * @param row The row of the button to check.
     * @return True if this button is stuck down
     */

    virtual bool isThisButtonStuckDown(int column, int row) const;

    /**
     * Force the button at this position into the stuck down state
     *
     * @param column The column containing the button to stick down
     * @param row The rowtcontaining the button to stick down
     * @return False(0) is returned if the indices are out of range.
     */

    virtual bool stickDown(int column, int row);

    /**
     * Unstick all buttons
     */

    virtual void unstick(void);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSTICKYBUTTONARRAY_HXX
