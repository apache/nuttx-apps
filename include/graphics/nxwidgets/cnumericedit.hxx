/****************************************************************************
 * apps/include/graphics/nxwidgets/cnumericedit.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CNUMERICEDIT_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CNUMERICEDIT_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cnxwidget.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cnxstring.hxx"

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
  class CRect;
  class CLabel;
  class CButton;
  class CNxTimer;

  /**
   * Numeric edit control, with plus and minus buttons.
   */

  class CNumericEdit : public CNxWidget, public CWidgetEventHandler
  {
  protected:
    CLabel *m_label;
    CButton *m_button_minus;
    CButton *m_button_plus;
    CNxTimer *m_timer;
    CNxString m_unittext;
    int m_value;
    int m_minimum;
    int m_maximum;
    int m_increment;
    int m_timercount;

    /**
     * Resize the widget to the new dimensions.
     *
     * @param width The new width.
     * @param height The new height.
     */

    virtual void onResize(nxgl_coord_t width, nxgl_coord_t height);

    virtual void handleClickEvent(const CWidgetEventArgs &e);

    virtual void handleReleaseEvent(const CWidgetEventArgs &e);

    virtual void handleReleaseOutsideEvent(const CWidgetEventArgs &e);

    virtual void handleActionEvent(const CWidgetEventArgs &e);

    virtual void handleDragEvent(const CWidgetEventArgs &e);

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CNumericEdit(const CNumericEdit &num) : CNxWidget(num) { };

    void updateText();

  public:

    /**
     * Constructor for a numeric edit control.
     *
     * @param pWidgetControl The controlling widget for the display
     * @param x The x coordinate of the text box, relative to its parent.
     * @param y The y coordinate of the text box, relative to its parent.
     * @param width The width of the textbox.
     * @param height The height of the textbox.
     * @param style The style that the button should use.  If this is not
     *        specified, the button will use the global default widget
     *        style.
     */

    CNumericEdit(CWidgetControl *pWidgetControl, nxgl_coord_t x, nxgl_coord_t y,
           nxgl_coord_t width, nxgl_coord_t height,
           CWidgetStyle *style = NULL);

    /**
     * Destructor.
     */

    virtual ~CNumericEdit();

    /**
     * Insert the dimensions that this widget wants to have into the rect
     * passed in as a parameter.  All coordinates are relative to the
     * widget's parent.
     *
     * @param rect Reference to a rect to populate with data.
     */

    virtual void getPreferredDimensions(CRect &rect) const;

    /**
     * Sets the font.
     *
     * @param font A pointer to the font to use.
     */

    virtual void setFont(CNxFont *font);

    /**
     * Sets the text to display after the numeric value.
     */

    void setUnit(const CNxString& text);

    inline int getValue() const { return m_value; }
    void setValue(int value);

    inline int getMaximum() const { return m_maximum; }
    inline void setMaximum(int value) { m_maximum = value; setValue(m_value); }

    inline int getMinimum() const { return m_minimum; }
    inline void setMinimum(int value) { m_minimum = value; setValue(m_value); }

    inline int getIncrement() const { return m_increment; }
    inline void setIncrement(int value) { m_increment = value; setValue(m_value); }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CLABEL_HXX
