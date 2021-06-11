/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cmainmenu.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CMAINMENU_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CMAINMENU_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <nuttx/input/mouse.h>

#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/cmenus.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Class Definition
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  class CTwm4Nx;                            // Forward Reference
  class IApplication;                       // Forward Reference
  struct SEventMsg;                         // Forward Reference

  /**
   * This structure describes on Main Menu item.
   */

  struct SMainMenuItem
  {
    FAR struct SMainMenuItem *flink;        /**< Forward link */
    FAR struct SMainMenuItem *blink;        /**< Backward link */
    FAR IApplication *app;                  /**< Application information */
  };

  /**
   * The Twm4Nx main menu is present on a left click anwyere on the
   * background (except for icons that also lie on the background).  It
   * supports starting of applications that are not inherently part of
   * Twm4Nx.  A registration method is provided that uses an instance of
   * a class that derives from Twm4Nx::IApplication to provide all necessary
   * support for a menu item.
   */

  class CMainMenu : public CTwm4NxEvent
  {
    private:
      FAR CTwm4Nx              *m_twm4nx;   /**< Cached Twm4Nx session instance */
      FAR CMenus               *m_mainMenu; /**< The main menu instance */
      FAR struct SMainMenuItem *m_appHead;  /**< The head of the main menu item list */
      FAR struct SMainMenuItem *m_appTail;  /**< The tail of the main menu item list */

      /**
       * Put an allocated entry into the main menu in name order
       *
       *  @param mmitem The entry to insert
       */

      void insertEntry(FAR struct SMainMenuItem *mmitem);

      /**
       * Remove an entry from an main menu
       *
       *  @param mmitem the entry to remove
       */

      void removeEntry(FAR struct SMainMenuItem *mmitem);

      /**
       * Select a position for the Main Menu which is as close as possible
       * the background click position.
       *
       * @param clickPos The background click position
       * @return True is returned if the position was set correctly
       */

      bool selectMainMenuPosition(FAR const struct nxgl_point_s &clickPos);

    public:

      /**
       * CMainMenu Constructor
       *
       * @param twm4nx   The Twm4Nx session
       */

      CMainMenu(FAR CTwm4Nx *twm4nx);

      /**
       * CMainMenu Destructor
       */

      ~CMainMenu(void);

      /**
       * CMainMenu Initializer.  This function performs the parts of the
       * initialization that may fail.
       *
       * @return True if the main menu was properly initialized.   false is
       *    return on any failure.
       */

      bool initialize(void);

      /**
       * Register one main menu item
       *
       * @param app An instance of a class that derives from IApplication
       * @return True if the menu item was properly added to the main menu.
       *   false is return on any failure.
       */

      bool addApplication(FAR IApplication *app);

      /**
       * Return true if the main menu is currently being displayed
       */

      inline bool isVisible(void)
      {
        return m_mainMenu->isVisible();
      }

      /**
       * Handle MAIN MENU events.
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

       bool event(FAR struct SEventMsg *eventmsg);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CMAINMENU_HXX
