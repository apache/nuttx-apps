/****************************************************************************
 * apps/graphics/nxwidgets/include/cimage.cxx
 *
 *   Copyright (C) 2012-2013 Gregory Nutt. All rights reserved.
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

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"
#include "graphics/nxwidgets/ibitmap.hxx"
#include "graphics/nxwidgets/cbitmap.hxx"
#include "graphics/nxwidgets/cimage.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

/**
 * Constructor for a label containing a string.
 *
 * @param pWidgetControl The controlling widget for the display
 * @param x The x coordinate of the image box, relative to its parent.
 * @param y The y coordinate of the image box, relative to its parent.
 * @param width The width of the textbox.
 * @param height The height of the textbox.
 * @param bitmap The source bitmap image.
 * @param style The style that the widget should use.  If this is not
 *        specified, the button will use the global default widget
 *        style.
 */

CImage::CImage(CWidgetControl *pWidgetControl, nxgl_coord_t x, nxgl_coord_t y,
               nxgl_coord_t width, nxgl_coord_t height, FAR IBitmap *bitmap,
               CWidgetStyle *style)
   : CNxWidget(pWidgetControl, x, y, width, height, 0, style)
{
  // Save the IBitmap instance

  m_bitmap      = bitmap;

  // Not highlighted

  m_highlighted = false;

  // Position the top/left corner of the bitmap in the top/left corner of the display

  m_origin.x    = 0;
  m_origin.y    = 0;
}

/**
 * Insert the dimensions that this widget wants to have into the rect
 * passed in as a parameter.  All coordinates are relative to the
 * widget's parent.
 *
 * @param rect Reference to a rect to populate with data.
 */

void CImage::getPreferredDimensions(CRect &rect) const
{
  if (!m_bitmap)
    {
      rect.setX(m_rect.getX());
      rect.setY(m_rect.getY());
      rect.setWidth(0);
      rect.setHeight(0);
    }
  else
    {
      nxgl_coord_t width  = m_bitmap->getWidth();
      nxgl_coord_t height = m_bitmap->getHeight();

      if (!m_flags.borderless)
        {
          width  += (m_borderSize.left + m_borderSize.right);
          height += (m_borderSize.top + m_borderSize.bottom);
        }

      rect.setX(m_rect.getX());
      rect.setY(m_rect.getY());
      rect.setWidth(width);
      rect.setHeight(height);
    }
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the drawContents(port) and by classes that inherit from
 * CImage.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CImage::drawContents(CGraphicsPort *port, bool selected)
{
  if (!m_bitmap)
    {
      // No image to draw

      return;
    }

  // Get the the drawable region

  CRect rect;
  getRect(rect);

  // Allocate a working buffer that will hold one row of the bitmap

  FAR nxwidget_pixel_t *buffer = new nxwidget_pixel_t[rect.getWidth()];

  // Set up a simple bitmap structure to describe one row

  struct SBitmap bitmap;
  bitmap.bpp    = m_bitmap->getBitsPerPixel();
  bitmap.fmt    = m_bitmap->getColorFormat();
  bitmap.width  = rect.getWidth();
  bitmap.height = 1;
  bitmap.stride = (rect.getWidth() * m_bitmap->getBitsPerPixel()) >> 3;
  bitmap.data   = buffer;

  // Select the correct colorization

  m_bitmap->setSelected(selected || m_highlighted);

  // This is the end row + 1 that we can write into

  nxgl_coord_t bottomRow = m_bitmap->getHeight() + m_origin.y;
  if (bottomRow > rect.getHeight())
    {
      bottomRow = rect.getHeight();
    }

  // Apply padding entire line buffer.

  FAR nxwidget_pixel_t *ptr       = buffer;
  nxwidget_pixel_t      backColor = getBackgroundColor();

  for (int column = 0; column < rect.getWidth(); column++)
    {
      *ptr++ = backColor;
    }

  // This the starting row in the display image where we will begin drawing.

  nxgl_coord_t destRow = rect.getY();

  // Draw any padded rows at the top of the widget before this

  for (int padRow = 0; padRow < m_origin.y; padRow++, destRow++)
    {
      // Put the padded row on the display (never greyscale)

      port->drawBitmap(rect.getX(), destRow, rect.getWidth(), 1,
                       &bitmap, 0, 0);
    }

  // This is the number of rows that we can draw at the top of the display

  nxgl_coord_t nTopRows = bottomRow - m_origin.y;

  // Are we going to draw any rows at the top of the display?

  if (nTopRows > 0)
    {
      // This is the end column + 1 that we can write into

      nxgl_coord_t rightColumn = m_bitmap->getWidth() + m_origin.x;
      if (rightColumn > rect.getWidth())
        {
          rightColumn = rect.getWidth();
        }

      // This is the number of columns that we can draw on the left side of
      // the display at the offset m_origin.x.

      nxgl_coord_t nLeftPixels = rightColumn - m_origin.x;

      // This is the row number of the first row that we cannot draw into

      nxgl_coord_t lastTopRow = nTopRows + destRow;

      for (int srcRow = 0; destRow < lastTopRow; srcRow++, destRow++)
        {
          // Read the graphics data for the left hand side of this row and
          // place it in the row buffer at offset origin.x.

          if (!m_bitmap->getRun(0, srcRow, nLeftPixels, &buffer[m_origin.x]))
            {
              ginfo("IBitmap::getRun failed at image row\n", srcRow);
              delete buffer;
              return;
            }

          // Pre-process special pixel values... Then we can use the faster
          // opaque drawBitmap() function.

          ptr = &buffer[m_origin.x];
          for (int i = 0; i < nLeftPixels; i++, ptr++)
            {
              // Replace any transparent pixels with the background color.

              if (*ptr == CONFIG_NXWIDGETS_TRANSPARENT_COLOR)
                {
                  *ptr = backColor;
                }

              // Convert pixels (other than the background color) to grey
              // scale if the image is disabled.  We can't use
              // CGraphicsPort::drawBitmapGreyColor because it does not
              // (yet) understand transparent pixels and has no idea what it
              // should do with background colors.

              else if (*ptr != backColor && !isEnabled())
                {
                  // Get the next RGB pixel and break out the individual
                  // components

                  nxwidget_pixel_t rgb = *ptr;
                  nxwidget_pixel_t r   = RGB2RED(rgb);
                  nxwidget_pixel_t g   = RGB2GREEN(rgb);
                  nxwidget_pixel_t b   = RGB2BLUE(rgb);

                  // A truly accurate greyscale conversion would be complex.
                  // Let's just average.

                  nxwidget_pixel_t avg = (r + g + b) / 3;
                  *ptr = MKRGB(avg, avg, avg);
                }
            }

          // And put these on the display

          port->drawBitmap(rect.getX(), destRow, rect.getWidth(), 1,
                           &bitmap, 0, 0);
        }
    }

  // Are we going to draw any rows at the bottom of the display?

  if (nTopRows < rect.getHeight())
    {
      // Pad the entire row

      ptr  = buffer;
      for (int column = 0; column < rect.getWidth(); column++)
        {
          *ptr++ = backColor;
        }

      // Now draw the rows from the offset position

      for (; destRow < rect.getHeight(); destRow++)
        {
          // Put the padded row on the display (never greyscale)

          port->drawBitmap(rect.getX(), destRow, rect.getWidth(), 1,
                           &bitmap, 0, 0);
        }
    }

   delete buffer;
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CImage::drawContents(CGraphicsPort *port)
{
  drawContents(port, isClicked());
}

/**
 * Draw the border of this widget.  Called by the indirectly via
 * drawBoard(port) and also by classes that inherit from CImage.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CImage::drawBorder(CGraphicsPort *port, bool selected)
{
  // Stop drawing if the widget indicates it should not have an outline

  if (isBorderless())
    {
      return;
    }

  // Work out which colors to use

  nxgl_coord_t color1;
  nxgl_coord_t color2;

  if (selected)
    {
      // Bevelled into the screen

      color1 = getShadowEdgeColor();
      color2 = getShineEdgeColor();
    }
  else
    {
      // Bevelled out of the screen

      color1 = getShineEdgeColor();
      color2 = getShadowEdgeColor();
    }

  port->drawBevelledRect(getX(), getY(), getWidth(), getHeight(), color1, color2);
}

/**
 * Draw the border of this widget. Called by the redraw() function to draw
 * all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CImage::drawBorder(CGraphicsPort *port)
{
  drawBorder(port, isClicked());
}

/**
 * Control the highlight state.
 *
 * @param highlightOn True(1), the image will be highlighted
 */

void CImage::highlight(bool highlightOn)
{
  if (m_highlighted != highlightOn)
    {
      m_highlighted = highlightOn;
      redraw();
    }
}

/**
 * Redraws the button.
 *
 * @param x The x coordinate of the click.
 * @param y The y coordinate of the click.
 */

void CImage::onClick(nxgl_coord_t x, nxgl_coord_t y)
{
  redraw();
}

/**
 * Raises an action.
 *
 * @param x The x coordinate of the mouse.
 * @param y The y coordinate of the mouse.
 */

void CImage::onPreRelease(nxgl_coord_t x, nxgl_coord_t y)
{
  m_widgetEventHandlers->raiseActionEvent();
}

/**
 * Redraws the image.
 *
 * @param x The x coordinate of the mouse.
 * @param y The y coordinate of the mouse.
 */

void CImage::onRelease(nxgl_coord_t x, nxgl_coord_t y)
{
  redraw();
}

/**
 * Redraws the image.
 *
 * @param x The x coordinate of the mouse.
 * @param y The y coordinate of the mouse.
 */

void CImage::onReleaseOutside(nxgl_coord_t x, nxgl_coord_t y)
{
  redraw();
}

/**
 * Set the horizontal position of the bitmap.  Zero is the left edge
 * of the bitmap and values > 0 will move the bit map to the right.
 * This method is useful for horizontal scrolling a large bitmap
 * within a smaller window
 *
 * REVISIT: m_origin.x should be permitted to go negative.
 */

void CImage::setImageLeft(nxgl_coord_t column)
{
  // Get the the drawable region

  CRect rect;
  getRect(rect);

  if (m_bitmap && column >= 0 && column < rect.getWidth())
    {
      m_origin.x = column;
    }
}

/**
 * Align the image at the left of the widget region.
 *
 * NOTE: The CImage widget does not support any persistent alignment
 * attribute (at least not at the moment).  As a result, this alignment
 * can be lost if the image is changed or if the widget is resized.
 */

void CImage::alignHorizontalCenter(void)
{
  // Get the the drawable region

  CRect rect;
  getRect(rect);

  // Center the image

  setImageLeft((rect.getWidth() - m_bitmap->getWidth()) >> 1);
}

/**
 * Align the image at the left of the widget region.
 *
 * NOTE: The CImage widget does not support any persistent alignment
 * attribute (at least not at the moment).  As a result, this alignment
 * can be lost if the image is changed or if the widget is resized.
 */

void CImage::alignHorizontalRight(void)
{
  // Get the the drawable region

  CRect rect;
  getRect(rect);

  // Position the image at the right of the widget region

  setImageLeft(rect.getWidth() - m_bitmap->getWidth());
}

/**
 * Set the vertical position of the bitmap.  Zero is the top edge
 * of the bitmap and values >0 will move the bit map down.
 * This method is useful for vertical scrolling a large bitmap
 * within a smaller window
 *
 * REVISIT: m_origin.y should be permitted to go negative.
 */

void CImage::setImageTop(nxgl_coord_t row)
{
  // Get the the drawable region

  CRect rect;
  getRect(rect);

  // Check the attempted Y position

  if (m_bitmap && row >= 0 && row < rect.getHeight())
    {
      m_origin.y = row;
    }
}

/**
 * Align the image at the middle of the widget region.
 *
 * NOTE: The CImage widget does not support any persistent alignment
 * attribute (at least not at the moment).  As a result, this alignment
 * can be lost if the image is changed or if the widget is resized.
 */

void CImage::alignVerticalCenter(void)
{
  // Get the the drawable region

  CRect rect;
  getRect(rect);

  // Center the image

  setImageTop((rect.getHeight() - m_bitmap->getHeight()) >> 1);
}

/**
 * Align the image at the left of the widget region.
 *
 * NOTE: The CImage widget does not support any persistent alignment
 * attribute (at least not at the moment).  As a result, this alignment
 * can be lost if the image is changed or if the widget is resized.
 */

void CImage::alignVerticalBottom(void)
{
  // Get the the drawable region

  CRect rect;
  getRect(rect);

  // Position the image at the bottom of the widget region

  setImageTop(rect.getHeight() - m_bitmap->getHeight());
}
