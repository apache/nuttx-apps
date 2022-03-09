/****************************************************************************
 * apps/graphics/nxwidgets/src/cwindoweventhandlerlist.cxx
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

#include "graphics/nxwidgets/nxconfig.hxx"

#include "graphics/nxwidgets/cwindoweventhandler.hxx"
#include "graphics/nxwidgets/cwindoweventhandlerlist.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

/**
 * Adds a window event handler.  The event handler will receive
 * all events raised by this object.
 * @param eventHandler A pointer to the event handler.
 */

void CWindowEventHandlerList::addWindowEventHandler(CWindowEventHandler *eventHandler)
{
  // Make sure that the event handler does not already exist

  int index;
  if (!findWindowEventHandler(eventHandler, index))
    {
      // Add the new handler

      m_eventHandlers.push_back(eventHandler);
    }
}

/**
 * Remove a window event handler.
 *
 * @param eventHandler A pointer to the event handler to remove.
 */

void CWindowEventHandlerList::removeWindowEventHandler(CWindowEventHandler *eventHandler)
{
  // Find the event handler to be removed

  int index;
  if (findWindowEventHandler(eventHandler, index))
    {
      // and remove it

      m_eventHandlers.erase(index);
    }
}

/**
 * Return the index to the window event handler.
 */

bool CWindowEventHandlerList::findWindowEventHandler(CWindowEventHandler *eventHandler, int &index)
{
  for (int i = 0; i < m_eventHandlers.size(); ++i)
    {
      if (m_eventHandlers.at(i) == eventHandler)
        {
          index = i;
          return true;
        }
    }

  return false;
}

/**
 * Raise the NX window redraw event.
 */

void CWindowEventHandlerList::raiseRedrawEvent(FAR const struct nxgl_rect_s *nxRect, bool more)
{
  for (int i = 0; i < m_eventHandlers.size(); ++i)
    {
      m_eventHandlers.at(i)->handleRedrawEvent(nxRect, more);
    }
}

/**
 * Raise an NX window position/size change event.
 */

void CWindowEventHandlerList::raiseGeometryEvent(void)
{
  for (int i = 0; i < m_eventHandlers.size(); ++i)
    {
      m_eventHandlers.at(i)->handleGeometryEvent();
    }
}

#ifdef CONFIG_NX_XYINPUT
/**
 * Raise an NX mouse window input event.
 */

void CWindowEventHandlerList::raiseMouseEvent(FAR const struct nxgl_point_s *pos,
                                              uint8_t buttons)
{
  for (int i = 0; i < m_eventHandlers.size(); ++i)
    {
      m_eventHandlers.at(i)->handleMouseEvent(pos, buttons);
    }
}
#endif

#ifdef CONFIG_NX_KBD
/**
 * Raise an NX keyboard input event
 */

void CWindowEventHandlerList::raiseKeyboardEvent(void)
{
  for (int i = 0; i < m_eventHandlers.size(); ++i)
    {
      m_eventHandlers.at(i)->handleKeyboardEvent();
    }
#endif
}

/**
 * Raise an NX window blocked event.
 *
 * @param arg - User provided argument (see nx_block or nxtk_block)
 */

void CWindowEventHandlerList::raiseBlockedEvent(FAR void *arg)
{
  for (int i = 0; i < m_eventHandlers.size(); ++i)
    {
      m_eventHandlers.at(i)->handleBlockedEvent(arg);
    }
}
