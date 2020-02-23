/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/cnxterm.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CNXTERM_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CNXTERM_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <sys/types.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxterm.h>

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/iapplication.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

// CNxTerm application events
// Window Events

#define EVENT_NXTERM_REDRAW   (EVENT_RECIPIENT_APP | EVENT_CRITICAL | 0x0000)
#define EVENT_NXTERM_RESIZE   (EVENT_RECIPIENT_APP | EVENT_CRITICAL | 0x0001)
#define EVENT_NXTERM_XYINPUT   EVENT_SYSTEM_NOP
#define EVENT_NXTERM_KBDINPUT  EVENT_SYSTEM_NOP
#define EVENT_NXTERM_DELETE    EVENT_WINDOW_DELETE

// Button Events

#define EVENT_NXTERM_CLOSE    (EVENT_RECIPIENT_APP | 0x0002)

// Menu Events

#define EVENT_NXTERM_START    (EVENT_RECIPIENT_APP | 0x0003)

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
      FAR CTwm4Nx *m_twm4nx;        /**< Reference to the Twm4Nx session instance */
      FAR CWindow *m_nxtermWindow;  /**< Reference to the NxTerm application window */
      NXTERM       m_NxTerm;        /**< NxTerm handle */
      pid_t        m_pid;           /**< Task ID of the NxTerm thread */
      int          m_minor;         /**< Terminal device minor number */

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
       * Handle the NxTerm redraw event.
       */

      void redraw(void);

      /**
       * inform NxTerm of a new window size.
       */

      void resize(void);

      /**
       * This is the close window event handler.  It will stop the NxTerm
       * application thread.
       */

      void stop(void);

  public:

      /**
       * CNxTerm constructor
       *
       * @param twm4nx.  The Twm4Nx session instance
       */

      CNxTerm(FAR CTwm4Nx *twm4nx);

      /**
       * CNxTerm destructor
       */

      ~CNxTerm(void);

      /**
       * CNxTerm initializers.  Perform miscellaneous post-construction
       * initialization that may fail (and hence is not appropriate to be
       * done in the constructor)
       *
       * @return True if the NxTerm application was successfully initialized.
       */

      bool initialize(void);

      /**
       * Start the NxTerm.
       *
       * @return True if the NxTerm application was successfully started.
       */

      bool run(void);
  };

  class CNxTermFactory : public IApplication,
                         public IApplicationFactory,
                         public CTwm4NxEvent
  {
    private:

      FAR CTwm4Nx *m_twm4nx; /**< Twm4Nx session instance */

      /**
       * One time NSH initialization. This function must be called exactly
       * once during the boot-up sequence to initialize the NSH library.
       *
       * @return True on successful initialization
       */

      bool nshlibInitialize(void);

      /**
       * Handle CNxTermFactory events.  This overrides a method from
       * CTwm4NXEvent
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);

      /**
       * Create and start a new instance of an CNxTerm.
       */

      bool startFunction(void);

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
       * There is no custom event handler.  We use the common event handler.
       *
       * @return.  null is always returned in this implementation.
       */

      inline FAR CTwm4NxEvent *getEventHandler(void)
      {
        return (FAR CTwm4NxEvent *)this;
      }

      /**
       * Return the Twm4Nx event that will be generated when the Main Menu
       * item is selected.
       *
       * @return. This function always returns EVENT_SYSTEM_NOP.
       */

      inline uint16_t getEvent(void)
      {
        return EVENT_NXTERM_START;
      }

    public:

      /**
       * CNxTermFactory Constructor
       *
       * @param twm4nx.  The Twm4Nx session instance
       */

      inline CNxTermFactory(void)
      {
        m_twm4nx = (FAR CTwm4Nx *)0;
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
       * construction that may fail.  In this implementation, it will
       * initialize the NSH library and register an menu item in the
       * Main Menu.
       */

      bool initialize(FAR CTwm4Nx *twm4nx);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CNXTERM_HXX
