/****************************************************************************
 * apps/graphics/nxwidgets/src/cglyphsliderhorizontal.cxx
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
 ****************************************************************************
 *
 * Portions of this package derive from Woopsi (http://woopsi.org/) and
 * portions are original efforts.  It is difficult to determine at this
 * point what parts are original efforts and which parts derive from Woopsi.
 * However, in any event, the work of  Antony Dzeryn will be acknowledged
 * in most NxWidget files.  Thanks Antony!
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <cstdint>
#include <cstdbool>

#include "graphics/nxwidgets/ibitmap.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cglyphsliderhorizontal.hxx"
#include "graphics/nxwidgets/cglyphsliderhorizontalgrip.hxx"
#include "graphics/nxwidgets/cimage.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"

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
 * @param control The widget control instance for the window.
 * @param x The x coordinate of the slider, relative to its parent.
 * @param y The y coordinate of the slider, relative to its parent.
 * @param width The width of the slider.
 * @param thickness The thickness of the slider.
 * @param gripBitmap The slider grip image
 * @param fillColor The color to use when filling the grip
 * @param fill True: The grip will be filled with fillColor
 */

CGlyphSliderHorizontal::CGlyphSliderHorizontal(CWidgetControl *control,
                           nxgl_coord_t x, nxgl_coord_t y, nxgl_coord_t width,
                           nxgl_coord_t thickness, IBitmap *gripBitmap,
                           nxwidget_pixel_t fillColor, bool fill)
:CNxWidget(control, x, y, width, thickness, WIDGET_DRAGGABLE)
{
  // Initialize state data

  m_minimumValue          = 0;
  m_maximumValue          = 0;
  m_contentSize           = 0;
  m_value                 = 0;
  m_minimumGripWidth      = 10;
  m_pageSize              = 1;
  m_fillColor             = fillColor;
  m_fill                  = fill;
  m_barThickness          = thickness;

  // Correct the height of the widget.  The widget height was initially set
  // to the thickness of the grip.  But the grip image is normally a little
  // taller than the slider is thick.
  //
  // Do use the resize method here; we are not ready for the resize events.

  nxgl_coord_t gripHeight = gripBitmap->getHeight() + 4;
  if (gripHeight > thickness)
    {
      // Reset the widget height to the height of the grip image

      m_rect.setHeight(gripHeight);
    }

  // Set widget attributes

  m_flags.permeable       = false;
  m_flags.borderless      = false;
  m_flags.doubleClickable = false;

  // Set the "gutter" width

  CRect rect;
  getClientRect(rect);
  m_gutterWidth           = rect.getWidth();

  // Create the grip

  m_grip = new CGlyphSliderHorizontalGrip(control, x, y,
                                          gripBitmap->getWidth() + 4,
                                          gripHeight, gripBitmap);
  m_grip->setBorderless(true);
  m_grip->addWidgetEventHandler(this);
  addWidget(m_grip);
}

/**
 * Set the value of the slider.  This will reposition and redraw the grip.
 *
 * @param value The new value.
 */

void CGlyphSliderHorizontal::setValue(const int value)
{
  setValueWithBitshift((int32_t)value << 16);
}

/**
 * Set the value that of the slider.  This will reposition and redraw
 * the grip.  The supplied value should be shifted left 16 places.
 * This ensures greater accuracy than the standard setValue() method if
 * the slider is being used as a scrollbar.
 *
 * @param value The new value.
 */

void CGlyphSliderHorizontal::setValueWithBitshift(const int32_t value)
{
  CRect rect;
  getClientRect(rect);

  // Can the grip move?

  if ((rect.getWidth() > m_grip->getWidth()) &&
      (m_maximumValue != m_minimumValue))
    {
      int32_t newValue = value;
      int32_t maxValue = getPhysicalMaximumValueWithBitshift();

      // Limit to max/min values

      if (newValue > maxValue)
        {
          newValue = maxValue;
        }

      if (newValue >> 16 < m_minimumValue)
        {
          newValue = m_minimumValue << 16;
        }

      uint32_t scrollRatio = newValue / m_contentSize;
      int32_t newGripX = m_gutterWidth * scrollRatio;
      newGripX += newGripX & 0x8000;
      newGripX >>= 16;
      newGripX += rect.getX();

      m_grip->moveTo(newGripX, (rect.getHeight() - m_grip->getHeight()) >> 1);

      // Update stored value if necessary

      if (m_value != newValue)
        {
          m_value = newValue;
          m_widgetEventHandlers->raiseValueChangeEvent();
        }
    }
}

/**
 * Process events fired by the grip.
 *
 * @param e The event details.
 */

void CGlyphSliderHorizontal::handleDragEvent(const CWidgetEventArgs & e)
{
  // Handle grip events

  if (e.getSource() == m_grip && e.getSource() != NULL)
    {
      int32_t newValue = getGripValue() >> 16;

      // Grip has moved - compare values and raise event if the
      // value has changed.  Compare using integer values rather
      // than fixed-point.

      if (m_value >> 16 != newValue)
        {
          m_value = newValue << 16;
          m_widgetEventHandlers->raiseValueChangeEvent();
        }
    }
}

/**
 * Get the smallest value that the slider can move through when
 * dragged.
 *
 * @return The smallest value that the slider can move through when
 * dragged.
 */

nxgl_coord_t CGlyphSliderHorizontal::getMinimumStep(void) const
{
  // If the ratio of content to gutter is greater than or equal to one,
  // the minimum step that the slider can represent will be that ratio.

  uint32_t gutterRatio = m_contentSize << 16 / m_gutterWidth;
  gutterRatio += gutterRatio & 0x8000;
  gutterRatio >>= 16;

  if (gutterRatio > 0)
    {
      return gutterRatio;
    }

  return 1;
}

/**
 * Get the maximum possible value that the slider can represent.  Useful when
 * using the slider as a scrollbar, as the height of the grip prevents the full
 * range of values being accessed (intentionally).
 * The returned value is shifted left 16 places for more accuracy in fixed-point
 * calculations.
 *
 * @return The maximum possible value that the slider can represent.
 */

int32_t CGlyphSliderHorizontal::getPhysicalMaximumValueWithBitshift(void) const
{
  uint32_t maxX = m_gutterWidth - m_grip->getWidth();
  uint32_t scrollRatio = (maxX << 16) / m_gutterWidth;
  int32_t value = (scrollRatio * m_contentSize);

  return value;
}

/**
 * Get the value represented by the top of the grip.
 * return The value represented by the top of the grip.
 */

const int32_t CGlyphSliderHorizontal::getGripValue(void) const
{
  // Calculate the current value represented by the top of the grip

  CRect rect;
  getClientRect(rect);

  uint32_t gripPos = ((m_grip->getX() - getX()) - rect.getX());
  uint32_t scrollRatio = (gripPos << 16) / m_gutterWidth;
  int32_t value = (scrollRatio * m_contentSize);

  return value;
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CGlyphSliderHorizontal::drawContents(CGraphicsPort * port)
{
  int halfGripWidth = m_grip->getWidth() >> 1;
  int halfBar = m_barThickness >> 1;
  int halfHeight = getHeight() >> 1;
  int width;
  nxwidget_pixel_t color;

  // Fill in (erase) the region to the left and right of the fuel gauge

  width =
    m_grip->getX() > halfGripWidth ? halfGripWidth : m_grip->getX() - getX();
  port->drawFilledRect(getX(), getY(), width, getHeight(),
                       getBackgroundColor());

  // Fill in (erase) the region above the "fuel gauge"

  port->drawFilledRect(getX(), getY(), getWidth(), halfHeight - halfBar,
                       getBackgroundColor());

  // Fill in the bar area to the left of the grip

  if (m_fill)
    color = m_fillColor;
  else
    color = getBackgroundColor();
  if (m_grip->getX() > halfGripWidth)
    port->drawFilledRect(getX() + 1 + halfGripWidth,
                         getY() + halfHeight - halfBar + 1,
                         m_grip->getX() - getX() - halfGripWidth, m_barThickness - 2,
                         color);

  // Fill in bar the area to the right of the grip

  width = getWidth() - (m_grip->getX() - getX() + m_grip->getWidth()) - 1 -
    halfGripWidth;
  if (width > 0)
    port->drawFilledRect(m_grip->getX() + m_grip->getWidth(),
                         getY() + halfHeight - halfBar + 1, width,
                         m_barThickness - 2, getBackgroundColor());

  // Fill in (erase) the area to right of the bar

  width = m_grip->getX() - getX() + m_grip->getWidth() <
    getWidth() - halfGripWidth ? halfGripWidth : getWidth() - (m_grip->getX() -
                                                               getX() +
                                                               halfGripWidth);
  if (width > 0)
    port->drawFilledRect(getX() + getWidth() - halfGripWidth, getY(), width,
                         getHeight(), getBackgroundColor());

  // Fill in (erase) the region below the "fuel gauge"

  port->drawFilledRect(getX(), getY() + halfHeight + (m_barThickness - halfBar),
                       getWidth(), halfHeight - (m_barThickness - halfBar),
                       getBackgroundColor());

  // Now redraw the grip

  m_grip->enableDrawing();
  m_grip->redraw();
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CGlyphSliderHorizontal::drawBorder(CGraphicsPort * port)
{
  // Stop drawing if the widget indicates it should not have an outline

  if (!isBorderless())
    {
      int y1, y2;
      int x = getX();
      int gripWidth = m_grip->getWidth();
      int halfGripWidth = gripWidth >> 1;
      int gripX = m_grip->getX();
      int width;
      nxwidget_pixel_t shine;
      nxwidget_pixel_t shadow;

      y1 = getY() + (getHeight() >> 1) - (m_barThickness >> 1);
      y2 = getY() + (getHeight() >> 1) + m_barThickness -
          (m_barThickness >> 1) - 1;

      // To the Left of the grip.  Only draw if the icon isn't covering the
      // edge of the bar

      width = gripX - x - halfGripWidth;
      if (width > 0)
        {
          // If we aren't filling, then don't draw a "raised" bar

          if (m_fill)
            {
              shine = getShineEdgeColor();
              shadow = getShadowEdgeColor();
            }
          else
            {
              shadow = getShineEdgeColor();
              shine = getShadowEdgeColor();
            }

          port->drawHorizLine(x + halfGripWidth, y1, width, shine);
          port->drawHorizLine(x + halfGripWidth, y2, width, shadow);
        }

      // To the Right of the grip
      // Only draw if the icon isn't covering the edge of the bar

      width = getWidth() - (gripX - x + gripWidth) - 1 - halfGripWidth;
      if (width > 0)
        {

          port->drawHorizLine(gripX + gripWidth, y1,
                              getWidth() - (gripX - x + gripWidth) - 1 -
                              halfGripWidth, getShadowEdgeColor());
          port->drawHorizLine(gripX + gripWidth, y2,
                              getWidth() - (gripX - x + gripWidth) - 1 -
                              halfGripWidth, getShineEdgeColor());
        }

      // Left edge

      if (gripX > x + halfGripWidth)
        {
          port->drawVertLine(x + halfGripWidth, y1, m_barThickness,
                getShineEdgeColor());
        }

      // Right edge

      if (gripX + gripWidth < x + getWidth() - 1 - halfGripWidth)
        {
          port->drawVertLine(x + getWidth() - 1 - halfGripWidth,
                y1, m_barThickness, getShadowEdgeColor());
        }
    }
}

/**
 * Resize the slider to the new dimensions.
 *
 * @param width The new width.
 * @param height The new height.
 */

void CGlyphSliderHorizontal::onResize(nxgl_coord_t width, nxgl_coord_t height)
{
  // Remember current values

  int32_t oldValue = m_value;
  bool events = raisesEvents();

  // Disable event raising

  setRaisesEvents(false);
  // resizeGrip();

  // Set back to current value

  setValue(oldValue);

  // Reset event raising

  setRaisesEvents(events);
}

/**
 * Moves the grip towards the mouse.
 *
 * @param x The x coordinate of the click.
 * @param y The y coordinate of the click.
 */

void CGlyphSliderHorizontal::onClick(nxgl_coord_t x, nxgl_coord_t y)
{
  // Which way should the grip move?

  if (x > m_grip->getX())
    {
      // Move grip right

      setValueWithBitshift(m_value + (m_pageSize << 16));
    }
  else
    {
      // Move grip left

      setValueWithBitshift(m_value - (m_pageSize << 16));
    }

  redraw();
}
