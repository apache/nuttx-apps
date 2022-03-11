/****************************************************************************
 * apps/graphics/nxwidgets/src/clatchbuttonarray.cxx
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
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cstickybuttonarray.hxx"
#include "graphics/nxwidgets/clatchbuttonarray.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * CButton Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

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

CLatchButtonArray::CLatchButtonArray(CWidgetControl *pWidgetControl,
                                     nxgl_coord_t x, nxgl_coord_t y,
                                     uint8_t buttonColumns, uint8_t buttonRows,
                                     nxgl_coord_t buttonWidth, nxgl_coord_t buttonHeight,
                                     CWidgetStyle *style)
: CStickyButtonArray(pWidgetControl, x, y, buttonColumns, buttonRows, buttonWidth, buttonHeight, style)
{
}

/**
 * Handles button click events
 *
 * @param x The x coordinate of the click.
 * @param y The y coordinate of the click.
 */

void CLatchButtonArray::onClick(nxgl_coord_t x, nxgl_coord_t y)
{
  // We need to save the button position so that when CButtonArray
  // gets the release, it will have the correct position.  This would
  // look nicer if we just called CButtonArray::onClock() but that
  // would cause duplicate redraw()

  m_clickX = x;
  m_clickY = y;

  // Convert the new key press to row/columin indices

  int column;
  int row;

  if (posToButton(x, y, column, row))
    {
      // Change the stuck down button on each in-range button press
      // NOTE that normally button click events are not processed (but
      // button release events will be processed).

      stickDown(column, row);
    }
}
