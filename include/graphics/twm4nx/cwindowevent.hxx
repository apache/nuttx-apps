/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cwindowevent.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWEVENT_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWEVENT_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <sys/types.h>
#include <cstdbool>
#include <mqueue.h>

#include "graphics/nxwidgets/cwindoweventhandler.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  class CWindow;                // Forward reference

  // This structure provides information to support application events

  struct SAppEvents
  {
    FAR void     *eventObj;     /**< Object reference that accompanies events */
    nxgl_coord_t  minWidth;     /**< The minimum width of the window */
    uint16_t      redrawEvent;  /**< Redraw event ID */
    uint16_t      mouseEvent;   /**< Mouse/touchscreen event ID */
    uint16_t      kbdEvent;     /**< Keyboard event ID */
    uint16_t      closeEvent;   /**< Window close event ID */
  };

  /**
   * This abstract base class provides add on methods to support movement
   * of a window.
   */

  class IEventTap
  {
    public:
      /**
       * A virtual destructor is required in order to override the ITextBox
       * destructor.  We do this because if we delete ITextBox, we want the
       * destructor of the class that inherits from ITextBox to run, not this
       * one.
       */

      virtual ~IEventTap(void)
      {
      }

      /**
       * This function is called when there is any movement of the mouse or
       * touch position that would indicate that the object is being moved.
       *
       * @param pos The current mouse/touch X/Y position in toolbar relative
       *   coordinates.
       * @param arg The user-argument provided that accompanies the callback
       * @return True: if the movement event was processed; false it is was
       *   ignored.  The event should be ignored if there is not actually
       *   a movement event in progress
       */

      virtual bool moveEvent(FAR const struct nxgl_point_s &pos,
                             uintptr_t arg) = 0;

      /**
       * This function is called if the mouse left button is released or
       * if the touchscreen touch is lost.  This indicates that the
       * movement sequence is complete.
       *
       * @param pos The last mouse/touch X/Y position in toolbar relative
       *   coordinates.
       * @param arg The user-argument provided that accompanies the callback
       * @return True: If the movement event was processed; false it is was
       *   ignored.  The event should be ignored if there is not actually
       *   a movement event in progress
       */

      virtual bool dropEvent(FAR const struct nxgl_point_s &pos,
                             uintptr_t arg) = 0;

      /**
       * Is the tap active?
       *
       * @param arg The user-argument provided that accompanies the callback
       * @return True: If the tap is enabled.
       */

      virtual bool isActive(uintptr_t arg) = 0;

      /**
       * Enable/disable movement
       *
       * @param enable.  True:  Enable movement
       * @param arg The user-argument provided that accompanies the callback
       */

      virtual void enableMovement(bool enable, uintptr_t arg) = 0;
  };

  /**
   * The class CWindowEvent integrates the widget control with some special
   * handling of mouse and keyboard inputs needs by NxWM.  It use used
   * in place of CWidgetControl whenever an NxWM window is created.
   *
   * CWindowEvent cohabitates with CWidgetControl only because it needs the
   * CWidgetControl as an argument in its messaging.
   */

  class CWindowEvent : public NXWidgets::CWindowEventHandler,
                       public NXWidgets::CWidgetControl
  {
    private:
      FAR CTwm4Nx         *m_twm4nx;        /**< Cached instance of CTwm4Nx */
      FAR CWindow         *m_clientWindow;  /**< The client window instance */
      mqd_t                m_eventq;        /**< NxWidget event message queue */
      struct SAppEvents    m_appEvents;     /**< Application event information */

      // Dragging

      FAR IEventTap       *m_tapHandler;    /**< Event tap handlers (may be NULL) */
      uintptr_t            m_tapArg;        /**< User argument associated with callback */

      // Override CWidgetEventHandler virtual methods ///////////////////////

      /**
       * Handle a NX window redraw request event
       *
       * @param nxRect The region in the window to be redrawn
       * @param more More redraw requests will follow
       */

      void handleRedrawEvent(FAR const nxgl_rect_s *nxRect, bool more);

#ifdef CONFIG_NX_XYINPUT
      /**
       * Handle an NX window mouse input event.
       */

      void handleMouseEvent(FAR const struct nxgl_point_s *pos,
                            uint8_t buttons);
#endif

#ifdef CONFIG_NX_KBD
      /**
       * Handle a NX window keyboard input event.
       */

      void handleKeyboardEvent(void);
#endif

      /**
       * Handle a NX window blocked event
       *
       * @param arg - User provided argument (see nx_block or nxtk_block)
       */

      void handleBlockedEvent(FAR void *arg);

    public:

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

       CWindowEvent(FAR CTwm4Nx *twm4nx, FAR CWindow *client,
                    FAR const struct SAppEvents &events,
                    FAR const NXWidgets::CWidgetStyle *style =
                    (const NXWidgets::CWidgetStyle *)NULL);

      /**
       * CWindowEvent Destructor.
       */

      ~CWindowEvent(void);

      /**
       * Register an IEventTap instance to provide callbacks when mouse
       * movement is received.  A mouse movement with the left button down
       * or a touchscreen touch movement are treated as a drag event. 
       * Release of the mouse left button or loss of the touchscreen touch
       * is treated as a drop event.
       *
       * @param tapHandler A reference to the IEventTap callback interface.
       * @param arg The argument returned with the IEventTap callbacks.
       */

      inline void installEventTap(FAR IEventTap *tapHandler, uintptr_t arg)
      {
         m_tapHandler = tapHandler;
         m_tapArg     = arg;
      }

      /**
       * Return the installed event tap.  This is useful if you want to
       * install a different event tap, then restore the event tap returned
       * by this method when you are finished.
       *
       * @param tapHandler The location to return IEventTap callback interface.
       * @param arg The loation to return the IEventTap argument
       */

      inline void getEventTap(FAR IEventTap *&tapHandler, uintptr_t &arg)
      {
         m_tapHandler = tapHandler;
         m_tapArg     = arg;
      }

      /**
       * Modify event handlers.
       *
       * @param events Describes the application event configuration
       * @return True is returned on success
       */

      inline bool configureEvents(FAR const struct SAppEvents &events)
      {
        m_appEvents.eventObj    = events.eventObj;    // Event object reference
        m_appEvents.redrawEvent = events.redrawEvent; // Redraw event ID
        m_appEvents.mouseEvent  = events.mouseEvent;  // Mouse/touchscreen event ID
        m_appEvents.kbdEvent    = events.kbdEvent;    // Keyboard event ID
        m_appEvents.closeEvent  = events.closeEvent;  // Window close event ID
        return true;
      }
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWEVENT_HXX
