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
#include "graphics/nxwidgets/clabel.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"
#include "graphics/twm4nx/cresize.hxx"

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
  m_twm4nx      = twm4nx;                           // Save the Twm4Nx session
  m_eventq      = (mqd_t)-1;                        // No widget message queue yet
  m_sizeWindow  = (FAR NXWidgets::CNxTkWindow *)0;  // No resize dimension windows yet
  m_sizeLabel   = (FAR NXWidgets::CLabel *)0;       // No resize dismsion label
  m_stringWidth = 0;                                // String width
}

/**
 * CResize Destructor
 */

CResize::~CResize(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      (void)mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Delete the resize dimension label

  if (m_sizeLabel != (FAR NXWidgets::CLabel *)0)
    {
      delete m_sizeLabel;
      m_sizeLabel = (FAR NXWidgets::CLabel *)0;
    }

  // Delete the resize dimensions window

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
  // Open a message queue to NX events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();
  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      gerr("ERROR: Failed open message queue '%s': %d\n",
           mqname, errno);
      return false;
    }

  // Create the size window

  if (!createSizeWindow())
    {
      gerr("ERROR:  Failed to create menu window\n");
      return false;
    }

  // Create the size label widget

  if (!createSizeLabel())
    {
      gerr("ERROR:  Failed to recreate size label\n");

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

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  struct nxgl_size_s size;
  size.w = m_stringWidth + CONFIG_TWM4NX_ICONMGR_HSPACING * 2;
  size.h = sizeFont->getHeight() + CONFIG_TWM4NX_ICONMGR_VSPACING * 2;

  // Set the window size

  if (!setWindowSize(&size))
    {
      gerr("ERROR: setWindowSize() failed\n");
      return;
    }

  // Move the window to the top of the hierarchy

  m_sizeWindow->raise();

  m_last.w = 0;
  m_last.h = 0;

  updateSizeLabel(cwin, &m_origsize);

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

  if (!setWindowSize(&winsize))
    {
      gerr("ERROR: setWindowSize() failed\n");
      return;
    }

  // Move the size window it to the top of the hieararchy

  m_sizeWindow->raise();
  updateSizeLabel(cwin, &m_origsize);

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
  updateSizeLabel(cwin, &m_origsize);
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

  updateSizeLabel(cwin, &m_dragsize);
}

/**
 * Resize the window.  This is called for each motion event while we are
 * resizing
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

  updateSizeLabel(cwin, &m_dragsize);
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

      unsigned int currcol = iconMgr->getNumberOfColumns();
      if (currcol == 0)
        {
          currcol = 1;
        }

      struct nxgl_size_s iconMgrSize;
      iconMgr->getSize(&iconMgrSize);

      iconMgrSize.w = (unsigned int)
        ((m_dragsize.w * (unsigned long)iconMgr->getDisplayColumns()) / currcol);

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
                 eventmsg->context == EVENT_CONTEXT_TOOLBAR)
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

/**
 * Create the size window
 */

bool CResize::createSizeWindow(void)
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

/**
 * Create the size label widget
 */

bool CResize::createSizeLabel(void)
{
  // The size of the selected is selected to fill the entire size window

  struct nxgl_size_s labelSize;
  if (!m_sizeWindow->setSize(&labelSize))
    {
      gerr("ERROR: Failed to get window size\n");
      return false;
    }

  // Position the label at the origin of the window.

  struct nxgl_point_s labelPos;
  labelPos.x = 0;
  labelPos.y = 0;

  // Get the Widget control instance from the size window.  This
  // will force all widget drawing to go to the size window.

  FAR NXWidgets:: CWidgetControl *control =
    m_sizeWindow->getWidgetControl();

  if (control == (FAR NXWidgets:: CWidgetControl *)0)
    {
      // Should not fail

      return false;
    }

  // Create the size label widget

  m_sizeLabel = new NXWidgets::CLabel(control, labelPos.x, labelPos.y,
                                      labelSize.w, labelSize.h,
                                      " 8888 x 8888 ");
  if (m_sizeLabel == (FAR NXWidgets::CLabel *)0)
    {
      gerr("ERROR: Failed to construct size label widget\n");
      return false;
    }

  // Configure the size label

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  m_sizeLabel->setFont(sizeFont);
  m_sizeLabel->setBorderless(true);
  m_sizeLabel->disableDrawing();
  m_sizeLabel->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_CENTER);
  m_sizeLabel->setTextAlignmentVert(NXWidgets::CLabel::TEXT_ALIGNMENT_VERT_CENTER);
  m_sizeLabel->setRaisesEvents(false);

  // Register to get events from the mouse clicks on the image

  m_sizeLabel->addWidgetEventHandler(this);
  return true;
}

/**
 * Set the Window Size
 */

bool CResize::setWindowSize(FAR struct nxgl_size_s *size)
{
  // Set the window size

  if (!m_sizeWindow->setSize(size))
    {
      return false;
    }

  // Set the label size to match

  if (!m_sizeLabel->resize(size->w, size->h))
    {
      return false;
    }

  return true;
}

/**
 * Update the size show in the size dimension label.
 *
 * @param cwin   The current window being resized
 * @param size   She size of the rubber band
 */

void CResize::updateSizeLabel(FAR CWindow *cwin, FAR struct nxgl_size_s *size)
{
  // Do nothing if the size has not changed

  if (m_last.w == size->w && m_last.h == size->h)
    {
      return;
    }

  m_last.w = size->w;
  m_last.h = size->h;

  FAR char *str;
  (void)asprintf(&str, " %4d x %-4d ", size->w, size->h);
  if (str == (FAR char *)0)
    {
      gerr("ERROR: Failed to get size string\n");
      return;
    }

  // Bring the window to the top of the hierarchy

  m_sizeWindow->raise();

  // Add the string to the label widget

  m_sizeLabel->disableDrawing();
  m_sizeLabel->setRaisesEvents(false);

  m_sizeLabel->setText(str);
  std::free(str);

  // Re-enable and redraw the label widget

  m_sizeLabel->enableDrawing();
  m_sizeLabel->setRaisesEvents(true);
  m_sizeLabel->redraw();
}
