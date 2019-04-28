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
#include <debug.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxglyphs.hxx"

#include "graphics/nxwidgets/cnxtkwindow.hxx"
#include "graphics/nxwidgets/cnxtoolbar.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxwidgets/cscaledbitmap.hxx"
#include "graphics/nxwidgets/cimage.hxx"
#include "graphics/nxwidgets/clabel.hxx"
#include "graphics/nxwidgets/cnxfont.hxx"

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
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"
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
    &CONFIG_TWM4NX_RESIZE_IMAGE, true, EVENT_TOOLBAR_RESIZE
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
  m_twm4nx               = twm4nx;       // Save the Twm4Nx session
  m_eventq               = (mqd_t)-1;    // No widget message queue yet

  // Windows

  m_nxWin                = (FAR NXWidgets::CNxTkWindow *)0;
  m_toolbar              = (FAR NXWidgets::CNxToolbar *)0;
  m_zoom                 = ZOOM_NONE;
  m_modal                = false;

  // Toolbar

  m_tbTitle              = (FAR NXWidgets::CLabel *)0;
  m_tbHeight             = 0;             // Height of the toolbar
  m_tbLeftX              = 0;             // Temporaries
  m_tbRightX             = 0;

  // Icons/Icon Manager

  m_iconBitMap           = (FAR NXWidgets::CRlePaletteBitmap *)0;
  m_iconWidget           = (FAR CIconWidget *)0;
  m_iconMgr              = (FAR CIconMgr *)0;
  m_isIconMgr            = false;
  m_iconOn               = false;
  m_iconified            = false;

  // Dragging

  m_drag                 = false;
  m_dragOffset.x         = 0;
  m_dragOffset.y         = 0;
  m_dragCSize.w          = 0;
  m_dragCSize.h          = 0;

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
 * @param name      The the name of the window (and its icon)
 * @param pos       The initialize position of the window
 * @param size      The initial size of the window
 * @param sbitmap   The Icon bitmap image
 * @param isIconMgr Flag to tell if this is an icon manager window
 * @param iconMgr   Pointer to icon manager instance
 * @param noToolbar True: Don't add Title Bar
 * @return True if the window was successfully initialize; false on
 *   any failure,
 */

bool CWindow::initialize(FAR const char *name,
                         FAR const struct nxgl_point_s *pos,
                         FAR const struct nxgl_size_s *size,
                         FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                         bool isIconMgr, FAR CIconMgr *iconMgr,
                         bool noToolbar)
{
  // Open a message queue to send fully digested NxWidget events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();

  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      gerr("ERROR: Failed open message queue '%s': %d\n",
           mqname, errno);
      return false;
    }

  m_isIconMgr = isIconMgr;
  m_iconMgr   = iconMgr;

  if (name == (FAR const char *)0)
    {
      m_name = std::strdup(GNoName);
    }
  else
    {
      m_name = std::strdup(name);
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

  m_iconOn = false;

  // Create the window

  m_nxWin   = (FAR NXWidgets::CNxTkWindow *)0;
  m_toolbar = (FAR NXWidgets::CNxToolbar *)0;

  // Create the main window

  if (!createMainWindow(&winsize, pos))
    {
      gerr("ERROR: createMainWindow() failed\n");
      cleanup();
      return false;
    }

  m_tbHeight = 0;
  m_toolbar  = (FAR NXWidgets::CNxToolbar *)0;

  if (!noToolbar)
    {
      // Toolbar height should be based on size of images and fonts.

      if (!getToolbarHeight(name))
        {
          gerr("ERROR: getToolbarHeight() failed\n");
          cleanup();
          return false;
        }

      // Create the toolbar

      if (!createToolbar())
        {
          gerr("ERROR: createToolbar() failed\n");
          cleanup();
          return false;
        }

      // Add buttons to the toolbar

      if (!createToolbarButtons())
        {
          gerr("ERROR: createToolbarButtons() failed\n");
          cleanup();
          return false;
        }

      // Add the title to the toolbar

      if (!createToolbarTitle(name))
        {
          gerr("ERROR: createToolbarTitle() failed\n");
          cleanup();
          return false;
        }
    }

  // Create the icon image instance

  m_iconBitMap = new NXWidgets::CRlePaletteBitmap(sbitmap);
  if (m_iconBitMap == (NXWidgets::CRlePaletteBitmap *)0)
    {
      gerr("ERROR: Failed to create icon image\n");
      cleanup();
      return false;
    }

  // Get the widget control instance from the background.  This is needed
  // to force the icon widgets to be draw on the background

  FAR CBackground *background = m_twm4nx->getBackground();
  FAR NXWidgets::CWidgetControl *control = background->getWidgetControl();

  // Create and initialize the icon widget

  m_iconWidget = new CIconWidget(m_twm4nx, control, pos->x, pos->y);
  if (m_iconWidget == (FAR CIconWidget *)0)
    {
      gerr("ERROR: Failed to create the icon widget\n");
      cleanup();
      return false;
    }

  if (!m_iconWidget->initialize(m_iconBitMap, m_name))
    {
      gerr("ERROR: Failed to initialize the icon widget\n");
      cleanup();
      return false;
    }

  enableWidgets();
  return true;
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
 * @param size The new window frame size
 * @param pos  The frame location which may also have changed
 */

bool CWindow::resizeFrame(FAR const struct nxgl_size_s *size,
                          FAR struct nxgl_point_s *pos)
{
  // Account for toolbar and border

  struct nxgl_size_s delta;
  delta.w = 2 * CONFIG_NXTK_BORDERWIDTH;
  delta.h = m_tbHeight + 2 * CONFIG_NXTK_BORDERWIDTH;

  // Don't set the window size smaller than one pixel

  struct nxgl_size_s winsize;
  if (size->w <= delta.w)
    {
      winsize.w = 1;
    }
  else
    {
      winsize.w = size->w - delta.w;
    }

  if (size->h <= delta.h)
    {
      winsize.h = 1;
    }
  else
    {
      winsize.h = size->h - delta.h;
    }

  // Set the usable window size

  bool success = m_nxWin->setSize(&winsize);
  if (!success)
    {
      gerr("ERROR: Failed to setSize()\n");
      return false;
    }

  // Set the new frame position (in case it changed too)

  success = setFramePosition(pos);
  if (!success)
    {
      gerr("ERROR: Failed to setSize()\n");
      return false;
    }

  // Synchronize with the NX server to make sure that the new geometry is
  // truly in effect.

  m_nxWin->synchronize();

  // Then update the toolbar layout

  return updateToolbarLayout();
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

void CWindow::iconify(void)
{
  if (!isIconified())
    {
      // Make sure to exit any modal state before minimizing

      m_modal = false;
      m_nxWin->modal(false);

      // Raise the icon window and lower the main window

      m_iconified = true;
      m_nxWin->lower();
      m_iconOn = true;
      m_nxWin->synchronize();
    }
}

void CWindow::deIconify(void)
{
  // De-iconify the window

  if (isIconified())
    {
      // Raise the main window and lower the icon window

      m_iconified = false;
      m_nxWin->raise();
      m_iconOn = false;
      m_nxWin->synchronize();
    }
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
      case EVENT_WINDOW_RAISE:     // Raise window to the top of the heirarchy
        m_nxWin->raise();          // Could be the main or the icon window
        break;

      case EVENT_WINDOW_LOWER:     // Lower window to the bottom of the heirarchy
        m_nxWin->lower();          // Could be the main or the icon window
        break;

      case EVENT_WINDOW_DEICONIFY: // De-iconify and raise the main window
        {
          deIconify();
        }
        break;

      case EVENT_WINDOW_FOCUS:
        if (!isIconified())
          {
            m_modal = !m_modal;
            m_nxWin->modal(m_modal);
          }

        break;

      case EVENT_TOOLBAR_TERMINATE:  // Toolbar terminate button pressed
        if (isIconMgr())
          {
            // Don't terminate the Icon manager, just hide it

            CIconMgr *iconMgr = m_twm4nx->getIconMgr();
            DEBUGASSERT(iconMgr != (CIconMgr *)0);

            iconMgr->hide();
          }
        else
          {
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

      case EVENT_WINDOW_UNFOCUS:  // Exit modal state
        {
          m_modal = false;
          m_nxWin->modal(false);
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
 * @param winsize   The initial window size
 * @param winpos    The initial window position
 */

bool CWindow::createMainWindow(FAR const nxgl_size_s *winsize,
                               FAR const nxgl_point_s *winpos)
{
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx);

  // 4. Create the window

  m_nxWin = m_twm4nx->createFramedWindow(control);
  if (m_nxWin == (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete control;
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

  return true;
}

/**
 * Calculate the height of the tool bar
 */

bool CWindow::getToolbarHeight(FAR const char *name)
{
  // The tool bar height is the largest of the toolbar button heights or the
  // title text font

  // Check if there is a title.  If so, get the font height.

  m_tbHeight = 0;
  if (name != (FAR const char *)0)
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

  // Plus somoe lines for good separation

  m_tbHeight += CONFIG_TWM4NX_FRAME_VSPACING;
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

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx);

  // 3. Get the toolbar sub-window from the framed window

  m_toolbar = m_nxWin->openToolbar(m_tbHeight, control);
  if (m_toolbar == (FAR NXWidgets::CNxToolbar *)0)
    {
      delete control;
      return false;
    }

  // 4. Open and initialize the tool bar

  return m_toolbar->open();
}

/**
 * Update the toolbar layout, resizing the title text window and
 * repositioning all windows on the toolbar.
 */

bool CWindow::updateToolbarLayout(void)
{
  // Disable widget drawing and events while we do this

  disableWidgets();

  // Reposition all right buttons.  Change the width of the
  // toolbar does not effect the left side spacing.

  m_tbRightX = 0;
  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      if (GToolBarInfo[btindex].rightSide)
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
             gerr("ERROR: Faile to move button image\n");
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
  titleSize.w = m_tbRightX - m_tbLeftX - CONFIG_TWM4NX_FRAME_VSPACING + 1;

  bool success = m_tbTitle->resize(titleSize.w, titleSize.h);
  enableWidgets();
  return success;
}

/**
 * Disable widget drawing and widget events.
 */

bool CWindow::disableWidgets(void)
{
  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      FAR NXWidgets::CImage *cimage = m_tbButtons[btindex];
      cimage->disableDrawing();
      cimage->setRaisesEvents(false);
    }

  m_tbTitle->disableDrawing();
  m_tbTitle->setRaisesEvents(false);
  return true;
}

/**
 * Enable widget drawing and widget events.
 */

bool CWindow::enableWidgets(void)
{
  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
      FAR NXWidgets::CImage *cimage = m_tbButtons[btindex];
      cimage->enableDrawing();
      cimage->setRaisesEvents(true);
      cimage->redraw();
    }

  m_tbTitle->enableDrawing();
  m_tbTitle->setRaisesEvents(true);
  m_tbTitle->redraw();
  return true;
}

/**
 * Create all toolbar buttons
 */

bool CWindow::createToolbarButtons(void)
{
  struct nxgl_size_s winsize;
  if (!getWindowSize(&winsize))
    {
      return false;
    }

  // Create the title bar windows
  // Set up the toolbar horizonal spacing

  m_tbRightX = winsize.w;
  m_tbLeftX  = 0;

  for (int btindex = 0; btindex < NTOOLBAR_BUTTONS; btindex++)
    {
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
          gerr("ERROR: Failed to created scaled bitmap\n");
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
        new NXWidgets::CImage(control, 0, 0, w, h, scaler, 0);
      if (m_tbButtons[btindex] == (FAR NXWidgets::CImage *)0)
        {
          gerr("ERROR: Failed to create image\n");
          delete scalar;
          return false;
        }
#else
      FAR NXWidgets::CRlePaletteBitmap *cbitmap =
        new NXWidgets::CRlePaletteBitmap(sbitmap);
      if (cbitmap == (FAR NXWidgets::CRlePaletteBitmap *)0)
        {
          gerr("ERROR: Failed to create CrlPaletteBitmap\n");
          return false;
        }

      w = cbitmap->getWidth();
      h = cbitmap->getHeight();

      m_tbButtons[btindex] =
        new NXWidgets::CImage(control, 0, 0, w, h, cbitmap, 0);
      if (m_tbButtons[btindex] == (FAR NXWidgets::CImage *)0)
        {
          gerr("ERROR: Failed to create image\n");
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

bool CWindow::createToolbarTitle(FAR const char *name)
{
  // Is there a title?

  if (name != (FAR const char *)0)
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
  titleSize.w = m_tbRightX - m_tbLeftX - CONFIG_TWM4NX_FRAME_VSPACING + 1;

  // Position the title.  Packed to the left horizontally, positioned at the
  // top of the toolbar.

  struct nxgl_point_s titlePos;
  titlePos.x = m_tbLeftX + CONFIG_TWM4NX_FRAME_VSPACING;
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
                                    titleSize.w, titleSize.h, name);
  if (m_tbTitle == (FAR NXWidgets::CLabel *)0)
    {
      gerr("ERROR: Failed to construct tool bar title widget\n");
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
 * may be simply "un-grabbed". Both cases are handled here.
 *
 * NOTE: Unlike the other event handlers, this does NOT override any
 * virtual event handling methods.  It just combines some common event-
 * handling logic.
 *
 * @param e The event data.
 */

void CWindow::handleUngrabEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Exit the dragging state

  m_drag = false;

  // Generate the un-grab event

  struct SEventMsg msg;
  msg.eventID = EVENT_TOOLBAR_UNGRAB;
  msg.pos.x   = e.getX();
  msg.pos.y   = e.getY();
  msg.delta.x = 0;
  msg.delta.y = 0;
  msg.context = EVENT_CONTEXT_TOOLBAR;
  msg.handler = (FAR CTwm4NxEvent *)0;
  msg.obj     = (FAR void *)this;

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
      gerr("ERROR: mq_send failed: %d\n", ret);
    }
}

/**
 * Override the mouse button drag event.
 *
 * @param e The event data.
 */

void CWindow::handleDragEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // We are interested only the the drag event on the title box while we are
  // in the dragging state.

  if (m_drag && m_tbTitle->isBeingDragged())
    {
      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_WINDOW_DRAG;
      msg.pos.x   = e.getX();
      msg.pos.y   = e.getY();
      msg.delta.x = e.getVX();
      msg.delta.y = e.getVY();
      msg.context = EVENT_CONTEXT_TOOLBAR;
      msg.handler = (FAR CTwm4NxEvent *)0;
      msg.obj     = (FAR void *)this;

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
          gerr("ERROR: mq_send failed: %d\n", ret);
        }
    }
}

/**
 * Override a drop event, triggered when the widget has been dragged-and-dropped.
 *
 * @param e The event data.
 */

void CWindow::handleDropEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // We are interested only the the drag drop event on the title box while we
  // are in the dragging state.
  //
  // When the Drop Event is received, both isClicked and isBeingDragged()
  // will return false.  It is sufficient to verify that the isClicked() is
  // not true to exit the drag.

  if (m_drag && !m_tbTitle->isClicked())
    {
      // Yes.. handle the drop event

      handleUngrabEvent(e);
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

  if (!m_drag && m_tbTitle->isClicked())
    {
      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_TOOLBAR_GRAB;
      msg.pos.x   = e.getX();
      msg.pos.y   = e.getY();
      msg.delta.x = 0;
      msg.delta.y = 0;
      msg.context = EVENT_CONTEXT_TOOLBAR;
      msg.handler = (FAR CTwm4NxEvent *)0;
      msg.obj     = (FAR void *)this;

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
          gerr("ERROR: mq_send failed: %d\n", ret);
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

  if (m_drag && !m_tbTitle->isClicked())
    {
      // A click with no drag should raise the window.

      m_nxWin->raise();

      // Handle the non-drag drop event

      handleUngrabEvent(e);
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
      // Check if the widget is clicked

      if (m_tbButtons[btindex]->isClicked())
        {
          // Yes.. generate the event

          struct SEventMsg msg;
          msg.eventID = GToolBarInfo[btindex].event;
          msg.pos.x   = e.getX();
          msg.pos.y   = e.getY();
          msg.delta.x = 0;
          msg.delta.y = 0;
          msg.context = EVENT_CONTEXT_TOOLBAR;
          msg.handler = (FAR CTwm4NxEvent *)0;
          msg.obj     = (FAR void *)this;

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
              gerr("ERROR: mq_send failed: %d\n", ret);
            }
        }
    }
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
  // Promote the window to a modal window

  m_modal = true;
  m_nxWin->modal(true);

  // Indicate that dragging has started.

  m_drag = true;

  // Get the frame position.

  struct nxgl_point_s framePos;
  getFramePosition(&framePos);

  // Determine the relative position of the frame and the mouse

  m_dragOffset.x = framePos.x - eventmsg->pos.x;
  m_dragOffset.y = framePos.y - eventmsg->pos.y;

  // Select the grab cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_GBCURSOR_IMAGE);

  // Remember the grab cursor size

  m_dragCSize.w = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.w;
  m_dragCSize.h = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.h;
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
  if (m_drag)
    {
      // Calculate the new Window position

      struct nxgl_point_s newpos;
      newpos.x = eventmsg->pos.x + m_dragOffset.x;
      newpos.y = eventmsg->pos.y + m_dragOffset.y;

      // Keep the window on the display (at least enough of it so that we
      // can still grab it)

      struct nxgl_size_s displaySize;
      m_twm4nx->getDisplaySize(&displaySize);

      if (newpos.x < 0)
        {
          newpos.x = 0;
        }
      else if (newpos.x + m_dragCSize.w > displaySize.w)
        {
          newpos.x = displaySize.w - m_dragCSize.w;
        }

      if (newpos.y < 0)
        {
          newpos.y = 0;
        }
      else if (newpos.y + m_dragCSize.h > displaySize.h)
        {
          newpos.y = displaySize.h - m_dragCSize.h;
        }

      // Set the new window position

      return setFramePosition(&newpos);
    }

  return false;
}

/**
 * Handle the TOOLBAR_UNGRAB event.  The corresponds to a mouse
 * left button release while in the grabbed state
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CWindow::toolbarUngrab(FAR struct SEventMsg *eventmsg)
{
  // One last position update

  if (!windowDrag(eventmsg))
    {
      return false;
    }

  // Indicate no longer dragging

  m_drag = false;

  // No longer modal

  m_modal = false;
  m_nxWin->modal(false);

  // Restore the normal cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_CURSOR_IMAGE);
  return false;
}

/**
 * Free windows from Twm4Nx window structure.
 */

void CWindow::cleanup(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      (void)mq_close(m_eventq);
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

  // Free memory

  if (m_name != (FAR char *)0)
    {
      std::free(m_name);
      m_name = (FAR char *)0;
    }
}
