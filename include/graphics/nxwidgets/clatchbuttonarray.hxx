/****************************************************************************
 * apps/include/clatchbuttonarray.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CLATCHBUTTONARRAY_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CLATCHBUTTONARRAY_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cstickybuttonarray.hxx"
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
   * Manages a two-dimensional array of buttons as one widget.  When a button
   * is clicked is latches (i.e., it stays pushed after the press is released.
   * The behavior is then like radio buttons:  Pressing each each button in
   * the array unlatches the previous button and latches the new button.
   *
   * Unlike CLatchButton, pressing the same button more than once has no
   * effect.
   */

  class CLatchButtonArray : public CStickyButtonArray
  {
  protected:
    /**
     * Handles button click events
     *
     * @param x The x coordinate of the click.
     * @param y The y coordinate of the click.
     */

    virtual void onClick(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Redraws the button.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CLatchButtonArray(const CLatchButtonArray &button) : CStickyButtonArray(button) { }

  public:

    /**
     * Constructor for an array of latch buttons.
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

    CLatchButtonArray(CWidgetControl *pWidgetControl,
                       nxgl_coord_t x, nxgl_coord_t y,
                       uint8_t buttonColumns, uint8_t buttonRows,
                       nxgl_coord_t buttonWidth, nxgl_coord_t buttonHeight,
                       CWidgetStyle *style = (CWidgetStyle *)NULL);

    /**
     * CLatchButtonArray Destructor.
     */

    virtual inline ~CLatchButtonArray(void) { }

    /**
     * Return the position of the last latched button (0,0 will be returned
     * the no button has every been latched).  The button at this position
     * is currently latched then, in addition, return true.
     *
     * @param column The location to return the column index of the button
     *    of interest
     * @param row The location to return the row index of the button of
     *    interest
     * @return True if a button in the array is latched
     */

    inline const bool isAnyButtonLatched(int &column, int &row) const
    {
      return isAnyButtonStuckDown(column, row);
    }

    /**
     * Check if this specific button in the array is latched
     *
     * @param column The column of the button to check.
     * @param row The row of the button to check.
     * @return True if this button is clicked
     */

    inline const bool isThisButtonLatched(int column, int row) const
    {
      return isThisButtonStuckDown(column, row);
    }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CLATCHBUTTONARRAY_HXX
