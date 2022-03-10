/****************************************************************************
 * apps/graphics/nxwidgets/src/cnxtoolbar.cxx
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cnxtoolbar.hxx"
#include "graphics/nxwidgets/cbitmap.hxx"

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
 * @param pNxTkWindow Parent framed window instance
 * @param hNxTkWindow Parent framed window NX handler
 * @param widgetControl Controlling widget for this toolbar.
 * @param height The height of the toolbar.
 */

CNxToolbar::CNxToolbar(CNxTkWindow *pNxTkWindow, NXTKWINDOW hNxTkWindow,
                       CWidgetControl *pWidgetControl, nxgl_coord_t height)
  : CCallback(pWidgetControl)
{
  // Initialize toolbar state data

  m_nxTkWindow    = pNxTkWindow;
  m_hNxTkWindow   = hNxTkWindow;
  m_widgetControl = pWidgetControl;
  m_height        = height;

  // Create the CGraphicsPort instance for this window

  m_widgetControl->createGraphicsPort(static_cast<INxWindow*>(this));
}

/**
 * Destructor.
 */

CNxToolbar::~CNxToolbar(void)
{
  // Inform the parent window instance that the toolbar is gone

  m_nxTkWindow->detachToolbar();

  // Release the widget control instance.  Normally the lifetime
  // of the widget control instance is managed by logic outside
  // of the window instance.  But here, as in the real world,
  // our parent expects us to clean up after ourselves.

  delete m_widgetControl;

  // Release the toolbar.

  nxtk_closetoolbar(m_hNxTkWindow);
}

/**
 * Creates a new toolbar.  Toolbar creation is separate from
 * object instantiation so that failures can be reported.
 *
 * @return True if the toolbar was successfully created.
 */

bool CNxToolbar::open(void)
{
  // Get the C-callable callback vtable

  FAR struct nx_callback_s *vtable = getCallbackVTable();

  // Create the toolbar

  int ret = nxtk_opentoolbar(m_hNxTkWindow, m_height, vtable,
                             (FAR void *)static_cast<CCallback*>(this));
  return ret == OK;
}

/**
 * Each implementation of INxWindow must provide a method to recover
 * the contained CWidgetControl instance.
 *
 * @return The contained CWidgetControl instance
 */

CWidgetControl *CNxToolbar::getWidgetControl(void) const
{
  return m_widgetControl;
}

/**
 * Request the position and size information of the toolbar. The values
 * will be returned asynchronously through the client callback method.
 * The GetPosition() method may than be called to obtain the positional
 * data as provided by the callback.
 *
 * @return True on success, false on any failure.
 */

bool CNxToolbar::requestPosition(void)
{
  // Request the position of the entire framed window containing the
  // toolbar.  The NXTK callback will route toolbar specific
  // information back to us.

  nxtk_getposition(m_hNxTkWindow);
  return false;
}

/**
 * Get the position of the toolbar in the physical display coordinates
 * (as reported by the NX callback).
 *
 * @return The position.
 */

bool CNxToolbar::getPosition(FAR struct nxgl_point_s *pPos)
{
  return m_widgetControl->getWindowPosition(pPos);
}

/**
 * Get the size of the toolbar (as reported by the NX callback).
 *
 * @return The size.
 */

bool CNxToolbar::getSize(FAR struct nxgl_size_s *pSize)
{
  return m_widgetControl->getWindowSize(pSize);
}

/**
 * Set an individual pixel in the toolbar with the specified color.
 *
 * @param pPos The location of the pixel to be filled.
 * @param color The color to use in the fill.
 *
 * @return True on success; false on failure.
 */

bool CNxToolbar::setPixel(FAR const struct nxgl_point_s *pPos,
                         nxgl_mxpixel_t color)
{
#if 0
  // Set an individual pixel to the specified color

  return nxtk_setpixel(m_hNxTkWindow, pPos, &color) == OK;
#else
  // REVISIT

  return false;
#endif
}

/**
 * Fill the specified rectangle in the toolbar with the specified color.
 *
 * @param pRect The location to be filled.
 * @param color The color to use in the fill.
 *
 * @return True on success; false on failure.
 */

bool CNxToolbar::fill(FAR const struct nxgl_rect_s *pRect,
                     nxgl_mxpixel_t color)
{
  // Fill a rectangular region with a solid color

  return nxtk_filltoolbar(m_hNxTkWindow, pRect, &color) == OK;
}

/**
 * Get the raw contents of graphic memory within a rectangular region. NOTE:
 * Since raw graphic memory is returned, the returned memory content may be
 * the memory of windows above this one and may not necessarily belong to
 * this window unless you assure that this is the top window.
 *
 * @param rect The location to be copied
 * @param dest - The describes the destination bitmap to receive the
 *   graphics data.
 */

void CNxToolbar::getRectangle(FAR const struct nxgl_rect_s *rect, struct SBitmap *dest)
{
  // Get a rectangule region from the toolbar

  nxtk_gettoolbar(m_hNxTkWindow, rect, 0, (FAR uint8_t*)dest->data, dest->stride);
}

/**
 * Fill the specified trapezoidal region in the toolbar with the specified
 * color.
 *
 * @param pClip Clipping rectangle relative to toolbar (may be null).
 * @param pTrap The trapezoidal region to be filled.
 * @param color The color to use in the fill.
 *
 * @return True on success; false on failure.
 */

bool CNxToolbar::fillTrapezoid(FAR const struct nxgl_rect_s *pClip,
                               FAR const struct nxgl_trapezoid_s *pTrap,
                               nxgl_mxpixel_t color)
{
  // Fill a trapezoidal region with a solid color

  return nxtk_filltraptoolbar(m_hNxTkWindow, pTrap, &color) == OK;
}

/**
 * Fill the specified line in the toolbar with the specified color.
 *
 * @param vector - Describes the line to be drawn
 * @param width  - The width of the line
 * @param color  - The color to use to fill the line
 * @param caps   - Draw a circular cap on the ends of the line to support
 *                 better line joins
 *
 * @return True on success; false on failure.
 */

bool CNxToolbar::drawLine(FAR struct nxgl_vector_s *vector,
                          nxgl_coord_t width, nxgl_mxpixel_t color,
                          enum ELineCaps caps)
{
  // Draw a line with the specified color

  return nxtk_drawlinetoolbar(m_hNxTkWindow, vector, width, &color, (uint8_t)caps) == OK;
}

/**
 * Draw a filled circle at the specified position, size, and color.
 *
 * @param center The window-relative coordinates of the circle center.
 * @param radius The radius of the rectangle in pixels.
 * @param color The color of the rectangle.
 */

bool CNxToolbar::drawFilledCircle(struct nxgl_point_s *center, nxgl_coord_t radius,
                                  nxgl_mxpixel_t color)
{
  return nxtk_fillcircletoolbar(m_hNxTkWindow, center, radius, &color) == OK;
}

/**
 * Move a rectangular region within the toolbar.
 *
 * @param pRect Describes the rectangular region to move.
 * @param pOffset The offset to move the region.
 *
 * @return True on success; false on failure.
 */

bool CNxToolbar::move(FAR const struct nxgl_rect_s *pRect,
                      FAR const struct nxgl_point_s *pOffset)
{
  // Move a rectangular region of the display

  return nxtk_movetoolbar(m_hNxTkWindow, pRect, pOffset) == OK;
}

/**
 * Copy a rectangular region of a larger image into the rectangle in the
 * specified toolbar.  The source image is treated as an opaque image.
 *
 * @param pDest Describes the rectangular on the display that will receive
 * the bitmap.
 * @param pSrc The start of the source image.
 * @param pOrigin the pOrigin of the upper, left-most corner of the full
 * bitmap. Both pDest and pOrigin are in toolbar coordinates, however,
 * pOrigin may lie outside of the display.
 * @param stride The width of the full source image in bytes.
 *
 * @return True on success; false on failure.
 */

bool CNxToolbar::bitmap(FAR const struct nxgl_rect_s *pDest,
                        FAR const void *pSrc,
                        FAR const struct nxgl_point_s *pOrigin,
                        unsigned int stride)
{
  // Copy a rectangular bitmap image in a region on the display

  return nxtk_bitmaptoolbar(m_hNxTkWindow, pDest, &pSrc, pOrigin, stride) == OK;
}
