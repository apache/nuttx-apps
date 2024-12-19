/****************************************************************************
 * apps/include/graphics/nxwidgets/cwidgeteventhandler.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWIDGETEVENTHANDLER_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWIDGETEVENTHANDLER_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include "graphics/nxwidgets/cwidgeteventargs.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class CNxWidget;

  /**
   * Base CWidgetEventHandler class, intended to be subclassed.  Any class that
   * needs to listen for widget events should inherit from this class.
   */

  class CWidgetEventHandler
  {
  public:
    /**
     * Constructor.
     */

    inline CWidgetEventHandler() { }

    /**
     * Destructor.
     */

    virtual inline ~CWidgetEventHandler() { }

    /**
     * Handle a mouse button click event.
     *
     * @param e The event data.
     */

    virtual void handleClickEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a mouse button drag event.
     *
     * @param e The event data.
     */

    virtual void handleDragEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a drop event, triggered when the widget has been dragged-and-dropped.
     *
     * @param e The event data.
     */

    virtual void handleDropEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a mouse button release event that occurred within the bounds of
     * the source widget.
     *
     * @param e The event data.
     */

    virtual void handleReleaseEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a mouse button release event that occurred outside the bounds of
     * the source widget.
     *
     * @param e The event data.
     */

    virtual void handleReleaseOutsideEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a key press event.
     *
     * @param e The event data.
     */

    virtual void handleKeyPressEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a cursor control event.
     *
     * @param e The event data.
     */

    virtual void handleCursorControlEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a key repeat event.
     *
     * @param e The event data.
     */

    virtual void handleFocusEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget blur event.
     *
     * @param e The event data.
     */

    virtual void handleBlurEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget close event.
     *
     * @param e The event data.
     */

    virtual void handleCloseEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget hide event.
     *
     * @param e The event data.
     */

    virtual void handleHideEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget show event.
     *
     * @param e The event data.
     */

    virtual void handleShowEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget enable event.
     *
     * @param e The event data.
     */

    virtual void handleEnableEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget disable event.
     *
     * @param e The event data.
     */

    virtual void handleDisableEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget value change event.
     *
     * @param e The event data.
     */

    virtual void handleValueChangeEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget resize event.
     *
     * @param e The event data.
     */

    virtual void handleResizeEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget move event.
     *
     * @param e The event data.
     */

    virtual void handleMoveEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget scroll event.
     *
     * @param e The event data.
     */

    virtual void handleScrollEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget double-click event.
     *
     * @param e The event data.
     */

    virtual void handleDoubleClickEvent(const CWidgetEventArgs &e) { }

    /**
     * Handle a widget action event.  For CImage, this is a mouse button pre-release event.
     *
     * @param e The event data.
     */

    virtual void handleActionEvent(const CWidgetEventArgs &e) { }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWIDGETEVENTHANDLER_HXX
