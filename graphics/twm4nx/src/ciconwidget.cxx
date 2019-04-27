/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ciconwidget.cxx
// Represents on desktop icon
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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cgraphicsport.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/ciconwidget.hxx"

/////////////////////////////////////////////////////////////////////////////
// CIconWidget Definitions
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * Constructor.  Note that the group determines its width and height
 * from the position and dimensions of its children.
 *
 * @param widgetControl The controlling widget for the display.
 * @param x The x coordinate of the group.
 * @param y The y coordinate of the group.
 * @param style The style that the button should use.  If this is not
 *        specified, the button will use the global default widget
 *        style.
 */

CIconWidget::CIconWidget(FAR CTwm4Nx *twm4nx,
                         FAR NXWidgets::CWidgetControl *widgetControl,
                         nxgl_coord_t x, nxgl_coord_t y,
                         FAR NXWidgets::CWidgetStyle *style)
: CNxWidget(widgetControl, x, y, 0, 0, WIDGET_BORDERLESS, style)
{
  m_twm4nx         = twm4nx;
  m_widgetControl  = widgetControl;

  // Configure the widget

  m_flags.borderless = true;
}

/**
 * Perform widget initialization that could fail and so it not appropriate
 * for the constructor
 *
 * @param cbitmp The bitmap image representing the icon
 * @param title The icon title string
 * @return True is returned if the widget is successfully initialized.
 */

bool CIconWidget::initialize(FAR NXWidgets::CRlePaletteBitmap *cbitmap,
                             FAR NXWidgets::CNxString &title)
{
  // Get the size of the Icon bitmap

  struct nxgl_size_s iconImageSize;
  iconImageSize.w = cbitmap->getWidth();
  iconImageSize.h = cbitmap->getHeight();

  // Get the size of the Icon name

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconFont = fonts->getIconFont();

  struct nxgl_size_s iconLabelSize;
  iconLabelSize.w  = iconFont->getStringWidth(title);
  iconLabelSize.h  = iconFont->getHeight();

  // Determine the new size of the containing widget

  struct nxgl_size_s iconWidgetSize;
  iconWidgetSize.w = ngl_max(iconImageSize.w, iconLabelSize.w);
  iconWidgetSize.h = iconImageSize.h + iconLabelSize.h + 2;

  // Update the widget size

  resize(iconWidgetSize.w, iconWidgetSize.h);

  // Get the position bitmap image, centering horizontally if the text
  // width is larger than the image width

  struct nxgl_point_s iconImagePos;
  iconImagePos.x = 0;
  iconImagePos.y = 0;

  if (iconLabelSize.w > (iconImageSize.w + 1))
    {
      iconImagePos.x = (iconLabelSize.w - iconImageSize.w) / 2;
    }

  // Create a new CImage to hold the bitmap image

  FAR NXWidgets::CImage *image =
    new NXWidgets::CImage(m_widgetControl, iconImagePos.x,
                          iconImagePos.y, iconImageSize.w, iconImageSize.h,
                          cbitmap, m_style);
  if (image == (FAR NXWidgets::CImage *)0)
    {
      gerr("ERROR: Failed to create image\n");
      return false;
    }

  image->setBorderless(true);

  // Get the position icon text, centering horizontally if the image
  // width is larger than the text width

  struct nxgl_point_s iconLabelPos;
  iconLabelPos.x = 0;
  iconLabelPos.y = iconImageSize.h + 2;

  if (iconImageSize.w > (iconLabelSize.w + 1))
    {
      iconLabelPos.x = (iconImageSize.w - iconLabelSize.w) / 2;
    }

  // Create a new CLabel to hold the icon text

  FAR NXWidgets::CLabel *label =
    new NXWidgets::CLabel(m_widgetControl, iconLabelPos.x, iconLabelPos.y,
                          iconLabelSize.w, iconLabelSize.h, title);
  if (label == (FAR NXWidgets::CLabel *)0)
    {
      gerr("ERROR: Failed to create icon label\n");
      delete image;
      return false;
    }

  label->setBorderless(true);

  // Add the CImage to to the containing widget

  image->addWidgetEventHandler(this);
  addWidget(image);

  label->addWidgetEventHandler(this);
  addWidget(label);
  return true;
}

/**
 * Insert the dimensions that this widget wants to have into the rect
 * passed in as a parameter.  All coordinates are relative to the
 * widget's parent.  Value is based on the length of the largest string
 * in the set of options.
 *
 * @param rect Reference to a rect to populate with data.
 */

void CIconWidget::getPreferredDimensions(NXWidgets::CRect &rect) const
{
  struct nxgl_size_s widgetSize;
  getSize(widgetSize);

  struct nxgl_point_s widgetPos;
  getPos(widgetPos);

  rect.setX(widgetPos.x);
  rect.setY(widgetPos.y);
  rect.setWidth(widgetSize.w);
  rect.setHeight(widgetSize.h);
}

/**
 * Handle a mouse click event.
 *
 * @param e The event data.
 */

void CIconWidget::handleClickEvent(const NXWidgets::CWidgetEventArgs &e)
{
  m_widgetEventHandlers->raiseClickEvent(e.getX(), e.getY());
}

/**
 * Handle a mouse double-click event.
 *
 * @param e The event data.
 */

void CIconWidget::handleDoubleClickEvent(const NXWidgets::CWidgetEventArgs &e)
{
  m_widgetEventHandlers->raiseDoubleClickEvent(e.getX(), e.getY());
}

/**
 * Handle a mouse button release event that occurred within the bounds of
 * the source widget.
 * @param e The event data.
 */

void CIconWidget::handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e)
{
  m_widgetEventHandlers->raiseReleaseEvent(e.getX(), e.getY());
}

/**
 * Handle a mouse button release event that occurred outside the bounds of
 * the source widget.
 *
 * @param e The event data.
 */

void CIconWidget::handleReleaseOutsideEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Child raised a release outside event, but we need to raise a different
  // event if the release occurred within the bounds of this parent widget

  if (checkCollision(e.getX(), e.getY()))
    {
      m_widgetEventHandlers->raiseReleaseEvent(e.getX(), e.getY());
    }
  else
    {
      m_widgetEventHandlers->raiseReleaseOutsideEvent(e.getX(), e.getY());
    }
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 * @param port The NXWidgets::CGraphicsPort to draw to.
 * @see redraw()
 */

void CIconWidget::drawContents(NXWidgets::CGraphicsPort *port)
{
  port->drawFilledRect(getX(), getY(), getWidth(), getHeight(),
                       getBackgroundColor());
}
