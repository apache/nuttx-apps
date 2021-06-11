/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/cclock.hxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CCLOCK_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CCLOCK_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <sys/types.h>
#include <nuttx/nx/nxtk.h>

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/iapplication.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

// CClock application events
// Window Events

#define EVENT_CLOCK_REDRAW    (EVENT_RECIPIENT_APP | 0x0000) /* Not necessary */
#define EVENT_CLOCK_RESIZE    EVENT_SYSTEM_NOP
#define EVENT_CLOCK_XYINPUT   EVENT_SYSTEM_NOP
#define EVENT_CLOCK_KBDINPUT  EVENT_SYSTEM_NOP
#define EVENT_CLOCK_DELETE    EVENT_WINDOW_DELETE

// Button Events

#define EVENT_CLOCK_CLOSE    (EVENT_RECIPIENT_APP | 0x0001)

// Menu Events

#define EVENT_CLOCK_START    (EVENT_RECIPIENT_APP | 0x0002)

// Number of digits in clock display

#define CLOCK_NDIGITS        4

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace SLcd
{
  class CSLcd;                   // Forward reference
}

namespace Twm4Nx
{
  /**
   * This structure describes the state of one clock digit
   */

  struct SClockDigit
  {
    nxgl_coord_t xOffset;         /**< X offset from left side of the window */
    uint8_t segments;             /**< LCD segment code of number */
  };

  /**
   * This class implements the Clock application.
   */

  class CClock : public CTwm4NxEvent
  {
    private:
      FAR CTwm4Nx     *m_twm4nx;  /**< Reference to the Twm4Nx session instance */
      FAR CWindow     *m_window;  /**< Reference to the Clock application window */
      FAR SLcd::CSLcd *m_slcd;    /**< Reference to the segment LCD helper */
      pid_t            m_pid;     /**< Task ID of the Clock thread */

      // The current clock state, digit-by-digit:

      struct SClockDigit m_digits[CLOCK_NDIGITS];

      /**
       * This is the Clock task.
       */

      static int clock(int argc, char *argv[]);

      /**
       * Handle Twm4Nx events.  This overrides a method from CTwm4NXEvent
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);

      /**
       * Update the Clock.
       */

      void update(void);

      /**
       * Redraw the entire clock.
       */

      void redraw(void);

      /**
       * This is the close window event handler.  It will stop the Clock
       * application thread.
       */

      void stop(void);

  public:

      /**
       * CClock constructor
       *
       * @param twm4nx.  The Twm4Nx session instance
       */

      CClock(FAR CTwm4Nx *twm4nx);

      /**
       * CClock destructor
       */

      ~CClock(void);

      /**
       * CClock initializers.  Perform miscellaneous post-construction
       * initialization that may fail (and hence is not appropriate to be
       * done in the constructor)
       *
       * @return True if the Clock application was successfully initialized.
       */

      bool initialize(void);

      /**
       * Start the Clock.
       *
       * @return True if the Clock application was successfully started.
       */

      bool run(void);
  };

  class CClockFactory : public IApplication,
                        public IApplicationFactory,
                        public CTwm4NxEvent
  {
    private:

      FAR CTwm4Nx *m_twm4nx; /**< Twm4Nx session instance */

      /**
       * Handle CClockFactory events.  This overrides a method from
       * CTwm4NXEvent
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);

      /**
       * Create and start a new instance of an CClock.
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
        return NXWidgets::CNxString("Clock");
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
        return EVENT_CLOCK_START;
      }

    public:

      /**
       * CClockFactory Constructor
       *
       * @param twm4nx.  The Twm4Nx session instance
       */

      inline CClockFactory(void)
      {
        m_twm4nx = (FAR CTwm4Nx *)0;
      }

      /**
       * CClockFactory Destructor
       */

      inline ~CClockFactory(void)
      {
        // REVISIT:  Would need to remove Main Menu item
      }

      /**
       * CClockFactory Initializer.  Performs parts of the instance
       * construction that may fail.  In this implementation, it will
       * initialize the NSH library and register an menu item in the
       * Main Menu.
       */

      bool initialize(FAR CTwm4Nx *twm4nx);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CCLOCK_HXX
