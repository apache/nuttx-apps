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

#include <cstdio>
#include <cstring>
#include <cerrno>

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
#include "graphics/twm4nx/cmainmenu.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
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
  m_twm4nx   = twm4nx;                            // Cached the Twm4Nx session
  m_eventq   = (mqd_t)-1;                         // No widget message queue yet
  m_head     = (FAR struct SWindowEntry *)0;      // Head of the winow list
  m_tail     = (FAR struct SWindowEntry *)0;      // Tail of the winow list
  m_window   = (FAR CWindow *)0;                  // No icon manager Window
  m_buttons  = (FAR NXWidgets::CButtonArray *)0;  // The button array
  m_nColumns = ncolumns;                          // Max columns per row
  m_nrows    = 0;                                 // No rows yet
  m_nWindows = 0;                                 // No windows yet
}

/**
 * CIconMgr Destructor
 */

CIconMgr::~CIconMgr(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      mq_close(m_eventq);
    }

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
  // Open a message queue to NX events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();
  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             mqname, errno);
      return false;
    }

  // Create the icon manager window

  if (!createIconManagerWindow(prefix))
    {
      twmerr("ERROR:  Failed to create window\n");
      return false;
    }

  // Create the button array widget

  if (!createButtonArray())
    {
      twmerr("ERROR:  Failed to create button array\n");

      CWindowFactory *factory = m_twm4nx->getWindowFactory();
      factory->destroyWindow(m_window);
      m_window = (FAR CWindow *)0;
      return false;
    }

  return true;
}

/**
 * Add Icon Manager menu items to the Main menu.  This is really a
 * part of the logic that belongs in initialize() but cannot be
 * executed in that context because it assumes that the Main Menu
 * logic is ready.
 *
 * @return True on success
 */

bool CIconMgr::addMenuItems(void)
{
  // Add the Icon Manager entry to the Main Menu.  This provides a quick
  // way to de-iconfigy or to bring the Icon Manager to the top in a
  // crowded desktop.

  FAR CMainMenu *cmain = m_twm4nx->getMainMenu();
  if (!cmain->addApplication(this))
    {
      twmerr("ERROR: Failed to add to the Main Menu\n");
      return false;
    }

  return true;
}

/**
 * Add a window to an icon manager
 *
 *  @param win the TWM window structure
 */

bool CIconMgr::addWindow(FAR CWindow *cwin)
{
  // Don't add the icon manager to itself

  if (cwin->isIconMgr())
    {
      return false;
    }

  // Allocate a new icon manager entry

  FAR struct SWindowEntry *wentry =
     (FAR struct SWindowEntry *)std::malloc(sizeof(struct SWindowEntry));

  if (wentry == (FAR struct SWindowEntry *)0)
    {
      return false;
    }

  wentry->flink   = NULL;
  wentry->cwin    = cwin;
  wentry->row     = -1;
  wentry->column  = -1;

  // Insert the new entry into the list

  insertEntry(wentry, cwin);

  // Increment the window count, calculate the new number of rows

  m_nWindows++;

  uint8_t oldrows = m_nrows == 0 ? 1 : m_nrows;
  m_nrows         = (m_nWindows + m_nColumns - 1) / m_nColumns;

  // Did the number of rows change?

  if (oldrows != m_nrows)
    {
      // Yes.. Resize the button array and containing window to account for the
      // change in the number of windows

      if (!resizeIconManager())
        {
          twmerr("ERROR: resizeIconManager failed\n");
          removeWindow(cwin);
          return false;
        }
    }
  else
    {
      // No.. Just re-label the buttons

      labelButtons();
    }

  return true;
}

/**
 * Remove a window from the icon manager
 *
 * @param win the TWM window structure
 */

void CIconMgr::removeWindow(FAR CWindow *cwin)
{
  if (cwin != (FAR CWindow *)0)
    {
      // Find the entry containing this Window

      FAR struct SWindowEntry *wentry = findEntry(cwin);
      if (wentry != (FAR struct SWindowEntry *)0)
        {
          // Remove the Window from the icon manager list

          removeEntry(wentry);
          m_nWindows--;
          std::free(wentry);

          // Check if the number of rows in the button array changed

          uint8_t oldrows = m_nrows == 0 ? 1 : m_nrows;
          m_nrows         = (m_nWindows + m_nColumns - 1) / m_nColumns;
          uint8_t newrows = m_nrows == 0 ? 1 : m_nrows;

          // Did the number of rows change?

          if (oldrows != newrows)
            {
              // Yes.. Resize the button array to account for the change in the
              // number of windows

              if (!resizeIconManager())
                {
                  twmerr("ERROR: resizeIconManager failed\n");
                }
            }
          else
            {
              // No.. Just re-label the buttons

              labelButtons();
            }
        }
    }
}

/**
 * Resize the button array and containing window
 *
 * @return True if the button array was resized successfully
 */

bool CIconMgr::resizeIconManager(void)
{
  // Get the number of rows in the button array but there needs to be at
  // least one row

  uint8_t newrows = m_nrows == 0 ? 1 : m_nrows;

  // Resize the window.  It will change only in height so we not have to
  // have to use resizeFrame().

  struct nxgl_size_s windowSize;
  if (!m_window->getWindowSize(&windowSize))
    {
      twmerr("ERROR: Failed to get old window size\n");
      return false;
    }

  nxgl_coord_t rowHeight = getButtonHeight();
  windowSize.h = newrows * rowHeight;
  if (!m_window->setWindowSize(&windowSize))
    {
      twmerr("ERROR: Failed to set new window size\n");
      return false;
    }

  m_window->synchronize();

  // Redimension the button array

  nxgl_coord_t buttonWidth  = windowSize.w / m_nColumns;
  if (!m_buttons->resizeArray(m_nColumns, newrows, buttonWidth,
                              rowHeight))
    {
      twmerr("ERROR: CButtonArray::resizeArray failed\n");
      return false;
    }

  // Re-apply all of the button labels

  labelButtons();
  return true;
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

          NXWidgets::CNxString windowName = tmpwin1->cwin->getWindowName();
          if (windowName.compareTo(tmpwin2->cwin->getWindowName()) > 0)
            {
              // Take it out and put it back in

              removeEntry(tmpwin2);
              insertEntry(tmpwin2, tmpwin2->cwin);
              break;
            }
        }
    }
  while (!done);

  // Re-apply the button labels in the new order

  labelButtons();
}

/**
 * Handle ICONMGR events.
 *
 * @param msg.  The received NxWidget ICONMGR event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconMgr::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_ICONMGR_XYINPUT:     // Poll for button array events
        {
          // This event message is sent from CWindowEvent whenever mouse,
          // touchscreen, or keyboard entry events are received in the
          // Icon Manager application window that contains the button
          // array.

          NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

          // Poll for button array events.
          //
          // pollEvents() returns true if any interesting event in the
          // button array.  handleActionEvent() will be called in that
          // case.  false is not a failure.

          control->pollEvents();
        }
        break;

      case EVENT_ICONMGR_DEICONIFY:   // De-iconify or raise the Icon Manager
        {
          // Is the Icon manager iconified?

          if (m_window->isIconified())
            {
              // Yes.. De-iconify it

              if (!m_window->deIconify())
                {
                  twmerr("ERROR: Failed to de-iconify\n");
                  success = false;
                }
            }
          else
            {
              // No.. Just bring it to the top of the hierarchy

              if (!m_window->raiseWindow())
                {
                  twmerr("ERROR: Failed to raise window\n");
                  success = false;
                }
            }
        }
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Return the width of one button
 *
 * @return The width of one button
 */

nxgl_coord_t CIconMgr::getButtonWidth(void)
{
  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  // Fudge factors:  Width is 8 characters of the width of 'M' width plus 4

  return 8 * iconManagerFont->getCharWidth('M') + 4;
}

/**
 * Return the height of one row
 *
 * @return The height of one row
 */

nxgl_coord_t CIconMgr::getButtonHeight(void)
{
  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  // Fudge factors:  Width is the maximal font height plus 6 rows

  return iconManagerFont->getHeight() + 6;
}

/**
 * Create and initialize the icon manager window
 *
 * @param name  The prefix for this icon manager name
 */

bool CIconMgr::createIconManagerWindow(FAR const char *prefix)
{
  // Create the icon manager name using any prefix provided by the creator

  if (prefix != (FAR const char *)0)
    {
      m_name.setText(prefix);
      m_name.append(" Icon Manager");
    }
  else
    {
      m_name.setText("Icon Manager");
    }

  // Create the icon manager window.  Customizations:
  //
  // WFLAGS_NO_MENU_BUTTON:   There is no menu associated with the Icon
  //                          Manager
  // WFLAGS_NO_DELETE_BUTTON: The user cannot delete the Icon Manager window
  // WFLAGS_NO_RESIZE_BUTTON: The user cannot control the Icon Manager
  //                          window size
  // WFLAGS_ICONMGR:          Yes, this is the Icon Manager window
  // WFLAGS_HIDDEN:           The window is created in the hidden state

  CWindowFactory *factory = m_twm4nx->getWindowFactory();

  uint8_t wflags = (WFLAGS_NO_MENU_BUTTON | WFLAGS_NO_DELETE_BUTTON |
                    WFLAGS_NO_RESIZE_BUTTON | WFLAGS_ICONMGR |
                    WFLAGS_HIDDEN);

  m_window = factory->createWindow(m_name, &CONFIG_TWM4NX_ICONMGR_IMAGE,
                                   this, wflags);

  if (m_window == (FAR CWindow *)0)
    {
      twmerr("ERROR: Failed to create icon manager window");
      return false;
    }

  // Configure mouse events needed by the button array.

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_SYSTEM_NOP;
  events.resizeEvent = EVENT_SYSTEM_NOP;
  events.mouseEvent  = EVENT_ICONMGR_XYINPUT;
  events.kbdEvent    = EVENT_SYSTEM_NOP;
  events.closeEvent  = EVENT_SYSTEM_NOP;
  events.deleteEvent = EVENT_WINDOW_DELETE;

  bool success = m_window->configureEvents(events);
  if (!success)
    {
      delete m_window;
      m_window = (FAR CWindow *)0;
      return false;
    }

  // Get the height and width of the Icon manager window.  The width is
  // determined by the typical string length the maximum character width,
  // The height of one row is determined (mostly) by the maximum font
  // height

  struct nxgl_size_s windowSize;
  windowSize.w = m_nColumns * getButtonWidth();
  windowSize.h = getButtonHeight();

  // Get the Icon manager frame size (includes border and toolbar)

  struct nxgl_size_s frameSize;
  m_window->windowToFrameSize(&windowSize, &frameSize);

  // Position the icon manager at the upper right initially

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  struct nxgl_point_s framePos;
  framePos.x = displaySize.w - frameSize.w - 1;
  framePos.y = 0;

  // Set the new window size and position

  if (!m_window->resizeFrame(&frameSize, &framePos))
    {
      twmerr("ERROR: Failed to set window size/position\n");
      delete m_window;
      m_window = (FAR CWindow *)0;
      return false;
    }

  // Now show the window in all its glory

  m_window->showWindow();
  m_window->synchronize();
  return true;
}

/**
 * Create the button array widget
 */

bool CIconMgr::createButtonArray(void)
{
  // Get the width of the window

  struct nxgl_size_s windowSize;
  if (!m_window->getWindowSize(&windowSize))
    {
      twmerr("ERROR: Failed to get window size\n");
      return false;
    }

  // Create the button array
  // REVISIT:  Hmm.. Button array cannot be dynamically resized!

  uint8_t nrows = m_nrows > 0 ? m_nrows : 1;

  nxgl_coord_t buttonWidth  = windowSize.w / m_nColumns;
  nxgl_coord_t buttonHeight = windowSize.h / nrows;

  // Get the Widget control instance from the Icon Manager window.  This
  // will force all widget drawing to go to the Icon Manager window.

  FAR NXWidgets:: CWidgetControl *control = m_window->getWidgetControl();
  if (control == (FAR NXWidgets:: CWidgetControl *)0)
    {
      // Should not fail

      return false;
    }

  // Now we have enough information to create the button array
  // The button must be positioned at the upper left of the window

  m_buttons = new NXWidgets::CButtonArray(control, 0, 0,
                                          m_nColumns, nrows,
                                          buttonWidth, buttonHeight);
  if (m_buttons == (FAR NXWidgets::CButtonArray *)0)
    {
      twmerr("ERROR: Failed to create the button array\n");
      return false;
    }

  // Configure the button array widget

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconManagerFont = fonts->getIconManagerFont();

  m_buttons->setFont(iconManagerFont);
  m_buttons->setBorderless(true);
  m_buttons->setRaisesEvents(true);

  // Draw the button array

  m_buttons->enableDrawing();
  m_buttons->redraw();

  // Register to get events from the mouse clicks on the image

  m_buttons->addWidgetEventHandler(this);
  return true;
}

/**
 * Label each button with the window name
 */

void CIconMgr::labelButtons(void)
{
  FAR struct SWindowEntry *swin = m_head;
  uint8_t nrows = (m_nrows == 0) ? 1 : m_nrows;

  for (int rowndx = 0; rowndx < nrows; rowndx++)
    {
      for (int colndx = 0; colndx < m_nColumns; colndx++)
        {
          // Check if we should just clear any buttons on the right

          if (swin != (FAR struct SWindowEntry *)0)
            {
              // Get the window name as an NWidgets::CNxString

              NXWidgets::CNxString string = swin->cwin->getWindowName();

              // Apply the window name to the button

              m_buttons->setText(colndx, rowndx, string);

              // Assign this button to the window

              swin->row    = rowndx;
              swin->column = colndx;

              // Advance to the next window

              swin = swin->flink;
            }
          else
            {
              // Clear the button text on the right.

              m_buttons->setText(colndx, rowndx, "");
            }
        }
    }
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

  // Handle the case where the the window list is empty

  if (m_head == NULL)
    {
      m_head        = wentry;
      wentry->blink = NULL;
      m_tail        = wentry;
      return;
    }

  // The list is not empty.  Search for the location in name-order where we
  // should insert this new entry

  for (tmpwin = m_head;
       tmpwin != (FAR struct SWindowEntry *)0;
       tmpwin = tmpwin->flink)
    {
      // Insert the new window mid-list, in name order

      NXWidgets::CNxString windowName = cwin->getWindowName();
      if (windowName.compareTo( tmpwin->cwin->getWindowName()) > 0)
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

          return;
        }
    }

  // Insert the new entry at the tail of the list

  m_tail->flink = wentry;
  wentry->blink = m_tail;
  m_tail        = wentry;
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
 * Find an entry in the icon manager
 *
 *  @param cwin The window to find
 *  @return The incon manager entry (unless an error occurred)
 */

FAR struct SWindowEntry *CIconMgr::findEntry(FAR CWindow *cwin)
{
  // Check each entry

  FAR struct SWindowEntry *wentry;

  for (wentry = m_head;
       wentry != (FAR struct SWindowEntry *)0;
       wentry = wentry->flink)
    {
      // Does this entry carry the window we are looking for?

      if (wentry->cwin == cwin)
        {
          // Yes.. return the reference to this entry

          return wentry;
        }
    }

  // No matching entry found

  return wentry;
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
      // Now find the window assigned to this row x column

      for (FAR struct SWindowEntry *swin = m_head;
           swin != (FAR struct SWindowEntry *)0;
           swin = swin->flink)
        {
          // Compare row and column

          if (row == swin->row && column == swin->column)
            {
              // Got it... send an event message

              struct SEventMsg msg;
              msg.obj     = swin->cwin;
              msg.pos.x   = e.getX();
              msg.pos.y   = e.getY();
              msg.context = EVENT_CONTEXT_ICONMGR;
              msg.handler = (FAR void *)0;

              // Is the window Iconified?

              if (swin->cwin->isIconified())
                {
                  // Yes, de-Iconify it

                  msg.eventID = EVENT_WINDOW_DEICONIFY;
                }
              else
                {
                  // Otherwise, raise the window to the top of the hierarchy

                  msg.eventID = EVENT_WINDOW_RAISE;
                }

              // NOTE that we cannot block because we are on the same thread
              // as the message reader.  If the event queue becomes full
              // then we have no other option but to lose events.
              //
              // I suppose we could recurse raise() or de-Iconifiy directly
              // here at the risk of runaway stack usage (we are already deep
              // in the stack here).

              int ret = mq_send(m_eventq, (FAR const char *)&msg,
                                sizeof(struct SEventMsg), 100);
              if (ret < 0)
                {
                  twmerr("ERROR: mq_send failed: %d\n", errno);
                }

              break;
            }
        }

      twmwarn("WARNING:  No matching window name\n");
    }
}
