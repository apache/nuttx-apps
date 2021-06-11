/****************************************************************************
 * apps/include/graphics/nxwidgets/cwindoweventhandlerlist.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWINDOWEVENTHANDLERLIST_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWINDOWEVENTHANDLERLIST_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cwindoweventhandler.hxx"
#include "graphics/nxwidgets/tnxarray.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class CListDataItem;

  /**
   * List of window event handlers.
   */
  class CWindowEventHandlerList
  {
  protected:
    TNxArray<CWindowEventHandler*> m_eventHandlers; /**< List of event handlers */

    /**
     * Return the index to the window event handler.
     */

    bool findWindowEventHandler(CWindowEventHandler *eventHandler, int &index);

  public:

    /**
     * Constructor.
     *
     * @param widget The owning widget.
     */

    CWindowEventHandlerList(void) { }

    /**
     * Destructor.
     */

    inline ~CWindowEventHandlerList(void) { }

    /**
     * Get the event handler at the specified index.
     *
     * @param index The index of the event handler.
     * @return The event handler at the specified index.
     */

    inline CWindowEventHandler *at(const int index) const
    {
      return m_eventHandlers.at(index);
    }

    /**
     * Get the size of the array.
     *
     * @return The size of the array.
     */

    inline const nxgl_coord_t size(void) const
    {
      return m_eventHandlers.size();
    }

    /**
     * Adds a window event handler.  The event handler will receive
     * all events raised by this object.
     * @param eventHandler A pointer to the event handler.
     */

    void addWindowEventHandler(CWindowEventHandler *eventHandler);

    /**
     * Remove a window event handler.
     *
     * @param eventHandler A pointer to the event handler to remove.
     */

    void removeWindowEventHandler(CWindowEventHandler *eventHandler);

    /**
     * Raise the NX window redraw event.
     *
     * @param nxRect The region in the window to be redrawn
     * @param more More redraw requests will follow
     */

    void raiseRedrawEvent(FAR const nxgl_rect_s *nxRect, bool more);

    /**
     * Raise an NX window position/size change event.
     */

    void raiseGeometryEvent(void);

#ifdef CONFIG_NX_XYINPUT
    /**
     * Raise an NX mouse window input event.
     */

    void raiseMouseEvent(FAR const struct nxgl_point_s *pos,
                         uint8_t buttons);
#endif

#ifdef CONFIG_NX_KBD
    /**
     * Raise an NX keyboard input event
     */

    void raiseKeyboardEvent(void);
#endif

    /**
     * Raise an NX window blocked event.
     *
     * @param arg - User provided argument (see nx_block or nxtk_block)
     */

    void raiseBlockedEvent(FAR void *arg);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWINDOWEVENTHANDLERLIST_HXX
