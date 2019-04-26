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
#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/cbuttonarray.hxx"

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

CIconMgr::CIconMgr(CTwm4Nx *twm4nx, uint8_t ncolumns)
{
  m_twm4nx     = twm4nx;                            // Cached the Twm4Nx session
  m_head       = (FAR struct SWindowEntry *)0;      // Head of the winow list
  m_tail       = (FAR struct SWindowEntry *)0;      // Tail of the winow list
  m_active     = (FAR struct SWindowEntry *)0;      // No active window
  m_window     = (FAR CWindow *)0;                  // No icon manager Window
  m_maxColumns = ncolumns;                          // Max columns per row
  m_nrows      = 0;                                 // No rows yet
  m_ncolumns   = 0;                                 // No columns yet
  m_nWindows   = 0;                                 // No windows yet
}

/**
 * CIconMgr Destructor
 */

CIconMgr::~CIconMgr(void)
{
  // Free the icon manager window

  if (m_window != (FAR CWindow *)0)
    {
      delete m_window;
    }

  // Free the button array

  if (m_buttons != (FAR NXWidgets::CButtonArray *)0)
    {
      delete m_buttons;
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
  wentry->pos.x     = -1;
  wentry->pos.y     = -1;

  // Insert the new entry into the list

  insertEntry(wentry, cwin);

  // The height of one row is determined (mostly) by the font height

  nxgl_coord_t rowHeight = getRowHeight();

  // Increase the icon window size

  struct nxgl_size_s windowSize;
  if (!m_window->getWindowSize(&windowSize))
    {
      gerr("ERROR: Failed to get window size\n");
    }
  else
    {
      windowSize.h = rowHeight * m_nWindows;
      m_window->setWindowSize(&windowSize);
    }

  // Increment the window count

  m_nWindows++;

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

      // Destroy the window

      CWindowFactory *factory = m_twm4nx->getWindowFactory();
      factory->destroyWindow(wentry->cwin);

      m_nWindows--;
      std::free(wentry);
      pack();
    }
}

/**
 * Pack the icon manager windows following an addition or deletion
 */

void CIconMgr::pack(void)
{
  struct nxgl_size_s colsize;
  colsize.h = getRowHeight();

  struct nxgl_size_s windowSize;
  if (!m_window->getWindowSize(&windowSize))
    {
      gerr("ERROR: Failed to get window size\n");
      return;
    }

  colsize.w = windowSize.w / m_maxColumns;

  int rowIncr = colsize.h;
  int colIncr = colsize.w;

  int row    = 0;
  int col    = m_maxColumns;
  int maxcol = 0;

  FAR struct SWindowEntry *wentry;
  int i;

  for (i = 0, wentry = m_head;
       wentry != (FAR struct SWindowEntry *)0;
       i++, wentry = wentry->flink)
    {
      if (++col >= (int)m_maxColumns)
        {
          col  = 0;
          row += 1;
        }

      if (col > maxcol)
        {
          maxcol = col;
        }

      struct nxgl_point_s newpos;
      newpos.x    = col * colIncr;
      newpos.y    = (row - 1) * rowIncr;

      wentry->row = row - 1;
      wentry->col = col;

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

  maxcol++;

  // Check if there is a change in the dimension of the button array

  if (m_nrows != row && m_ncolumns != maxcol)
    {
      // Yes.. remember the new size

      m_nrows     = row;
      m_ncolumns  = maxcol;

      // The height of one row is determined (mostly) by the font height

      windowSize.h = getRowHeight() * m_nWindows;
      if (!m_window->getWindowSize(&windowSize))
        {
          gerr("ERROR: getWindowSize() failed\n");
          return;
        }

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
          gerr("ERROR: setWindowSize() failed\n");
          return;
        }

      // Resize the button array

      nxgl_coord_t buttonWidth  = newsize.w / m_maxColumns;
      nxgl_coord_t buttonHeight = newsize.w / m_nrows;

      if (!m_buttons->resizeArray(m_maxColumns, m_nrows,
                                  buttonWidth, buttonHeight))
        {
          gerr("ERROR: CButtonArray::resizeArray failed\n");
          return;
        }

      // Re-apply all of the button labels

      int rowndx = 0;
      int colndx = 0;

      for (FAR struct SWindowEntry *swin = m_head;
           swin != (FAR struct SWindowEntry *)0;
           swin = swin->flink)
        {
          // Get the window name as an NWidgets::CNxString

          NXWidgets::CNxString string = swin->cwin->getWindowName();

          // Apply the window name to the button

          m_buttons->setText(colndx, rowndx, string);

          // Increment the column, rolling over to the next row if necessary

          if (++colndx >= m_maxColumns)
            {
              colndx = 0;
              rowndx++;
            }
        }
    }
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
      default:
        ret = false;
        break;
    }

  return ret;
}

/**
 * Return the height of one row
 *
 * @return The height of one row
 */

nxgl_coord_t CIconMgr::getRowHeight(void)
{
  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  nxgl_coord_t rowHeight = iconManagerFont->getHeight() + 10;
  if (rowHeight < (CONFIG_TWM4NX_ICONMGR_IMAGE.width + 4))
    {
      rowHeight = CONFIG_TWM4NX_ICONMGR_IMAGE.width + 4;
    }

  return rowHeight;
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
  // Create a Widget control instance for the window using the default style
  // for now.  CWindowEvent derives from CWidgetControl.
  // REVISIT: Create the style, using the selected colors.

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx);
  if (control == (FAR CWindowEvent *)0)
    {
      return false;
    }

  // Get the width of the window

  struct nxgl_size_s windowSize;
  if (!m_window->getWindowSize(&windowSize))
    {
      gerr("ERROR: Failed to get window size\n");
      delete control;
      return false;
    }

  // The button must be positioned at the upper left of the window

  struct nxgl_point_s arrayPos;
  m_window->getWindowPosition(&arrayPos);

  // Create the button array
  // REVISIT:  Hmm.. Button array cannot be dynamically resized!

  uint8_t nrows = m_nrows > 0 ? m_nrows : 1;

  nxgl_coord_t buttonWidth  = windowSize.w / m_maxColumns;
  nxgl_coord_t buttonHeight = windowSize.w / nrows;

  // Now we have enough information to create the button array

  m_buttons = new NXWidgets::CButtonArray(control,
                                          arrayPos.x, arrayPos.y,
                                          m_maxColumns, nrows,
                                          buttonWidth, buttonHeight);
  if (m_buttons == (FAR NXWidgets::CButtonArray *)0)
    {
      gerr("ERROR: Failed to get window size\n");
      delete control;
      return false;
    }

  // Configure the button array widget

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  m_buttons->setFont(iconManagerFont);
  m_buttons->setBorderless(true);
  m_buttons->disableDrawing();
  m_buttons->setRaisesEvents(false);

  // Register to get events from the mouse clicks on the image

  m_buttons->addWidgetEventHandler(this);
  return true;
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

/**
 * Handle a widget action event.  This will be a button pre-release event.
 *
 * @param e The event data.
 */

void CIconMgr::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // A button should now be clicked

  int column;
  int row;

  if (m_buttons->isButtonClicked(column, row))
    {
      // Get the text associated with this button

      const NXWidgets::CNxString string = m_buttons->getText(column, row);

      // Now find the window with this name

      for (FAR struct SWindowEntry *swin = m_head;
           swin != (FAR struct SWindowEntry *)0;
           swin = swin->flink)
        {
          // Check if the button string is the same as the window name

          if (string.compareTo(swin->cwin->getWindowName()) == 0)
            {
              // Got it.  Is the window Iconified?

              if (swin->cwin->isIconified())
                {
                  // Yes, de-Iconify it

                  swin->cwin->deIconify();
                }
              else
                {
                  // Otherwise, raise the window to the top of the heirarchy

                  swin->cwin->raiseWindow();
                }

              break;
            }
        }
    }
}

