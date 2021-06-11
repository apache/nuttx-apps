/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/Cmenus.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CMENUS_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CMENUS_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <mqueue.h>

#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"
#include "graphics/nxwidgets/cnxtkwindow.hxx"

#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/iapplication.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

#define TWM_WINDOWS     "TwmNxWindows"  // for f.menu "TwmNxWindows"

#define SIZE_HINDENT     10
#define SIZE_VINDENT     2

#define MAXMENUDEPTH     10    // max number of nested menus

#define MOVE_NONE        0     // modes of constrained move
#define MOVE_VERT        1
#define MOVE_HORIZ       2

#define SHADOWWIDTH      5     // in pixels

// Info stings defines

#define INFO_LINES       30
#define INFO_SIZE        200

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CButtonArray;                              // Forward reference
  class  CWidgetEventArgs;                          // Forward reference
  class  CWidgetEventArgs;                          // Forward reference
}

namespace Twm4Nx
{
  struct SEventMsg;                                 // Forward referernce
  class  CMenus;                                    // Forward reference

  struct SMenuItem
  {
    FAR struct SMenuItem *flink;                    /**< Forward link to next menu item */
    FAR struct SMenuItem *blink;                    /**< Backward link previous menu item */
    FAR NXWidgets::CNxString text;                  /**< The text string for the menu item */
    FAR CMenus *subMenu;                            /**< Menu root of a pull right menu */
    FAR CTwm4NxEvent *handler;                      /**< Application event handler */
    uint16_t event;                                 /**< Menu selection event */
  };

  class CMenus : protected NXWidgets::CWidgetEventHandler, public CTwm4NxEvent
  {
    private:

      CTwm4Nx                     *m_twm4nx;        /**< Cached Twm4Nx session */
      mqd_t                        m_eventq;        /**< NxWidget event message queue */
      FAR CWindow                 *m_menuWindow;    /**< The menu window */
      FAR NXWidgets::CButtonArray *m_buttons;       /**< The menu button array */
      FAR struct SMenuItem        *m_menuHead;      /**< First item in menu */
      FAR struct SMenuItem        *m_menuTail;      /**< Last item in menu */
      NXWidgets::CNxString         m_menuName;      /**< The name of the menu */
      nxgl_coord_t                 m_entryHeight;   /**< Menu entry height */
      uint16_t                     m_nMenuItems;    /**< Number of items in the menu */
      uint8_t                      m_nrows;         /**< Number of rows in the button array */
      char                         m_info[INFO_LINES][INFO_SIZE];

      /**
       * Convert the position of a menu window to the position of
       * the containing frame.
       */

      void menuToFramePos(FAR const struct nxgl_point_s *menupos,
                          FAR struct nxgl_point_s *framepos);

      /**
       * Convert the position of the containing frame to the position of
       * the menu window.
       */

      void frameToMenuPos(FAR const struct nxgl_point_s *framepos,
                          FAR struct nxgl_point_s *menupos);

      /**
       * Convert the size of a menu window to the size of the containing
       * frame.
       */

      void menuToFrameSize(FAR const struct nxgl_size_s *menusize,
                           FAR struct nxgl_size_s *framesize);

      /**
       * Convert the size of a containing frame to the size of the menu
       * window.
       */

      void frameToMenuSize(FAR const struct nxgl_size_s *framesize,
                           FAR struct nxgl_size_s *menusize);

      /**
       * Create the menu window.  Menu windows are always created in the
       * hidden state.  When the menu is selected, then it should be shown.
       *
       * @result True is returned on success
       */

      bool createMenuWindow(void);

      /**
       * Calculate the optimal menu frame size
       *
       * @param frameSize The location to return the calculated frame size
       */

      void getMenuFrameSize(FAR struct nxgl_size_s &frameSize);

      /**
       * Calculate the optimal menu window size
       *
       * @param size The location to return the calculated window size
       */

      void getMenuWindowSize(FAR struct nxgl_size_s &size);

      /**
       * Update the menu window size
       *
       * @result True is returned on success
       */

      bool setMenuWindowSize(void);

      /**
       * Set the position of the menu window.  Supports positioning of a
       * pop-up window.
       *
       * @param framePos The position of the menu window frame
       * @result True is returned on success
       */

      bool setMenuWindowPosition(FAR struct nxgl_point_s *framePos);

      /**
       * Set the position of the menu window.  Supports presentation of a
       * pop-up window.
       *
       * @param framePos The position of the menu window frame
       * @result True is returned on success
       */

      inline bool raiseMenuWindow()
      {
        return m_menuWindow->raiseWindow();
      }

      /**
       * Create the menu button array
       *
       * @result True is returned on success
       */

      bool createMenuButtonArray(void);

      void paintMenu(void);
      void destroyMenu(void);

      /**
       * Handle a widget action event, overriding the CWidgetEventHandler
       * method.  This will indicate a button pre-release event.
       *
       * @param e The event data.
       */

      void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Cleanup or initialization error or on deconstruction.
       */

      void cleanup(void);

    public:
      /**
       * CMenus Constructor
       *
       * @param twm4nx.  Twm4Nx session
       */

      CMenus(CTwm4Nx *twm4nx);

      /**
       * CMenus Destructor
       */

      ~CMenus(void);

      /**
       * CMenus Initializer.  Performs the parts of the CMenus construction
       * that may fail.  The menu window is created but is not initially
       * visible.  Use the show() method to make the menu visible.
       *
       * @param name The menu name
       * @result True is returned on success
       */

      bool initialize(FAR NXWidgets::CNxString &name);

      /**
       * Add an item to a menu
       *
       * @param item Describes the menu item entry
       * @return True if the menu item was added successfully
       */

      bool addMenuItem(FAR IApplication *item);

      /**
       * Return the size of the menu window frame
       *
       * @param frameSize The location in which to return the current menu
       *   window frame size.
       * @result True is returned on success
       */

      bool getFrameSize(FAR struct nxgl_size_s *frameSize)
      {
        return m_menuWindow->getFrameSize(frameSize);
      }

      /**
       * Set the position of the menu window frame
       *
       * @param framePos The new menum window frame position
       * @result True is returned on success
       */

      bool getFramePosition(FAR struct nxgl_point_s *framePos)
      {
        return m_menuWindow->getFramePosition(framePos);
      }

      /**
       * Set the position of the menu window frame
       *
       * @param framePos The new menum window frame position
       * @result True is returned on success
       */

      bool setFramePosition(FAR const struct nxgl_point_s *framePos)
      {
        return m_menuWindow->setFramePosition(framePos);
      }

      /**
       * Return true if the menu is currently being displayed
       *
       * @return True if the menu is visible
       */

      inline bool isVisible(void)
      {
        return !m_menuWindow->isIconified();
      }

      /**
       * Make the menu visible.
       *
       * @return True if the menu is shown.
       */

      inline bool show(void)
      {
        return m_menuWindow->deIconify();
      }

      /**
       * Hide the menu
       *
       * @return True if the menu was hidden.
       */

      inline bool hide(void)
      {
        return m_menuWindow->iconify();
      }

      /**
       * Handle MENU events.
       *
       * @param msg.  The received NxWidget MENU event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *msg);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CMENUS_HXX
