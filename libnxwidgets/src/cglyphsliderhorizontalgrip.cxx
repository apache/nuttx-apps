/****************************************************************************
 * NxWidgets/libnxwidgets/src/cglyphsliderhorizontalgrip.cxx
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
 *
 *   Mostly copied from CSliderHorizontalGrip written by Gregory Nutt
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
 ****************************************************************************
 *
 * Portions of this package derive from Woopsi (http://woopsi.org/) and
 * portions are original efforts.  It is difficult to determine at this
 * point what parts are original efforts and which parts derive from Woopsi.
 * However, in any event, the work of  Antony Dzeryn will be acknowledged
 * in most NxWidget files.  Thanks Antony!
 *
 *   Copyright (c) 2007-2011, Antony Dzeryn
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the names "Woopsi", "Simian Zombie" nor the
 *   names of its contributors may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Antony Dzeryn ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Antony Dzeryn BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
 
#include "cglyphsliderhorizontalgrip.hxx"
#include "ibitmap.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

/**
 * Constructor.
 *
 * @param pWidgetControl The controlling widget for the display
 * @param x The x coordinate of the grip, relative to its parent.
 * @param y The y coordinate of the grip, relative to its parent.
 * @param width The width of the grip.
 * @param height The height of the grip.
 */

CGlyphSliderHorizontalGrip::CGlyphSliderHorizontalGrip(CWidgetControl *pWidgetControl,
                                             nxgl_coord_t x, nxgl_coord_t y,
                                             nxgl_coord_t width, nxgl_coord_t height,
                                             IBitmap *pGripBitmap)
: CImage(pWidgetControl, 0, (height>>1) - (pGripBitmap->getHeight() >> 1), 
       pGripBitmap->getWidth(), pGripBitmap->getHeight(),
       pGripBitmap, 0)
{
  // Make the widget draggable

  setDraggable(true);
}

/**
 * Resize the slider to the new dimensions.
 *
 * @param width The new width.
 * @param height The new height.
 */

void CGlyphSliderHorizontalGrip::onResize(nxgl_coord_t width, nxgl_coord_t height)
{
}

/**
 * Starts dragging the grip and redraws it.
 *
 * @param x The x coordinate of the click.
 * @param y The y coordinate of the click.
 */

void CGlyphSliderHorizontalGrip::onClick(nxgl_coord_t x, nxgl_coord_t y)
{
  startDragging(x, y);
  redraw();
}

/**
 * Redraws the grip.
 *
 * @param x The x coordinate of the mouse.
 * @param y The y coordinate of the mouse.
 */

void CGlyphSliderHorizontalGrip::onRelease(nxgl_coord_t x, nxgl_coord_t y)
{
  redraw();
}

/**
 * Redraws the grip.
 *
 * @param x The x coordinate of the mouse.
 * @param y The y coordinate of the mouse.
 */

void CGlyphSliderHorizontalGrip::onReleaseOutside(nxgl_coord_t x, nxgl_coord_t y)
{
  redraw();
}

/**
 * Moves the grip to follow the mouse.
 *
 * @param x The x coordinate of the mouse.
 * @param y The y coordinate of the mouse.
 * @param vX The horizontal distance of the drag.
 * @param vY The vertical distance of the drag.
 */

void CGlyphSliderHorizontalGrip::onDrag(nxgl_coord_t x, nxgl_coord_t y,
                                   nxgl_coord_t vX, nxgl_coord_t vY)
{
  // Work out where we're moving to

  nxgl_coord_t destX = x - m_grabPointX - m_parent->getX();

  // Do we need to move?

  if (destX != m_rect.getX())
    {
      // Get parent rect

      CRect rect;
      m_parent->getClientRect(rect);

      // Prevent grip from moving outside parent

      if (destX < rect.getX())
        {
          destX = rect.getX();
        }
      else
        {
          if (destX + getWidth() > rect.getWidth() + rect.getX())
            {
              destX = (rect.getWidth() + rect.getX()) - getWidth() ;
            }
        }

      // Move to new location

      moveTo(destX, (rect.getHeight() - getHeight()) >> 1);
      m_parent->redraw();
    }
}
