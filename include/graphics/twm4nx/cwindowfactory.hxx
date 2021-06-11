/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cwindowfactory.hxx
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

// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWFACTORY_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWFACTORY_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdbool>

#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/iapplication.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  struct SRlePaletteBitmap;                // Forward reference
}

namespace Twm4Nx
{
  class CWindow;                           // Forward reference
  class CIconMgr;                          // Forward reference
  struct SWindowEntry;                     // Forward reference

  // For each window that is on the display, one of these structures
  // is allocated and linked into a list.  It is essentially just a
  // container for a window.

  struct SWindow
  {
    FAR struct SWindow *flink;             /**< Forward link tonext window */
    FAR struct SWindow *blink;             /**< Backward link to previous window */
    FAR struct SWindowEntry *wentry;       /**< Icon manager list entry (for list removal) */
    FAR CWindow *cwin;                     /**< Window object payload */
  };

  /**
   * This class is a simple implement of the interface to the Main Menu that
   * provides the Desktop Main Menu entry.
   */

  class CDesktopItem : public IApplication
  {
    public:

      /**
       * Return the Main Menu item string.  This overrides the method from
       * IApplication
       *
       * @param name The name of the application.
       */

      inline NXWidgets::CNxString getName(void)
      {
        return NXWidgets::CNxString("Desktop");
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
        return (FAR CTwm4NxEvent *)0;
      }

      /**
       * Return the Twm4Nx event that will be generated when the Main Menu
       * item is selected.
       *
       * @return. This function always returns EVENT_WINDOW_DESKTOP.
       */

      inline uint16_t getEvent(void)
      {
        return EVENT_WINDOW_DESKTOP;
      }
  };

  /**
   * The CWindowFactory class creates new window instances and manages some
   * things that are common to all windows.
   */

  class CWindowFactory: public CTwm4NxEvent
  {
    private:

      CTwm4Nx             *m_twm4nx;      /**< Cached Twm4Nx session */
      struct nxgl_point_s  m_winpos;      /**< Position of next window created */
      FAR struct SWindow  *m_windowHead;  /**< List of windows on the display */
      CDesktopItem         m_desktopItem; /**< For the "Desktop" Main Menu item */

      /**
       * Add a window container to the window list.
       *
       * @param win.  The window container to be added to the list.
       */

      void addWindowContainer(FAR struct SWindow *win);

      /**
       * Remove a window container from the window list.
       *
       * @param win.  The window container to be removed from the list.
       */

      void removeWindowContainer(FAR struct SWindow *win);

      /**
       * Find the window container that contains the specified window.
       *
       * @param cwin.  The window whose container is needed.
       * @return On success, the container of the specific window is returned;
       *   NULL is returned on failure.
       */

      FAR struct SWindow *findWindow(FAR CWindow *cwin);

      /**
       * This is the function that responds to the EVENT_WINDOW_DESKTOP.  It
       * iconifies all windows so that the desktop is visible.
       *
       * @return True is returned if the operation was successful.
       */

       bool showDesktop(void);

    public:

      /**
       * CWindowFactory Constructor
       *
       * @param twm4nx.  Twm4Nx session
       */

      CWindowFactory(CTwm4Nx *twm4nx);

      /**
       * CWindowFactory Destructor
       */

      ~CWindowFactory(void);

      /**
       * Add Icon Manager menu items to the Main menu.  This is really part
       * of the instance initialization, but cannot be executed until the
       * Main Menu logic is ready.
       *
       * @return True on success
       */

      bool addMenuItems(void);

      /**
       * Create a new window and add it to the window list.
       *
       * The window is initialized with all application events disabled.
       * The CWindows::configureEvents() method may be called as a second
       * initialization step in order to enable application events.
       *
       * @param name       The window name
       * @param sbitmap    The Icon bitmap
       * @param iconMgr    Pointer to icon manager instance
       * @param flags Toolbar customizations see WFLAGS_NO_* definitions
       * @return           Reference to the allocated CWindow instance
       */

      FAR CWindow *
        createWindow(FAR NXWidgets::CNxString &name,
                     FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                     FAR CIconMgr *iconMgr, uint8_t flags);

      /**
       * Handle the EVENT_WINDOW_DELETE event.  The logic sequence is as
       * follows:
       *
       * 1. The TERMINATE button in pressed in the Window Toolbar and
       *    CWindow::handleActionEvent() catches the button event on the
       *    event listener thread and generates the EVENT_WINDOW_TERMINATE
       * 2. CWindows::event receives the widget event, EVENT_WINDOW_TERMINATE,
       *    on the Twm4NX manin threadand requests to halt the  NX Server
       *    messages queues.
       * 3. when server responds, the CwindowsEvent::handleBlockedEvent
       *    generates the EVENT_WINDOW_DELETE which is caught by
       *    CWindows::event() and which, in turn calls this function.
       *
       * @param cwin The CWindow instance.  This will be deleted and its
       *   associated container will be freed.
       */

      void destroyWindow(FAR CWindow *cwin);

      /**
       * Pick a position for a new Icon on the desktop.  Tries to avoid
       * collisions with other Icons and reserved areas on the background
       *
       * @param cwin The window being iconified.
       * @param defPos The default position to use if there is no free
       *   region on the desktop.
       * @param iconPos The selected Icon position.  Might be the same as
       *   the default position.
       * @return True is returned on success
       */

      bool placeIcon(FAR CWindow *cwin,
                     FAR const struct nxgl_point_s &defPos,
                     FAR struct nxgl_point_s &iconPos);

      /**
       * Redraw icons.  The icons are drawn on the background window.  When
       * the background window receives a redraw request, it will call this
       * method in order to redraw any effected icons drawn in the
       * background.
       *
       * @param nxRect The region in the background to be redrawn
       */

       void redrawIcons(FAR const nxgl_rect_s *nxRect);

      /**
       * Check if the icon within iconBounds collides with any other icon on
       * the desktop.
       *
       * @param cwin The window containing the Icon of interest
       * @param iconBounds The candidate Icon bounding box
       * @param collision The bounding box of the icon that the candidate
       *   collides with
       * @return Returns true if there is a collision
       */

      bool checkCollision(FAR CWindow *cwin,
                          FAR const struct nxgl_rect_s &iconBounds,
                          FAR struct nxgl_rect_s &collision);

      /**
       * Handle WINDOW events.
       *
       * @param msg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *msg);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWFACTORY_HXX
