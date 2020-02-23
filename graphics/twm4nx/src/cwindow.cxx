/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cwindow.cxx
// Represents one window instance
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

#include <cfcntl>
#include <cstring>
#include <cassert>
#include <cerrno>

#include <mqueue.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxbe.h>

#include "graphics/nxglyphs.hxx"

#include "graphics/nxwidgets/cnxtkwindow.hxx"
#include "graphics/nxwidgets/cnxtoolbar.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxwidgets/cscaledbitmap.hxx"
#include "graphics/nxwidgets/cimage.hxx"
#include "graphics/nxwidgets/clabel.hxx"
#include "graphics/nxwidgets/cnxfont.hxx"
#include "graphics/nxwidgets/singletons.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cresize.hxx"
#include "graphics/twm4nx/cbackground.hxx"
#include "graphics/twm4nx/ciconwidget.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/twm4nx_cursor.hxx"

/////////////////////////////////////////////////////////////////////////////
// Private Types
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

struct SToolbarInfo
{
  FAR const NXWidgets::SRlePaletteBitmap *bitmap; /**< Bitmap configured for button */
  bool rightSide;                        /**< True: Button packed on the right */
  uint16_t event;                        /**< Event when button released */
};

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

// This array provides a static description of the toolbar buttons

struct SToolbarInfo GToolBarInfo[NTOOLBAR_BUTTONS] =
{
  [MENU_BUTTON] =
  {
    &CONFIG_TWM4NX_MENU_IMAGE, false, EVENT_TOOLBAR_MENU
  },
  [DELETE_BUTTON] =
  {
    &CONFIG_TWM4NX_TERMINATE_IMAGE, true, EVENT_TOOLBAR_TERMINATE
  },
  [RESIZE_BUTTON] =
  {
    &CONFIG_TWM4NX_RESIZE_IMAGE, true, EVENT_RESIZE_BUTTON
  },
  [MINIMIZE_BUTTON] =
  {
    &CONFIG_TWM4NX_MINIMIZE_IMAGE, true, EVENT_TOOLBAR_MINIMIZE
  }
};

/////////////////////////////////////////////////////////////////////////////
// CWindow Implementation
/////////////////////////////////////////////////////////////////////////////

/**
 * CWindow Constructor
 *
 * @param twm4nx.  Twm4Nx session
 */

CWindow::CWindow(CTwm4Nx *twm4nx)
{
  m_twm4nx                = twm4nx;       // Save the Twm4Nx session
  m_eventq                = (mqd_t)-1;    // No widget message queue yet

  // Windows

  m_nxWin                 = (FAR NXWidgets::CNxTkWindow *)0;
  m_toolbar               = (FAR NXWidgets::CNxToolbar *)0;
  m_windowEvent           = (FAR CWindowEvent *)0;
  m_minWidth              = 1;
  m_modal                 = false;

  // Events

  m_appEvents.eventObj    = (FAR void *)0;
  m_appEvents.redrawEvent = EVENT_SYSTEM_NOP; // Redraw event ID
  m_appEvents.mouseEvent  = EVENT_SYSTEM_NOP; // Mouse/touchscreen event ID
  m_appEvents.kbdEvent    = EVENT_SYSTEM_NOP; // Keyboard event ID
  m_appEvents.closeEvent  = EVENT_SYSTEM_NOP; // Window close event ID
  m_appEvents.deleteEvent = EVENT_SYSTEM_NOP; // Window delete event ID

  // Toolbar

  m_tbTitle               = (FAR NXWidgets::CLabel *)0;
  m_tbHeight              = 0;             // Height of the toolbar
  m_tbLeftX               = 0;             // Offset to end of left buttons
  m_tbRightX              = 0;             // Offset to start of right buttons
  m_tbFlags               = 0;             // No customizations
  m_tbDisables            = 0;             // No buttons disabled

  // Style for the toolbar widgets.  It is the same as the default
  // widget style, but using the color assigned to the toolbar background.

  m_tbStyle               = *NXWidgets::g_defaultWidgetStyle;
  m_tbStyle.colors.background         = CONFIG_TWM4NX_DEFAULT_TOOLBARCOLOR;
  m_tbStyle.colors.selectedBackground = CONFIG_TWM4NX_DEFAULT_TOOLBARCOLOR;

  // Icons/Icon Manager

  m_iconBitMap            = (FAR NXWidgets::CRlePaletteBitmap *)0;
  m_iconWidget            = (FAR CIconWidget *)0;
  m_iconMgr               = (FAR CIconMgr *)0;
  m_iconified             = false;

  // Dragging

  m_clicked               = false;
  m_dragging              = false;
  m_dragPos.x             = 0;
  m_dragPos.y             = 0;
  m_dragCSize.w           = 0;
  m_dragCSize.h           = 0;

  // Toolbar buttons

  std::memset(m_tbButtons, 0, NTOOLBAR_BUTTONS * sizeof(NXWidgets::CImage *));
}

/**
 * CWindow Destructor
 */

CWindow::~CWindow(void)
{
  cleanup();
}

/**
 * CWindow Initializer (unlike the constructor, this may fail)
 *
 * The window is initialized with all application events disabled.
 * The CWindows::configureEvents() method may be called as a second
 * initialization step in order to enable application events.
 *
 * @param name      The the name of the window (and its icon)
 * @param pos       The initial position of the window
 * @param size      The initial size of the window
 * @param sbitmap   The Icon bitmap image.  null if no icon.
 * @param iconMgr   Pointer to icon manager instance.  To support
 *                  multiple Icon Managers.
 * @param flags     Toolbar customizations see WFLAGS_NO_* definition
 * @return True if the window was successfully initialize; false on
 *   any failure,
 */

bool CWindow::initialize(FAR const NXWidgets::CNxString &name,
                         FAR const struct nxgl_point_s *pos,
                         FAR const struct nxgl_size_s *size,
                         FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                         FAR CIconMgr *iconMgr,  uint8_t flags)
{
  // Open a message queue to send fully digested NxWidget events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();

  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             mqname, errno);
      return false;
    }

  // If no Icon Manager was provided, we will use the standard Icon Manager

  if (iconMgr == (FAR CIconMgr *)0)
    {
      m_iconMgr = m_twm4nx->getIconMgr();
    }
  else
    {
      m_iconMgr = iconMgr;
    }

  if (name.getLength() == 0)
    {
      m_name.setText(GNoName);
    }
  else
    {
      m_name.setText(name);
    }

  // Get the minimum window size.  We need this minimum later for resizing.
  // If there is no toolbar, leave the minimum at one pixel as it was set by
  // the constructor.

  if (WFLAGS_HAVE_TOOLBAR(flags))
    {
      m_minWidth = minimumToolbarWidth(m_twm4nx, m_name, flags);
    }

  // Do initial clip to the maximum window size

  struct nxgl_size_s maxWindow;
  m_twm4nx->maxWindowSize(&maxWindow);

  struct nxgl_size_s winsize;
  winsize.w     = size->w;
  if (winsize.w > maxWindow.w)
    {
      winsize.w = maxWindow.w;
    }

  winsize.h     = size->h;
  if (winsize.h > maxWindow.h)
    {
      winsize.h = maxWindow.h;
    }

  // Create the window

  m_nxWin   = (FAR NXWidgets::CNxTkWindow *)0;
  m_toolbar = (FAR NXWidgets::CNxToolbar *)0;

  // Create the main window

  if (!createMainWindow(&winsize, pos, flags))
    {
      twmerr("ERROR: createMainWindow() failed\n");
      cleanup();
      return false;
    }

  m_tbHeight = 0;
  m_tbFlags  = flags;
  m_toolbar  = (FAR NXWidgets::CNxToolbar *)0;

  if (WFLAGS_HAVE_TOOLBAR(flags))
    {
      // Toolbar height should be based on size of images and fonts.

      if (!getToolbarHeight(name))
        {
          twmerr("ERROR: getToolbarHeight() failed\n");
          cleanup();
          return false;
        }

      // Create the toolbar

      if (!createToolbar())
        {
          twmerr("ERROR: createToolbar() failed\n");
          cleanup();
          return false;
        }

      // Add buttons to the toolbar

      if (!createToolbarButtons(flags))
        {
          twmerr("ERROR: createToolbarButtons() failed\n");
          cleanup();
          return false;
        }

      // Add the title to the toolbar

      if (!createToolbarTitle(name))
        {
          twmerr("ERROR: createToolbarTitle() failed\n");
          cleanup();
          return false;
        }
    }

  // Create and initialize the icon widget if a bitmap was provided

  if (sbitmap != (FAR const struct NXWidgets::SRlePaletteBitmap *)0)
    {
      // Create the icon image instance

      m_iconBitMap = new NXWidgets::CRlePaletteBitmap(sbitmap);
      if (m_iconBitMap == (NXWidgets::CRlePaletteBitmap *)0)
        {
          twmerr("ERROR: Failed to create icon image\n");
          cleanup();
          return false;
        }

      // Create a style for the Icon widget.  It is the same as the default
      // widget style, but using the color assigned to the background as the
      // widget background color.

      NXWidgets::CWidgetStyle style   = *NXWidgets::g_defaultWidgetStyle;
      style.colors.background         = CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR;
      style.colors.selectedBackground = CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR;

      // Get the widget control instance from the background.  This is needed
      // to force the icon widgets to be drawn on the background

      FAR CBackground *background = m_twm4nx->getBackground();
      FAR NXWidgets::CWidgetControl *control = background->getWidgetControl();

      m_iconWidget = new CIconWidget(m_twm4nx, control, pos->x, pos->y, &style);
      if (m_iconWidget == (FAR CIconWidget *)0)
        {
          twmerr("ERROR: Failed to create the icon widget\n");
          cleanup();
          return false;
        }

      if (!m_iconWidget->initialize(this, m_iconBitMap, m_name))
        {
          twmerr("ERROR: Failed to initialize the icon widget\n");
          cleanup();
          return false;
        }

      // Initialize the icon widget

      m_iconWidget->disable();
      m_iconWidget->disableDrawing();
      m_iconWidget->setRaisesEvents(true);
    }

  // Re-enable toolbar widgets

  enableToolbarWidgets();
  return true;
}

/**
 * Configure application window events.
 *
 * @param events Describes the application event configuration
 * @return True is returned on success
 */

bool CWindow::configureEvents(FAR const struct SAppEvents &events)
{
  m_appEvents.eventObj     = events.eventObj;    // Event object
  m_appEvents.redrawEvent  = events.redrawEvent; // Redraw event ID
  m_appEvents.resizeEvent  = events.resizeEvent; // Resize event ID
  m_appEvents.mouseEvent   = events.mouseEvent;  // Mouse/touchscreen event ID
  m_appEvents.kbdEvent     = events.kbdEvent;    // Keyboard event ID
  m_appEvents.closeEvent   = events.closeEvent;  // Window close event ID
  m_appEvents.deleteEvent  = events.deleteEvent; // Window delete event ID

  return m_windowEvent->configureEvents(events);
}

/**
 * Get the raw window size (including toolbar and frame)
 *
 * @param framesize Location to return the window frame size
 */

bool CWindow::getFrameSize(FAR struct nxgl_size_s *framesize)
{
  // Get the window size

  struct nxgl_size_s winsize;
  bool success = getWindowSize(&winsize);
  if (success)
    {
      // Convert the window size to the frame size

      windowToFrameSize(&winsize, framesize);
    }

  return success;
}

/**
 * Update the window frame after a resize operation (includes the toolbar
 * and user window)
 *
 * @param frameSize The new window frame size
 * @param framePos  The frame location which may also have changed
 */

bool CWindow::resizeFrame(FAR const struct nxgl_size_s *frameSize,
                          FAR const struct nxgl_point_s *framePos)
{
  // Account for toolbar and border

  struct nxgl_size_s delta;
  delta.w = 2 * CONFIG_NXTK_BORDERWIDTH;
  delta.h = m_tbHeight + 2 * CONFIG_NXTK_BORDERWIDTH;

  // Don't set the window size smaller than the minimum window size

  struct nxgl_size_s winsize;
  if (frameSize->w <= m_minWidth + delta.w)
    {
      winsize.w = m_minWidth;
    }
  else
    {
      winsize.w = frameSize->w - delta.w;
    }

  if (frameSize->h <= delta.h)
    {
      winsize.h = 1;
    }
  else
    {
      winsize.h = frameSize->h - delta.h;
    }

  // Set the usable window size

  bool success = m_nxWin->setSize(&winsize);
  if (!success)
    {
      twmerr("ERROR: Failed to setSize()\n");
      return false;
    }

  if (framePos != (FAR const struct nxgl_point_s *)0)
    {
      // Set the new frame position (in case it changed too)

      success = setFramePosition(framePos);
      if (!success)
        {
          twmerr("ERROR: Failed to setFramePosition()\n");
          return false;
        }
    }

  // Synchronize with the NX server to make sure that the new geometry is
  // truly in effect.

  m_nxWin->synchronize();

  // Then update the toolbar layout (if there is one)

  success = updateToolbarLayout();
  if (!success)
    {
      twmerr("ERROR: updateToolbarLayout() failed\n");
      return false;
    }

  // Check if the application using this window is interested in resize
  // events

  if (m_appEvents.resizeEvent != EVENT_SYSTEM_NOP)
    {
      twminfo("Close event...\n");

      // Send the application specific [pre-]close event

      struct SEventMsg outmsg;
      outmsg.eventID  = m_appEvents.resizeEvent;
      outmsg.obj      = (FAR void *)this;
      outmsg.pos.x    = 0;
      outmsg.pos.y    = 0;
      outmsg.context  = EVENT_CONTEXT_WINDOW;
      outmsg.handler  = m_appEvents.eventObj;

      int ret = mq_send(m_eventq, (FAR const char *)&outmsg,
                        sizeof(struct SEventMsg), 100);
      if (ret < 0)
        {
          twmerr("ERROR: mq_send failed: %d\n", errno);
          return false;
        }
   }

  return true;
}

/**
 * Get the window frame position (accounting for toolbar and frame)
 *
 * @param size Location to return the window frame position
 */

bool CWindow::getFramePosition(FAR struct nxgl_point_s *framepos)
{
  // Get the window position

  struct nxgl_point_s winpos;
  bool success = m_nxWin->getPosition(&winpos);
  if (success)
    {
      // Convert the window position to a frame position

      windowToFramePos(&winpos, framepos);
    }

  return success;
}

/**
 * Set the window frame position (accounting for toolbar and frame)
 *
 * @param size The new raw window position
 */

bool CWindow::setFramePosition(FAR const struct nxgl_point_s *framepos)
{
  // Convert the frame position to the contained, primary window positio

  struct nxgl_point_s winpos;
  frameToWindowPos(framepos, &winpos);

  // And set the window position

  return m_nxWin->setPosition(&winpos);
}

/**
 * Minimize (iconify) the window
 *
 * @return True if the operation was successful
 */

bool CWindow::iconify(void)
{
  if (!isIconified())
    {
     // Make sure to exit any modal state before minimizing

      m_modal = false;
      m_nxWin->modal(false);

      // Hide the main window

      m_nxWin->hide();

      // Menu windows don't have an icon

      if (hasIcon())
        {
          // Enable the widget

          m_iconWidget->enable();

          // Pick a position for icon

          struct nxgl_point_s iconPos;
          m_iconWidget->getPos(iconPos);

          FAR CWindowFactory *factory = m_twm4nx->getWindowFactory();
          if (factory->placeIcon(this, iconPos, iconPos))
            {
              m_iconWidget->moveTo(iconPos.x, iconPos.y);
            }

          // Redraw the icon widget

          m_iconWidget->enableDrawing();
          m_iconWidget->redraw();
        }

      m_iconified = true;
      m_nxWin->synchronize();
    }

  return true;
}

/**
 * De-iconify the window
 *
 * @return True if the operation was successful
 */

bool CWindow::deIconify(void)
{
  // De-iconify the window

  if (isIconified())
    {
      // Raise and the main window

      m_iconified = false;
      m_nxWin->show();

      if (hasIcon())
        {
          // Disable the icon widget

          m_iconWidget->disableDrawing();
          m_iconWidget->disable();

          // Redraw the background window in the rectangle previously
          // occupied by the widget.

          struct nxgl_size_s size;
          m_iconWidget->getSize(size);

          struct nxgl_rect_s rect;
          m_iconWidget->getPos(rect.pt1);

          rect.pt2.x = rect.pt1.x + size.w - 1;
          rect.pt2.y = rect.pt1.y + size.h - 1;

          FAR CBackground *backgd = m_twm4nx->getBackground();
          if (!backgd->redrawBackgroundWindow(&rect, false))
            {
              twmerr("ERROR: redrawBackgroundWindow() failed\n");
            }
        }

      // Make sure everything is in sync

      m_nxWin->synchronize();
    }

  return true;
}

/**
 * Handle WINDOW events.
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CWindow::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_WINDOW_RAISE:     // Raise window to the top of the hierarchy
        m_nxWin->raise();          // Could be the main or the icon window
        break;

      case EVENT_WINDOW_LOWER:     // Lower window to the bottom of the hierarchy
        m_nxWin->lower();          // Could be the main or the icon window
        break;

      case EVENT_WINDOW_DEICONIFY: // De-iconify and raise the main window
        {
          success = deIconify();
        }
        break;

      case EVENT_TOOLBAR_MENU:       // Toolbar menu button released
        {
          // REVISIT:  Not yet implemented (but don't raise an error)
        }
        break;

      case EVENT_TOOLBAR_MINIMIZE:   // Toolbar minimize button released
        {
          // Minimize (iconify) the window

          success = iconify();
        }
        break;

      case EVENT_TOOLBAR_TERMINATE:  // Toolbar terminate button pressed
        if (isIconMgr())
          {
            // Don't terminate the Icon manager, just hide it

            m_iconMgr->hide();
          }
        else
          {
            // Inform the application that the window is disappearing

            if (m_appEvents.closeEvent != EVENT_SYSTEM_NOP)
              {
                twminfo("Close event...\n");

                // Send the application specific [pre-]close event

                struct SEventMsg outmsg;
                outmsg.eventID  = m_appEvents.closeEvent;
                outmsg.obj      = (FAR void *)this;
                outmsg.pos.x    = eventmsg->pos.x;
                outmsg.pos.y    = eventmsg->pos.y;
                outmsg.context  = eventmsg->context;
                outmsg.handler  = m_appEvents.eventObj;

                int ret = mq_send(m_eventq, (FAR const char *)&outmsg,
                                  sizeof(struct SEventMsg), 100);
                if (ret < 0)
                  {
                    twmerr("ERROR: mq_send failed: %d\n", errno);
                  }
             }

            // Close the window... but not yet.  Send the blocked message.
            // The actual termination will no occur until the NX server
            // drains all of the message events.  We will get the
            // EVENT_WINDOW_DELETE event at that point

            NXWidgets::CWidgetControl *control = m_nxWin->getWidgetControl();
            nxtk_block(control->getWindowHandle(), (FAR void *)m_nxWin);
          }

        break;

      case EVENT_WINDOW_DELETE:  // Toolbar terminate button pressed
       {
          // Poll for pending events before closing.

          FAR CWindow *cwin = (FAR CWindow *)eventmsg->obj;
          success = cwin->pollToolbarEvents();

          FAR CWindowFactory *factory = m_twm4nx->getWindowFactory();
          factory->destroyWindow(cwin);
        }
        break;

      case EVENT_TOOLBAR_GRAB:   /* Left click on title widget.  Start drag */
        success = toolbarGrab(eventmsg);
        break;

      case EVENT_WINDOW_DRAG:   /* Mouse movement while clicked */
        success = windowDrag(eventmsg);
        break;

      case EVENT_TOOLBAR_UNGRAB: /* Left click release while dragging. */
        success = toolbarUngrab(eventmsg);
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Create the main window
 *
 * Initially, the application window will generate no window-related events
 * (redraw, mouse/touchscreen, keyboard input, etc.).  After creating the
 * window, the user may call the configureEvents() method to select the
 * eventIDs of the events to be generated.
 *
 * @param winsize   The initial window size
 * @param winpos    The initial window position
 * @param flags Toolbar customizations see WFLAGS_NO_* definitions
 */

bool CWindow::createMainWindow(FAR const nxgl_size_s *winsize,
                               FAR const nxgl_point_s *winpos,
                               uint8_t flags)
{
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.
  //    Setup the the CWindowEvent instance to use our inherited drag event
  //    handler

  m_windowEvent = new CWindowEvent(m_twm4nx, (FAR void *)this, m_appEvents);
  m_windowEvent->installEventTap(this, (uintptr_t)1);

  // 4. Create the window.  Handling provided flags. NOTE: that menu windows
  //    are always created hidden and in the iconified state (although they
  //    have no icons)

  uint8_t cflags = NXBE_WINDOW_RAMBACKED;
  if (WFLAGS_IS_HIDDEN(flags) | WFLAGS_IS_MENU(flags))
    {
      cflags |= NXBE_WINDOW_HIDDEN;
    }

  m_nxWin = m_twm4nx->createFramedWindow(m_windowEvent, cflags);
  if (m_nxWin == (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete m_windowEvent;
      m_windowEvent = (FAR CWindowEvent *)0;
      return false;
    }

  // 5. Open and initialize the window

  bool success = m_nxWin->open();
  if (!success)
    {
      return false;
    }

  // 6. Set the initial window size

  if (!m_nxWin->setSize(winsize))
    {
      return false;
    }

  // 7. Set the initial window position

  if (!m_nxWin->setPosition(winpos))
    {
      return false;
    }

  //  Menu windows are always created hidden and in the iconified state
  // (although they have no icons)

  m_iconified = WFLAGS_IS_MENU(flags);
  return true;
}

/**
 * Calculate the height of the tool bar
 */

bool CWindow::getToolbarHeight(FAR const NXWidgets::CNxString &name)
{
  // The tool bar height is the largest of the toolbar button heights or the
  // title text font

  // Check if there is a title.  If so, get the font height.

  m_tbHeight = 0;
  if (name.getLength() != 0)
    {
      FAR CFonts *fonts = m_twm4nx->getFonts();
      FAR NXWidgets::CNxFont *titleFont = fonts->getTitleFont();
      m_tbHeight = titleFont->getHeight();
    }

  // Now compare this to the height of each toolbar image

  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      nxgl_coord_t btnHeight = GToolBarInfo[btindex].bitmap->height;
      if (btnHeight > m_tbHeight)
        {
          m_tbHeight = btnHeight;
        }
    }

  // Plus some lines for good separation

  m_tbHeight += CONFIG_TWM4NX_TOOLBAR_VSPACING;
  return true;
}

/**
 * Create the tool bar
 */

bool CWindow::createToolbar(void)
{
  // Create the toolbar
  // 1. Create the style, using the selected colors (REVISIT)

  // 2. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_SYSTEM_NOP;
  events.resizeEvent = EVENT_SYSTEM_NOP;
  events.mouseEvent  = EVENT_TOOLBAR_XYINPUT;
  events.kbdEvent    = EVENT_SYSTEM_NOP;
  events.closeEvent  = EVENT_SYSTEM_NOP;
  events.deleteEvent = EVENT_WINDOW_DELETE;

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx, (FAR void *)this,
                                               events);
  control->installEventTap(this, (uintptr_t)0);

  // 3. Get the toolbar sub-window from the framed window

  m_toolbar = m_nxWin->openToolbar(m_tbHeight, control);
  if (m_toolbar == (FAR NXWidgets::CNxToolbar *)0)
    {
      delete control;
      return false;
    }

  // 4. Open and initialize the tool bar

  if (!m_toolbar->open())
    {
      delete m_toolbar;
      m_toolbar = (FAR NXWidgets::CNxToolbar *)0;
      return false;
    }

  // 5. Fill the entire tool bar with the background color from the
  //    current widget style.

  if (!fillToolbar())
    {
      delete m_toolbar;
      m_toolbar = (FAR NXWidgets::CNxToolbar *)0;
      return false;
    }

  return true;
}

/**
 * Fill the toolbar background color
 */

bool CWindow::fillToolbar(void)
{
  // Get the graphics port for drawing on the toolbar

  FAR NXWidgets::CWidgetControl *control = m_toolbar->getWidgetControl();
  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the window

  struct nxgl_size_s windowSize;
  if (!m_toolbar->getSize(&windowSize))
    {
      twmerr("ERROR: Failed to get the size of the toolbar\n");
      return false;
    }

  // Fill the toolbar with the background color of the current widget style
  // (which is always the default widget style for now).

  port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                       NXWidgets::g_defaultWidgetStyle->colors.background);
  return true;
}

/**
 * Update the toolbar layout, resizing the title text window and
 * repositioning all windows on the toolbar.
 */

bool CWindow::updateToolbarLayout(void)
{
  // Disable toolbar widget drawing and events while we do this

  disableToolbarWidgets();

  // Reposition all right buttons.  Change the width of the
  // toolbar does not effect the left side spacing.

  struct nxgl_size_s winsize;
  if (!getWindowSize(&winsize))
    {
      twmerr("ERROR: Failed to get window size\n");
      return false;
    }

  // Set up the toolbar horizontal spacing

  m_tbRightX = winsize.w;

  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      if (m_tbButtons[btindex] != (FAR NXWidgets::CImage *)0 &&
          GToolBarInfo[btindex].rightSide)
        {
          FAR NXWidgets::CImage *cimage = m_tbButtons[btindex];

          // Set the position of the Icon image in the toolbar

          struct nxgl_size_s windowSize;
          getWindowSize(&windowSize);

          // Center image vertically

          struct nxgl_point_s pos;
          pos.y = (m_tbHeight - cimage->getHeight()) / 2;

          // Pack on the right horizontally

          m_tbRightX -= (cimage->getWidth() + CONFIG_TWM4NX_TOOLBAR_HSPACING);
          pos.x       = m_tbRightX;

         if (!cimage->moveTo(pos.x, pos.y))
           {
             twmerr("ERROR: Failed to move button image\n");
             return false;
           }
        }
    }

  // Vertical size of the title window is selected to fill the entire
  // toolbar.  This really needs to be only the font height.  However,
  // this improves the click-ability of the widget for small fonts.
  //
  // The Horizontal size of the title widget is determined by the available
  // space between m_tbLeftX and m_tbRightX.

  struct nxgl_size_s titleSize;
  titleSize.h = m_tbHeight;
  titleSize.w = m_tbRightX - m_tbLeftX - CONFIG_TWM4NX_TOOLBAR_HSPACING + 1;

  if (!m_tbTitle->resize(titleSize.w, titleSize.h))
    {
      twmerr("ERROR: Failed to resize title\n");
      return false;
    }

  // Fill the entire tool bar with the background color from the current
  // widget style.

  if (!fillToolbar())
    {
      twmerr("ERROR: Failed to fill the toolbar\n");
      return false;
    }

  // Enable and re-draw all of the toolbar widgets

  enableToolbarWidgets();
  return true;
}

/**
 * Disable toolbar widget drawing and widget events.
 */

bool CWindow::disableToolbarWidgets(void)
{
  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      FAR NXWidgets::CImage *cimage = m_tbButtons[btindex];
      if (cimage != (FAR NXWidgets::CImage *)0)
        {
          cimage->disableDrawing();
          cimage->setRaisesEvents(false);
        }
    }

  m_tbTitle->disableDrawing();
  m_tbTitle->setRaisesEvents(false);
  return true;
}

/**
 * Enable and redraw toolbar widget drawing and widget events.
 */

bool CWindow::enableToolbarWidgets(void)
{
  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      FAR NXWidgets::CImage *cimage = m_tbButtons[btindex];
      if (cimage != (FAR NXWidgets::CImage *)0)
        {
          cimage->enableDrawing();
          cimage->setRaisesEvents(true);
          cimage->redraw();
        }
    }

  m_tbTitle->enableDrawing();
  m_tbTitle->setRaisesEvents(true);
  m_tbTitle->redraw();
  return true;
}

/**
 * Create all toolbar buttons
 *
 * @param flags Toolbar customizations see WFLAGS_NO_* definitions
 * @return True if the window was successfully initialize; false on
 *   any failure,
 */

bool CWindow::createToolbarButtons(uint8_t flags)
{
  struct nxgl_size_s winsize;
  if (!getWindowSize(&winsize))
    {
      return false;
    }

  // Create the title bar windows
  // Set up the toolbar horizontal spacing

  m_tbRightX = winsize.w;
  m_tbLeftX  = 0;

  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      // Check if this button is omitted by toolbar customizations

      if ((m_tbFlags & (1 << btindex)) != 0)
        {
          // Omitted.. skip to the next button

          continue;
        }

      // Create the bitmap instance

      FAR const NXWidgets::SRlePaletteBitmap *sbitmap =
        GToolBarInfo[btindex].bitmap;

#ifdef CONFIG_TWM4NX_TOOLBAR_ICONSCALE
      // Create a CScaledBitmap to scale the bitmap icon

      struct nxgl_size_s iconSize;
      iconSize.w = CONFIG_TWM4NX_TOOLBAR_ICONWIDTH;
      iconSize.h = CONFIG_TWM4NX_TOOLBAR_ICONHEIGHT;

      FAR NXWidgets::CScaledBitmap *scaler =
        new NXWidgets::CScaledBitmap(sbitmap, iconSize);

      if (scaler == (FAR NXWidgets::CScaledBitmap *)0)
        {
          twmerr("ERROR: Failed to created scaled bitmap\n");
          return false;
        }
#endif

      // Create the image.  The image will serve as button since it
      // can detect clicks and release just like a button.

      m_tbButtons[btindex] = (FAR NXWidgets::CImage *)0;
      nxgl_coord_t w = 1;
      nxgl_coord_t h = 1;

      // Get the toolbar CWdigetControl instance.  This will force all
      // widget drawing to go to the toolbar.

      NXWidgets::CWidgetControl *control = m_toolbar->getWidgetControl();

#ifdef CONFIG_TWM4NX_TOOLBAR_ICONSCALE
      w = scaler->getWidth();
      h = scaler->getHeight();

      m_tbButtons[btindex] =
        new NXWidgets::CImage(control, 0, 0, w, h, scaler, &m_tbStyle);
      if (m_tbButtons[btindex] == (FAR NXWidgets::CImage *)0)
        {
          twmerr("ERROR: Failed to create image\n");
          delete scalar;
          return false;
        }
#else
      FAR NXWidgets::CRlePaletteBitmap *cbitmap =
        new NXWidgets::CRlePaletteBitmap(sbitmap);
      if (cbitmap == (FAR NXWidgets::CRlePaletteBitmap *)0)
        {
          twmerr("ERROR: Failed to create CrlPaletteBitmap\n");
          return false;
        }

      w = cbitmap->getWidth();
      h = cbitmap->getHeight();

      m_tbButtons[btindex] =
        new NXWidgets::CImage(control, 0, 0, w, h, cbitmap, &m_tbStyle);
      if (m_tbButtons[btindex] == (FAR NXWidgets::CImage *)0)
        {
          twmerr("ERROR: Failed to create image\n");
          delete cbitmap;
          return false;
        }
#endif

      // Configure the image, disabling drawing for now

      FAR NXWidgets::CImage *cimage = m_tbButtons[btindex];
      cimage->setBorderless(true);
      cimage->disableDrawing();
      cimage->setRaisesEvents(false);

      // Register to get events from the mouse clicks on the image

      cimage->addWidgetEventHandler(this);

      // Set the position of the Icon image in the toolbar

      struct nxgl_size_s windowSize;
      getWindowSize(&windowSize);

      // Center image vertically

      struct nxgl_point_s pos;
      pos.y = (m_tbHeight - cimage->getHeight()) / 2;

      // Pack on the left or right horizontally

      if (GToolBarInfo[btindex].rightSide)
        {
          m_tbRightX -= (cimage->getWidth() + CONFIG_TWM4NX_TOOLBAR_HSPACING);
          pos.x       = m_tbRightX;
        }
      else
        {
          m_tbLeftX  += CONFIG_TWM4NX_TOOLBAR_HSPACING;
          pos.x       = m_tbLeftX;
          m_tbLeftX  += cimage->getWidth();
        }

      if (!cimage->moveTo(pos.x, pos.y))
        {
          delete m_tbButtons[btindex];
          m_tbButtons[btindex] = (FAR NXWidgets::CImage *)0;
          return false;
        }
    }

  return true;
}

/**
 * Add buttons and title widgets to the tool bar
 *
 * @param name The name to use for the toolbar title
 */

bool CWindow::createToolbarTitle(FAR const NXWidgets::CNxString &name)
{
  // Is there a title?

  if (name.getLength() == 0)
    {
      // No.. then there is nothing to be done here

      return true;
    }

  // Vertical size of the title window is selected to fill the entire
  // toolbar.  This really needs to be only the font height.  However,
  // this improves the click-ability of the widget for small fonts.
  //
  // The Horizontal size of the title widget is determined by the available
  // space between m_tbLeftX and m_tbRightX.

  struct nxgl_size_s titleSize;
  titleSize.h = m_tbHeight;
  titleSize.w = m_tbRightX - m_tbLeftX - 2 * CONFIG_TWM4NX_TOOLBAR_HSPACING + 1;

  // Position the title.  Packed to the left horizontally, positioned at the
  // top of the toolbar.

  struct nxgl_point_s titlePos;
  titlePos.x = m_tbLeftX + CONFIG_TWM4NX_TOOLBAR_HSPACING;
  titlePos.y = 0;

  // Get the Widget control instance from the toolbar window.  This
  // will force all widget drawing to go to the toolbar.

  FAR NXWidgets:: CWidgetControl *control = m_toolbar->getWidgetControl();
  if (control == (FAR NXWidgets:: CWidgetControl *)0)
    {
      // Should not fail

      return false;
    }

  // Create the toolbar title widget

  m_tbTitle = new NXWidgets::CLabel(control, titlePos.x, titlePos.y,
                                    titleSize.w, titleSize.h, name, &m_tbStyle);
  if (m_tbTitle == (FAR NXWidgets::CLabel *)0)
    {
      twmerr("ERROR: Failed to construct tool bar title widget\n");
      return false;
    }

  // Configure the title widget

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *titleFont = fonts->getTitleFont();

  m_tbTitle->setFont(titleFont);
  m_tbTitle->setBorderless(true);
  m_tbTitle->disableDrawing();
  m_tbTitle->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_LEFT);
  m_tbTitle->setTextAlignmentVert(NXWidgets::CLabel::TEXT_ALIGNMENT_VERT_CENTER);
  m_tbTitle->setRaisesEvents(false);

  // Register to get events from the mouse clicks on the image

  m_tbTitle->addWidgetEventHandler(this);
  return true;
}

/**
 * After the toolbar was grabbed, it may be dragged then dropped, or it
 * may be simply "un-grabbed".  Both cases are handled here.
 *
 * NOTE: Unlike the other event handlers, this does NOT override any
 * virtual event handling methods.  It just combines some common event-
 * handling logic.
 *
 * @param x The mouse/touch X position.
 * @param y The mouse/touch y position.
 */

void CWindow::handleUngrabEvent(nxgl_coord_t x, nxgl_coord_t y)
{
  // Generate the un-grab event

  struct SEventMsg msg;
  msg.eventID = EVENT_TOOLBAR_UNGRAB;
  msg.obj     = (FAR void *)this;
  msg.pos.x   = x;
  msg.pos.y   = y;
  msg.context = EVENT_CONTEXT_TOOLBAR;
  msg.handler = (FAR void *)0;

  // NOTE that we cannot block because we are on the same thread
  // as the message reader.  If the event queue becomes full then
  // we have no other option but to lose events.
  //
  // I suppose we could recurse and call Twm4Nx::dispatchEvent at
  // the risk of runaway stack usage.

  int ret = mq_send(m_eventq, (FAR const char *)&msg,
                    sizeof(struct SEventMsg), 100);
  if (ret < 0)
    {
      twmerr("ERROR: mq_send failed: %d\n", errno);
    }
}

/**
 * Handle a mouse click event.
 *
 * @param e The event data.
 */

void CWindow::handleClickEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // We are interested only the the press event on the title box and
  // only if we are not already dragging the window

  if (!m_dragging && m_tbTitle->isClicked())
    {
      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_TOOLBAR_GRAB;
      msg.obj     = (FAR void *)this;
      msg.pos.x   = e.getX();
      msg.pos.y   = e.getY();
      msg.context = EVENT_CONTEXT_TOOLBAR;
      msg.handler = (FAR void *)0;

      // NOTE that we cannot block because we are on the same thread
      // as the message reader.  If the event queue becomes full then
      // we have no other option but to lose events.
      //
      // I suppose we could recurse and call Twm4Nx::dispatchEvent at
      // the risk of runaway stack usage.

      int ret = mq_send(m_eventq, (FAR const char *)&msg,
                        sizeof(struct SEventMsg), 100);
      if (ret < 0)
        {
          twmerr("ERROR: mq_send failed: %d\n", errno);
        }
    }
}

/**
 * Override the virtual CWidgetEventHandler::handleReleaseEvent.  This
 * event will fire when the title widget is released.  isClicked()
 * will return false for the title widget.
 *
 * @param e The event data.
 */

void CWindow::handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Handle the case where a release event was received, but the
  // window was not dragged.

  if (m_dragging && !m_tbTitle->isClicked())
    {
      // A click with no drag should raise the window.

      m_nxWin->raise();

      // Handle the non-drag drop event

      handleUngrabEvent(e.getX(), e.getY());
    }
}

/**
 * Override the virtual CWidgetEventHandler::handleActionEvent.  This
 * event will fire when the image is released but before it has been
 * has been drawn.  isClicked() will return true for the appropriate
 * images.
 *
 * @param e The event data.
 */

void CWindow::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // We are are interested in the pre-release event for any of the
  // toolbar buttons.

  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      // Check if this button is omitted by toolbar customizations or if the
      // button is temporarily disabled.

      if (((m_tbFlags | m_tbDisables) & (1 << btindex)) != 0)
        {
          continue;
        }

      // Check if the widget is clicked

      if (m_tbButtons[btindex] != (FAR NXWidgets::CImage *)0 &&
          m_tbButtons[btindex]->isClicked())
        {
          // Yes.. generate the event

          struct SEventMsg msg;
          msg.eventID = GToolBarInfo[btindex].event;
          msg.obj     = (FAR void *)this;
          msg.pos.x   = e.getX();
          msg.pos.y   = e.getY();
          msg.context = EVENT_CONTEXT_TOOLBAR;
          msg.handler = (FAR void *)0;

          // NOTE that we cannot block because we are on the same thread
          // as the message reader.  If the event queue becomes full then
          // we have no other option but to lose events.
          //
          // I suppose we could recurse and call Twm4Nx::dispatchEvent at
          // the risk of runaway stack usage.

          int ret = mq_send(m_eventq, (FAR const char *)&msg,
                            sizeof(struct SEventMsg), 100);
          if (ret < 0)
            {
              twmerr("ERROR: mq_send failed: %d\n", errno);
            }
        }
    }
}

/**
 * This function is called when there is any movement of the mouse or
 * touch position that would indicate that the object is being moved.
 *
 * This function overrides the virtual IEventTap::moveEvent method.
 *
 * @param pos The current mouse/touch X/Y position in toolbar relative
 *   coordinates.
 * @return True: if the drag event was processed; false it was
 *   ignored.  The event should be ignored if there is not actually
 *   a drag event in progress
 */

bool CWindow::moveEvent(FAR const struct nxgl_point_s &pos,
                        uintptr_t arg)
{
  twminfo("m_dragging=%u pos=(%d,%d)\n", m_dragging, pos.x, pos.y);

  // We are interested only the drag event while we are in the dragging
  // state.

  if (m_dragging)
    {
      // arg == 0 means that this a toolbar event vs s main window event.
      // Since the position is relative in both cases, we need a fix-up in
      // the height to keep the same toolbar relative position in all cases.

      nxgl_coord_t yIncr = ((arg == 0) ? 0 : m_tbHeight);

      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_WINDOW_DRAG;
      msg.obj     = (FAR void *)this;
      msg.pos.x   = pos.x;
      msg.pos.y   = pos.y + yIncr;
      msg.context = EVENT_CONTEXT_TOOLBAR;
      msg.handler = (FAR void *)0;

      // NOTE that we cannot block because we are on the same thread
      // as the message reader.  If the event queue becomes full then
      // we have no other option but to lose events.
      //
      // I suppose we could recurse and call Twm4Nx::dispatchEvent at
      // the risk of runaway stack usage.

      int ret = mq_send(m_eventq, (FAR const char *)&msg,
                        sizeof(struct SEventMsg), 100);
      if (ret < 0)
        {
          twmerr("ERROR: mq_send failed: %d\n", errno);
        }

      return true;
    }

  return false;
}

/**
 * This function is called if the mouse left button is released or
 * if the touchscreen touch is lost.  This indicates that the
 * dragging sequence is complete.
 *
 * This function overrides the virtual IEventTap::dropEvent method.
 *
 * @param pos The last mouse/touch X/Y position in toolbar relative
 *   coordinates.
 * @return True: If the drag event was processed; false it was
 *   ignored.  The event should be ignored if there is not actually
 *   a drag event in progress
 */

bool CWindow::dropEvent(FAR const struct nxgl_point_s &pos,
                        uintptr_t arg)
{
  twminfo("m_dragging=%u pos=(%d,%d)\n", m_dragging, pos.x, pos.y);

  // We are interested only the the drag drop event on the title box while we
  // are in the dragging state.
  //
  // When the Drop Event is received, both isClicked and isBeingDragged()
  // will return false.  It is sufficient to verify that the isClicked() is
  // not true to exit the drag.

  if (m_dragging)
    {
      // Yes.. handle the drop event

      // arg == 0 means that this a tooolbar event vs s main window event.
      // Since the position is relative in both cases, we need a fix-up in
      // the height to keep the same toolbar relative position in all cases.

      nxgl_coord_t yIncr = ((arg == 0) ? 0 : m_tbHeight);

      handleUngrabEvent(pos.x, pos.y + yIncr);
      return true;
    }

  return false;
}

/**
 * Is dragging enabled?
 *
 * @param arg The user-argument provided that accompanies the callback
 * @return True: If the dragging is enabled.
 */

bool CWindow::isActive(uintptr_t arg)
{
  return m_clicked;
}

/**
 * Enable/disable dragging
 *
 * True is provided when (1) isActive() returns false, but (2) a mouse
 *   report with a left-click is received.
 * False is provided when (1) isActive() returns true, but (2) a mouse
 *   report without a left-click is received.
 *
 * In the latter is redundant since dropEvent() will be called immediately
 * afterward.
 *
 * @param pos.  The mouse position at the time of the click or release
 * @param enable.  True:  Enable dragging
 * @param arg The user-argument provided that accompanies the callback
 */

void CWindow::enableMovement(FAR const struct nxgl_point_s &pos,
                             bool enable, uintptr_t arg)
{
  m_clicked = enable;
}

/**
 * Handle the TOOLBAR_GRAB event.  That corresponds to a left
 * mouse click on the title widget in the toolbar
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CWindow::toolbarGrab(FAR struct SEventMsg *eventmsg)
{
  twminfo("GRAB (%d,%d)\n", eventmsg->pos.x, eventmsg->pos.y);

  // Override application mouse events while dragging.  This is necessary to
  // to handle cases where the drag that starts in the toolbar is moved
  // into the application window area.

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_SYSTEM_NOP;
  events.resizeEvent = EVENT_SYSTEM_NOP;
  events.mouseEvent  = EVENT_TOOLBAR_XYINPUT;
  events.kbdEvent    = EVENT_SYSTEM_NOP;
  events.closeEvent  = m_appEvents.closeEvent;
  events.deleteEvent = m_appEvents.deleteEvent;

  bool success = m_windowEvent->configureEvents(events);
  if (!success)
    {
      return false;
    }

  // Promote the window to a modal window

  m_modal = true;
  m_nxWin->modal(true);

  // Indicate that dragging has started.

  m_dragging = true;

  // Get the frame position.

  struct nxgl_point_s framePos;
  getFramePosition(&framePos);

  twminfo("Position (%d,%d)\n", framePos.x, framePos.y);

  // Save the toolbar-relative mouse position in order to detect the amount
  // of movement in the next drag event.

  m_dragPos.x = eventmsg->pos.x;
  m_dragPos.y = eventmsg->pos.y;

#ifdef CONFIG_TWM4NX_MOUSE
  // Select the grab cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_GBCURSOR_IMAGE);

  // Remember the grab cursor size

  m_dragCSize.w = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.w;
  m_dragCSize.h = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.h;

#else
  // Fudge a value for the case where we are using a touchscreen.

  m_dragCSize.w = 16;
  m_dragCSize.h = 16;
#endif

  return true;
}

/**
 * Handle the WINDOW_DRAG event.  That corresponds to a mouse
 * movement when the window is in a grabbed state.
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CWindow::windowDrag(FAR struct SEventMsg *eventmsg)
{
  twminfo("DRAG (%d,%d)\n", eventmsg->pos.x, eventmsg->pos.y);

  if (m_dragging)
    {
      // The coordinates in the eventmsg are relative to the origin
      // of the toolbar.

      // Get the current (old) frame position

      struct nxgl_point_s oldPos;
      if (!getFramePosition(&oldPos))
        {
          twmerr("ERROR: getFramePosition() failed\n")  ;
          return false;
        }

      // We want to set the new frame position so that the new
      // mouse position is at the same relative position as it
      // was when the toolbar title was first grabbed.

      struct nxgl_point_s newPos;
      newPos.x = oldPos.x + eventmsg->pos.x - m_dragPos.x;
      newPos.y = oldPos.y + eventmsg->pos.y - m_dragPos.y;

      // Save the new mouse position

      m_dragPos.x = eventmsg->pos.x;
      m_dragPos.y = eventmsg->pos.y;

      // Keep the window on the display (at least enough of it so that we
      // can still grab it)

      struct nxgl_size_s displaySize;
      m_twm4nx->getDisplaySize(&displaySize);

      if (newPos.x < 0)
        {
          newPos.x = 0;
        }
      else if (newPos.x + m_dragCSize.w > displaySize.w)
        {
          newPos.x = displaySize.w - m_dragCSize.w;
        }

      if (newPos.y < 0)
        {
          newPos.y = 0;
        }
      else if (newPos.y + m_dragCSize.h > displaySize.h)
        {
          newPos.y = displaySize.h - m_dragCSize.h;
        }

      // Set the new window position if it has changed

      twminfo("Position (%d,%d)->(%d,%d)\n",
              oldPos.x, oldPos.y, newPos.x, newPos.y);

      if (newPos.x != oldPos.x || newPos.y != oldPos.y)
        {
          if (!setFramePosition(&newPos))
            {
              twmerr("ERROR: setFramePosition failed\n");
              return false;
            }

          m_nxWin->synchronize();
        }

      return true;
    }

  return false;
}

/**
 * Handle the TOOLBAR_UNGRAB event.  The corresponds to a mouse
 * left button release while in the grabbed state
 *
 * @param eventmsg.  The received NxWidget event message in window relative
 *   coordinates.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CWindow::toolbarUngrab(FAR struct SEventMsg *eventmsg)
{
  twminfo("UNGRAB (%d,%d)\n", eventmsg->pos.x, eventmsg->pos.y);

  // One last position update

  if (!windowDrag(eventmsg))
    {
      return false;
    }

  // Indicate no longer dragging

  m_dragging = false;

  // No longer modal

  m_modal = false;
  m_nxWin->modal(false);

#ifdef CONFIG_TWM4NX_MOUSE
  // Restore the normal cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_CURSOR_IMAGE);
#endif

  // Restore normal application event handling.

  return m_windowEvent->configureEvents(m_appEvents);
}

/**
 * Free windows from Twm4Nx window structure.
 */

void CWindow::cleanup(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Delete toolbar images

  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      FAR NXWidgets::CImage *cimage = m_tbButtons[btindex];
      if (cimage != (NXWidgets::CImage *)0)
        {
          delete m_tbButtons[btindex];
          m_tbButtons[btindex] = (NXWidgets::CImage *)0;
        }
    }

  if (m_tbTitle != (FAR NXWidgets::CLabel *)0)
    {
      delete m_tbTitle;
      m_tbTitle = (FAR NXWidgets::CLabel *)0;
    }

  // Delete the toolbar

  if (m_toolbar != (FAR NXWidgets::CNxToolbar *)0)
    {
      delete m_toolbar;
      m_toolbar  = (FAR NXWidgets::CNxToolbar *)0;
    }

  // Delete the window

  if (m_nxWin != (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete m_nxWin;
      m_nxWin  = (FAR NXWidgets::CNxTkWindow *)0;
    }

  // Delete the Icon

  if (m_iconWidget != (FAR CIconWidget *)0)
    {
      delete m_iconWidget;
      m_iconWidget  = (FAR CIconWidget *)0;
    }

  if (m_iconBitMap != (FAR NXWidgets::CRlePaletteBitmap *)0)
    {
      delete m_iconBitMap;
      m_iconBitMap  = (FAR NXWidgets::CRlePaletteBitmap *)0;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  /**
   * Return the minimum width of a toolbar window.  If the window is
   * resized smaller than this width, then the items in the toolbar will
   * overlap.
   *
   * @param twm4nx The Twm4Nx session object
   * @param title The window title string
   * @param flags Window toolbar properties
   * @return The minimum recommended window width.
   */

  nxgl_coord_t minimumToolbarWidth(FAR CTwm4Nx *twm4nx,
                                   FAR const NXWidgets::CNxString &title,
                                   uint8_t flags)
  {
    // Get the width of the title string

    FAR CFonts *fonts = twm4nx->getFonts();
    FAR NXWidgets::CNxFont *titleFont = fonts->getTitleFont();

    nxgl_coord_t tbWidth = titleFont->getStringWidth(title) +
                           CONFIG_TWM4NX_TOOLBAR_HSPACING;

    for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
      {
        // Check if this button is omitted by toolbar customizations

        if ((flags & (1 << btindex)) == 0)
          {
            // Add the button image width

            tbWidth += GToolBarInfo[btindex].bitmap->width +
                       CONFIG_TWM4NX_TOOLBAR_HSPACING;
          }
      }

    return tbWidth;
  }
}
