/****************************************************************************
 * apps/graphics/nxwidgets/src/cnxwindow.cxx
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
#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cbitmap.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

/**
 * Constructor.  Creates an uninitialized instance of the CNxWindow
 * object.  The open() method must be called to initialize the instance.
 *
 * @param hNxServer Handle to the NX server.
 * @param widgetControl Controlling widget for this window.
 * @param flags Window properties
 */

CNxWindow::CNxWindow(NXHANDLE hNxServer, CWidgetControl *pWidgetControl,
                     uint8_t flags)
  : CCallback(pWidgetControl), m_hNxServer(hNxServer), m_hNxWindow(0),
    m_widgetControl(pWidgetControl), m_flags(flags)
{
  // Create the CGraphicsPort instance for this window

  m_widgetControl->createGraphicsPort(static_cast<INxWindow*>(this));
}

/**
 * Destructor.
 */

CNxWindow::~CNxWindow(void)
{
  // Release the window.

  nx_closewindow(m_hNxWindow);

  // Release the widget control instance. Whether its lifetime
  // should be handled by the window, or the owner-of-the-window
  // is debatable. However in general the widget control is stored
  // only in the window, so it makes sense to release it here also.

  delete m_widgetControl;
  m_widgetControl = NULL;
}

/**
 * Creates a new window.  Window creation is separate from
 * object instantiation so that failures can be reported.
 *
 * @return True if the window was successfully opened.
 */

bool CNxWindow::open(void)
{
  // Get the C-callable callback vtable

  FAR struct nx_callback_s *vtable = getCallbackVTable();

  // Create the window

  m_hNxWindow = nx_openwindow(m_hNxServer, m_flags, vtable,
                              (FAR void *)static_cast<CCallback*>(this));
  return m_hNxWindow != NULL;
}

/**
 * Each implementation of INxWindow must provide a method to recover
 * the contained CWidgetControl instance.
 *
 * @return The contained CWidgetControl instance
 */

CWidgetControl *CNxWindow::getWidgetControl(void) const
{
  return m_widgetControl;
}

/**
 * Request the position and size information of the window. The values
 * will be returned asynchronously through the client callback method.
 * The GetPosition() method may than be called to obtain the positional
 * data as provided by the callback.
 *
 * @return True on success, false on any failure.
 */

bool CNxWindow::requestPosition(void)
{
  // Request the window position

  return nx_getposition(m_hNxWindow) == OK;
}

/**
 * Get the position of the window in the physical display coordinates
 * (as reported by the NX callback).
 *
 * @return The position.
 */

bool CNxWindow::getPosition(FAR struct nxgl_point_s *pPos)
{
  return m_widgetControl->getWindowPosition(pPos);
}

/**
 * Get the size of the window (as reported by the NX callback).
 *
 * @return The size.
 */

bool CNxWindow::getSize(FAR struct nxgl_size_s *pSize)
{
  return m_widgetControl->getWindowSize(pSize);
}

/**
 * Set the position and size of the window.
 *
 * @param pPos The new position of the window.
 * @return True on success, false on any failure.
 */

bool CNxWindow::setPosition(FAR const struct nxgl_point_s *pPos)
{
  // Set the window size and position

  return nx_setposition(m_hNxWindow, pPos) == OK;
}

/**
 * Set the size of the selected window.
 *
 * @param pSize The new size of the window.
 * @return True on success, false on any failure.
 */

bool CNxWindow::setSize(FAR const struct nxgl_size_s *pSize)
{
  // Set the window size

  return nx_setsize(m_hNxWindow, pSize) == OK;
}

/**
 * May be used to either (1) raise a window to the top of the display and
 * select modal behavior, or (2) disable modal behavior.
 *
 * @param enable True: enter modal state; False: leave modal state
 * @return True on success, false on any failure.
 */

bool CNxWindow::modal(bool enable)
{
  // Select/de-select window modal state

  return nx_modal(m_hNxWindow, enable) == OK;
}

/**
 * Set an individual pixel in the window with the specified color.
 *
 * @param pPos The location of the pixel to be filled.
 * @param color The color to use in the fill.
 *
 * @return True on success; false on failure.
 */

bool CNxWindow::setPixel(FAR const struct nxgl_point_s *pPos,
                         nxgl_mxpixel_t color)
{
  // Set an individual pixel to the specified color

  return nx_setpixel(m_hNxWindow, pPos, &color) == OK;
}

/**
 * Fill the specified rectangle in the window with the specified color.
 *
 * @param pRect The location to be filled.
 * @param color The color to use in the fill.
 *
 * @return True on success; false on failure.
 */

bool CNxWindow::fill(FAR const struct nxgl_rect_s *pRect,
                     nxgl_mxpixel_t color)
{
  // Fill a rectangular region with a solid color

  return nx_fill(m_hNxWindow, pRect, &color) == OK;
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

void CNxWindow::getRectangle(FAR const struct nxgl_rect_s *rect, struct SBitmap *dest)
{
  // Get a rectangule region from the window

  nx_getrectangle(m_hNxWindow, rect, 0, (FAR uint8_t*)dest->data, dest->stride);
}
/**
 * Fill the specified trapezoidal region in the window with the specified
 * color.
 *
 * @param pClip Clipping rectangle relative to window (may be null).
 * @param pTrap The trapezoidal region to be filled.
 * @param color The color to use in the fill.
 *
 * @return True on success; false on failure.
 */

bool CNxWindow::fillTrapezoid(FAR const struct nxgl_rect_s *pClip,
                              FAR const struct nxgl_trapezoid_s *pTrap,
                              nxgl_mxpixel_t color)
{
  // Fill a trapezoidal region with a solid color

  return nx_filltrapezoid(m_hNxWindow, pClip, pTrap, &color) == OK;
}

/**
 * Fill the specified line in the window with the specified color.
 *
 * @param vector - Describes the line to be drawn
 * @param width  - The width of the line
 * @param color  - The color to use to fill the line
 * @param caps   - Draw a circular cap on the ends of the line to support
 *                 better line joins
 *
 * @return True on success; false on failure.
 */

bool CNxWindow::drawLine(FAR struct nxgl_vector_s *vector,
                         nxgl_coord_t width, nxgl_mxpixel_t color,
                         enum ELineCaps caps)
{
  // Draw a line with the specified color

  return nx_drawline(m_hNxWindow, vector, width, &color, (uint8_t)caps) == OK;
}

/**
 * Draw a filled circle at the specified position, size, and color.
 *
 * @param center The window-relative coordinates of the circle center.
 * @param radius The radius of the rectangle in pixels.
 * @param color The color of the rectangle.
 */

bool CNxWindow::drawFilledCircle(struct nxgl_point_s *center, nxgl_coord_t radius,
                                 nxgl_mxpixel_t color)
{
  return nx_fillcircle(m_hNxWindow, center, radius, &color) == OK;
}

/**
 * Move a rectangular region within the window.
 *
 * @param pRect Describes the rectangular region to move.
 * @param pOffset The offset to move the region.
 *
 * @return True on success; false on failure.
 */

bool CNxWindow::move(FAR const struct nxgl_rect_s *pRect,
                     FAR const struct nxgl_point_s *pOffset)
{
  // Move a rectangular region of the display

  return nx_move(m_hNxWindow, pRect, pOffset) == OK;
}

/**
 * Copy a rectangular region of a larger image into the rectangle in the
 * specified window.  The source image is treated as an opaque image.
 *
 * @param pDest Describes the rectangular on the display that will receive
 * the bitmap.
 * @param pSrc The start of the source image.
 * @param pOrigin the pOrigin of the upper, left-most corner of the full
 * bitmap. Both pDest and pOrigin are in window coordinates, however,
 * pOrigin may lie outside of the display.
 * @param stride The width of the full source image in bytes.
 *
 * @return True on success; false on failure.
 */

bool CNxWindow::bitmap(FAR const struct nxgl_rect_s *pDest,
                       FAR const void *pSrc,
                       FAR const struct nxgl_point_s *pOrigin,
                       unsigned int stride)
{
  // Copy a rectangular bitmap image in a region on the display

  return nx_bitmap(m_hNxWindow, pDest, &pSrc, pOrigin, stride) == OK;
}
