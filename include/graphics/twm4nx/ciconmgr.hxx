/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/ciconmgr.hxx
// Icon Manager includes
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CICONMGR_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CICONMGR_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include <mqueue.h>

#include <nuttx/nx/nxglib.h>
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cmainmenu.hxx"
#include "graphics/twm4nx/iapplication.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CNxTkWindow;                              // Forward reference
  class  CButtonArray;                             // Forward reference
  class  CWidgetEventHandler;                      // Forward reference
  class  CWidgetEventArgs;                         // Forward reference
  struct SRlePaletteBitmap;                        // Forward reference
}

namespace Twm4Nx
{
  struct SWindowEntry
  {
    FAR struct SWindowEntry *flink;                 /**< Forward link to next window entry */
    FAR struct SWindowEntry *blink;                 /**< Backward link to previous window entry */
    FAR CWindow *cwin;                              /**< The window payload */
    int row;                                        /**< Y position in the button array */
    int column;                                     /**< X position in the button array */
  };

  class CIconMgr : protected NXWidgets::CWidgetEventHandler,
                   protected IApplication,
                   public CTwm4NxEvent
  {
    private:

      FAR CTwm4Nx                    *m_twm4nx;     /**< Cached Twm4Nx session */
      mqd_t                           m_eventq;     /**< NxWidget event message queue */
      NXWidgets::CNxString            m_name;       /**< The Icon Manager name */
      FAR struct SWindowEntry        *m_head;       /**< Head of the window list */
      FAR struct SWindowEntry        *m_tail;       /**< Tail of the window list */
      FAR struct CWindow             *m_window;     /**< Parent window */
      FAR NXWidgets::CButtonArray    *m_buttons;    /**< The contained button array */
      uint16_t                        m_nWindows;   /**< The number of windows in the icon mgr. */
      uint8_t                         m_nColumns;   /**< Fixed number of columns per row */
      uint8_t                         m_nrows;      /**< Number of rows in the button array */

      /**
       * Return the width of one button
       *
       * @return The width of one button
       */

      inline nxgl_coord_t getButtonWidth(void);

      /**
       * Return the height of one button
       *
       * @return The height of one button
       */

      inline nxgl_coord_t getButtonHeight(void);

      /**
       * Create and initialize the icon manager window
       *
       * @param name  The prefix for this icon manager name
       */

      bool createIconManagerWindow(FAR const char *prefix);

      /**
       * Create the button array widget
       */

      bool createButtonArray(void);

      /**
       * Label each button with the window name
       */

      void labelButtons(void);

      /**
       * Put an allocated entry into an icon manager
       *
       *  @param wentry the entry to insert
       */

      void insertEntry(FAR struct SWindowEntry *wentry,
                       FAR CWindow *cwin);

      /**
       * Remove an entry from an icon manager
       *
       *  @param wentry the entry to remove
       */

      void removeEntry(FAR struct SWindowEntry *wentry);

      /**
       * Find an entry in the icon manager
       *
       *  @param cwin The window to find
       *  @return The incon manager entry (unless an error occurred)
       */

      FAR struct SWindowEntry *findEntry(FAR CWindow *cwin);

      /**
       * Free window list entry.
       */

      void freeWEntry(FAR struct SWindowEntry *wentry);

      /**
       * Handle a widget action event, overriding the CWidgetEventHandler
       * method.  This will indicate a button pre-release event.
       *
       * @param e The event data.
       */

      void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Return the Main Menu item string.  This overrides the method from
       * IApplication
       *
       * @param name The name of the application.
       */

      inline NXWidgets::CNxString getName(void)
      {
        return m_name;
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
       * @return. This function always returns EVENT_ICONMGR_DEICONIFY.
       */

      inline uint16_t getEvent(void)
      {
        return EVENT_ICONMGR_DEICONIFY;
      }

    public:

      /**
       * CIconMgr Constructor
       *
       * @param twm4nx   The Twm4Nx session
       * @param ncolumns The number of columns this icon manager has
       */

      CIconMgr(CTwm4Nx *twm4nx, uint8_t ncolumns);

      /**
       * CIconMgr Destructor
       */

      ~CIconMgr(void);

      /**
       * Create and initialize the icon manager window.
       *
       * @param name  The prefix for this icon manager name
       * @return True on success
       */

      bool initialize(FAR const char *prefix);

      /**
       * Add Icon Manager menu items to the Main menu.  This is really a
       * part of the logic that belongs in initialize() but cannot be
       * executed in that context because it assumes that the Main Menu
       * logic is ready.
       *
       * @return True on success
       */

      bool addMenuItems(void);

      /**
       * Add a window to an the icon manager
       *
       *  @param cwin the TWM window structure
       */

      bool addWindow(FAR CWindow *cwin);

      /**
       * Remove a window from the icon manager
       *
       * @param cwin the TWM window structure
       */

      void removeWindow(FAR CWindow *cwin);

      /**
       * Hide the icon manager
       */

      inline void hide(void)
      {
        if (m_window != (FAR CWindow *)0)
          {
            m_window->iconify();
          }
      }

      /**
       * Get the number of columns
       */

      inline unsigned int getNumberOfColumns(void)
      {
         return m_nColumns;
      }

      /**
       * Get the current size
       */

      inline bool getSize(FAR struct nxgl_size_s *size)
      {
         return m_window->getFrameSize(size);
      }

      /**
       * Resize the button array, possibly adjusting the window height
       *
       * @return True if the button array was resized successfully
       */

      bool resizeIconManager(void);

      /**
       * sort the windows
       */

      void sort(void);

      /**
       * Handle ICONMGR events.
       *
       * @param eventmsg.  The received NxWidget ICONMGR event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);
  };
}

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CICONMGR_HXX
