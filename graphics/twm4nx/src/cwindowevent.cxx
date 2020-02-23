// apps/graphics/twm4nx/src/cwindowevent.cxx
// Shim to manage the interface between NX messages and NxWidgets
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

// The logic path for mouse/touchscreen input is tortuous but flexible:
//
//  1. A listener thread receives mouse or touchscreen input and injects
//     that into NX via nx_mousein
//  2. In the multi-user mode, this will send a message to the NX server
//  3. The NX server will determine which window gets the mouse input
//     and send a window event message to the NX listener thread.
//  4. The NX listener thread receives a windows event.  The NX listener thread
//     is part of CTwm4Nx and was created when NX server connection was
//     established.  This event may be a positional change notification, a
//     redraw request, or mouse or keyboard input.  In this case, mouse input.
//  5. The NX listener thread handles the message by calling nx_eventhandler().
//     nx_eventhandler() dispatches the message by calling a method in the
//     NXWidgets::CCallback instance associated with the window.
//     NXWidgets::CCallback is a part of the CWidgetControl.
//  6. NXWidgets::CCallback calls into NXWidgets::CWidgetControl to process
//     the event.
//  7. NXWidgets::CWidgetControl records the new state data and raises a
//     window event.
//  8. NXWidgets::CWindowEventHandlerList will give the event to this method
//     NxWM::CWindowEvent.
//  9. This NxWM::CWindowEvent method will send a message to the Event
//     loop running in the Twm4Nx main thread.
// 10. The Twm4Nx main thread will call the CWindowEvent::event() method
//     which
// 11. Finally call pollEvents() to execute whatever actions the input event
//     should trigger.
// 12. This might call an event handler in and overridden method of
//     CWidgetEventHandler which will again notify the Event loop running
//     in the Twm4Nx main thread.  The event will, finally be delivered
//     to the recipient in its fully digested and decorated form.

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cfcntl>
#include <cerrno>

#include <semaphore.h>
#include <mqueue.h>

#include <nuttx/input/mouse.h>

#include "graphics/nxwidgets/cwidgetcontrol.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"

/////////////////////////////////////////////////////////////////////////////
// CWindowEvent Method Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CWindowEvent Constructor
 *
 * @param twm4nx The Twm4Nx session instance.
 * @param client The client window instance.
 * @param events Describes the application event configuration
 * @param style The default style that all widgets on this display
 *   should use.  If this is not specified, the widget will use the
 *   values stored in the defaultCWidgetStyle object.
 */

CWindowEvent::CWindowEvent(FAR CTwm4Nx *twm4nx, FAR void *client,
                           FAR const struct SAppEvents &events,
                           FAR const NXWidgets::CWidgetStyle *style)
: NXWidgets::CWidgetControl(style)
{
  m_twm4nx                = twm4nx;              // Cache the Twm4Nx session
  m_clientWindow          = client;              // Cache the client window instance

  // Events

  m_appEvents.eventObj    = events.eventObj;     // Event object reference
  m_appEvents.redrawEvent = events.redrawEvent;  // Redraw event ID
  m_appEvents.mouseEvent  = events.mouseEvent;   // Mouse/touchscreen event ID
  m_appEvents.kbdEvent    = events.kbdEvent;     // Keyboard event ID
  m_appEvents.closeEvent  = events.closeEvent;   // Window close event ID
  m_appEvents.deleteEvent = events.deleteEvent;  // Window delete event ID

  // Dragging

  m_tapHandler            = (FAR IEventTap *)0;  // No event tap handler callbacks
  m_tapArg                = (uintptr_t)0;        // No callback argument

  // Open a message queue to send raw NX events.  This cannot fail!

  FAR const char *mqname  = twm4nx->getEventQueueName();
  m_eventq = mq_open(mqname, O_WRONLY);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             mqname, errno);
    }

  // Add ourself to the list of window event handlers

  addWindowEventHandler(this);
}

/**
 * CWindowEvent Destructor.
 */

CWindowEvent::~CWindowEvent(void)
{
 // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Remove ourself from the list of the window event handlers

  removeWindowEventHandler(this);
}

/**
 * Handle a NX window redraw request event
 *
 * @param nxRect The region in the window to be redrawn
 * @param more More redraw requests will follow
 */

void CWindowEvent::handleRedrawEvent(FAR const nxgl_rect_s *nxRect,
                                     bool more)
{
  twminfo("Redraw events\n");

  // Does the user need redraw events?

  if (m_appEvents.redrawEvent != EVENT_SYSTEM_NOP)
    {
      struct SRedrawEventMsg msg;
      msg.eventID    = m_appEvents.redrawEvent;
      msg.obj        = m_appEvents.eventObj;  // For CWindow events
      msg.handler    = m_appEvents.eventObj;  // For external applications
      msg.rect.pt1.x = nxRect->pt1.x;
      msg.rect.pt1.y = nxRect->pt1.y;
      msg.rect.pt2.x = nxRect->pt2.x;
      msg.rect.pt2.y = nxRect->pt2.y;
      msg.more       = more;

      // NOTE that we cannot block because we are on the same thread
      // as the message reader.  If the event queue becomes full then
      // we have no other option but to lose events.
      //
      // I suppose we could recurse and call Twm4Nx::dispatchEvent at
      // the risk of runaway stack usage.

      int ret = mq_send(m_eventq, (FAR const char *)&msg,
                        sizeof(struct SRedrawEventMsg), 100);
      if (ret < 0)
        {
          twmerr("ERROR: mq_send failed: %d\n", errno);
        }
    }
}

#ifdef CONFIG_NX_XYINPUT
/**
 * Handle an NX window mouse input event.
 *
 * One complexity is that with framed windows, the click starts in the
 * toolbar, but can easily move into the main window (or even outside
 * of the window!).  To these case, there may be two instances of
 * CWindowEvent, one for the toolbar and one for the main window.  The
 * IEventTap implementation (along with the user argument) can keep a
 * consistent movement context across both instances.
 *
 * NOTE:  NX will continually forward the mouse events to the same raw
 * window in all cases.. even when the mouse position moves outside of
 * the window.  It is the NxTK layer that converts the reports mouse
 * event to either toolbar or main window reports.
 */

void CWindowEvent::handleMouseEvent(FAR const struct nxgl_point_s *pos,
                                    uint8_t buttons)
{
  // Check if There is an active tap on mouse events

  if (m_tapHandler != (FAR IEventTap *)0)
    {
      twminfo("Mouse input: active=%u\n",
               m_tapHandler->isActive(m_tapArg));

      // The new mouse position in window relative display coordinates

      struct nxgl_point_s mousePos;
      mousePos.x = pos->x;
      mousePos.y = pos->y;

      // STATE         LEFT BUTTON       ACTION
      // active        clicked           moveEvent
      // active        released          dropEvent
      // NOT active    clicked           May be detected as a grab
      // NOT active    released          None

      if (m_tapHandler->isActive(m_tapArg))
        {
          // Is the left button still pressed?

          if ((buttons & MOUSE_BUTTON_1) != 0)
            {
              twminfo("Continue movemenht (%d,%d) buttons=%02x m_tapHandler=%p\n",
                      mousePos.x, mousePos.y, buttons, m_tapHandler);

              // Yes.. generate a movement event if we have a tap event handler

              if (m_tapHandler->moveEvent(mousePos, m_tapArg))
                {
                  // Skip the input poll until the movement completes

                  return;
                }
            }
          else
            {
              twminfo("Stop movement (%d,%d) buttons=%02x m_tapHandler=%p\n",
                      mousePos.x, mousePos.y, buttons, m_tapHandler);

              // No.. then the tap is no longer active

               m_tapHandler->enableMovement(mousePos, false, m_tapArg);

              // Generate a dropEvent

              if (m_tapHandler->dropEvent(mousePos, m_tapArg))
                {
                  // If the drop event was processed then skip the
                  // input poll until AFTER the movement completes

                  return;
                }
            }
        }

      // If we are not currently moving anything but the left button is
      // pressed, then start a movement event

      else if ((buttons & MOUSE_BUTTON_1) != 0)
        {
          // Indicate that we are (or may be) moving

          m_tapHandler->enableMovement(mousePos, true, m_tapArg);

          twminfo("Start moving (%d,%d) buttons=%02x m_tapHandler=%p\n",
                  pos->x, pos->y, buttons, m_tapHandler);

          // But take no other actions until the window recognizes the grab
        }
    }

  // Does the user want to know about mouse input?

  if (m_appEvents.mouseEvent != EVENT_SYSTEM_NOP)
    {
      // Stimulate an XY input poll

      twminfo("Mouse Input...\n");

      struct SXyInputEventMsg msg;
      msg.eventID = m_appEvents.mouseEvent;
      msg.obj     = m_appEvents.eventObj;  // For CWindow events
      msg.handler = m_appEvents.eventObj;  // For external applications
      msg.pos.x   = pos->x;
      msg.pos.y   = pos->y;
      msg.buttons = buttons;

      int ret = mq_send(m_eventq, (FAR const char *)&msg,
                        sizeof(struct SXyInputEventMsg), 100);
      if (ret < 0)
        {
          twmerr("ERROR: mq_send failed: %d\n", errno);
        }
    }
}
#endif

#ifdef CONFIG_NX_KBD
/**
 * Handle a NX window keyboard input event.
 */

void CWindowEvent::handleKeyboardEvent(void)
{
  // Does the user want to know about keyboard input?

  if (m_appEvents.kbdEvent != EVENT_SYSTEM_NOP)
    {
      twminfo("Keyboard input...\n");

      // Stimulate an keyboard event widget poll

      struct SNxEventMsg msg;
      msg.eventID  = m_appEvents.kbdEvent;
      msg.obj      = m_appEvents.eventObj;  // For CWindow events
      msg.handler  = m_appEvents.eventObj;  // For external applications
      msg.instance = this;

      int ret = mq_send(m_eventq, (FAR const char *)&msg,
                        sizeof(struct SNxEventMsg), 100);
      if (ret < 0)
        {
          twmerr("ERROR: mq_send failed: %d\n", errno);
        }
    }
}
#endif

/**
 * Handle a NX window blocked event.  This handler is called when we
 * receive the BLOCKED message meaning that there are no further pending
 * actions on the window.  It is now safe to delete the window.
 *
 * This is handled by sending a message to the start window thread (vs just
 * calling the destructors) because in the case where an application
 * destroys itself (because of pressing the stop button), then we need to
 * unwind and get out of the application logic before destroying all of its
 * objects.
 *
 * @param arg - User provided argument (see nx_block or nxtk_block)
 */

void CWindowEvent::handleBlockedEvent(FAR void *arg)
{
  twminfo("Blocked...\n");

  struct SNxEventMsg msg;
  msg.eventID  = m_appEvents.deleteEvent;
  msg.obj      = m_clientWindow;          // For CWindow events
  msg.handler  = m_appEvents.eventObj;    // For external applications
  msg.instance = this;

  int ret = mq_send(m_eventq, (FAR const char *)&msg,
                    sizeof(struct SNxEventMsg), 100);
  if (ret < 0)
    {
      twmerr("ERROR: mq_send failed: %d\n", errno);
    }
}
