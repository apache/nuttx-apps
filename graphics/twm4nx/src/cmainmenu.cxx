/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cmainmenu.cxx
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cunistd>
#include <debug.h>

#include <graphics/nxwidgets/cnxstring.hxx>

#include <graphics/twm4nx/twm4nx_widgetevents.hxx>
#include <graphics/twm4nx/iapplication.hxx>
#include <graphics/twm4nx/ctwm4nx.hxx>
#include <graphics/twm4nx/cmenus.hxx>
#include <graphics/twm4nx/cmainmenu.hxx>

/////////////////////////////////////////////////////////////////////////////
// Implementation Class Definition
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CMainMenu Constructor
 *
 * @param twm4nx   The Twm4Nx session
 */

CMainMenu::CMainMenu(FAR CTwm4Nx *twm4nx)
{
  m_twm4nx    = twm4nx;                         // Cache the Twm4Nx session instance
  m_mainMenu  = (FAR CMenus *)0;                // The main menu instance
  m_appHead   = (FAR struct SMainMenuItem *)0;  // The head of the main menu item list
  m_appTail   = (FAR struct SMainMenuItem *)0;  // The tail of the main menu item list
}

/**
 * CMainMenu Destructor
 */

CMainMenu::~CMainMenu(void)
{
  if (m_mainMenu != (FAR CMenus *)0)
    {
      delete m_mainMenu;
    }
}

/**
 * CMainMenu Initializer.  This function performs the parts of the
 * initialization that may fail.
 *
 * @return True if the main menu was properly initialized.   false is
 *    return on any failure.
 */

bool CMainMenu::initialize(void)
{
  // Create the main menu

  m_mainMenu = new CMenus(m_twm4nx);
  if (m_mainMenu == (FAR CMenus *)0)
    {
      gerr("ERROR: Failed to create the CMenus instance\n");
      return false;
    }

  NXWidgets::CNxString menuName("Main Menu");
  if (!m_mainMenu->initialize(menuName))
    {
      gerr("ERROR: Failed to initialize the CMenus instance\n");
      delete m_mainMenu;
      m_mainMenu = (FAR CMenus *)0;
      return false;
    }

  return true;
}

/**
 * Register one main menu item
 *
 * @param app An instance of a class that derives from IApplication
 * @return True if the menu item was properly added to the main menu.
 *   false is return on any failure.
 */

bool CMainMenu::addApplication(FAR IApplication *app)
{
  // Allocate a new main menu entry

  FAR struct SMainMenuItem *mmitem =
     (FAR struct SMainMenuItem *)malloc(sizeof(struct SMainMenuItem));

  if (mmitem == (FAR struct SMainMenuItem *)0)
    {
      gerr("ERROR:  Failed to allocate the main menu entry\n");
      return false;
    }

  mmitem->flink     = NULL;
  mmitem->blink     = NULL;
  mmitem->app       = app;

  // Add the new menu item to the main menu

  FAR NXWidgets::CNxString appName;
  app->getName(appName);

  if (!m_mainMenu->addMenuItem(appName, app->getSubMenu(),
                               app->getEventHandler(), app->getEvent()))
    {
      gerr("ERROR: addMenuItem failed\n");
      std::free(mmitem);
      return false;
    }

  // Insert the new entry into the list of main menu items

  insertEntry(mmitem);
  return true;
}

/**
 * Handle MAIN MENU events.
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CMainMenu::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      // This event is sent from CBackground when a left mouse click
      // is received in the background window (and not on an icon)

      case EVENT_MAINMENU_SELECT:  // Main menu selection
        // Check if the main menu is already visible

        if (!m_mainMenu->isVisible())
          {
            // No.. then make it visible now

            // REVISIT:  Need to reset the menu to its initial state
            // REVISIT:  Need to position the main menu as close as
            //           possible to the click position in eventmsg.

            m_mainMenu->show();    // Make the main menu visible
          }

        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Put an allocated entry into the main menu in name order
 *
 *  @param mmitem The entry to insert
 */

void CMainMenu::insertEntry(FAR struct SMainMenuItem *mmitem)
{
  // Inserted the new menu item at the tail of the list

  mmitem->flink = NULL;
  mmitem->blink = m_appTail;

  if (!m_appHead)
    {
      m_appHead = mmitem;
      m_appTail = mmitem;
    }
  else
    {
      m_appTail->flink = mmitem;
      m_appTail        = mmitem;
    }
}

/**
 * Remove an entry from an main menu
 *
 *  @param mmitem the entry to remove
 */

void CMainMenu::removeEntry(FAR struct SMainMenuItem *mmitem)
{
  FAR struct SMainMenuItem *prev = mmitem->blink;
  FAR struct SMainMenuItem *next = mmitem->flink;

  if (!prev)
    {
      m_appHead = next;
    }
  else
    {
      prev->flink = next;
    }

  if (!next)
    {
      m_appTail = prev;
    }
  else
    {
      next->blink = prev;
    }

  mmitem->flink = NULL;
  mmitem->blink = NULL;
}
