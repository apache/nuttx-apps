/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cbackground.hxx
// Manage background image
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <debug.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cscaledbitmap.hxx"
#include "graphics/nxwidgets/cbgwindow.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxwidgets/cimage.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cbackground.hxx"

/////////////////////////////////////////////////////////////////////////////
// CBackground Method Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CBackground Constructor
 *
 * @param hWnd - NX server handle
 */

CBackground::CBackground(FAR CTwm4Nx *twm4nx)
{
  m_twm4nx      = twm4nx;
  m_backWindow  = (NXWidgets::CBgWindow *)0;
  m_backImage   = (NXWidgets::CImage *)0;
}

/**
 * CBackground Destructor
 */

CBackground::~CBackground(void)
{
  // Delete the background

  if (m_backWindow != (NXWidgets::CBgWindow *)0)
    {
      // Delete the contained widget control.  We are responsible for it
      // because we created it

      NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();
      if (control != (NXWidgets::CWidgetControl *)0)
        {
          delete control;
        }

      // Then delete the background

      delete m_backWindow;
      m_backWindow = (NXWidgets::CBgWindow *)0;
    }

  // Delete the background image

  if (m_backImage != (NXWidgets::CImage *)0)
    {
      delete m_backImage;
      m_backImage = (NXWidgets::CImage *)0;
    }
}

/**
 * Set the background image
 *
 * @param sbitmap.  Identifies the bitmap to paint on background
 * @return true on success
 */

bool CBackground::
  setBackgroundImage(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap)
{
  // Create the background window (if we have not already done so)

  if (m_backWindow == (NXWidgets::CBgWindow *)0 &&
      !createBackgroundWindow())
    {
      return false;
    }

  // Free any existing background image

  if (m_backImage != (NXWidgets::CImage *)0)
    {
      delete m_backImage;
      m_backImage = (NXWidgets::CImage *)0;
    }

  // Create the new background image

  if (!createBackgroundImage(sbitmap))
    {
      return false;
    }

  return true;
}

/**
 * Get the size of the physical display device which is equivalent to
 * size of the background window.
 *
 * @return The size of the display
 */

void CBackground::getDisplaySize(FAR struct nxgl_size_s &size)
{
  // Get the widget control from the task bar window.  The physical window geometry
  // should be the same for all windows.

  NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();

  // Get the window bounding box from the widget control

  NXWidgets::CRect rect = control->getWindowBoundingBox();

  // And return the size of the window

  rect.getSize(size);
}

/**
 * Create the background window.
 *
 * @return true on success
 */

bool CBackground::createBackgroundWindow(void)
{
  // Create an instance of the background window
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx);

  m_backWindow = m_twm4nx->getBgWindow(control);
  if (m_backWindow == (FAR NXWidgets::CBgWindow *)0)
    {
      return false;
    }

  return true;
}

/**
 * Create the background image.
 *
 * @return true on success
 */

bool CBackground::
  createBackgroundImage(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap)
{
 // Get the size of the display

  struct nxgl_size_s windowSize;
  if (!m_backWindow->getSize(&windowSize))
    {
      return false;
    }

  // Get the widget control from the background window

  NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();

  // Create the sbitmap object

  NXWidgets::CRlePaletteBitmap *cbitmap =
    new NXWidgets::CRlePaletteBitmap(sbitmap);

  if (cbitmap == (NXWidgets::CRlePaletteBitmap *)0)
    {
      return false;
    }

  // Get the size of the bitmap image

  struct nxgl_size_s imageSize;
  imageSize.w = cbitmap->getWidth();
  imageSize.h = (nxgl_coord_t)cbitmap->getHeight();

  // Pick an X/Y position such that the image will be centered in the display

  struct nxgl_point_s imagePos;
  if (imageSize.w >= windowSize.w)
    {
      imagePos.x = 0;
    }
  else
    {
      imagePos.x = (windowSize.w - imageSize.w) >> 1;
    }

  if (imageSize.h >= windowSize.h)
    {
      imagePos.y = 0;
    }
  else
    {
      imagePos.y = (windowSize.h - imageSize.h) >> 1;
    }

  // Now we have enough information to create the image

  m_backImage = new NXWidgets::CImage(control, imagePos.x, imagePos.y,
                                      imageSize.w, imageSize.h, cbitmap);
  if (!m_backImage)
    {
      delete cbitmap;
      return false;
    }

  // Configure the background image

  m_backImage->setBorderless(true);
  m_backImage->setRaisesEvents(false);

  return true;
}

/**
 * (Re-)draw the background window.
 *
 * @return true on success
 */

bool CBackground::redrawBackgroundWindow(void)
{
  // Get the widget control from the background window

  NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();

  // Get the graphics port for drawing on the background window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the window

  struct nxgl_size_s windowSize;
  if (!m_backWindow->getSize(&windowSize))
    {
      return false;
    }

  // Fill the entire window with the background color

  port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                       CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR);

  // Then re-draw the background image on the window

  if (m_backImage)
    {
      m_backImage->enableDrawing();
      m_backImage->redraw();
    }

  return true;
}
