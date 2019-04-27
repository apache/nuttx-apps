/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cwindowfactory.hxx
// A collection of Window Helpers:   Add a new window, put the titlebar and
// other stuff around the window
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWFACTORY_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWFACTORY_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdbool>

#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"

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
    FAR struct SWindow *flink;             /**< Foward link tonext window */
    FAR struct SWindow *blink;             /**< Backward link to previous window */
    FAR struct SWindowEntry *wentry;       /**< Icon manager list entry (for list removal) */
    FAR CWindow *cwin;                     /**< Window object payload */
  };

  /**
   * The CWindowFactory class creates new window instances and manages some
   * things that are common to all windows.
   */

  class CWindowFactory: public CTwm4NxEvent
  {
    private:

      CTwm4Nx             *m_twm4nx;     /**< Cached Twm4Nx session */
      struct nxgl_point_s  m_winpos;     /**< Position of next window created */
      FAR struct SWindow  *m_windowHead; /**< List of all windows on the display */

      /**
       * Add a window container to the window list.
       *
       * @param win.  The window container to be added to the list.
       */

      void addWindow(FAR struct SWindow *win);

      /**
       * Remove a window container from the window list.
       *
       * @param win.  The window container to be removed from the list.
       */

      void removeWindow(FAR struct SWindow *win);

      /**
       * Find the window container that contains the specified window.
       *
       * @param cwin.  The window whose container is needed.
       * @return On success, the container of the specific window is returned;
       *   NULL is returned on failure.
       */

      FAR struct SWindow *findWindow(FAR CWindow *cwin);

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
       * Create a new window and add it to the window list.
       *
       * @param name      The window name
       * @param sbitmap    The Icon bitmap
       * @param isIconMgr Flag to tell if this is an icon manager window
       * @param iconMgr   Pointer to icon manager instance
       * @param noToolbar True: Don't add Title Bar
       * @return          Reference to the allocated CWindow instance
       */

      FAR CWindow *
        createWindow(FAR const char *name,
                     FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                     bool isIconMgr, FAR CIconMgr *iconMgr, bool noToolbar);

      /**
       * Handle the EVENT_WINDOW_DELETE event.  The logic sequence is as
       * follows:
       *
       * 1. The TERMINATE button in pressed in the Window Toolbar
       * 2. CWindowFactory::event receives the widget event,
       *    EVENT_WINDOW_TERMINATE and request to halt the  NX Server
       *    messages queues.
       * 3. The server responds with the EVENT_WINDOW_DELETE which is
       *    caught by this function.
       *
       * @param cwin The CWindow instance.  This will be deleted and its
       *   associated container will be freed.
       */

      void destroyWindow(FAR CWindow *cwin);

      /**
       * Return the head of the window list.
       *
       * @return The head of the window list.
       */

      inline FAR struct SWindow *windowHead(void)
      {
        return m_windowHead;
      }

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

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOWFACTORY_HXX
