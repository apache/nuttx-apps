/****************************************************************************
 * apps/include/graphics/nxwidgets/inxwindow.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_INXWINDOW_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_INXWINDOW_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/nx/nxglib.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef CONFIG_NXTERM_NXKBDIN
#  include <nuttx/nx/nxterm.h>
#endif

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Pure Virtual Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  struct SBitmap;
  class  CWidgetControl;

  /**
   * This class defines common operations on a any NX window.
   * There are three instances that represent an NX window from the
   * perspective of NXWidgets.
   *
   * - There is one widget control instance per NX window,
   * - One CCallback instance per window,
   * - One window instance.
   *
   * There a various kinds of of window instances, but each inherits
   * (1) CCallback and dispatches the Windows callbacks and (2) INxWindow
   * that describes the common window behavior.
   */

  class INxWindow
  {
  public:
    enum ELineCaps
    {
      LINECAP_NONE = NX_LINECAP_NONE, // No line caps
      LINECAP_PT1  = NX_LINECAP_PT1,  // Line cap on pt1 of the vector only
      LINECAP_PT2  = NX_LINECAP_PT2,  // Line cap on pt2 of the vector only
      LINECAP_BOTH = NX_LINECAP_BOTH  // Line cap on both ends of the vector only
    };

    /**
     * A virtual destructor is required in order to override the INxWindow
     * destructor.  We do this because if we delete INxWindow, we want the
     * destructor of the class that inherits from INxWindow to run, not this
     * one.
     */

    virtual ~INxWindow(void) { }

    /**
     * Creates a new window.  Window creation is separate from
     * object instantiation so that window creation failures can
     * be properly reported.
     *
     * @return True if the window was successfully created.
     */

    virtual bool open(void) = 0;

    /**
     * Each implementation of INxWindow must provide a method to recover
     * the contained CWidgetControl instance.
     *
     * @return The contained CWidgetControl instance
     */

    virtual CWidgetControl *getWidgetControl(void) const = 0;

    /**
     * Synchronize the window with the NX server.  This function will delay
     * until the the NX server has caught up with all of the queued requests.
     * When this function returns, the state of the NX server will be the
     * same as the state of application.
     */

    virtual void synchronize(void) = 0;

    /**
     * Request the position and size information of the window. The values
     * will be returned asynchronously through the client callback method.
     * The GetPosition() method may than be called to obtain the positional
     * data as provided by the callback.
     *
     * @return OK on success; ERROR on failure with errno set appropriately.
     */

    virtual bool requestPosition(void) = 0;

    /**
     * Get the position of the window (as reported by the NX callback).
     *
     * @return The position.
     */

    virtual bool getPosition(FAR struct nxgl_point_s *pPos) = 0;

    /**
     * Get the size of the window (as reported by the NX callback).
     *
     * @param pSize The location to return window size.
     * @return True on success, false on any failure.
     */

    virtual bool getSize(FAR struct nxgl_size_s *pSize) = 0;

    /**
     * Set the position and size of the window.
     *
     * @param pPos The new position of the window.
     * @return True on success, false on any failure.
     */

    virtual bool setPosition(FAR const struct nxgl_point_s *pPos) = 0;

    /**
     * Set the size of the selected window.
     *
     * @param pSize The new size of the window.
     * @return True on success, false on any failure.
     */

    virtual bool setSize(FAR const struct nxgl_size_s *pSize) = 0;

    /**
     * Bring the window to the top of the display.
     *
     * @return True on success, false on any failure.
     */

    virtual bool raise(void) = 0;

    /**
     * Lower the window to the bottom of the display.
     *
     * @return True on success, false on any failure.
     */

    virtual bool lower(void) = 0;

    /**
     * Return true if the window is currently being displayed
     *
     * @return True if the window is visible
     */

    virtual bool isVisible(void) = 0;

    /**
     * Show a hidden window
     *
     * @return True on success, false on any failure.
     */

    virtual bool show(void) = 0;

    /**
     * Hide a visible window
     *
     * @return True on success, false on any failure.
     */

    virtual bool hide(void) = 0;

    /**
     * May be used to either (1) raise a window to the top of the display and
     * select modal behavior, or (2) disable modal behavior.
     *
     * @param enable True: enter modal state; False: leave modal state
     * @return True on success, false on any failure.
     */

    virtual bool modal(bool enable) = 0;

#ifdef CONFIG_NXTERM_NXKBDIN
    /**
     * Each window implementation also inherits from CCallback.  CCallback,
     * by default, forwards NX keyboard input to the various widgets residing
     * in the window. But NxTerm is a different usage model; In this case,
     * keyboard input needs to be directed to the NxTerm character driver.
     * This method can be used to enable (or disable) redirection of NX
     * keyboard input from the window widgets to the NxTerm
     *
     * @param handle.  The NXTERM handle.  If non-NULL, NX keyboard
     *    input will be directed to the NxTerm driver using this
     *    handle;  If NULL (the default), NX keyboard input will be
     *    directed to the widgets within the window.
     */

    virtual void redirectNxTerm(NXTERM handle) = 0;
#endif

    /**
     * Set an individual pixel in the window with the specified color.
     *
     * @param pPos The location of the pixel to be filled.
     * @param color The color to use in the fill.
     *
     * @return True on success; false on failure.
     */

    virtual bool setPixel(FAR const struct nxgl_point_s *pPos,
                          nxgl_mxpixel_t color) = 0;

    /**
     * Fill the specified rectangle in the window with the specified color.
     *
     * @param pRect The location to be filled.
     * @param color The color to use in the fill.
     *
     * @return True on success; false on failure.
     */

    virtual bool fill(FAR const struct nxgl_rect_s *pRect,
                      nxgl_mxpixel_t color) = 0;

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

    virtual void getRectangle(FAR const struct nxgl_rect_s *rect,
                              struct SBitmap *dest) = 0;

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

    virtual bool fillTrapezoid(FAR const struct nxgl_rect_s *pClip,
                               FAR const struct nxgl_trapezoid_s *pTrap,
                               nxgl_mxpixel_t color) = 0;

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

    virtual bool drawLine(FAR struct nxgl_vector_s *vector,
                          nxgl_coord_t width, nxgl_mxpixel_t color,
                          enum ELineCaps caps) = 0;

    /**
     * Draw a filled circle at the specified position, size, and color.
     *
     * @param center The window-relative coordinates of the circle center.
     * @param radius The radius of the rectangle in pixels.
     * @param color The color of the rectangle.
     */

    virtual bool drawFilledCircle(struct nxgl_point_s *center, nxgl_coord_t radius,
                                  nxgl_mxpixel_t color) = 0;

    /**
     * Move a rectangular region within the window.
     *
     * @param pRect Describes the rectangular region to move.
     * @param pOffset The offset to move the region.
     *
     * @return True on success; false on failure.
     */

    virtual bool move(FAR const struct nxgl_rect_s *pRect,
                      FAR const struct nxgl_point_s *pOffset) = 0;

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

    virtual bool bitmap(FAR const struct nxgl_rect_s *pDest,
                        FAR const void *pSrc,
                        FAR const struct nxgl_point_s *pOrigin,
                        unsigned int stride) = 0;
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_INXWINDOW_HXX
