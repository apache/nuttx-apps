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
#include <debug.h>

#include <nuttx/version.h>

#include "graphics/nxwidgets/cnxfont.hxx"
#include "graphics/nxwidgets/clistbox.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/cresize.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cicon.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"
#include "graphics/twm4nx/cmenus.hxx"

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

  m_popUpMenu    = (FAR CMenus *)0;            // No pop-up menu
  m_activeItem   = (FAR struct SMenuItem *)0;  // No active menu item
  m_nMenuItems   = 0;                          // No menu items yet
  m_menuDepth    = 0;                          // No menus up
  m_entryHeight  = 0;                          // Menu entry height
  m_menuPull     = false;                      // No pull right entry

  // Windows

  m_menuWindow   = (FAR NXWidgets::CNxTkWindow *)0;  // The menu window

  // Widgets

  m_menuListBox  = (FAR NXWidgets::CListBox *)0;  //The menu list box
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
 * that may fail.
 *
 * @result True is returned on success
 */

bool CMenus::initialize(FAR const char *name)
{
  // Open a message queue to NX events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();
  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      gerr("ERROR: Failed open message queue '%s': %d\n",
           mqname, errno);
      return false;
    }

  // Save the menu name

  m_menuName = strdup(name);
  if (m_menuName == (FAR char *)0)
    {
      return false;
    }

  // Create the menu window

  if (!createMenuWindow())
    {
      gerr("ERROR: Failed to create menu window\n");
      cleanup();
      return false;
    }

  // Create the menu list box

  if (!createMenuListBox())
    {
      gerr("ERROR: Failed to create menu list box\n");
      cleanup();
      return false;
    }

  return true;
}

/**
 * Add an item to a menu
 *
 *  \param text    The text to appear in the menu
 *  \param subMenu The menu if it is a pull-right entry
 *  \param handler The application event handler.  Should be null unless
 *                 the event recipient is EVENT_RECIPIENT_APP
 *  \param event   The event to generate on menu item selection
 */

bool CMenus::addMenuItem(FAR const char *text, FAR CMenus *subMenu,
                         FAR CTwm4NxEvent *handler, uint16_t event)
{
  ginfo("Adding menu text=\"%s\", subMenu=%p, event=%04x\n",
        text, subMenu, event);

  // Allocate a new menu item entry

  FAR struct SMenuItem *item = new SMenuItem;

  if (item == (FAR struct SMenuItem *)0)
  {
      gerr("ERROR:  Failed to allocate menu item\n");
      return false;
  }

  // Clone the item name so that we have control over its lifespan

  item->text = std::strdup(text);
  if (item->text == (FAR char *)0)
    {
      gerr("ERROR:  strdup of item text failed\n");
      std::free(item);
      return false;
    }

  // Save information about the menu item

  item->flink    = NULL;
  item->subMenu  = NULL;
  item->handler  = handler;
  item->event    = event;

  CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *menuFont = fonts->getMenuFont();
  int width = menuFont->getStringWidth(text);

  if (width <= 0)
    {
      width = 1;
    }

  struct nxgl_size_s menuSize;
  m_menuWindow->getSize(&menuSize);

  if (width > menuSize.w)
    {
      menuSize.w = width;
      m_menuWindow->setSize(&menuSize);
    }

  if (subMenu != NULL)
    {
      item->subMenu   = subMenu;
      m_menuPull = true;
    }

  // Save the index to this item and increment the total number of menu
  // items

  item->index = m_nMenuItems++;

  // Add the menu item to the tail of the item list

  if (m_menuHead == NULL)
    {
      m_menuHead  = item;
      item->blink = (FAR struct SMenuItem *)0;
    }
  else
    {
      m_menuTail->flink = item;
      item->blink = m_menuTail;
    }

  m_menuTail  = item;
  item->flink = (FAR struct SMenuItem *)0;

  // Add the item text to the list box

  m_menuListBox->addOption(item->text, item->index);

  // Update the menu window size

  setMenuWindowSize();

  // Redraw the list box

  m_menuListBox->enable();
  m_menuListBox->enableDrawing();
  m_menuListBox->setRaisesEvents(true);
  m_menuListBox->redraw();
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
      case EVENT_MENU_IDENTIFY:  // Describe the window
        {
          identify((FAR CWindow *)eventmsg->obj);
        }
        break;

      case EVENT_MENU_VERSION:  // Show the Twm4Nx version
        identify((FAR CWindow *) NULL);
        break;

      case EVENT_MENU_DEICONIFY:  // Window icon pressed
      case EVENT_MENU_ICONIFY:    // Tool bar minimize button pressed
        {
          FAR CWindow *cwin = (FAR CWindow *)eventmsg->obj;
          if (cwin->isIconified())
            {
              cwin->deIconify();
            }
          else if (eventmsg->eventID == EVENT_MENU_ICONIFY)
            {
              cwin->iconify();
            }
        }
        break;

      case EVENT_MENU_FUNCTION:  // Perform function on unknown menu
        {
          FAR struct SMenuItem *item;

          for (item = m_menuHead; item != NULL; item = item->flink)
            {
              // Send another event message to the session manager

              struct SEventMsg newmsg;
              newmsg.eventID  = item->event;
              newmsg.pos.x    = eventmsg->pos.x;
              newmsg.pos.y    = eventmsg->pos.y;
              newmsg.delta.x  = 0;
              newmsg.delta.y  = 0;
              newmsg.context  = eventmsg->context;
              newmsg.handler  = item->handler;
              newmsg.obj      = eventmsg->obj;

              // NOTE that we cannot block because we are on the same thread
              // as the message reader.  If the event queue becomes full then
              // we have no other option but to lose events.
              //
              // I suppose we could recurse and call Twm4Nx::dispatchEvent at
              // the risk of runaway stack usage.

              int ret = mq_send(m_eventq, (FAR const char *)&newmsg,
                             sizeof(struct SEventMsg), 100);
              if (ret < 0)
                {
                  gerr("ERROR: mq_send failed: %d\n", ret);
                  success = false;
                }
            }
        }
        break;

      case EVENT_MENU_TITLE: // Really an action not an event
      case EVENT_MENU_ROOT:  // Popup root menu, really an action not an event
      default:
        success = false;
        break;
    }

  return success;
}

void CMenus::identify(FAR CWindow *cwin)
{
  int n = 0;
#if CONFIG_VERSION_MAJOR != 0 || CONFIG_VERSION_MINOR != 0
  std::snprintf(m_info[n], INFO_SIZE, "Twm4Nx: NuttX-" CONFIG_VERSION_STRING);
#else
  std::snprintf(m_info[n], INFO_SIZE, "Twm4Nx:");
#endif
  m_info[n++][0] = '\0';

  if (cwin != (FAR CWindow *)0)
    {
      // Get the size of the window

      struct nxgl_size_s windowSize;
      if (!cwin->getFrameSize(&windowSize))
        {
          return;
        }

      struct nxgl_point_s windowPos;
      if (!cwin->getFramePosition(&windowPos))
        {
          return;
        }

      std::snprintf(m_info[n++], INFO_SIZE, "Name             = \"%s\"",
                    cwin->getWindowName());
      m_info[n++][0] = '\0';
      std::snprintf(m_info[n++], INFO_SIZE, "Geometry/root    = %dx%d+%d+%d",
                    windowSize.w, windowSize.h, windowPos.x, windowPos.y);
    }

  m_info[n++][0] = '\0';
  std::snprintf(m_info[n++], INFO_SIZE, "Click to dismiss....");

  // Figure out the width and height of the info window

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *defaultFont = fonts->getDefaultFont();

  struct nxgl_size_s menuSize;
  menuSize.h = n * (defaultFont->getHeight() + 2);
  menuSize.w = 1;

  for (int i = 0; i < n; i++)
    {
      int twidth = defaultFont->getStringWidth(m_info[i]);
      if (twidth > menuSize.w)
        {
          menuSize.w = twidth;
        }
    }

  menuSize.w += 10;    // some padding

  // Make sure that the window is on the display

  struct nxgl_point_s menuPos;
  if (m_menuWindow->getPosition(&menuPos))
    {
      menuPos.x -= (menuSize.w / 2);
      menuPos.y -= (menuSize.h / 3);

      struct nxgl_size_s displaySize;
      m_twm4nx->getDisplaySize(&displaySize);

      struct nxgl_size_s frameSize;
      menuToFrameSize(&menuSize, &frameSize);

      if (menuPos.x + frameSize.w >= displaySize.w)
        {
          menuPos.x = displaySize.w - frameSize.w;
        }

      if (menuPos.y + frameSize.h >= displaySize.h)
        {
          menuPos.y = displaySize.h - frameSize.h;
        }

      if (menuPos.x < 0)
        {
          menuPos.x = 0;
        }

      if (menuPos.y < 0)
        {
          menuPos.y = 0;
        }

      frameToMenuSize(&frameSize, &menuSize);
    }
  else
    {
      menuPos.x = 0;
      menuPos.y = 0;
    }

  // Set the new window size and position

  if (!m_menuWindow->setPosition(&menuPos) ||
      !m_menuWindow->setSize(&menuSize))
    {
      return;
    }

  // Raise it to the top of the hiearchy

  m_menuWindow->raise();
}

/**
 * Create the menu window
 *
 * @result True is returned on success
 */

bool CMenus::createMenuWindow(void)
{
  // Create the menu window
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx);

  // 4. Create the menu window

  m_menuWindow = m_twm4nx->createFramedWindow(control);
  if (m_menuWindow == (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete control;
      return false;
    }

 // 5. Open and initialize the menu window

  bool success = m_menuWindow->open();
  if (!success)
    {
      delete m_menuWindow;
      m_menuWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  // 6. Set the initial window size

  if (!setMenuWindowSize())
    {
      delete m_menuWindow;
      m_menuWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  // 7. Set the initial window position

  struct nxgl_point_s pos =
  {
    .x = 0,
    .y = 0
  };

  if (!m_menuWindow->setPosition(&pos))
    {
      delete m_menuWindow;
      m_menuWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  return true;
}

/**
 * Update the menu window size
 *
 * @result True is returned on success
 */

bool CMenus::setMenuWindowSize(void)
{
  CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *menuFont = fonts->getMenuFont();

  m_entryHeight = menuFont->getHeight() + 4;

  // Get the length of the longest item string in in the menu

  nxgl_coord_t maxstring = 0;
  for (FAR struct SMenuItem *curr = m_menuHead;
       curr != NULL;
       curr = curr->flink)
    {
      if (curr->text != (FAR char *)0)
        {
          nxgl_coord_t stringlen = menuFont->getStringWidth(curr->text);
          if (stringlen > maxstring)
            {
              maxstring = stringlen;
            }
        }
    }

  // Lets first size the window accordingly

  struct nxgl_size_s menuSize;
  menuSize.w = maxstring + 10;

  unsigned int nMenuItems = m_nMenuItems > 0 ? m_nMenuItems : 1;
  menuSize.h = nMenuItems * m_entryHeight;

  if (m_menuPull == true)
    {
      menuSize.w += 16;
    }

  // Clip to the size of the display

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  struct nxgl_size_s frameSize;
  menuToFrameSize(&menuSize, &frameSize);

  if (frameSize.w > displaySize.w)
    {
      frameSize.w = displaySize.w;
    }

  if (frameSize.h > displaySize.h)
    {
      frameSize.h = displaySize.h;
    }

  // Set the new menu window size

  frameToMenuSize(&frameSize, &menuSize);

  if (!m_menuWindow->setSize(&menuSize))
    {
      gerr("ERROR: Failed to set window size\n");
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
  return m_menuWindow->setPosition(&menuPos);
}

/**
 * Create the menu list box
 *
 * @result True is returned on success
 */

bool CMenus::createMenuListBox(void)
{
  // Get the Widget control instance from the menu window.  This
  // will force all widget drawing to go to the Menu window.

  FAR NXWidgets:: CWidgetControl *control = m_menuWindow->getWidgetControl();
  if (control == (FAR NXWidgets:: CWidgetControl *)0)
    {
      // Should not fail

      return false;
    }

  // Create the menu list box

  struct nxgl_point_s pos;
  pos.x = 0;
  pos.y = 0;

  struct nxgl_size_s size;
  pos.x = 0;
  pos.y = 0;

  m_menuListBox = new NXWidgets::CListBox(control, pos.x, pos.y,
                                          size.w, size.h);
  if (m_menuListBox == (FAR NXWidgets::CListBox *)0)
    {
      gerr("ERROR: Failed to instantiate list box\n");
      return false;
    }

  // Get the menu font

  FAR CFonts *cfont = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *menuFont = cfont->getMenuFont();

  // Configure the list box

  m_menuListBox->disable();
  m_menuListBox->disableDrawing();
  m_menuListBox->setRaisesEvents(false);
  m_menuListBox->setAllowMultipleSelections(false);
  m_menuListBox->setFont(menuFont);
  m_menuListBox->setBorderless(false);

  // Register to get events from the mouse clicks on the image

  m_menuListBox->addWidgetEventHandler(this);
  return true;
}

/**
 * Pop up a pull down menu.
 *
 * @param pos    Location of upper left of menu frame
 */

bool CMenus::popUpMenu(FAR struct nxgl_point_s *pos)
{
  // If there is already a popup menu, then delete it.  We only permit
  // one popup at this level.

  if (m_popUpMenu != (FAR CMenus *)0)
    {
      delete m_popUpMenu;
    }

  // Create and initialize a new menu

  m_popUpMenu = new CMenus(m_twm4nx);
  if (m_popUpMenu == (FAR CMenus *)0)
    {
      gerr("ERROR: Failed to create popup menu.\n");
      return false;
    }

  if (!m_popUpMenu->initialize(m_activeItem->text))
    {
      gerr("ERROR: Failed to intialize popup menu.\n");
      delete m_popUpMenu;
      m_popUpMenu = (FAR CMenus *)0;
      return false;
    }

  m_popUpMenu->addMenuItem("TWM Windows", (FAR CMenus *)0,
                           (FAR CTwm4NxEvent *)0, EVENT_SYSTEM_NOP);

  FAR CWindowFactory *factory = m_twm4nx->getWindowFactory();
  int nWindowNames;

  FAR struct SWindow *swin;
  for (swin = factory->windowHead(), nWindowNames = 0;
       swin != NULL; swin = swin->flink)
    {
      nWindowNames++;
    }

  if (nWindowNames != 0)
    {
      FAR CWindow **windowNames =
        (FAR CWindow **)std::malloc(sizeof(FAR CWindow *) * nWindowNames);

      if (windowNames == (FAR CWindow **)0)
        {
          gerr("ERROR: Failed to allocat window name\n");
          return false;
        }

      swin = factory->windowHead();
      windowNames[0] = swin->cwin;

      for (nWindowNames = 1;
           swin != NULL;
           swin = swin->flink, nWindowNames++)
        {
          FAR CWindow *tmpcwin1 = swin->cwin;
          for (int i = 0; i < nWindowNames; i++)
            {
              FAR const char *windowName1 = tmpcwin1->getWindowName();
              FAR const char *windowName2 = windowNames[i]->getWindowName();

              if (std::strcmp(windowName1, windowName2) < 0)
                {
                  FAR CWindow *tmpcwin2;
                  tmpcwin2       = tmpcwin1;
                  tmpcwin1       = windowNames[i];
                  windowNames[i] = tmpcwin2;
                }
            }

          windowNames[nWindowNames] = tmpcwin1;
        }

      for (int i = 0; i < nWindowNames; i++)
        {
          m_popUpMenu->addMenuItem(windowNames[i]->getWindowName(),
                                   (FAR CMenus *)0, (FAR CTwm4NxEvent *)0,
                                   EVENT_WINDOW_DEICONIFY);
        }

      std::free(windowNames);
    }

  if (m_nMenuItems == 0)
    {
      delete m_popUpMenu;
      m_popUpMenu = (FAR CMenus *)0;
      return false;
    }

  // Clip to screen

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  struct nxgl_size_s menuSize;
  m_menuWindow->getSize(&menuSize);

  struct nxgl_size_s frameSize;
  menuToFrameSize(&menuSize, &frameSize);

  if (pos->x + frameSize.w > displaySize.w)
    {
      pos->x = displaySize.w - frameSize.w;
    }

  if (pos->x < 0)
    {
      pos->x = 0;
    }

  if (pos->y + frameSize.h > displaySize.h)
    {
      pos->y = displaySize.h - frameSize.h;
    }

  if (pos->y < 0)
    {
      pos->y = 0;
    }

  DEBUGASSERT(m_menuDepth < UINT8_MAX);
  m_menuDepth++;

  if (!m_popUpMenu->setMenuWindowPosition(pos))
    {
      delete m_popUpMenu;
      m_popUpMenu = (FAR CMenus *)0;
      return false;
    }

  m_popUpMenu->raiseMenuWindow();
  m_menuWindow->synchronize();
  return m_popUpMenu;
}

/**
 * Override the virtual value change event.  This will get events
 * when there is a change in the list box selection.
 *
 * @param e The event data.
 */

void CMenus::handleValueChangeEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Check if anything is selected

  int menuSelection = m_menuListBox->getSelectedIndex();
  if (menuSelection >= 0)
    {
      // Yes.. Get the selection menu item list box entry

      FAR const NXWidgets::CListBoxDataItem *option =
        m_menuListBox->getSelectedOption();

      // Get the menu item string

      FAR const NXWidgets::CNxString itemText = option->getText();

      // Now find the window with this name

      for (FAR struct SMenuItem *item = m_menuHead;
           item != (FAR struct SMenuItem *)0;
           item = item->flink)
        {
          // Check if the menu item string matches the listbox selection
          // string

          if (itemText.compareTo(item->text) == 0)
            {
               // Generate the menu event

               struct SEventMsg msg;
               msg.eventID = item->event;
               msg.pos.x   = e.getX();
               msg.pos.y   = e.getY();
               msg.delta.x = 0;
               msg.delta.y = 0;
               msg.context = EVENT_CONTEXT_TOOLBAR;
               msg.handler = item->handler;
               msg.obj     = (FAR void *)this;

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
                  gerr("ERROR: mq_send failed: %d\n", ret);
                }

              break;
            }
        }

      gwarn("WARNING:  No matching menu item\n");
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
      (void)mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Free any popup menus

  if (m_popUpMenu != (FAR CMenus *)0)
    {
      delete m_popUpMenu;
    }

  // Free the menu window

  if (m_menuWindow != (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete m_menuWindow;
      m_menuWindow = (FAR NXWidgets::CNxTkWindow *)0;
    }

  // Free each menu item

  FAR struct SMenuItem *curr;
  FAR struct SMenuItem *next;

  for (curr = m_menuHead; curr != (FAR struct SMenuItem *)0; curr = next)
    {
      next = curr->flink;

      // Free the menu item text

      if (curr->text != (FAR char *)0)
        {
          std::free(curr->text);
        }

      // Free any subMenu

      if (curr->subMenu != (FAR CMenus *)0)
        {
          delete curr->subMenu;
        }

      // Free the menu item

      delete curr;
    }

  // Free allocated memory

  if (m_menuName != (FAR char *)0)
    {
      std::free(m_menuName);
      m_menuName = (FAR char *)0;
    }
}
