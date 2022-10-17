/****************************************************************************
 * apps/include/graphics/nxwidgets/cscrollbarvertical.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSCROLLBARVERTICAL_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSCROLLBARVERTICAL_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cnxwidget.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/islider.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class CSliderVertical;
  class CGlyphButton;
  class CNxTimer;

  /**
   * Container class that holds a slider widget and two arrow buttons.
   * The interface is presents is virtually identical to the CSliderVertical
   * widget, which means the two are easily interchangeable.  All events
   * raised by the internal slider widget are re-raised by this widget
   * to this widget's event handler, meaning its events are also identical
   * to the CSliderVertical's.
   */
  class CScrollbarVertical : public ISlider, public CNxWidget, public CWidgetEventHandler {

  protected:
    CSliderVertical *m_slider;         /**< Pointer to the slider widget */
    CGlyphButton    *m_upButton;       /**< Pointer to the up button */
    CGlyphButton    *m_downButton;     /**< Pointer to the down button */
    nxgl_coord_t     m_buttonHeight;   /**< Height of the buttons */
    uint8_t          m_scrollTimeout;  /**< Time until a button triggers another grip
                                            movement */
    CNxTimer        *m_timer;          /**< Controls slider button repeats */

    /**
     * Resize the scrollbar to the new dimensions.
     *
     * @param width The new width.
     * @param height The new height.
     */

    virtual void onResize(nxgl_coord_t width, nxgl_coord_t height);

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CScrollbarVertical(const CScrollbarVertical& scrollbarVertical)
    : CNxWidget(scrollbarVertical) { }

  public:

    /**
     * Constructor.
     *
     * @param pWidgetControl The widget control instance for the window.
     * @param x The x coordinate of the slider, relative to its parent.
     * @param y The y coordinate of the slider, relative to its parent.
     * @param width The width of the slider.
     * @param height The height of the slider.
     * @param style The style that the widget should use.  If this is not
     * specified, the widget will use the values stored in the global
     * g_defaultWidgetStyle object.  The widget will copy the properties of
     * the style into its own internal style object.
     */

    CScrollbarVertical(CWidgetControl *pWidgetControl,
                       nxgl_coord_t x, nxgl_coord_t y,
                       nxgl_coord_t width, nxgl_coord_t height,
                       CWidgetStyle *style = NULL);

    /**
     * Destructor.
     */

    virtual inline ~CScrollbarVertical(void) { }

    /**
     * Get the smallest value that the slider can represent.
     *
     * @return The smallest value.
     */

    const int getMinimumValue(void) const;

    /**
     * Get the largest value that the slider can represent.
     *
     * @return The largest value.
     */

    const int getMaximumValue(void) const;

    /**
     * Get the current value of the slider.
     *
     * @return The current slider value.
     */

    const int getValue(void) const;

    /**
     * Get the value represented by the height of the grip.
     * For sliders, this would typically be 1 (so each new
     * grip position is worth 1).  For scrollbars, this
     * would be the height of the scrolling widget.
     *
     * @return The page size.
     */

    const nxgl_coord_t getPageSize(void) const;

    /**
     * Set the smallest value that the slider can represent.
     *
     * @param value The smallest value.
     */

    void setMinimumValue(const int value);

    /**
     * Set the largest value that the slider can represent.
     *
     * @param value The largest value.
     */

    void setMaximumValue(const int value);

    /**
     * Set the value that of the slider.  This will reposition
     * and redraw the grip.
     *
     * @param value The new value.
     */

    void setValue(const int value);

    /**
     * Set the value that of the slider.  This will reposition and redraw
     * the grip.  The supplied value should be bitshifted left 16 places.
     * This ensures greater accuracy than the standard setValue() method if
     * the slider is being used as a scrollbar.
     *
     * @param value The new value.
     */

    void setValueWithBitshift(const int32_t value);

    /**
     * Set the page size represented by the grip.
     *
     * @param pageSize The page size.
     * @see getPageSize().
     */

    void setPageSize(const nxgl_coord_t pageSize);

    /**
     * Process events fired by the grip.
     *
     * @param e The event details.
     */

    virtual void handleActionEvent(const CWidgetEventArgs &e);

    /**
     * Process events fired by the grip.
     *
     * @param e The event details.
     */

    virtual void handleClickEvent(const CWidgetEventArgs &e);

    /**
     * Process events fired by the grip.
     *
     * @param e The event details.
     */

    virtual void handleReleaseEvent(const CWidgetEventArgs &e);

    /**
     * Process events fired by the grip.
     *
     * @param e The event details.
     */

    virtual void handleReleaseOutsideEvent(const CWidgetEventArgs &e);

    /**
     * Process events fired by the grip.
     *
     * @param e The event details.
     */

    virtual void handleValueChangeEvent(const CWidgetEventArgs &e);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CSCROLLBARVERTICAL_HXX
