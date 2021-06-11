/****************************************************************************
 * apps/include/graphics/nxwidgets/cwindoweventhandler.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWINDOWEVENTHANDLER_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWINDOWEVENTHANDLER_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "graphics/nxwidgets/nxconfig.hxx"

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
   * Base CWindowEventHandler class, intended to be subclassed.  Any class that
   * needs to listen for window events should inherit from this class.
   */

  class CWindowEventHandler
  {
  public:
    /**
     * Constructor.
     */

    inline CWindowEventHandler() { }

    /**
     * Destructor.
     */

    virtual inline ~CWindowEventHandler() { }

    /**
     * Handle a NX window redraw request event
     *
     * @param nxRect The region in the window to be redrawn
     * @param more More redraw requests will follow
     */

    virtual void handleRedrawEvent(FAR const nxgl_rect_s *nxRect, bool more)
    {
    }

    /**
     * Handle a NX window position/size change event
     */

    virtual void handleGeometryEvent(void) { }

#ifdef CONFIG_NX_XYINPUT
    /**
     * Handle an NX window mouse input event.
     */

    virtual void handleMouseEvent(FAR const struct nxgl_point_s *pos,
                                  uint8_t buttons)
    {
    }
#endif

#ifdef CONFIG_NX_KBD
    /**
     * Handle a NX window keyboard input event.
     */

    virtual void handleKeyboardEvent(void) { }
#endif

    /**
     * Handle a NX window blocked event
     *
     * @param arg - User provided argument (see nx_block or nxtk_block)
     */

    virtual void handleBlockedEvent(FAR void *arg) { }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWINDOWEVENTHANDLER_HXX
