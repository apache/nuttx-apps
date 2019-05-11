/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cwindow.hxx
// NxTerm window
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CNXTERM_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CNXTERM_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <sys/types.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxterm.h>

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/iapplication.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  /**
   * This class implements the NxTerm application.
   */

  class CNxTerm : public CTwm4NxEvent
  {
    private:
      CTaskbar            *m_twm4nx;      /**< Reference to the Twm4Nx session instance */
      CApplicationWindow  *m_window;      /**< Reference to the application window */
      NXTERM               m_nxterm;      /**< NxTerm handle */
      pid_t                m_pid;         /**< Task ID of the NxTerm thread */
      int                  m_minor;       /**< Terminal device minor number */

      /**
       * This is the NxTerm task.  This function first redirects output to the
       * console window then calls to start the NSH logic.
       */

      static int nxterm(int argc, char *argv[]);

      /**
       * Handle Twm4Nx events.  This overrides a method from CTwm4NXEvent
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

       bool event(FAR struct SEventMsg *eventmsg);

      /**
       * This is the NxTerm task exit handler.  It is registered with on_exit()
       * and called automatically when the nxterm task exits.
       */

      static void exitHandler(int code, FAR void *arg);

      /**
       * Called when the window minimize button is pressed.
       */

      void minimize(void);

      /**
       * Called when the window close button is pressed.
       */

      void close(void);

      /**
       * Start the application (perhaps in the minimized state).
       *
       * @return True if the application was successfully started.
       */

      bool run(void);

      /**
       * Stop the application.
       */

      void stop(void);

      /**
       * Destroy the application and free all of its resources.  This method
       * will initiate blocking of messages from the NX server.  The server
       * will flush the window message queue and reply with the blocked
       * message.  When the block message is received by CWindowMessenger,
       * it will send the destroy message to the start window task which
       * will, finally, safely delete the application.
       */

      void destroy(void);

      /**
       * The application window is hidden (either it is minimized or it is
       * maximized, but not at the top of the hierarchy
       */

      void hide(void);

      /**
       * Redraw the entire window.  The application has been maximized or
       * otherwise moved to the top of the hierarchy.  This method is call from
       * CTaskbar when the application window must be displayed
       */

      void redraw(void);

      /**
       * Report of this is a "normal" window or a full screen window.  The
       * primary purpose of this method is so that window manager will know
       * whether or not it show draw the task bar.
       *
       * @return True if this is a full screen window.
       */

      bool isFullScreen(void) const;
  };

  class CNxTermFactory : public IApplication, public IApplicationFactory
  {
    private:

      /**
       * One time NSH initialization. This function must be called exactly
       * once during the boot-up sequence to initialize the NSH library.
       *
       * @return True on successful initialization
       */

      bool nshlibInitialize(void);

      /**
       * Create and start a new instance of an CNxTerm.
       *
       * @param twm4nx.  The Twm4Nx session instance
       */

      static bool startFunction(CTwm4Nx *twm4n);

      /**
       * Return the Main Menu item string.  This overrides the method from
       * IApplication
       *
       * @param name The name of the application.
       */

      inline NXWidgets::CNxString getName(void)
      {
        return NXWidgets::CNxString("NuttShell");
      }

      /**
       * There is no sub-menu for this Main Menu item.  This overrides
       * the method from IApplication.
       *
       * @return This implementation will always return a null value.
       */

      inline FAR CMenus *getSubMenu(void)
      {
        return (FAR CMenus *)0;
      }

      /**
       * There is no application start-up function.  This function will not
       * be called in this implementation
       */

      inline TStartFunction getStartFunction(void)
      {
        return (TStartFunction)startFunction;
      }

      /**
       * There is no custom event handler.  We use the common event handler.
       *
       * @return.  null is always returned in this impementation.
       */

      inline FAR CTwm4NxEvent *getEventHandler(void)
      {
        return (FAR CTwm4NxEvent *)0;
      }

      /**
       * Return the Twm4Nx event that will be generated when the Main Menu
       * item is selected.
       *
       * @return. This function always returns EVENT_SYSTEM_NOP.
       */

      inline uint16_t getEvent(void)
      {
        return EVENT_SYSTEM_NOP;
      }

    public:

      /**
       * CNxTermFactory Constructor
       *
       * @param twm4nx.  The twm4nx instance used to terminate calibration
       */

      inline CNxTermFactory(void)
      {
      }

      /**
       * CNxTermFactory Destructor
       */

      inline ~CNxTermFactory(void)
      {
        // REVISIT:  Would need to remove Main Menu item
      }

      /**
       * CNxTermFactory Initializer.  Performs parts of the instance
       * construction that may fail.  In this implemenation, it will
       * initialize the NSH library and register an menu item in the
       * Main Menu.
       *
       * @param twm4nx.  The Twm4Nx session instance
       */

      bool CNxTermFactory::initialize(FAR CTwm4Nx *twm4nx);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CNXTERM_HXX
