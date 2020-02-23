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

#include <cstdbool>
#include <cassert>
#include <mqueue.h>

#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cnxtkwindow.hxx"
#include "graphics/nxwidgets/cnxtoolbar.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cbackground.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cmainmenu.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"

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
 * Add Icon Manager menu items to the Main menu.  This is really part
 * of the instance initialization, but cannot be executed until the
 * Main Menu logic is ready.
 *
 * @return True on success
 */

bool CWindowFactory::addMenuItems(void)
{
  // Add the Icon Manager entry to the Main Menu.  This provides a quick
  // way to de-iconfigy all windows and show a clean desktop.

  FAR CMainMenu *cmain = m_twm4nx->getMainMenu();
  if (!cmain->addApplication(&m_desktopItem))
    {
      twmerr("ERROR: Failed to add to the Main Menu\n");
      return false;
    }

  return true;
}

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
  CWindowFactory::createWindow(FAR NXWidgets::CNxString &name,
                               FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                               FAR CIconMgr *iconMgr, uint8_t flags)
{
  twminfo("flags=%02x\n", flags);

  // Allocate a container for the Twm4NX window

  FAR struct SWindow *win =
    (FAR struct SWindow *)std::zalloc(sizeof(struct SWindow));
  if (win == (FAR struct SWindow *)0)
    {
      twmerr("ERROR: Unable to allocate memory to manage window\n");
      return (FAR CWindow *)0;
    }

  // Create and initialize the window itself

  win->cwin = new CWindow(m_twm4nx);
  if (win->cwin == (FAR CWindow *)0)
    {
      twmerr("ERROR: Failed to create CWindow\n");
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

  twminfo("Position window at (%d,%d), size (%d,%d)\n",
          m_winpos.x, m_winpos.y, winsize.w, winsize.h);

  if (!win->cwin->initialize(name, &m_winpos, &winsize, sbitmap,
                             iconMgr, flags))
    {
      twmerr("ERROR: Failed to initialize CWindow\n");
      delete win->cwin;
      std::free(win);
      return (FAR CWindow *)0;
    }

  // Update the position for the next window

  m_winpos.x += 30;
  m_winpos.y += 30;

  // Add the window into the window list

  addWindowContainer(win);

  // Add the window to the icon manager if it is a regular window (i.e., if
  // it is not the Icon Manager Window and it is not a Menu Window)

  if (!WFLAGS_IS_ICONMGR(flags) && !WFLAGS_IS_MENU(flags))
    {
      if (iconMgr == (FAR CIconMgr *)0)
        {
          iconMgr = m_twm4nx->getIconMgr();
        }

      iconMgr->addWindow(win->cwin);
    }

  // Return the contained window

  return win->cwin;
}

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

void CWindowFactory::destroyWindow(FAR CWindow *cwin)
{
  // Find the container of the window

  FAR struct SWindow *win = findWindow(cwin);
  if (win == (FAR struct SWindow *)0)
    {
      // This should not happen.. worthy of an assertion

      twmerr("ERROR: Failed to find CWindow container\n");
    }
  else
    {
      // Remove the window container from the window list

      removeWindowContainer(win);
    }

  // Remove the window from the Icon Manager

  // Add the window to the icon manager

  CIconMgr *iconmgr = cwin->getIconMgr();
  DEBUGASSERT(iconmgr != (CIconMgr *)0);

  iconmgr->removeWindow(cwin);

  // Delete the contained CWindow instance

  delete cwin;

  // And, finally, free the CWindow container

  free(win);
}

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

bool CWindowFactory::placeIcon(FAR CWindow *cwin,
                               FAR const struct nxgl_point_s &defPos,
                               FAR struct nxgl_point_s &iconPos)
{
  // Does this window have an Icon?

  bool success = false;
  if (cwin->hasIcon())
    {
      // Get the size of the Display (i.e., the size of the background)

      struct nxgl_size_s displaySize;
      m_twm4nx->getDisplaySize(&displaySize);

      // Get the size of the Icon

      struct nxgl_size_s iconSize;
      cwin->getIconWidgetSize(iconSize);

      // Get the background instance

      FAR CBackground *backgd = m_twm4nx->getBackground();

      // Search for a free region.  Start at the at the left size

      struct nxgl_point_s tmpPos;
      tmpPos.x = CONFIG_TWM4NX_ICON_HSPACING;

      // Try each possible horizontal position until we find a free location or
      // until we run out of positions to test

      nxgl_coord_t iconWidth;
      for (; tmpPos.x < (displaySize.w - iconSize.w); tmpPos.x += iconWidth)
        {
          // Start at the top of the next column

          tmpPos.y = CONFIG_TWM4NX_ICON_VSPACING;

          // Try each possible vertical position until we find a free
          // location or until we run out of positions to test

          iconWidth = 0;
          nxgl_coord_t iconHeight;

          for (; tmpPos.y < (displaySize.h - iconSize.h); tmpPos.y += iconHeight)
            {
              // Create a bounding box at this position

              struct nxgl_rect_s iconBounds;
              iconBounds.pt1.x = tmpPos.x;
              iconBounds.pt1.y = tmpPos.y;
              iconBounds.pt2.x = tmpPos.x + iconSize.w - 1;
              iconBounds.pt2.y = tmpPos.y + iconSize.h - 1;

              // Check if this box intersects any reserved region on the
              // background.

              struct nxgl_rect_s collision;
              if (backgd->checkCollision(iconBounds, collision))
                {
                  // Yes.. Set the width to some small, arbitrary non-zero
                  // value.  This is because the actual colliding object may
                  // be quite wide.  But we may still be able to position
                  // icons above or below the object.

                  if (iconWidth < 20)
                    {
                      iconWidth = 20;
                    }

                  // and reset the vertical search position to move past
                  // the collision.  This may terminate the inner loop.

                  iconHeight = collision.pt2.y - tmpPos.y +
                               CONFIG_TWM4NX_ICON_VSPACING + 1;
                }

              // No.. check if some other icon is already occupying this
              // position

              else if (checkCollision(cwin, iconBounds, collision))
                {
                 // Yes.. We need to keep track of the widest icon for the
                 // case where we move to the next column.

                  nxgl_coord_t tmpWidth = collision.pt2.x - tmpPos.x + 1;
                  if (tmpWidth > iconWidth)
                    {
                      iconWidth = tmpWidth;
                    }

                  // And reset the vertical search position to move past the
                  // collision.  This may terminate the inner loop.

                  iconHeight = collision.pt2.y - tmpPos.y +
                               CONFIG_TWM4NX_ICON_VSPACING + 1;
                }

              // No collision.. place the icon at this position

              else
                {
                  iconPos.x = tmpPos.x;
                  iconPos.y = tmpPos.y;
                  return true;
                }
            }

          // Add some configurable spacing to the maximum width of this
          // column.  The next column will skip 'right' by this width.

          iconWidth += CONFIG_TWM4NX_ICON_HSPACING;
        }

      // No free region found, use the user provided default

      iconPos.x = defPos.x;
      iconPos.y = defPos.y;
      success   = true;
    }

  return success;
}

/**
 * Redraw icons.  The icons are drawn on the background window.  When
 * the background window receives a redraw request, it will call this
 * method in order to redraw any effected icons drawn in the
 * background.
 *
 * @param nxRect The region in the background to be redrawn
 */

void CWindowFactory::redrawIcons(FAR const nxgl_rect_s *nxRect)
{
  twminfo("Redrawing...\n");

  // Try each window

  for (FAR struct SWindow *win = m_windowHead;
       win != (FAR struct SWindow *)0;
       win = win->flink)
    {
      // Check if the window has an icon and it is in the iconified state

      FAR CWindow *cwin = win->cwin;
      if (cwin->hasIcon() && cwin->isIconified())
        {
          // Yes.. Create a bounding box for the icon

          struct nxgl_size_s iconSize;
          cwin->getIconWidgetSize(iconSize);

          struct nxgl_point_s iconPos;
          cwin->getIconWidgetPosition(iconPos);

          struct nxgl_rect_s iconBounds;
          iconBounds.pt1.x = iconPos.x;
          iconBounds.pt1.y = iconPos.y;
          iconBounds.pt2.x = iconPos.x + iconSize.w - 1;
          iconBounds.pt2.y = iconPos.y + iconSize.h - 1;

          // Does anything within bounding box need to be redrawn?

          struct nxgl_rect_s intersection;
          nxgl_rectintersect(&intersection, nxRect, &iconBounds);

          if (!nxgl_nullrect(&intersection))
            {
              // Yes.. Redraw the icon (or a portion of the icon)

              twminfo("Redraw icon\n");
              cwin->redrawIcon();
            }
        }
    }
}

/**
 * Check if the icon within iconBounds collides with any other icon on the
 * desktop.
 *
 * @param cwin The window containing the Icon of interest
 * @param iconBounds The candidate Icon bounding box
 * @param collision The bounding box of the icon that the candidate collides
 *   with
 * @return Returns true if there is a collision
 */

bool CWindowFactory::checkCollision(FAR CWindow *cwin,
                                    FAR const struct nxgl_rect_s &iconBounds,
                                    FAR struct nxgl_rect_s &collision)
{
  // Try every window

  for (FAR struct SWindow *win = m_windowHead;
       win != (FAR struct SWindow *)0;
       win = win->flink)
    {
      // Ignore 'this' window, any windows that are not iconified, and any
      // windows that have no icons.

      if (win->cwin != cwin && win->cwin->hasIcon() &&
          win->cwin->isIconified())
        {
          // Create a bounding box for the icon

          struct nxgl_size_s iconSize;
          win->cwin->getIconWidgetSize(iconSize);

          struct nxgl_point_s iconPos;
          win->cwin->getIconWidgetPosition(iconPos);

          collision.pt1.x = iconPos.x;
          collision.pt1.y = iconPos.y;
          collision.pt2.x = iconPos.x + iconSize.w - 1;
          collision.pt2.y = iconPos.y + iconSize.h - 1;

          // Return true if there is an intersection

          if (nxgl_intersecting(&iconBounds, &collision))
            {
              return true;
            }
        }
    }

  // No collision

  return false;
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
  twminfo("eventID: %d\n", eventmsg->eventID);

  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_TOOLBAR_XYINPUT:  // Poll for toolbar mouse/touch events
        {
          FAR struct SXyInputEventMsg *nxmsg =
            (FAR struct SXyInputEventMsg *)eventmsg;
          FAR CWindow *cwin = (FAR CWindow *)nxmsg->obj;
          DEBUGASSERT(cwin != (FAR CWindow *)0);

          success = cwin->pollToolbarEvents();
        }
        break;

      case EVENT_WINDOW_DESKTOP:   // Show the desktop
        success = showDesktop();
        break;

      // Forward the event to the appropriate window

      default:                     // All other window messages
        {
          FAR CWindow *cwin = (FAR CWindow *)eventmsg->obj;
          DEBUGASSERT(cwin != (FAR CWindow *)0);

          success = cwin->event(eventmsg);
        }
        break;
    }

  return success;
}

/**
 * Add a window container to the window list.
 *
 * @param win.  The window container to be added to the list.
 */

void CWindowFactory::addWindowContainer(FAR struct SWindow *win)
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

void CWindowFactory::removeWindowContainer(FAR struct SWindow *win)
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

/**
 * This is the function that responds to the EVENT_WINDOW_DESKTOP.  It
 * iconifies all windows so that the desktop is visible.
 */

bool CWindowFactory::showDesktop(void)
{
  // Add the Icon Manager entry to the Main Menu.  This provides a quick
  // way to de-iconfigy all windows and show a clean desktop.

  for (FAR struct SWindow *win = m_windowHead;
       win != (FAR struct SWindow *)0;
       win = win->flink)
    {
      // Iconify everything:  Application windows, the Icon Manage, all
      // Menus, etc.

      win->cwin->iconify();
    }

  return true;
}
