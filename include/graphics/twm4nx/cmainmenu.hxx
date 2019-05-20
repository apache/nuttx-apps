/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cmainmenu.hxx
// Twm4Nx main menu class
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
