/****************************************************************************
 * apps/graphics/nxwidgets/src/cstickyimage.cxx
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
 *    me be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cstickyimage.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * CImage Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

/**
 * Constructor for an image.
 *
 * @param pWidgetControl The controlling widget for the display
 * @param x The x coordinate of the image box, relative to its parent.
 * @param y The y coordinate of the image box, relative to its parent.
 * @param width The width of the image box.
 * @param height The height of the image box.
 * @param bitmap The source bitmap image.
 * @param style The style that the widget should use.  If this is not
 *        specified, the image will use the global default widget
 *        style.
 */

CStickyImage::CStickyImage(CWidgetControl *pWidgetControl,
                           nxgl_coord_t x, nxgl_coord_t y,
                           nxgl_coord_t width, nxgl_coord_t height,
                           FAR IBitmap *bitmap, CWidgetStyle *style)
: CImage(pWidgetControl, x, y, width, height, bitmap, style)
{
  m_stuckSelection = false;
}

/**
 * Sets the image's stuck selection state.
 *
 * @param selection The new stuck selection state.
 */

void CStickyImage::setStuckSelection(bool selection)
{
  m_stuckSelection = selection;
  redraw();
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CStickyImage::drawContents(CGraphicsPort *port)
{
  CImage::drawContents(port, m_stuckSelection);
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CStickyImage::drawBorder(CGraphicsPort *port)
{
  CImage::drawBorder(port, m_stuckSelection);
}
