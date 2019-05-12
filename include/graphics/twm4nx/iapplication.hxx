/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/iapplication.hxx
// Application/Main Menu Interface
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_IAPPLICATION_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_IAPPLICATION_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <cstdint>

/////////////////////////////////////////////////////////////////////////////
// Abstract Base Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class CNxString;    // Forward reference
}

namespace Twm4Nx
{
  class CTwm4Nx;      // Forward reference
  class CMenus;       // Forward reference
  class CTwm4NxEvent; // Forward reference

  /**
   * Defines the interface of an application to the Main Menu.  "Built-In"
   * applications are started via CMainMenu.  This interface class defines
   * the interface requirements to add an application to the Main Menu.
   */

  class IApplication
  {
  public:

      /**
       * A virtual destructor is required in order to override the
       * IApplication destructor.  We do this because if we delete
       * IApplication, we want the destructor of the class that inherits from
       * IApplication to run, not this one.
       */

      virtual ~IApplication(void)
      {
      }

      /**
       * Return the name of the application.  This is the string that will
       * appear in the Main Menu item.
       *
       * @return  The name of the application.
       */

      virtual NXWidgets::CNxString getName(void) = 0;

      /**
       * Return any submenu item associated with the menu entry.  If a non-
       * null value is returned, then this sub-menu will be brought up when
       * the menu entry is selected.
       *
       * NOTEs:
       * - The getSubMenu() method is ignored if the event() method returns
       *   an event with recipient = EVENT_RECIPIENT_APP.  In that case, the
       *   application will be fully responsible for handling  the menu
       *   selection event.  Otherwise, the sub-menu takes precedence over
       *   the event.
       *
       * Precedence:
       * 1. Event with recipient == EVENT_RECIPIENT_APP.  getEventHandler()
       *    must return a non-NULL instance in this case.
       * 2. Sub-menu
       * 3. Event with other recipients
       *
       * @return.  A reference to any sub-menu that should be brought up if
       *   the menu item is selected.  This must be null if the menu item
       *   does not bring up a sub-menu
       */

      virtual FAR CMenus *getSubMenu(void) = 0;

      /**
       * External applications may provide their own event handler that runs
       * when the the menu item is selection.  If so, then this method will
       * return the instance of CTwm4NxEvent that will handle the event.
       *
       * NOTE:  This handler is only used if the event returned by getEvent is
       * destined for recipient EVENT_RECIPIENT_APP.  Otherwise, the event
       * handling is handling internally by Twm4Nx and this method will never
       * be called.
       *
       * This method may return null in that case.  It would be an error if
       * this method returned null but the event() method returned a event
       * destined for EVENT_RECIPIENT_APP.
       *
       * @return.  A reference to an instance of the CTwm4NxWevent handler
       *   class or NULL if the event will be handled by Twm4Nx (i.e., the
       *   recipient is not EVENT_RECIPIENT_APP).
       */

      virtual FAR CTwm4NxEvent *getEventHandler(void) = 0;

      /**
       * Get the Twm4Nx event that will be generated when the menu item is
       * selected.  If the returned value has the event recipient of
       * EVENT_RECIPIENT_APP, then getEventHandler() must return the event
       * handling instance.  Otherwise, the event will be handled by internal
       * Twm4Nx logic based on the internal recipient.
       *
       * This method may return EVENT_SYSTEM_NOP if a subMenu should be
       * used.  It would be an error if this method returned an event with
       * EVENT_RECIPIENT_APP but the getEventHandler() method returned null.
       *
       * NOTEs:
       * - The getSubMenu() method is ignored if the event() method returns
       *   an event with recipient = EVENT_RECIPIENT_APP.  In that case, the
       *   application will be fully responsible for handling  the menu
       *   selection event.  Otherwise, the sub-menu takes precedence over
       *   the event.
       *
       * Precedence:
       * 1. Event with recipient == EVENT_RECIPIENT_APP.  getEventHandler()
       *    must return a non-NULL instance in this case.
       * 2. Sub-menu
       * 3. Event with other recipients
       *
       * @return. Either, (1) an event with recipient = EVENT_RECIPIENT_APP
       *   that will be generated when menu item is selected, or (2) any other
       *   value (preferabley zero) that indicates that standard, built-in
       *   event handling should be used.
       */

      virtual uint16_t getEvent(void) = 0;
  };

  /**
   * IAppliation Factory provides a standard interface for adding multiple
   * copies of an application.
   */

  class IApplicationFactory
  {
    public:

      /**
       * A virtual destructor is required in order to override the
       * IApplication destructor.  We do this because if we delete
       * IApplication, we want the destructor of the class that inherits from
       * IApplication to run, not this one.
       */

      virtual ~IApplicationFactory(void)
      {
      }

      /**
       * CNxTermFactory Initializer.  Performs parts of the instance
       * construction that may fail.  Aside from general initialization, the
       * main responsibility of this function is to register the factory
       * IApplication interface with the Main Menu by calling the
       * CMainMenu::addApplication() method.
       *
       * @param twm4nx.  The Twm4Nx session instance.
       */

      virtual bool initialize(FAR CTwm4Nx *twm4nx) = 0;
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_IAPPLICATION_HXX
