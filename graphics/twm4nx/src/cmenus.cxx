/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cmenus.cxx
// twm menu code
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.
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

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <fcntl.h>

#include <nuttx/version.h>
#include <nuttx/nx/nxbe.h>

#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/cnxfont.hxx"
#include "graphics/nxwidgets/cbuttonarray.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/cresize.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/cmenus.hxx"

////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

// Window flags:
//
//   WFLAGS_NO_MENU_BUTTON:    No menu buttons on menus
//   WFLAGS_NO_DELETE_BUTTON:  Menus cannot be deleted in this manner.
//   WFLAGS_NO_RESIZE_BUTTON:  Menus cannot be resized
//   WFLAGS_MENU:              Menu windows are always created in the
//                             hidden and iconifed state.  When the menu is
//                             selected, then it should be de-iconfied to
//                             be shown.
//   WFLAGS_HIDDEN:            Redundant

#define MENU_WINDOW_FLAGS (WFLAGS_NO_MENU_BUTTON | WFLAGS_NO_DELETE_BUTTON | \
                           WFLAGS_NO_RESIZE_BUTTON | WFLAGS_MENU | \
                           WFLAGS_HIDDEN)

////////////////////////////////////////////////////////////////////////////
// Class Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CMenus Constructor
 */

CMenus::CMenus(CTwm4Nx *twm4nx)
{
  // Save the Twm4Nx session

  m_twm4nx       = twm4nx;                     // Save the Twm4Nx session
  m_eventq       = (mqd_t)-1;                  // No widget message queue yet

  // Menus

  m_menuHead     = (FAR struct SMenuItem *)0;  // No menu items
  m_menuTail     = (FAR struct SMenuItem *)0;  // No menu items
  m_nMenuItems   = 0;                          // No menu items yet
  m_entryHeight  = 0;                          // Menu entry height

  // Windows

  m_menuWindow   = (FAR CWindow *)0;           // The menu window

  // Widgets

  m_buttons      = (FAR NXWidgets::CButtonArray *)0;  // The menu button array
}

/**
 * CMenus Destructor
 */

CMenus::~CMenus(void)
{
  cleanup();
}

/**
 * CMenus Initializer.  Performs the parts of the CMenus construction
 * that may fail.  The menu window is created but is not initially
 * visible.  Use the show() method to make the menu visible.
 *
 * @param name The name of the menu
 * @result True is returned on success
 */

bool CMenus::initialize(FAR NXWidgets::CNxString &name)
{
  // Open a message queue to NX events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();
  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             mqname, errno);
      return false;
    }

  // Clone the menu name

  m_menuName = name;

  // Create the menu window

  if (!createMenuWindow())
    {
      twmerr("ERROR: Failed to create menu window\n");
      cleanup();
      return false;
    }

  // Create the menu button array

  if (!createMenuButtonArray())
    {
      twmerr("ERROR: Failed to create menu button array\n");
      cleanup();
      return false;
    }

  return true;
}

/**
 * Add an item to a menu
 *
 * @param item Describes the menu item entry
  * @return True if the menu item was added successfully
 */

bool  CMenus::addMenuItem(FAR IApplication *item)
{
  twminfo("Adding menu item\n");

  // Allocate a new menu item entry

  FAR struct SMenuItem *newitem = new SMenuItem;
  if (newitem == (FAR struct SMenuItem *)0)
    {
      twmerr("ERROR:  Failed to allocate menu item\n");
      return false;
    }

  // Save information about the menu item

  newitem->flink    = NULL;
  newitem->text     = item->getName();
  newitem->subMenu  = item->getSubMenu();
  newitem->handler  = item->getEventHandler();
  newitem->event    = item->getEvent();

  // Increment the total number of menu items

  m_nMenuItems++;

  // Add the menu item to the tail of the item list

  if (m_menuHead == NULL)
    {
      m_menuHead     = newitem;
      newitem->blink = (FAR struct SMenuItem *)0;
    }
  else
    {
      m_menuTail->flink = newitem;
      newitem->blink    = m_menuTail;
    }

  m_menuTail     = newitem;
  newitem->flink = (FAR struct SMenuItem *)0;

  // Update the menu window size

  setMenuWindowSize();
  m_menuWindow->synchronize();

  // Get the updated window size

  struct nxgl_size_s menuSize;
  m_menuWindow->getWindowSize(&menuSize);

  // Resize the button array

  nxgl_coord_t buttonHeight = menuSize.h / m_nMenuItems;

  if (!m_buttons->resizeArray(1, m_nMenuItems, menuSize.w, buttonHeight))
    {
      twmerr("ERROR: CButtonArray::resizeArray failed\n");
      return false;
    }

  // We have to update all button labels after resizing

  FAR struct SMenuItem *tmpitem;
  int index;

  for (index = 0, tmpitem = m_menuHead;
       tmpitem != (FAR struct SMenuItem *)0;
       index++, tmpitem = tmpitem->flink)
    {
      m_buttons->setText(0, index, tmpitem->text);
    }

  return true;
}

/**
 * Handle MENU events.
 *
 * @param eventmsg.  The received NxWidget MENU event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CMenus::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_MENU_XYINPUT:     // Poll for button array events
        {
          // This event message is sent from CWindowEvent whenever mouse,
          // touchscreen, or keyboard entry events are received in the
          // menu application window that contains the button array.

          NXWidgets::CWidgetControl *control =
            m_menuWindow->getWidgetControl();

          // Poll for button array events.
          //
          // pollEvents() returns true if any interesting event in the
          // button array.  handleActionEvent() will be called in that
          // case.  false is not a failure.

          control->pollEvents();
        }
        break;

      case EVENT_MENU_COMPLETE:    // Menu selection complete
        {
          if (!m_menuWindow->isIconified())
            {
              m_menuWindow->iconify();
            }
        }
        break;

      case EVENT_MENU_SUBMENU:    // Sub-menu selected
        {
          // Bring up the sub-menu

          FAR CMenus *cmenu = (FAR CMenus *)eventmsg->obj;
          success = cmenu->show();
        }
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Convert the position of a menu window to the position of
 * the containing frame.
 */

void CMenus::menuToFramePos(FAR const struct nxgl_point_s *menupos,
                            FAR struct nxgl_point_s *framepos)
{
  nxgl_coord_t tbheight = m_menuWindow->getToolbarHeight();
  framepos->x = menupos->x - CONFIG_NXTK_BORDERWIDTH;
  framepos->y = menupos->y - tbheight - CONFIG_NXTK_BORDERWIDTH;
}

/**
 * Convert the position of the containing frame to the position of
 * the menu window.
 */

void CMenus::frameToMenuPos(FAR const struct nxgl_point_s *framepos,
                            FAR struct nxgl_point_s *menupos)
{
  nxgl_coord_t tbheight = m_menuWindow->getToolbarHeight();
  menupos->x = framepos->x + CONFIG_NXTK_BORDERWIDTH;
  menupos->y = framepos->y + tbheight + CONFIG_NXTK_BORDERWIDTH;
}

/**
 * Convert the size of a menu window to the size of the containing
 * frame.
 */

void CMenus::menuToFrameSize(FAR const struct nxgl_size_s *menusize,
                                  FAR struct nxgl_size_s *framesize)
{
  nxgl_coord_t tbheight = m_menuWindow->getToolbarHeight();
  framesize->w = menusize->w + 2 * CONFIG_NXTK_BORDERWIDTH;
  framesize->h = menusize->h + tbheight + 2 * CONFIG_NXTK_BORDERWIDTH;
}

/**
 * Convert the size of a containing frame to the size of the menu
 * window.
 */

void CMenus::frameToMenuSize(FAR const struct nxgl_size_s *framesize,
                                  FAR struct nxgl_size_s *menusize)
{
  nxgl_coord_t tbheight = m_menuWindow->getToolbarHeight();
  menusize->w = framesize->w - 2 * CONFIG_NXTK_BORDERWIDTH;
  menusize->h = framesize->h - tbheight - 2 * CONFIG_NXTK_BORDERWIDTH;
}

/**
 * Create the menu window.  Menu windows are always created in the hidden
 * state.  When the menu is selected, then it should be shown.
 *
 * @result True is returned on success
 */

bool CMenus::createMenuWindow(void)
{
  // Create the menu window

  CWindowFactory *factory = m_twm4nx->getWindowFactory();

  m_menuWindow =
    factory->createWindow(m_menuName,
                          (FAR const struct NXWidgets::SRlePaletteBitmap *)0,
                          (FAR CIconMgr *)0, MENU_WINDOW_FLAGS);

  if (m_menuWindow == (FAR CWindow *)0)
    {
      twmerr("ERROR: Failed to create icon manager window");
      return false;
    }

  // Configure mouse events needed by the button array.

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_SYSTEM_NOP;
  events.resizeEvent = EVENT_SYSTEM_NOP;
  events.mouseEvent  = EVENT_MENU_XYINPUT;
  events.kbdEvent    = EVENT_SYSTEM_NOP;
  events.closeEvent  = EVENT_SYSTEM_NOP;
  events.deleteEvent = EVENT_WINDOW_DELETE;

  bool success = m_menuWindow->configureEvents(events);
  if (!success)
    {
      delete m_menuWindow;
      m_menuWindow = (FAR CWindow *)0;
      return false;
    }

  // Adjust the size of the window

  struct nxgl_size_s windowSize;
  getMenuWindowSize(windowSize);

  if (!m_menuWindow->setWindowSize(&windowSize))
    {
      twmerr("ERROR: Failed to set window size\n");
      delete m_menuWindow;
      m_menuWindow = (FAR CWindow *)0;
      return false;
    }

  return true;
}

/**
 * Calculate the optimal menu frame size
 *
 * @param frameSize The location to return the calculated frame size
 */

void CMenus::getMenuFrameSize(FAR struct nxgl_size_s &frameSize)
{
  CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *menuFont = fonts->getMenuFont();

  m_entryHeight = menuFont->getHeight() + CONFIG_TWM4NX_MENU_VSPACING;

  // Get the minimum width of the toolbar

  nxgl_coord_t maxWidth = minimumToolbarWidth(m_twm4nx, m_menuName,
                                              MENU_WINDOW_FLAGS);

  // Compare that to the length of the longest item string in in the menu

  for (FAR struct SMenuItem *curr = m_menuHead;
       curr != NULL;
       curr = curr->flink)
    {
      nxgl_coord_t stringlen = menuFont->getStringWidth(curr->text);
      if (stringlen > maxWidth)
        {
          maxWidth = stringlen;
        }
    }

  // Lets first size the window accordingly

  struct nxgl_size_s menuSize;
  menuSize.w = maxWidth + CONFIG_TWM4NX_MENU_HSPACING;

  unsigned int nMenuItems = m_nMenuItems > 0 ? m_nMenuItems : 1;
  menuSize.h = nMenuItems * m_entryHeight;

  // Clip to the size of the display

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  menuToFrameSize(&menuSize, &frameSize);
  if (frameSize.w > displaySize.w)
    {
      frameSize.w = displaySize.w;
    }

  if (frameSize.h > displaySize.h)
    {
      frameSize.h = displaySize.h;
    }
}

/**
 * Calculate the optimal menu window size
 *
 * @param frameSize The location to return the calculated window size
 */

void CMenus::getMenuWindowSize(FAR struct nxgl_size_s &size)
{
  struct nxgl_size_s frameSize;
  getMenuFrameSize(frameSize);
  frameToMenuSize(&frameSize, &size);
}

/**
 * Update the menu window size
 *
 * @result True is returned on success
 */

bool CMenus::setMenuWindowSize(void)
{
  // Get the optimal menu window size

  struct nxgl_size_s frameSize;
  getMenuFrameSize(frameSize);

  if (!m_menuWindow->resizeFrame(&frameSize, (FAR const struct nxgl_point_s *)0))
    {
      twmerr("ERROR: Failed to resize menu window\n");
      return false;
    }

  return true;
}

/**
 * Set the position of the menu window.  Supports positioning of a
 * pop-up window.
 *
 * @param framePos The position of the menu window frame
 * @result True is returned on success
 */

bool CMenus::setMenuWindowPosition(FAR struct nxgl_point_s *framePos)
{
  struct nxgl_point_s menuPos;
  frameToMenuPos(framePos, &menuPos);
  return m_menuWindow->setWindowPosition(&menuPos);
}

/**
 * Create the menu button array
 *
 * @result True is returned on success
 */

bool CMenus::createMenuButtonArray(void)
{
  // Get the width of the window

  struct nxgl_size_s windowSize;
  if (!m_menuWindow->getWindowSize(&windowSize))
    {
      twmerr("ERROR: Failed to get window size\n");
      return false;
    }

  // Create the button array

  uint8_t nrows = m_nMenuItems > 0 ? m_nMenuItems : 1;

  nxgl_coord_t buttonWidth  = windowSize.w;
  nxgl_coord_t buttonHeight = windowSize.h / nrows;

  // Get the Widget control instance from the Icon Manager window.  This
  // will force all widget drawing to go to the Icon Manager window.

  FAR NXWidgets:: CWidgetControl *control = m_menuWindow->getWidgetControl();
  if (control == (FAR NXWidgets:: CWidgetControl *)0)
    {
      // Should not fail

      return false;
    }

  // Now we have enough information to create the button array
  // The button must be positioned at the upper left of the window

  m_buttons = new NXWidgets::CButtonArray(control, 0, 0, 1, nrows,
                                          buttonWidth, buttonHeight);
  if (m_buttons == (FAR NXWidgets::CButtonArray *)0)
    {
      twmerr("ERROR: Failed to create the button array\n");
      return false;
    }

  // Configure the button array widget

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  m_buttons->setFont(iconManagerFont);
  m_buttons->setBorderless(true);
  m_buttons->setRaisesEvents(true);

  // Enable and draw the button array

  m_buttons->enable();
  m_buttons->enableDrawing();
  m_buttons->redraw();

  // Register to get events from the mouse clicks on the image

  m_buttons->addWidgetEventHandler(this);
  return true;
}

/**
 * Handle a widget action event, overriding the CWidgetEventHandler
 * method.  This will indicate a button pre-release event.
 *
 * @param e The event data.
 */

void CMenus::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // A button should now be clicked

  int column;
  int row;

  if (m_buttons->isButtonClicked(column, row) && column == 0)
    {
      // The row number is sufficient to locate the menu entry info
      // But we have to search through the menu items to find the
      // at this row.

      FAR struct SMenuItem *item;
      int index;

      for (item = m_menuHead, index = 0;
           item != (FAR struct SMenuItem *)0;
           item = item->flink, index++)
        {
          // When the index matches the row number, then we have
          // the entry.
          // REVISIT:  Are there any race conditions we need to
          // concerned with here?  Such as menuitems being removed
          // while the menu is up?

          if (row == index)
            {
              // Send an event to the Twm4Nx event handler

              struct SEventMsg msg;

              // Precedence:
              // 1. Event with recipient == EVENT_RECIPIENT_APP.
              //    getEventHandler() must return a non-NULL instance in this
              //    case.
              // 2. Sub-menu
              // 3. Event with other recipients

              bool subMenu =
                ((item->event & EVENT_RECIPIENT_MASK) != EVENT_RECIPIENT_APP &&
                 item->subMenu != (FAR CMenus *)0);

              if (subMenu)
                {
                  msg.eventID = EVENT_MENU_SUBMENU;
                  msg.obj     = (FAR void *)item->subMenu;
                  msg.handler = (FAR void *)0;
                }

              // Otherwise, send the event specified for the menu item.  The
              // handler is only used if the recipient of the event is
              // EVENT_RECIPIENT_APP

              else
                {
                  msg.eventID = item->event;
                  msg.obj     = (FAR void *)this;
                  msg.handler = (FAR void *)item->handler;
                }

              // Fill in the remaining, common stuff

              msg.pos.x   = e.getX();
              msg.pos.y   = e.getY();
              msg.context = EVENT_CONTEXT_MENU;

              // NOTE that we cannot block because we are on the same thread
              // as the message reader.  If the event queue becomes full then
              // we have no other option but to lose events.
              //
              // I suppose we could recurse and call Twm4Nx::dispatchEvent at
              // the risk of runaway stack usage.

              int ret = mq_send(m_eventq, (FAR const char *)&msg,
                                sizeof(struct SEventMsg), 100);
              if (ret < 0)
                {
                  twmerr("ERROR: mq_send failed: %d\n", errno);
                }

              // If this is a terminal option (i.e., not a submenu) then
              // make the menu inaccessible

              if (!subMenu)
                {
                  msg.eventID = EVENT_MENU_COMPLETE;
                  msg.obj     = (FAR void *)this;
                  msg.handler = (FAR void *)0;
                  msg.pos.x   = e.getX();
                  msg.pos.y   = e.getY();
                  msg.context = EVENT_CONTEXT_MENU;

                  ret = mq_send(m_eventq, (FAR const char *)&msg,
                                sizeof(struct SEventMsg), 100);
                  if (ret < 0)
                    {
                      twmerr("ERROR: mq_send failed: %d\n", errno);
                    }

                  return;
                }
            }
        }

      twmwarn("WARNING:  No matching menu at index %d\n", row);
    }
}

/**
 * Cleanup or initialization error or on deconstruction.
 */

void CMenus::cleanup(void)
{
 // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Free the menu window

  if (m_menuWindow != (FAR CWindow *)0)
    {
      delete m_menuWindow;
      m_menuWindow = (FAR CWindow *)0;
    }

  // Free each menu item

  FAR struct SMenuItem *curr;
  FAR struct SMenuItem *next;

  for (curr = m_menuHead; curr != (FAR struct SMenuItem *)0; curr = next)
    {
      next = curr->flink;

      // Free any subMenu

      if (curr->subMenu != (FAR CMenus *)0)
        {
          delete curr->subMenu;
        }

      // Free the menu item

      delete curr;
    }

  m_menuHead = (FAR struct SMenuItem *)0;
  m_menuTail = (FAR struct SMenuItem *)0;

  // Free the button array

  if (m_buttons != (FAR NXWidgets::CButtonArray *)0)
    {
      delete m_buttons;
      m_buttons = (FAR NXWidgets::CButtonArray *)0;
    }
}
