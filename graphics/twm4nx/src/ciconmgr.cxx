/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ciconmgr.cxx
// Icon Manager routines
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdio>
#include <cstring>
#include <debug.h>

#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cnxfont.hxx"

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

#include "graphics/nxglyphs.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cresize.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"

/////////////////////////////////////////////////////////////////////////////
// Class Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CIconMgr Constructor
 *
 * @param twm4nx   The Twm4Nx session
 * @param ncolumns The number of columns this icon manager has
 */

CIconMgr::CIconMgr(CTwm4Nx *twm4nx, int ncolumns)
{
  m_twm4nx     = twm4nx;                            // Cached the Twm4Nx session
  m_head       = (FAR struct SWindowEntry *)0;      // Head of the winow list
  m_tail       = (FAR struct SWindowEntry *)0;      // Tail of the winow list
  m_active     = (FAR struct SWindowEntry *)0;      // No active window
  m_window     = (FAR CWindow *)0;                  // No icon manager Window
  m_columns    = ncolumns;
  m_currows    = 0;
  m_curcolumns = 0;
  m_count      = 0;
}

/**
 * CIconMgr Destructor
 */

CIconMgr::~CIconMgr(void)
{
  // Free memory allocations

  // Free the icon manager window

  if (m_window != (FAR CWindow *)0)
    {
      delete m_window;
    }
}

/**
 * Create and initialize the icon manager window
 *
 * @param name  The prefix for this icon manager name
 */

bool CIconMgr::initialize(FAR const char *prefix)
{
  // Create the icon manager window

  if (!createWindow(prefix))
    {
      gerr("ERROR:  Failed to create window\n");
      return false;
    }

  // Create the button array widget

  if (!createButtonArray())
    {
      gerr("ERROR:  Failed to button array\n");

      CWindowFactory *factory = m_twm4nx->getWindowFactory();
      factory->destroyWindow(m_window);
      m_window = (FAR CWindow *)0;
      return false;
    }

  return true;
}

/**
 * Add a window to an icon manager
 *
 *  @param win the TWM window structure
 */

bool CIconMgr::add(FAR CWindow *cwin)
{
  // Don't add the icon manager to itself

  if (cwin->isIconMgr())
    {
      return false;
    }

  // Allocate a new icon manager entry

  FAR struct SWindowEntry *wentry =
     (FAR struct SWindowEntry *)malloc(sizeof(struct SWindowEntry));

  if (wentry == (FAR struct SWindowEntry *)0)
    {
      return false;
    }

  wentry->flink     = NULL;
  wentry->iconmgr   = this;
  wentry->active    = false;
  wentry->down      = false;
  wentry->cwin      = cwin;

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  wentry->me = m_count;
  wentry->pos.x  = -1;
  wentry->pos.y  = -1;

  // Insert the new entry into the list

  insertEntry(wentry, cwin);

  // The height of one row is determined (mostly) by the fond height

  int rowHeight = iconManagerFont->getHeight() + 10;
  if (rowHeight < (CONFIG_TWM4NX_ICONMGR_IMAGE.width + 4))
    {
      rowHeight = CONFIG_TWM4NX_ICONMGR_IMAGE.width + 4;
    }

  // Increase the icon window size

  struct nxgl_size_s windowSize;
  if (!m_window->getWindowSize(&windowSize))
    {
      gerr("ERROR: Failed to get window size\n");
    }
  else
    {
      windowSize.h = rowHeight * m_count;
      m_window->setWindowSize(&windowSize);
    }

  // Increment the window count

  m_count++;

  // Pack the windows

  pack();

  // If no other window is active, then mark this as the active window

  if (m_active == NULL)
    {
      m_active = wentry;
    }

  return true;
}

/**
 * Remove a window from the icon manager
 *
 * @param win the TWM window structure
 */

void CIconMgr::remove(FAR struct SWindow *win)
{
  FAR struct SWindowEntry *wentry = win->wentry;

  if (wentry != NULL)
    {
      // Remove the list from the window structure

      removeEntry(wentry);

      // Destroy the button array widget
#warning Missing logic

      // Destroy the window

      CWindowFactory *factory = m_twm4nx->getWindowFactory();
      factory->destroyWindow(wentry->cwin);

      m_count -= 1;
      std::free(wentry);
      pack();
    }
}

/**
 * Move the pointer around in an icon manager
 *
 *  @param dir one of the following:
 *    - EVENT_ICONMGR_FORWARD: Forward in the window list
 *    - EVENT_ICONMGR_BACK:    Backward in the window list
 *    - EVENT_ICONMGR_UP:      Up one row
 *    - EVENT_ICONMGR_DOWN:    Down one row
 *    - EVENT_ICONMGR_LEFT:    Left one column
 *    - EVENT_ICONMGR_RIGHT:   Right one column
 */

void CIconMgr::move(int dir)
{
  if (!m_active)
    {
      return;
    }

  int curRow = m_active->row;
  int curCol = m_active->col;

  int rowIncr = 0;
  int colIncr = 0;
  bool gotIt  = false;

  FAR struct SWindowEntry *wentry = (FAR struct SWindowEntry *)0;

  switch (dir)
    {
    case EVENT_ICONMGR_FORWARD:
      if ((wentry = m_active->flink) == (FAR struct SWindowEntry *)0)
        {
          wentry = m_head;
        }

      gotIt = true;
      break;

    case EVENT_ICONMGR_BACK:
      if ((wentry = m_active->blink) == (FAR struct SWindowEntry *)0)
        {
          wentry = m_tail;
        }

      gotIt = true;
      break;

    case EVENT_ICONMGR_UP:
      rowIncr = -1;
      break;

    case EVENT_ICONMGR_DOWN:
      rowIncr = 1;
      break;

    case EVENT_ICONMGR_LEFT:
      colIncr = -1;
      break;

    case EVENT_ICONMGR_RIGHT:
      colIncr = 1;
      break;
    }

  // If gotIt is false ast this point then we got a left, right, up, or down,
  // command.

  int newRow = curRow;
  int newCol = curCol;

  while (!gotIt)
    {
      newRow += rowIncr;
      newCol += colIncr;

      if (newRow < 0)
        {
          newRow = m_currows - 1;
        }

      if (newCol < 0)
        {
          newCol = m_curcolumns - 1;
        }

      if (newRow >= (int)m_currows)
        {
          newRow = 0;
        }

      if (newCol >= (int)m_curcolumns)
        {
          newCol = 0;
        }

      // Now let's go through the list to see if there is an entry with this
      // new position.

      for (wentry = m_head; wentry != NULL; wentry = wentry->flink)
        {
          if (wentry->row == newRow && wentry->col == newCol)
            {
              gotIt = true;
              break;
            }
        }
    }

  if (!gotIt)
    {
      gwarn("WARNING:  unable to find window (%d, %d) in icon manager\n",
            newRow, newCol);
      return;
    }

  // raise the frame so the icon manager is visible
}

/**
 * Pack the icon manager windows following an addition or deletion
 */

void CIconMgr::pack(void)
{
  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  struct nxgl_size_s colsize;

  colsize.h = iconManagerFont->getHeight() + 10;
  if (colsize.h < (CONFIG_TWM4NX_ICONMGR_IMAGE.height + 4))
    {
      colsize.h = CONFIG_TWM4NX_ICONMGR_IMAGE.height + 4;
    }

  struct nxgl_size_s windowSize;
  if (!m_window->getWindowSize(&windowSize))
    {
      gerr("ERROR: Failed to get window size\n");
      return;
    }

  colsize.w = windowSize.w / m_columns;

  int rowIncr = colsize.h;
  int colIncr = colsize.w;

  int row    = 0;
  int col    = m_columns;
  int maxcol = 0;

  FAR struct SWindowEntry *wentry;
  int i;

  for (i = 0, wentry = m_head;
       wentry != (FAR struct SWindowEntry *)0;
       i++, wentry = wentry->flink)
    {
      wentry->me = i;
      if (++col >= (int)m_columns)
        {
          col = 0;
          row += 1;
        }

      if (col > maxcol)
        {
          maxcol = col;
        }

      struct nxgl_point_s newpos;
      newpos.x = col * colIncr;
      newpos.y = (row - 1) * rowIncr;

      wentry->row    = row - 1;
      wentry->col    = col;

      // If the position or size has not changed, don't touch it

      if (wentry->pos.x  != newpos.x  || wentry->size.w != colsize.w)
        {
          if (!wentry->cwin->setWindowSize(&colsize))
            {
              return;
            }

          wentry->pos.x  = newpos.x;
          wentry->pos.y  = newpos.y;
          wentry->size.w = colsize.w;
          wentry->size.h = colsize.h;
        }
    }

  maxcol      += 1;
  m_currows    = row;
  m_curcolumns = maxcol;

  // The height of one row is determined (mostly) by the fond height

  int rowHeight = iconManagerFont->getHeight() + 10;
  if (rowHeight < (CONFIG_TWM4NX_ICONMGR_IMAGE.width + 4))
    {
      rowHeight = CONFIG_TWM4NX_ICONMGR_IMAGE.width + 4;
    }

  windowSize.h = rowHeight * m_count;
  m_window->setWindowSize(&windowSize);

  if (windowSize.h == 0)
    {
      windowSize.h = rowIncr;
    }

  struct nxgl_size_s newsize;
  newsize.w = maxcol * colIncr;

  if (newsize.w == 0)
    {
      newsize.w = colIncr;
    }

  newsize.h = windowSize.h;

  if (!m_window->setWindowSize(&newsize))
    {
      return;
    }

  // Get the net size of the containing frame

  struct nxgl_size_s frameSize;
  m_window->windowToFrameSize(&windowSize, &frameSize);

  struct nxgl_point_s framePos;
  m_window->getFramePosition(&framePos);

  // Resize the frame

  FAR CResize *resize = m_twm4nx->getResize();
  resize->setupWindow(m_window, &framePos, &frameSize);
}

/**
 * Sort the windows
 */

void CIconMgr::sort(void)
{
  FAR struct SWindowEntry *tmpwin1;
  FAR struct SWindowEntry *tmpwin2;
  bool done;

  done = false;
  do
    {
      for (tmpwin1 = m_head; tmpwin1 != NULL; tmpwin1 = tmpwin1->flink)
        {
          if ((tmpwin2 = tmpwin1->flink) == NULL)
            {
              done = true;
              break;
            }

          if (std::strcmp(tmpwin1->cwin->getWindowName(),
                          tmpwin2->cwin->getWindowName()) > 0)
            {
              // Take it out and put it back in

              removeEntry(tmpwin2);
              insertEntry(tmpwin2, tmpwin2->cwin);
              break;
            }
        }
    }
  while (!done);

  pack();
}

/**
 * Handle ICONMGR events.
 *
 * @param msg.  The received NxWidget ICONMGR event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconMgr::event(FAR struct SEventMsg *msg)
{
  bool ret = true;

  switch (msg->eventID)
    {
      case EVENT_ICONMGR_UP:
      case EVENT_ICONMGR_DOWN:
      case EVENT_ICONMGR_LEFT:
      case EVENT_ICONMGR_RIGHT:
      case EVENT_ICONMGR_FORWARD:
      case EVENT_ICONMGR_BACK:
        {
          move(msg->eventID);
        }
        break;

      case EVENT_ICONMGR_SHOWPARENT:  // Raise Icon manager parent window
        {
          m_window->deIconify();
        }
        break;

      case EVENT_ICONMGR_HIDE:  // Hide the Icon Manager
        {
          hide();
        }
        break;

      case EVENT_ICONMGR_SORT:  // Sort the Icon Manager
        {
          sort();
        }
        break;

      default:
        ret = false;
        break;
    }

  return ret;
}

/**
 * Create and initialize the icon manager window
 *
 * @param name  The prefix for this icon manager name
 */

bool CIconMgr::createWindow(FAR const char *prefix)
{
  static FAR const char *rootName = "Icon Manager";

  // Create the icon manager name using any prefix provided by the creator

  FAR char *allocName = (FAR char *)0;

  if (prefix != (FAR const char *)0)
    {
      std::asprintf(&allocName, "%s %s", prefix, rootName);
    }

  FAR const char *name = (allocName == (FAR char *)0) ? rootName : allocName;

  // Create the icon manager window

  CWindowFactory *factory = m_twm4nx->getWindowFactory();
  bool success = true;

  m_window = factory->createWindow(name, &CONFIG_TWM4NX_ICONMGR_IMAGE,
                                   true, this, false);

  if (m_window == (FAR CWindow *)0)
    {
      gerr("ERROR: Failed to create icon manager window");
      success = false;
    }

  // Free any temporary name strings

  if (allocName != (FAR char *)0)
    {
      std::free(allocName);
    }

  return success;
}

/**
 * Create the button array widget
 */

bool CIconMgr::createButtonArray(void)
{
#warning Missing logic
  return false;
}

/**
 * Put an allocated entry into an icon manager
 *
 *  @param wentry the entry to insert
 */

void CIconMgr::insertEntry(FAR struct SWindowEntry *wentry,
                           FAR CWindow *cwin)
{
  FAR struct SWindowEntry *tmpwin;
  bool added;

  added = false;
  if (m_head == NULL)
    {
      m_head        = wentry;
      wentry->blink = NULL;
      m_tail        = wentry;
      added         = true;
    }

  for (tmpwin = m_head; tmpwin != NULL; tmpwin = tmpwin->flink)
    {
      // Insert the new window in name order

      if (strcmp(cwin->getWindowName(), tmpwin->cwin->getWindowName()) < 0)
        {
          wentry->flink = tmpwin;
          wentry->blink = tmpwin->blink;
          tmpwin->blink = wentry;

          if (wentry->blink == NULL)
            {
              m_head    = wentry;
            }
          else
            {
              wentry->blink->flink = wentry;
            }

          added = true;
          break;
        }
    }

  if (!added)
    {
      m_tail->flink = wentry;
      wentry->blink = m_tail;
      m_tail = wentry;
    }
}

/**
 * Remove an entry from an icon manager
 *
 *  @param wentry the entry to remove
 */

void CIconMgr::removeEntry(FAR struct SWindowEntry *wentry)
{
  if (wentry->blink == NULL)
    {
      m_head = wentry->flink;
    }
  else
    {
      wentry->blink->flink = wentry->flink;
    }

  if (wentry->flink == NULL)
    {
      m_tail = wentry->blink;
    }
  else
    {
      wentry->flink->blink = wentry->blink;
    }
}

/**
 * Set active window
 *
 * @active Window to become active.
 */

void CIconMgr::active(FAR struct SWindowEntry *wentry)
{
  wentry->active = true;
  m_active = wentry;
}

/**
 * Set window inactive
 *
 * @active windows to become inactive.
 */

void CIconMgr::inactive(FAR struct SWindowEntry *wentry)
{
  wentry->active = false;
}

/**
 * Free window list entry.
 */

void CIconMgr::freeWEntry(FAR struct SWindowEntry *wentry)
{
  if (wentry->cwin != (FAR CWindow *)0)
    {
      delete wentry->cwin;
      wentry->cwin = (FAR CWindow *)0;
    }

  free(wentry);
}
