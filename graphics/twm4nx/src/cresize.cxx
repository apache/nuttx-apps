/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cresize.cxx
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

#include <stdio.h>

#include "graphics/nxwidgets/cnxfont.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/cresize.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cwindow.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

#define MINHEIGHT     0             // had been 32
#define MINWIDTH      0             // had been 60

#define makemult(a,b) ((b==1) ? (a) : (((int)((a) / (b))) * (b)))

#ifndef MIN
#  define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

/////////////////////////////////////////////////////////////////////////////
// Class Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CResize Constructor
 */

CResize::CResize(CTwm4Nx *twm4nx)
{
  // Save the Twm4Nx session

  m_twm4nx      = twm4nx;

  // Windows

  m_sizeWindow  = (FAR NXWidgets::CNxTkWindow *)0;

  // Strings

  m_stringWidth = 0;
}

/**
 * CResize Destructor
 */

CResize::~CResize(void)
{
  // Free the resize dimensions window

  if (m_sizeWindow != (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
    }
}

/**
 * CResize Initializer.  Performs the parts of the CResize construction
 * that may fail.
 *
 * @result True is returned on success
 */

bool CResize::initialize(void)
{
  // Create the main window
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx);

  // 4. Create the main window

  m_sizeWindow = m_twm4nx->createFramedWindow(control);
  if (m_sizeWindow == (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete control;
      return false;
    }

  // 5. Open and initialize the main window

  bool success = m_sizeWindow->open();
  if (!success)
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  // 6. Set the initial window size

  // Create the resize dimension window

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  m_stringWidth = sizeFont->getStringWidth(" 8888 x 8888 ");

  struct nxgl_size_s size;
  size.w = m_stringWidth,
  size.h = (sizeFont->getHeight() + CONFIG_TWM4NX_ICONMGR_VSPACING * 2);

  if (!m_sizeWindow->setSize(&size))
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  // 7. Set the initial window position

  struct nxgl_point_s pos =
  {
    .x = 0,
    .y = 0
  };

  if (!m_sizeWindow->setPosition(&pos))
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  return true;
}

void CResize::resizeFromCenter(FAR CWindow *cwin)
{
  // Get the current frame size and position

  struct nxgl_point_s winpos;
  if (!cwin->getFramePosition(&winpos))
    {
      return;
    }

  struct nxgl_size_s winsize;
  if (!cwin->getFrameSize(&winsize))
    {
      return;
    }

  FAR CResize *resize = m_twm4nx->getResize();
  resize->addingSize(&winsize);

  resize->menuStartResize(cwin, &winpos, &winsize);
}

/**
 * Begin a window resize operation
 *
 * @param ev           the event structure (button press)
 * @param cwin         the TWM window pointer
 */

void CResize::startResize(FAR struct SEventMsg *eventmsg, FAR CWindow *cwin)
{
  m_resizeWindow = cwin;

  // Get the current position and size

  if (!cwin->getFramePosition(&m_dragpos))
    {
      return;
    }

  if (!cwin->getFrameSize(&m_dragsize))
    {
      return;
    }

  m_dragpos.x  += 0;
  m_dragpos.y  += 0;
  m_origpos.x   = m_dragpos.x;
  m_origpos.y   = m_dragpos.y;
  m_origsize.w  = m_dragsize.w;
  m_origsize.h  = m_dragsize.h;
  m_clamp.pt1.y = 0;
  m_clamp.pt2.y = 0;
  m_clamp.pt1.x = 0;
  m_clamp.pt2.x = 0;
  m_delta.x     = 0;
  m_delta.y     = 0;

#ifdef CONFIG_TWM4NX_AUTO_RERESIZE // Resize relative to position in quad
  autoClamp(cwin, eventmsg);
#endif

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  struct nxgl_size_s size;
  size.w = m_stringWidth + CONFIG_TWM4NX_ICONMGR_HSPACING * 2;
  size.h = sizeFont->getHeight() + CONFIG_TWM4NX_ICONMGR_VSPACING * 2;

  // Set the window size

  if (!m_sizeWindow->setSize(&size))
    {
      return;
    }

  // Move the window to the top of the hierarchy

  m_sizeWindow->raise();

  m_last.w = 0;
  m_last.h = 0;

  displaySize(cwin, &m_origsize);

  // Set the new frame position and size

  if (!cwin->resizeFrame(&m_dragsize, &m_dragpos))
    {
      gerr("ERROR: Failed to resize frame\n");
    }
}

void CResize::menuStartResize(FAR CWindow *cwin,
                              FAR struct nxgl_point_s *pos,
                              FAR struct nxgl_size_s *size)
{
  m_dragpos.x   = pos->x;
  m_dragpos.y   = pos->y;
  m_origpos.x   = m_dragpos.x;
  m_origpos.y   = m_dragpos.y;
  m_origsize.w  = size->w;
  m_origsize.h  = size->h;
  m_dragsize.w  = size->w;
  m_dragsize.h  = size->h;
  m_clamp.pt1.x = 0;
  m_clamp.pt1.y = 0;
  m_clamp.pt2.x = 0;
  m_clamp.pt2.y = 0;
  m_delta.x     = 0;
  m_delta.y     = 0;
  m_last.w      = 0;
  m_last.h      = 0;

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  // Set the window size

  struct nxgl_size_s winsize;
  winsize.w = m_stringWidth + CONFIG_TWM4NX_ICONMGR_HSPACING * 2;
  winsize.h = sizeFont->getHeight() + CONFIG_TWM4NX_ICONMGR_VSPACING * 2;

  if (!m_sizeWindow->setSize(&winsize))
    {
      return;
    }

  // Move the size window it to the top of the hieararchy

  m_sizeWindow->raise();
  displaySize(cwin, &m_origsize);

  // Set the new frame position and size

  if (!cwin->resizeFrame(&m_dragsize, &m_dragpos))
    {
      gerr("ERROR: Failed to resize frame\n");
    }
}

/**
 * Begin a window resize operation
 *
 * @param cwin the Twm4Nx window pointer
 */

void CResize::addStartResize(FAR CWindow *cwin,
                             FAR struct nxgl_point_s *pos,
                             FAR struct nxgl_size_s *size)
{
  m_dragpos.x   = pos->x;
  m_dragpos.y   = pos->y;
  m_origpos.x   = m_dragpos.x;
  m_origpos.y   = m_dragpos.y;
  m_origsize.w  = size->w;
  m_origsize.h  = size->h;
  m_dragsize.w  = m_origsize.w;
  m_dragsize.h  = m_origsize.h;

  m_clamp.pt1.x = 0;
  m_clamp.pt1.y = 0;
  m_clamp.pt2.x = 0;
  m_clamp.pt2.y = 0;
  m_delta.x     = 0;
  m_delta.y     = 0;
  m_last.w      = 0;
  m_last.h      = 0;

  m_last.w      = 0;
  m_last.h      = 0;
  displaySize(cwin, &m_origsize);
}

/**
 * @param cwin  The current Twm4Nx window
 * @param root  The X position in the root window
 */

void CResize::menuDoResize(FAR CWindow *cwin,
                           FAR struct nxgl_point_s *root)
{
  int action;

  action = 0;

  root->x -= m_delta.x;
  root->y -= m_delta.y;

  if (m_clamp.pt1.y)
    {
      int delta = root->y - m_dragpos.y;
      if (m_dragsize.h - delta < MINHEIGHT)
        {
          delta = m_dragsize.h - MINHEIGHT;
          m_clamp.pt1.y = 0;
        }

      m_dragpos.y += delta;
      m_dragsize.h -= delta;
      action = 1;
    }
  else if (root->y <= m_dragpos.y)
    {
      m_dragpos.y = root->y;
      m_dragsize.h = m_origpos.y + m_origsize.h - root->y;
      m_clamp.pt2.y = 0;
      m_clamp.pt1.y = 1;
      m_delta.y = 0;
      action = 1;
    }

  if (m_clamp.pt1.x)
    {
      int delta = root->x - m_dragpos.x;
      if (m_dragsize.w - delta < MINWIDTH)
        {
          delta = m_dragsize.w - MINWIDTH;
          m_clamp.pt1.x = 0;
        }

      m_dragpos.x += delta;
      m_dragsize.w -= delta;
      action = 1;
    }
  else if (root->x <= m_dragpos.x)
    {
      m_dragpos.x = root->x;
      m_dragsize.w = m_origpos.x + m_origsize.w - root->x;
      m_clamp.pt2.x = 0;
      m_clamp.pt1.x = 1;
      m_delta.x = 0;
      action = 1;
    }

  if (m_clamp.pt2.y)
    {
      int delta = root->y - m_dragpos.y - m_dragsize.h;
      if (m_dragsize.h + delta < MINHEIGHT)
        {
          delta = MINHEIGHT - m_dragsize.h;
          m_clamp.pt2.y = 0;
        }

      m_dragsize.h += delta;
      action = 1;
    }
  else if (root->y >= m_dragpos.y + m_dragsize.h)
    {
      m_dragpos.y = m_origpos.y;
      m_dragsize.h = 1 + root->y - m_dragpos.y;
      m_clamp.pt1.y = 0;
      m_clamp.pt2.y = 1;
      m_delta.y = 0;
      action = 1;
    }

  if (m_clamp.pt2.x)
    {
      int delta = root->x - m_dragpos.x - m_dragsize.w;
      if (m_dragsize.w + delta < MINWIDTH)
        {
          delta = MINWIDTH - m_dragsize.w;
          m_clamp.pt2.x = 0;
        }

      m_dragsize.w += delta;
      action = 1;
    }
  else if (root->x >= m_dragpos.x + m_dragsize.w)
    {
      m_dragpos.x = m_origpos.x;
      m_dragsize.w = 1 + root->x - m_origpos.x;
      m_clamp.pt1.x = 0;
      m_clamp.pt2.x = 1;
      m_delta.x = 0;
      action = 1;
    }

  if (action)
    {
      constrainSize(cwin, &m_dragsize);
      if (m_clamp.pt1.x)
        {
          m_dragpos.x = m_origpos.x + m_origsize.w - m_dragsize.w;
        }

      if (m_clamp.pt1.y)
        {
          m_dragpos.y = m_origpos.y + m_origsize.h - m_dragsize.h;
        }

      // Set the new frame position and size

      if (!cwin->resizeFrame(&m_dragsize, &m_dragpos))
        {
          gerr("ERROR: Failed to resize frame\n");
        }
    }

  displaySize(cwin, &m_dragsize);
}

/**
 * Move the rubberband around.  This is called for each motion event when
 * we are resizing
 *
 * @param cwin  The current Twm4Nx window
 * @param root  The X position in the root window
 */

void CResize::doResize(FAR CWindow *cwin,
                       FAR struct nxgl_point_s *root)
{
  int action;

  action = 0;

  root->x -= m_delta.x;
  root->y -= m_delta.y;

  if (m_clamp.pt1.y)
    {
      int delta = root->y - m_dragpos.y;
      if (m_dragsize.h - delta < MINHEIGHT)
        {
          delta = m_dragsize.h - MINHEIGHT;
          m_clamp.pt1.y = 0;
        }

      m_dragpos.y += delta;
      m_dragsize.h -= delta;
      action = 1;
    }
  else if (root->y <= m_dragpos.y)
    {
      m_dragpos.y = root->y;
      m_dragsize.h = m_origpos.y + m_origsize.h - root->y;
      m_clamp.pt2.y = 0;
      m_clamp.pt1.y = 1;
      m_delta.y = 0;
      action = 1;
    }

  if (m_clamp.pt1.x)
    {
      int delta = root->x - m_dragpos.x;
      if (m_dragsize.w - delta < MINWIDTH)
        {
          delta = m_dragsize.w - MINWIDTH;
          m_clamp.pt1.x = 0;
        }

      m_dragpos.x += delta;
      m_dragsize.w -= delta;
      action = 1;
    }
  else if (root->x <= m_dragpos.x)
    {
      m_dragpos.x = root->x;
      m_dragsize.w = m_origpos.x + m_origsize.w - root->x;
      m_clamp.pt2.x = 0;
      m_clamp.pt1.x = 1;
      m_delta.x = 0;
      action = 1;
    }

  if (m_clamp.pt2.y)
    {
      int delta = root->y - m_dragpos.y - m_dragsize.h;
      if (m_dragsize.h + delta < MINHEIGHT)
        {
          delta = MINHEIGHT - m_dragsize.h;
          m_clamp.pt2.y = 0;
        }

      m_dragsize.h += delta;
      action = 1;
    }
  else if (root->y >= m_dragpos.y + m_dragsize.h - 1)
    {
      m_dragpos.y = m_origpos.y;
      m_dragsize.h = 1 + root->y - m_dragpos.y;
      m_clamp.pt1.y = 0;
      m_clamp.pt2.y = 1;
      m_delta.y = 0;
      action = 1;
    }

  if (m_clamp.pt2.x)
    {
      int delta = root->x - m_dragpos.x - m_dragsize.w;
      if (m_dragsize.w + delta < MINWIDTH)
        {
          delta = MINWIDTH - m_dragsize.w;
          m_clamp.pt2.x = 0;
        }

      m_dragsize.w += delta;
      action = 1;
    }
  else if (root->x >= m_dragpos.x + m_dragsize.w - 1)
    {
      m_dragpos.x = m_origpos.x;
      m_dragsize.w = 1 + root->x - m_origpos.x;
      m_clamp.pt1.x = 0;
      m_clamp.pt2.x = 1;
      m_delta.x = 0;
      action = 1;
    }

  if (action)
    {
      constrainSize(cwin, &m_dragsize);
      if (m_clamp.pt1.x)
        {
          m_dragpos.x = m_origpos.x + m_origsize.w - m_dragsize.w;
        }

      if (m_clamp.pt1.y)
        {
          m_dragpos.y = m_origpos.y + m_origsize.h - m_dragsize.h;
        }

      // Set the new frame position and size

      if (!cwin->resizeFrame(&m_dragsize, &m_dragpos))
        {
          gerr("ERROR: Failed to resize frame\n");
        }
    }

  displaySize(cwin, &m_dragsize);
}

/**
 * Finish the resize operation
 */

void CResize::endResize(FAR CWindow *cwin)
{
  struct nxgl_point_s pos =
  {
    .x = 0,
    .y = 0
  };

  struct nxgl_size_s size =
  {
    .w = 0,
    .h = 0
  };

  if (!cwin->resizeFrame(&size, &pos))
    {
      gerr("ERROR: Failed to resize frame\n");
    }

  constrainSize(cwin, &m_dragsize);

  struct nxgl_size_s framesize;
  cwin->getFrameSize(&framesize);

  if (m_dragsize.w != framesize.w || m_dragsize.h != framesize.h)
    {
      cwin->setZoom(ZOOM_NONE);
    }

  setupWindow(cwin, &m_dragpos, &m_dragsize);

  if (cwin->isIconMgr())
    {
      CIconMgr *iconMgr = cwin->getIconMgr();
      DEBUGASSERT(iconMgr != (CIconMgr *)0);

      unsigned int currcol = iconMgr->getCurrColumn();
      if (currcol == 0)
        {
          currcol = 1;
        }

      struct nxgl_size_s iconMgrSize;
      iconMgr->getSize(&iconMgrSize);

      iconMgrSize.w = (unsigned int)
        ((m_dragsize.w * (unsigned long)iconMgr->getColumns()) / currcol);

      cwin->resizeFrame(&iconMgrSize, &pos);
      iconMgr->pack();
    }

  cwin->raiseWindow();
  m_resizeWindow = (FAR CWindow *)0;
}

void CResize::menuEndResize(FAR CWindow *cwin)
{
  struct nxgl_point_s pos =
  {
    .x = 0,
    .y = 0
  };

  struct nxgl_size_s size =
  {
    .w = 0,
    .h = 0
  };

  // Set the window frame size (and position)

  if (!cwin->resizeFrame(&size, &pos))
    {
      gerr("ERROR: CWindow::resizeFrame() failed\n");
    }

  constrainSize(cwin, &m_dragsize);
  setupWindow(cwin, &m_dragpos, &m_dragsize);
}

/**
 * Finish the resize operation for AddWindo<w
 */

// REVISIT: Not used.  Used to be used to handle prompt for window size
// vs. automatically sizing.

void CResize::addEndResize(FAR CWindow *cwin)
{
  constrainSize(cwin, &m_dragsize);
  m_addingPos.x  = m_dragpos.x;
  m_addingPos.y  = m_dragpos.y;
  m_addingSize.w = m_dragsize.w;
  m_addingSize.h = m_dragsize.h;
}

/**
 * Adjust the given width and height to account for the constraints
 */

void CResize::constrainSize(FAR CWindow *cwin, FAR nxgl_size_s *size)
{
  // Calculate the minimum frame size

  struct nxgl_size_s nullSize;
  nullSize.w = 0;
  nullSize.h = 0;

  struct nxgl_size_s minSize;
  cwin->windowToFrameSize(&nullSize, &minSize);

  // Clip to minimim size

  if (size->w < minSize.w)
    {
      size->w = minSize.w;
    }

  if (size->h < minSize.h)
    {
      size->h = minSize.h;
    }

  // Get the maximum window size from CTwm4Nx

  struct nxgl_size_s maxSize;
  m_twm4nx->maxWindowSize(&maxSize);

  // Clip to maximim size

  if (size->w > maxSize.w)
    {
      size->w = maxSize.w;
    }

  if (size->h > maxSize.h)
    {
      size->h = maxSize.h;
    }
}

/**
 * Set window sizes.
 *
 * Special Considerations:
 *   This routine will check to make sure the window is not completely off the
 *   display, if it is, it'll bring some of it back on.
 *
 * The cwin->frame_XXX variables should NOT be updated with the values of
 * x,y,w,h prior to calling this routine, since the new values are compared
 * against the old to see whether a synthetic ConfigureNotify event should be
 * sent.  (It should be sent if the window was moved but not resized.)
 *
 * @param cwin The CWindow instance
 * @param pos  The position of the upper-left outer corner of the frame
 * @param size The size of the frame window
 */

void CResize::setupWindow(FAR CWindow *cwin, FAR nxgl_point_s *pos,
                          FAR nxgl_size_s *size)
{
  ginfo("pos={%d, %d} size={%d, %d}\n",
        pos->x, pos->y, size->w, size->h);

  // Clip the position the so that it is within the display (with a little
  // extra space for the cursor)

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  if (pos->x >= displaySize.w - 16)
    {
      pos->x = displaySize.w - 16;   // one "average" cursor width
    }

  if (pos->y >= displaySize.h - 16)
    {
      pos->y = displaySize.h - 16;  // one "average" cursor width
    }

  if (cwin->isIconMgr())
    {
      struct nxgl_size_s imsize;
      cwin->getFrameSize(&imsize);
      size->h = imsize.h;
    }

  // Set the window frame size (and position)

  if (!cwin->resizeFrame(size, pos))
    {
      gerr("ERROR: CWindow::resizeFrame() failed\n");
    }
}

/**
 * Zooms window to full height of screen or to full height and width of screen.
 * (Toggles so that it can undo the zoom - even when switching between fullZoom
 * and vertical zoom.)
 *
 * @param cwin  the TWM window pointer
 */

void CResize::fullZoom(FAR CWindow *cwin, int flag)
{
  // Get the current position and size

  if (!cwin->getFramePosition(&m_dragpos))
    {
      return;
    }

  if (!cwin->getFrameSize(&m_dragsize))
    {
      return;
    }

  struct nxgl_point_s base;
  base.x = 0;
  base.y = 0;

  uint16_t zoom = cwin->getZoom();
  if (zoom == flag)
    {
      cwin->getFramePosition(&m_dragpos);
      cwin->getFrameSize(&m_dragsize);
      cwin->setZoom(ZOOM_NONE);
    }
  else
    {
      if (zoom == ZOOM_NONE)
        {
          cwin->resizeFrame(&m_dragsize, &m_dragpos);
        }

      cwin->setZoom(flag);

      struct nxgl_size_s displaySize;
      m_twm4nx->getDisplaySize(&displaySize);

      switch (flag)
        {
        case ZOOM_NONE:
          break;

        case EVENT_RESIZE_VERTZOOM:
          m_dragsize.h = displaySize.h;
          m_dragpos.y  = base.y;
          break;

        case EVENT_RESIZE_HORIZOOM:
          m_dragpos.x  = base.x;
          m_dragsize.w = displaySize.w;
          break;

        case EVENT_RESIZE_FULLZOOM:
          m_dragpos.x  = base.x;
          m_dragpos.y  = base.y;
          m_dragsize.h = displaySize.h;
          m_dragsize.w = displaySize.w;
          break;

        case EVENT_RESIZE_LEFTZOOM:
          m_dragpos.x  = base.x;
          m_dragpos.y  = base.y;
          m_dragsize.h = displaySize.h;
          m_dragsize.w = displaySize.w / 2;
          break;

        case EVENT_RESIZE_RIGHTZOOM:
          m_dragpos.x  = base.x + displaySize.w / 2;
          m_dragpos.y  = base.y;
          m_dragsize.h = displaySize.h;
          m_dragsize.w = displaySize.w / 2;
          break;

        case EVENT_RESIZE_TOPZOOM:
          m_dragpos.x  = base.x;
          m_dragpos.y  = base.y;
          m_dragsize.h = displaySize.h / 2;
          m_dragsize.w = displaySize.w;
          break;

        case EVENT_RESIZE_BOTTOMZOOM:
          m_dragpos.x  = base.x;
          m_dragpos.y  = base.y + displaySize.h / 2;
          m_dragsize.h = displaySize.h / 2;
          m_dragsize.w = displaySize.w;
          break;
        }
    }

  cwin->raiseWindow();
  constrainSize(cwin, &m_dragsize);
  setupWindow(cwin, &m_dragpos, &m_dragsize);
}

/**
 * Handle RESIZE events.
 *
 * @param eventmsg.  The received NxWidget RESIZE event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CResize::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_RESIZE_START:  // Start window resize
        {
          // Can't resize icons

          FAR CWindow *cwin = (FAR CWindow *)eventmsg->obj;
          DEBUGASSERT(cwin != (FAR CWindow *)0);

          if (cwin->isIconMgr())
            {
             // Check for resize from a menu

             if (eventmsg->context == EVENT_CONTEXT_FRAME ||
                 eventmsg->context == EVENT_CONTEXT_WINDOW ||
                 eventmsg->context == EVENT_CONTEXT_TITLE)
                {
                  resizeFromCenter(cwin);
                }
              else
                {
                  startResize(eventmsg, cwin);
                }
            }
        }
        break;

      case EVENT_RESIZE_VERTZOOM:   // Zoom vertically only
      case EVENT_RESIZE_HORIZOOM:   // Zoom horizontally only
      case EVENT_RESIZE_FULLZOOM:   // Zoom both vertically and horizontally
      case EVENT_RESIZE_LEFTZOOM:   // Zoom left only
      case EVENT_RESIZE_RIGHTZOOM:  // Zoom right only
      case EVENT_RESIZE_TOPZOOM:    // Zoom top only
      case EVENT_RESIZE_BOTTOMZOOM: // Zoom bottom only
        fullZoom((FAR CWindow *)eventmsg->obj, eventmsg->eventID);
        break;

      default:
        success = false;
        break;
    }

  return success;
}

#ifdef CONFIG_TWM4NX_AUTO_RERESIZE // Resize relative to position in quad
void CResize::autoClamp(FAR CWindow *cwin,
                        FAR struct SEventMsg *eventmsg *eventmsg)
{
  FAR NXWidgets::CNxTkWindow *root;
  struct nxgl_point_s pos;
  int h;
  int v;
  unsigned int mask;

  switch (eventmsg->type)
    {
    case ButtonPress:
      pos.x = eventmsg->pos.x;
      pos.y = eventmsg->pos.y;
      break;

    case KeyPress:
      pos.x = eventmsg->pos.x;
      pos.y = eventmsg->pos.y;
      break;

    default:
      // Root window position

      root.x = 0;  // REVISIT
      root.y = 0;

      // Relative window position

      if (!xxx->getPosition(&pos))
        {
          return;
        }
    }

  h = ((pos.x - m_dragpos.x) / (m_dragsize.w < 3 ? 1 : (m_dragsize.w / 3)));
  v = ((pos.y - m_dragpos.y) / (m_dragsize.h < 3 ? 1 : (m_dragsize.h / 3)));

  if (h <= 0)
    {
      m_clamp.pt1.x = 1;
      m_delta.x = (pos.x - m_dragpos.x);
    }
  else if (h >= 2)
    {
      m_clamp.pt2.x = 1;
      m_delta.x = (pos.x - m_dragpos.x - m_dragsize.w);
    }

  if (v <= 0)
    {
      m_clamp.pt1.y = 1;
      m_delta.y = (pos.y - m_dragpos.y);
    }
  else if (v >= 2)
    {
      m_clamp.pt2.y = 1;
      m_delta.y = (pos.y - m_dragpos.y - m_dragsize.h);
    }
}
#endif

/**
 * Display the size in the dimensions window.
 *
 * @param cwin   The current window being resize
 * @param size   She size of the rubber band
 */

void CResize::displaySize(FAR CWindow *cwin, FAR struct nxgl_size_s *size)
{
  if (m_last.w == size->w && m_last.h == size->h)
    {
      return;
    }

  m_last.w = size->w;
  m_last.h = size->h;

  char str[100];
  (void)snprintf(str, sizeof(str), " %4d x %-4d ", size->w, size->h);

  m_sizeWindow->raise();

#warning Missing logic
#if 0
  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  // Show the string in the CLable in the size window

  // REVISIT:  This no longer exists
  fonts->renderString(m_twm4nx, m_sizeWindow, sizeFont,
                      CONFIG_TWM4NX_ICONMGR_HSPACING,
                      sizeFont->getHeight() + CONFIG_TWM4NX_ICONMGR_VSPACING, str);
#endif
}
