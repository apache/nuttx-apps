/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/Cmenus.hxx
// Twm4Nx menus definitions
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CMENUS_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CMENUS_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <mqueue.h>

#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

#include "graphics/twm4nx/ctwm4nxevent.hxx"

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
  class  CNxTkWindow;                              // Forward reference
  class  CListBox;                                 // Forward reference
  class  CWidgetEventArgs;                         // Forward reference
  class  CWidgetEventArgs;                         // Forward reference
}

namespace Twm4Nx
{
  struct SEventMsg;                                // Forward referernce
  class  CWindow;                                  // Forward reference
  class  CMenus;                                   // Forward reference

  struct SMenuItem
  {
    FAR struct SMenuItem *flink;                   /**< Forward link to next menu item */
    FAR struct SMenuItem *blink;                   /**< Backward link previous menu item */
    FAR CMenus *subMenu;                           /**< Menu root of a pull right menu */
    FAR char *text;                                /**< The text string for the menu item */
    FAR CTwm4NxEvent *handler;                     /**< Application event handler */
    uint16_t index;                                /**< Index of this menu item */
    uint16_t event;                                /**< Menu selection event */
  };

  class CMenus : protected NXWidgets::CWidgetEventHandler, public CTwm4NxEvent
  {
    private:

      CTwm4Nx                    *m_twm4nx;        /**< Cached Twm4Nx session */
      mqd_t                       m_eventq;        /**< NxWidget event message queue */
      FAR NXWidgets::CNxTkWindow *m_menuWindow;    /**< The menu window */
      FAR CMenus                 *m_popUpMenu;     /**< Pop-up menu */
      FAR NXWidgets::CListBox    *m_menuListBox;   /**< The menu list box */
      FAR struct SMenuItem       *m_activeItem;    /**< The active menu item */
      FAR struct SMenuItem       *m_menuHead;      /**< First item in menu */
      FAR struct SMenuItem       *m_menuTail;      /**< Last item in menu */
      FAR char                   *m_menuName;      /**< The name of the menu */
      nxgl_coord_t                m_entryHeight;   /**< Menu entry height */
      uint16_t                    m_nMenuItems;    /**< Number of items in the menu */
      uint8_t                     m_menuDepth;     /**< Number of menus up */
      bool                        m_menuPull;      /**< Is there a pull right entry? */
      char                        m_info[INFO_LINES][INFO_SIZE];

      void identify(FAR CWindow *cwin);

      /**
       * Convert the position of a menu window to the position of
       * the containing frame.
       */

      inline void menuToFramePos(FAR const struct nxgl_point_s *menupos,
                                   FAR struct nxgl_point_s *framepos)
      {
        framepos->x = menupos->x - CONFIG_NXTK_BORDERWIDTH;
        framepos->y = menupos->y - CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Convert the position of the containing frame to the position of
       * the menu window.
       */

      inline void frameToMenuPos(FAR const struct nxgl_point_s *framepos,
                                   FAR struct nxgl_point_s *menupos)
      {
        menupos->x = framepos->x + CONFIG_NXTK_BORDERWIDTH;
        menupos->y = framepos->y + CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Convert the size of a menu window to the size of the containing
       * frame.
       */

      inline void menuToFrameSize(FAR const struct nxgl_size_s *menusize,
                                  FAR struct nxgl_size_s *framesize)
      {
        framesize->w = menusize->w + 2 * CONFIG_NXTK_BORDERWIDTH;
        framesize->h = menusize->h + 2 * CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Convert the size of a containing frame to the size of the menu
       * window.
       */

      inline void frameToMenuSize(FAR const struct nxgl_size_s *framesize,
                                  FAR struct nxgl_size_s *menusize)
      {
        menusize->w = framesize->w - 2 * CONFIG_NXTK_BORDERWIDTH;
        menusize->h = framesize->h - 2 * CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Create the menu window
       *
       * @result True is returned on success
       */

      bool createMenuWindow(void);

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
        return m_menuWindow->raise();
      }

      /**
       * Create the menu list box
       *
       * @result True is returned on success
       */

      bool createMenuListBox(void);

      void paintMenu(void);
      void destroyMenu(void);

      /**
       * Pop up a pull down menu.
       *
       * @param pos    Location of upper left of menu
       */

      bool popUpMenu(FAR struct nxgl_point_s *pos);

      /**
       * Override the virtual value change event.  This will get events
       * when there is a change in the list box selection.
       *
       * @param e The event data.
       */

      void handleValueChangeEvent(const NXWidgets::CWidgetEventArgs &e);

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
       * that may fail.
       *
       * @param name The menu name
       * @result True is returned on success
       */

      bool initialize(FAR const char *name);

      /**
       * Add an item to a root menu
       *
       *  \param text    The text to appear in the menu
       *  \param subMenu The menu root if it is a pull-right entry
       *  \param handler The application event handler.  Should be NULL unless
       *                 the event recipient is EVENT_RECIPIENT_APP
       *  \param event   The event to generate on menu item selection
       */

      bool addMenuItem(FAR const char *text, FAR CMenus *subMenu,
                       FAR CTwm4NxEvent *handler, uint16_t event);

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

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CMENUS_HXX
