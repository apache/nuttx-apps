/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cwindowfactory.cxx
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdbool>
#include <cassert>
#include <mqueue.h>
#include <debug.h>

#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cnxtkwindow.hxx"
#include "graphics/nxwidgets/cnxtoolbar.hxx"

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"

/////////////////////////////////////////////////////////////////////////////
// CWindowFactory Implementation
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CWindowFactory Constructor
 *
 * @param twm4nx.  Twm4Nx session
 */

CWindowFactory::CWindowFactory(FAR CTwm4Nx *twm4nx)
{
  m_twm4nx     = twm4nx;                  // Cached copy of the Twm4Nx session object
  m_windowHead = (FAR struct SWindow *)0; // List of all Windows

  // Set up the position where we will create the initial window

  m_winpos.x   = 50;
  m_winpos.y   = 50;
}

/**
 * CWindowFactory Destructor
 */

CWindowFactory::~CWindowFactory(void)
{
}

/**
 * Create a new window and add it to the window list.
 *
 * @param name       The window name
 * @param sbitmap    The Icon bitmap
 * @param isIconMgr  Flag to tell if this is an icon manager window
 * @param iconMgr    Pointer to icon manager instance
 * @param noToolbar  True: Don't add Title Bar
 * @return           Reference to the allocated CWindow instance
 */

FAR CWindow *
  CWindowFactory::createWindow(FAR const char *name,
                               FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                               bool isIconMgr, FAR CIconMgr *iconMgr,
                               bool noToolbar)
{
  ginfo("name=%p\n", name);

  // Allocate a container for the Twm4NX window

  FAR struct SWindow *win =
    (FAR struct SWindow *)std::zalloc(sizeof(struct SWindow));
  if (win == (FAR struct SWindow *)0)
    {
      gerr("ERROR: Unable to allocate memory to manage window %s\n",
           name);
      return (FAR CWindow *)0;
    }

  // Create and initialize the window itself

  win->cwin = new CWindow(m_twm4nx);
  if (win->cwin == (FAR CWindow *)0)
    {
      gerr("ERROR: Failed to create CWindow\n");
      std::free(win);
      return (FAR CWindow *)0;
    }

  // Place the window at a random position
  // Default size:  Try a one quarter of the display.

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  struct nxgl_size_s winsize;
  winsize.w = displaySize.w / 2;
  winsize.h = displaySize.h / 2;

  if ((m_winpos.x + winsize.w) > displaySize.w)
    {
      m_winpos.x = 50;
    }

  if ((m_winpos.x + winsize.w) > (displaySize.w - 16))
    {
      winsize.w = displaySize.w - m_winpos.x - 16;
    }

  if ((m_winpos.y + winsize.h) > displaySize.h)
    {
      m_winpos.y = 50;
    }

  if ((m_winpos.y + winsize.h) > (displaySize.h - 16))
    {
      winsize.h = displaySize.h - m_winpos.y - 16;
    }

  ginfo("Position window at (%d,%d), size (%d,%d)\n",
        m_winpos.x, m_winpos.y, winsize.w, winsize.h);

  if (!win->cwin->initialize(name, &m_winpos, &winsize, sbitmap,
                             isIconMgr, iconMgr, noToolbar))
    {
      gerr("ERROR: Failed to initialize CWindow\n");
      delete win->cwin;
      std::free(win);
      return (FAR CWindow *)0;
    }

  // Update the position for the next window

  m_winpos.x += 30;
  m_winpos.y += 30;

  // Add the window into the Twm4Nx window list

  addWindow(win);

  // Add the window container to the icon manager

  CIconMgr *iconmgr = m_twm4nx->getIconMgr();
  DEBUGASSERT(iconmgr != (CIconMgr *)0);

  (void)iconmgr->add(win->cwin);

  // Return the contained window

  return win->cwin;
}

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

void CWindowFactory::destroyWindow(FAR CWindow *cwin)
{
  // Find the container of the window

  FAR struct SWindow *win = findWindow(cwin);
  if (win == (FAR struct SWindow *)0)
    {
      // This should not happen.. worthy of an assertion

      gerr("ERROR: Failed to find CWindow container\n");
    }
  else
    {
      // Remove the window container from the window list

      removeWindow(win);
    }

  // Delete the contained CWindow instance

  delete cwin;

  // And, finally, free the CWindow container

  free(win);
}

/**
 * Handle WINDOW events.
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CWindowFactory::event(FAR struct SEventMsg *eventmsg)
{
  FAR CWindow *cwin = (FAR CWindow *)eventmsg->obj;
  DEBUGASSERT(cwin != (FAR CWindow *)0);

  // Forward the event to the appropriate window

  return cwin->event(eventmsg);
}

/**
 * Add a window container to the window list.
 *
 * @param win.  The window container to be added to the list.
 */

void CWindowFactory::addWindow(FAR struct SWindow *win)
{
  win->blink = (FAR struct SWindow *)0;
  win->flink = m_windowHead;

  if (m_windowHead != (FAR struct SWindow *)0)
    {
      m_windowHead->blink = win;
    }

  m_windowHead = win;
}

/**
 * Remove a window container from the window list.
 *
 * @param win.  The window container to be removed from the list.
 */

void CWindowFactory::removeWindow(FAR struct SWindow *win)
{
  FAR struct SWindow *prev = win->blink;
  FAR struct SWindow *next = win->flink;

  if (prev == (FAR struct SWindow *)0)
    {
      m_windowHead = next;
    }
  else
    {
      prev->flink = next;
    }

  if (next != (FAR struct SWindow *)0)
    {
      next->blink = prev;
    }

  win->flink = NULL;
  win->blink = NULL;
}

/**
 * Find the window container that contains the specified window.
 *
 * @param cwin.  The window whose container is needed.
 * @return On success, the container of the specific window is returned;
 *   NULL is returned on failure.
 */

FAR struct SWindow *CWindowFactory::findWindow(FAR CWindow *cwin)
{
  for (FAR struct SWindow *win = m_windowHead;
       win != (FAR struct SWindow *)0;
       win = win->flink)
    {
      if (win->cwin == cwin)
        {
          return win;
        }
    }

  return (FAR struct SWindow *)0;
}
