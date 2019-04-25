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
#include <mqueue.h>

#include "graphics/nxwidgets/cwindoweventhandler.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)

namespace Twm4Nx
{
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
      FAR CTwm4Nx *m_twm4nx;    /**< Cached instance of CTwm4Nx */
      mqd_t        m_eventq;    /**< NxWidget event message queue */

      /**
       * Send the EVENT_MSG_POLL input event message to the Twm4Nx event loop.
       */

      void sendInputEvent(void);

      // Override CWidgetEventHandler virutal methods ///////////////////////

#ifdef CONFIG_NX_XYINPUT
      /**
       * Handle an NX window mouse input event.
       */

      void handleMouseEvent(void);
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
       * @param twm4nx.  The Twm4Nx session instance.
       * @param style The default style that all widgets on this display
       *   should use.  If this is not specified, the widget will use the
       *   values stored in the defaultCWidgetStyle object.
       */

       CWindowEvent(FAR CTwm4Nx *twm4nx,
                    FAR const NXWidgets::CWidgetStyle *style =
                    (const NXWidgets::CWidgetStyle *)NULL);

      /**
       * CWindowEvent Destructor.
       */

     ~CWindowEvent(void);

      /**
       * Handle MSG events.
       *
       * @param msg.  The received system MSG event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      static bool event(FAR struct SEventMsg *msg);
  };
}
#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWEVENT_HXX
