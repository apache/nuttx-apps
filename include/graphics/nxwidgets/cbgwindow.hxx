/****************************************************************************
 * apps/include/graphics/nxwidgets/cbgwindow.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBGWINDOW_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBGWINDOW_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nx.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/ccallback.hxx"
#include "graphics/nxwidgets/inxwindow.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class INxWindow;
  struct SBitmap;

  /**
   * This class defines operations on a the NX background window.
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

  class CBgWindow : protected CCallback, public INxWindow
  {
  private:
    NXHANDLE        m_hNxServer;     /**< Handle to the NX server. */
    NXWINDOW        m_hWindow;       /**< Handle to the NX background window */
    CWidgetControl *m_widgetControl; /**< The controlling widget for the window */

  public:

    /**
     * Constructor.  Obtains the background window from server and wraps
     * the window as CBgWindow.  Creates an uninitialized instance of the
     * CBgWindow object.  The open() method must be called to initialize
     * the instance.
     *
     * The general steps to create any window include:
     * 1) Create a dumb CWigetControl instance
     * 2) Pass the dumb CWidgetControl instance to the window constructor
     *    that inherits from INxWindow.
     * 3) The window constructor call CWidgetControl methods to "smarten"
     *    the CWidgetControl instance with window-specific knowledge.
     * 4) Call the open() method on the window to display the window.
     * 5) After that, the fully smartend CWidgetControl instance can
     *    be used to generate additional widgets.
     * 6) After that, the fully smartened CWidgetControl instance can
     *    be used to generate additional widgets by passing it to the
     *    widget constructor
     *
     * @param hNxServer Handle to the NX server.
     * @param widgetControl Controlling widget for this window.
     */

    CBgWindow(NXHANDLE hNxServer, CWidgetControl *widgetControl);

    /**
     * Destructor.  Returns the background window to the server.
     */

    virtual ~CBgWindow(void);

    /**
     * Creates the new window.  Window creation is separate from
     * object instantiation so that failures can be reported.
     *
     * @return True if the window was successfully created.
     */

    bool open(void);

    /**
     * Each implementation of INxWindow must provide a method to recover
     * the contained CWidgetControl instance.
     *
     * @return The contained CWidgetControl instance
     */

    CWidgetControl *getWidgetControl(void) const;

    /**
     * Synchronize the window with the NX server.  This function will delay
     * until the the NX server has caught up with all of the queued requests.
     * When this function returns, the state of the NX server will be the
     * same as the state of the application.
     */

    inline void synchronize(void)
    {
      CCallback::synchronize(m_hWindow, CCallback::NX_RAWWINDOW);
    }

    /**
     * Request the position and size information of the window. The values
     * will be returned asynchronously through the client callback method.
     * The GetPosition() method may than be called to obtain the positional
     * data as provided by the callback.
     *
     * @return True on success, false on any failure.
     */

    bool requestPosition(void);

    /**
     * Get the position of the window (as reported by the NX callback). NOTE:
     * The background window is always positioned at {0,0}
     *
     * @return The position.
     */

    bool getPosition(FAR struct nxgl_point_s *pPos);

    /**
     * Get the size of the window (as reported by the NX callback).  NOTE:
     * The size of the background window is always the entire display.
     *
     * @return The size.
     */

    bool getSize(FAR struct nxgl_size_s *pSize);

    /**
     * Set the position and size of the window.
     *
     * @param pPos The new position of the window.
     * @return True on success, false on any failure.
     */

    bool setPosition(FAR const struct nxgl_point_s *pPos);

    /**
     * Set the size of the selected window.  NOTE:  The size of the
     * background window is always the entire display and cannot be
     * changed.
     *
     * @param pSize The new size of the window.
     * @return True on success, false on any failure.
     */

    bool setSize(FAR const struct nxgl_size_s *pSize);

    /**
     * Bring the window to the top of the display.  NOTE:  The background
     * window cannot be raised.
     *
     * @return Always returns false.
     */

    inline bool raise(void)
    {
      // The background cannot be raised

      return false;
    }

    /**
     * Lower the window to the bottom of the display.  NOTE:  The background
     * window is always at the bottom of the window hierarchy.
     *
     * @return Always returns false.
     */

    inline bool lower(void)
    {
      // The background cannot be lowered

      return false;
    }

    /**
     * Return true if the window is currently being displayed
     *
     * @return Always returns true.
     */

    inline bool isVisible(void)
    {
      // The background is always visible (although perhaps obscured)

      return true;
    }

    /**
     * Show a hidden window
     *
     * @return Always returns false.
     */

    inline bool show(void)
    {
      // The background is always visible (although perhaps obscured)

      return false;
    }

    /**
     * Hide a visible window
     *
     * @return Always returns false.
     */

    inline bool hide(void)
    {
      // The background cannot be hidden

      return false;
    }

    /**
     * May be used to either (1) raise a window to the top of the display and
     * select modal behavior, or (2) disable modal behavior.  NOTE:  The
     * background cannot be a modal window.
     *
     * @param enable True: enter modal state; False: leave modal state
     * @return Always returns false.
     */

    inline bool modal(bool enable)
    {
      // The background cannot a modal window

      return false;
    }

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

    inline void redirectNxTerm(NXTERM handle)
    {
      setNxTerm(handle);
    }
#endif

    /**
     * Set an individual pixel in the window with the specified color.
     *
     * @param pPos The location of the pixel to be filled.
     * @param color The color to use in the fill.
     *
     * @return True on success; false on failure.
     */

    bool setPixel(FAR const struct nxgl_point_s *pPos,
                  nxgl_mxpixel_t color);

    /**
     * Fill the specified rectangle in the window with the specified color.
     *
     * @param pRect The location to be filled.
     * @param color The color to use in the fill.
     *
     * @return True on success; false on failure.
     */

    bool fill(FAR const struct nxgl_rect_s *pRect,
              nxgl_mxpixel_t color);

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

    void getRectangle(FAR const struct nxgl_rect_s *rect, struct SBitmap *dest);

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

    bool fillTrapezoid(FAR const struct nxgl_rect_s *pClip,
                       FAR const struct nxgl_trapezoid_s *pTrap,
                       nxgl_mxpixel_t color);

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

    bool drawLine(FAR struct nxgl_vector_s *vector,
                  nxgl_coord_t width, nxgl_mxpixel_t color,
                  enum ELineCaps caps);

    /**
     * Draw a filled circle at the specified position, size, and color.
     *
     * @param center The window-relative coordinates of the circle center.
     * @param radius The radius of the rectangle in pixels.
     * @param color The color of the rectangle.
     */

    bool drawFilledCircle(struct nxgl_point_s *center, nxgl_coord_t radius,
                          nxgl_mxpixel_t color);

    /**
     * Move a rectangular region within the window.
     *
     * @param pRect Describes the rectangular region to move.
     * @param pOffset The offset to move the region.
     *
     * @return True on success; false on failure.
     */

    bool move(FAR const struct nxgl_rect_s *pRect,
              FAR const struct nxgl_point_s *pOffset);

    /**
     * Copy a rectangular region of a larger image into the rectangle in the
     * specified window.
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

    bool bitmap(FAR const struct nxgl_rect_s *pDest,
                FAR const void *pSrc,
                FAR const struct nxgl_point_s *pOrigin,
                unsigned int stride);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBGWINDOW_HXX
