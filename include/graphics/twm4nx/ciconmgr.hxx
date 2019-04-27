/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/ciconmgr.hxx
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
#include "graphics/twm4nx/ctwm4nxevent.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CNxTkWindow;                      // Forward reference
  class  CButtonArray;                     // Forward reference
  class  CWidgetEventHandler;              // Forward reference
  class  CWidgetEventArgs;                 // Forward reference
  struct SRlePaletteBitmap;                // Forward reference
}

namespace Twm4Nx
{
  struct SWindowEntry
  {
    FAR struct SWindowEntry *flink;
    FAR struct SWindowEntry *blink;
    FAR CWindow *cwin;
    FAR CIconMgr *iconmgr;
    nxgl_point_s pos;
    nxgl_size_s size;
    int row;
    int col;
    bool active;
    bool down;
  };

  class CIconMgr : protected NXWidgets::CWidgetEventHandler, public CTwm4NxEvent
  {
    private:

      FAR CTwm4Nx                    *m_twm4nx;     /**< Cached Twm4Nx session */
      mqd_t                           m_eventq;     /**< NxWidget event message queue */
      FAR struct SWindowEntry        *m_head;       /**< Head of the window list */
      FAR struct SWindowEntry        *m_tail;       /**< Tail of the window list */
      FAR struct SWindowEntry        *m_active;     /**< The active entry */
      FAR struct CWindow             *m_window;     /**< Parent window */
      FAR NXWidgets::CButtonArray    *m_buttons;    /**< The cotained button array */
      uint8_t                         m_maxColumns; /**< Max columns per row */
      uint8_t                         m_nrows;      /**< Number of rows in the button array */
      uint8_t                         m_ncolumns;   /**< Number of columns in the button array */
      unsigned int                    m_nWindows;   /**< The number of windows in the icon mgr. */

      /**
       * Return the height of one row
       *
       * @return The height of one row
       */

      nxgl_coord_t getRowHeight(void);

      /**
       * Create and initialize the icon manager window
       *
       * @param name  The prefix for this icon manager name
       */

      bool createWindow(FAR const char *prefix);

      /**
       * Create the button array widget
       */

      bool createButtonArray(void);

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
       * Set active window
       *
       * @param wentry Window to become active.
       */

      void active(FAR struct SWindowEntry *wentry);

      /**
       * Set window inactive
       *
       * @param wentry windows to become inactive.
       */

      void inactive(FAR struct SWindowEntry *wentry);

      /**
       * Free window list entry.
       */

      void freeWEntry(FAR struct SWindowEntry *wentry);

      /**
       * Handle a widget action event.  This will be a button pre-release event.
       *
       * @param e The event data.
       */

      void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

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
       * Create and initialize the icon manager window
       *
       * @param name  The prefix for this icon manager name
       */

      bool initialize(FAR const char *prefix);

      /**
       * Add a window to an the icon manager
       *
       *  @param win the TWM window structure
       */

      bool add(FAR CWindow *win);

      /**
       * Remove a window from the icon manager
       *
       * @param win the TWM window structure
       */

      void remove(FAR struct SWindow *win);

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

      inline unsigned int getDisplayColumns(void)
      {
         return m_maxColumns;
      }

      /**
       * Get the current column
       */

      inline unsigned int getNumberOfColumns(void)
      {
         return m_ncolumns;
      }

      /**
       * Get the current size
       */

      inline bool getSize(FAR struct nxgl_size_s *size)
      {
         return m_window->getFrameSize(size);
      }

      /**
       * Pack the icon manager windows following an addition or deletion
       */

      void pack(void);

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

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CICONMGR_HXX
