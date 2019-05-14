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

#include <graphics/twm4nx/twm4nx_config.hxx>
#include <graphics/twm4nx/iapplication.hxx>
#include <graphics/twm4nx/ctwm4nx.hxx>
#include <graphics/twm4nx/cmenus.hxx>
#include <graphics/twm4nx/cmainmenu.hxx>
#include <graphics/twm4nx/twm4nx_events.hxx>

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
      twmerr("ERROR: Failed to create the CMenus instance\n");
      return false;
    }

  NXWidgets::CNxString menuName("Main Menu");
  if (!m_mainMenu->initialize(menuName))
    {
      twmerr("ERROR: Failed to initialize the CMenus instance\n");
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
      twmerr("ERROR:  Failed to allocate the main menu entry\n");
      return false;
    }

  mmitem->flink     = NULL;
  mmitem->blink     = NULL;
  mmitem->app       = app;

  // Add the new menu item to the main menu

  if (!m_mainMenu->addMenuItem(app))
    {
      twmerr("ERROR: addMenuItem failed\n");
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
            // First, we select a position as close to the background click
            // as possible

            success = selectMainMenuPosition(eventmsg->pos);
            if (!success)
              {
                twmerr("ERROR: selectMainMenuPosition() failed\n");
              }
            else
              {
                // Make the main menu visible

                success = m_mainMenu->show();
                if (!success)
                  {
                    twmerr("ERROR: Failed to show the menu\n");
                  }
              }
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

/**
 * Select a position for the Main Menu which is as close as possible
 * the background click position.
 *
 * @param clickPos The background click position
 */

bool CMainMenu::selectMainMenuPosition(FAR const struct nxgl_point_s &clickPos)
{
  // Get the size of the main menu frame

  struct nxgl_size_s frameSize;
  m_mainMenu->getFrameSize(&frameSize);

  // Get the size of the display

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  // Determine the best new Y position for the menu.

  struct nxgl_point_s framePos;

  // Check if the menu does not fit on the display.  This is really an error
  // condition since menu items are not accessible
  //
  // REVISIT:  Consider using a scrolling text box to handle this case.

  if (frameSize.h > displaySize.h)
    {
      // Just position at the top of the display so that at least the
      // toolbar will be visible

      framePos.y = 0;
    }

  // Try to position the menu at the same Y position as the background click

  else if (clickPos.y + frameSize.h <= displaySize.h)
    {
      framePos.y = clickPos.y;
    }

  // Otherwise, set the Y position so that the entire menu is on the display
  // with the bottom of the menu at the bottom of the display.

  else
    {
      framePos.y = displaySize.h - frameSize.h;
    }

  // Determine the best new Y position for the menu.

  // Check if the menu does not fit on the display.  This is really unlikely.

  if (frameSize.w > displaySize.w)
    {
      // Just position at the right of the display so that at least the
      // toolbar minimize button will be visible

      framePos.x = displaySize.w - frameSize.w;  // Negative position!
    }

  // Try to position the menu at the same X position as the background click
  // So that it appears to the right of the click position.

  else if (clickPos.x + frameSize.w <= displaySize.w)
    {
      framePos.x = clickPos.x;
    }

  // Try to position the menu at the same X position as the background click
  // So that it appears to the left of the click position.

  else if (clickPos.x >= frameSize.w)
    {
      framePos.x = clickPos.x - frameSize.w;
    }

  // Otherwise, set the X position so that the entire menu is on the display
  // on the left or right of the display.  This cases are not possible unless
  // the width of the menu is greater than half of the width of the display.

  else if (clickPos.x > displaySize.w / 2)
    {
      // Position at the right of the display

      framePos.x = displaySize.w - frameSize.w;
    }
  else
    {
      // Position at the left of the display

      framePos.x = 0;
    }

  // And, finally, set the new main menu frame position

  twminfo("Click position: (%d,%d) Main menu position: (%d,%d)\n",
          clickPos.x, clickPos.y, framePos.x, framePos.y);

  return m_mainMenu->setFramePosition(&framePos);
}
