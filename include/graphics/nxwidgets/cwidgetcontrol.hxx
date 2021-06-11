/****************************************************************************
 * apps/include/graphics/nxwidgets/cwidgetcontrol.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWIDGETCONTROLT_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWIDGETCONTROLT_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"
#include "graphics/nxwidgets/cnxwidget.hxx"
#include "graphics/nxwidgets/crect.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cwindoweventhandler.hxx"
#include "graphics/nxwidgets/cwindoweventhandlerlist.hxx"
#include "graphics/nxwidgets/tnxarray.hxx"

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class INxWindow;
  class CNxWidget;

  /**
   * Class providing a top-level widget and an interface to the CWidgetControl
   * widget hierarchy.
   *
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

  class CWidgetControl
    {
  protected:
    /**
     * Structure holding the status of the Mouse or Touchscreen.  There must
     * be one instance of this structure per window instance.  The
     * content of this structure is update by the CGraphicsPort on each
     * NX mouse callback
     */

#ifdef CONFIG_NX_XYINPUT
    struct SXYInput
    {
      // A touchscreen has no buttons.  However, the convention is that
      // touchscreen contacts are reported with the LEFT button pressed.
      // The loss of contct is reported with no buttons pressed.

#if 0 // Center and right buttons are not used
      // But only a mouse has center and right buttons

      uint16_t        leftPressed      : 1;  /**< Left button pressed (or
                                                  touchscreen contact) */
      uint16_t        centerPressed    : 1;  /**< Center button pressed (not
                                                  used with touchscreen) */
      uint16_t        rightPressed     : 1;  /**< Right button pressed (not
                                                  used with touchscreen) */
      uint16_t        leftHeld         : 1;  /**< Left button held down (or
                                                  touchscreen contact) */
      uint16_t        centerHeld       : 1;  /**< Center button held down
                                                  (not used with touchscreen) */
      uint16_t        rightHeld        : 1;  /**< Right button held down
                                                  (not used with touchscreen) */
      uint16_t        leftDrag         : 1;  /**< Left button held down (or
                                                  touchscreen contact) */
      uint16_t        centerDrag       : 1;  /**< Center button held down (or
                                                  touchscreen contact) */
      uint16_t        rightDrag        : 1;  /**< Right button held down (or
                                                  touchscreen contact) */
      uint16_t        leftReleased     : 1;  /**< Left button release (or
                                                  loss of touchscreen contact) */
      uint16_t        centerReleased   : 1;  /**< Center button release (or
                                                  loss of touchscreen contact) */
      uint16_t        rightReleased    : 1;  /**< Right button release (or
                                                  loss of touchscreen contact) */
      uint16_t        doubleClick      : 1;  /**< Left button double click */
      uint16_t        unused           : 3;  /**< Padding bits */
#else
      uint8_t         leftPressed      : 1;  /**< Left button pressed (or
                                                  touchscreen contact) */
      uint8_t         leftHeld         : 1;  /**< Left button held down (or
                                                  touchscreen contact) */
      uint8_t         leftDrag         : 1;  /**< Left button held down (or
                                                  touchscreen contact) */
      uint8_t         leftReleased     : 1;  /**< Left button release (or
                                                  loss of touchscreen contact) */
      uint8_t         doubleClick      : 1;  /**< Left button double click */
      uint8_t         unused           : 3;  /**< Padding bits */
#endif

      // These are attributes common to both touchscreen and mouse input devices

      nxgl_coord_t    x;                     /**< Current X coordinate of
                                                  the mouse/touch */
      nxgl_coord_t    y;                     /**< Current Y coordinate of
                                                  the mouse/touch */
      nxgl_coord_t    lastX;                 /**< X coordinate of the mouse
                                                  at the previous poll */
      nxgl_coord_t    lastY;                 /**< Y coordinate of the mouse
                                                  at the previous poll */
      struct timespec leftPressTime;         /**< Time the left button was
                                                  pressed */
      struct timespec leftReleaseTime;       /**< Time the left button was
                                                  released */
    };
#endif

    /**
     * State data
     */

    CGraphicsPort              *m_port;           /**< The graphics port
                                                       that is used for
                                                       drawing on this window */
    TNxArray<CNxWidget*>        m_deleteQueue;    /**< Array of widgets
                                                       awaiting deletion. */
    TNxArray<CNxWidget*>        m_widgets;        /**< List of controlled
                                                       widgets. */
    volatile bool               m_haveGeometry;   /**< True: indicates that we
                                                       have valid geometry data. */
#ifdef CONFIG_NXWIDGET_EVENTWAIT
    bool                        m_waiting;        /**< True: External logic waiting for
                                                       window event */
    sem_t                       m_waitSem;        /**< External loops waits for
                                                       events on this semaphore */
#endif

    /**
     * I/O
     */

#ifdef CONFIG_NX_XYINPUT
    struct SXYInput             m_xyinput;        /**< Current XY input
                                                       device state */
#endif
    CNxWidget                  *m_clickedWidget;  /**< Pointer to the widget
                                                       that is clicked. */
    CNxWidget                  *m_focusedWidget;  /**< Pointer to the widget
                                                       that received keyboard
                                                       input. */
    uint8_t                     m_kbdbuf[CONFIG_NXWIDGETS_KBDBUFFER_SIZE];
    uint8_t                     m_nCh;            /**< Number of buffered
                                                       keyboard characters */
    uint8_t                     m_controls[CONFIG_NXWIDGETS_CURSORCONTROL_SIZE];
    uint8_t                     m_nCc;            /**< Number of buffered
                                                       cursor controls */
    /**
     * The following were picked off from the position callback.
     */

    NXHANDLE                    m_hWindow;        /**< Handle to the NX window */
    struct nxgl_size_s          m_size;           /**< Size of the window */
    struct nxgl_point_s         m_pos;            /**< Position in display space */
    struct nxgl_rect_s          m_bounds;         /**< Size of the display */
    sem_t                       m_geoSem;         /**< Posted when geometry is valid */
    sem_t                       m_boundsSem;      /**< Posted when bounds are valid */
    CWindowEventHandlerList     m_eventHandlers;  /**< List of event handlers. */

    /**
     * Style
     */

    CWidgetStyle                m_style;          /**< Default style used by all
                                                       widgets in the window. */

    /**
     * Copy a widget style
     *
     * @param dest The destination style
     * @param src The source to use
     */

    void copyWidgetStyle(CWidgetStyle *dest, const CWidgetStyle *src);

    /**
     * Return the elapsed time in millisconds
     *
     * @param startTime A time in the past from which to compute the elapsed time.
     * @return The elapsed time since startTime
     */

    uint32_t elapsedTime(FAR const struct timespec *startTime);

    /**
     * Pass clicks to the widget hierarchy.  Closes the context menu if
     * the clicked widget is not the context menu.  If a single widget
     * is supplied, only that widget is sent the click.
     *
     * @param x Click xcoordinate.
     * @param y Click ycoordinate.
     * @param widget. Specific widget to poll.  Use NULL to run the
     *    all widgets in the window.
     * @return True means an interesting mouse event occurred
     */

    bool handleLeftClick(nxgl_coord_t x, nxgl_coord_t y, CNxWidget *widget);

    /**
     * Get the index of the specified controlled widget.
     *
     * @param widget The widget to get the index of.
     * @return The index of the widget.  -1 if the widget is not found.
     */

    const int getWidgetIndex(const CNxWidget *widget) const;

    /**
     * Delete any widgets in the deletion queue.
     */

    void processDeleteQueue(void);

    /**
     * Process mouse/touchscreen events and send throughout the hierarchy.
     *
     * @param widget.  Specific widget to poll.  Use NULL to run the
     *    all widgets in the window.
     * @return True means a mouse event occurred
     */

    bool pollMouseEvents(CNxWidget* widget);

    /**
     * Process keypad events and send throughout the hierarchy.
     *
     * @return True means a keyboard event occurred
     */

    bool pollKeyboardEvents(void);

    /**
     * Process cursor control events and send throughout the hierarchy.
     *
     * @return True means a cursor control event was processes
     */

    bool pollCursorControlEvents(void);

#ifdef CONFIG_NXWIDGET_EVENTWAIT
    /**
     * Wake up and external logic that is waiting for a window event.
     */

    void postWindowEvent(void);
#endif

    /**
     * Take the geometry semaphore (handling signal interruptions)
     */

    void takeGeoSem(void);

    /**
     * Give the geometry semaphore
     */

    inline void giveGeoSem(void)
    {
      sem_post(&m_geoSem);
    }

    /**
     * Check if geometry data is available.  If not, [re-]request the
     * geometry data and wait for it to become valid.
     * CAREFUL:  This assumes that if we already have geometry data, then
     * it is valid.  This might not be true if the size position was
     * recently changed.
     */

    void waitGeoData(void);

    /**
     * Take the bounds semaphore (handling signal interruptions)
     */

    void takeBoundsSem(void);

    /**
     * Give the bounds semaphore
     */

    inline void giveBoundsSem(void)
    {
      sem_post(&m_boundsSem);
    }

    /**
     * Wait for bounds data
     */

    inline void waitBoundsData(void)
    {
      takeBoundsSem();
      giveBoundsSem();
    }

#ifdef CONFIG_NX_XYINPUT
    /**
     * Clear all mouse events
     */

    void clearMouseEvents(void);
#endif

  public:

    /**
     * Constructor
     *
     * @param style The default style that all widgets on this display
     *   should use.  If this is not specified, the widget will use the
     *   values stored in the defaultCWidgetStyle object.
     */

     CWidgetControl(FAR const CWidgetStyle *style = (const CWidgetStyle *)NULL);

    /**
     * Destructor.
     */

    virtual ~CWidgetControl(void);

#ifdef CONFIG_NXWIDGET_EVENTWAIT
    /**
     * Wait for an interesting window event to occur (like a mouse or keyboard event)
     * Caller's should exercise care to assure that the test for waiting and this
     * call are "atomic" .. perhaps by locking the scheduler like:
     *
     *  sched_lock();
     *  <check for events>
     *  if (<no interesting events>)
     *    {
     *      window->waitForWindowEvent();
     *    }
     *  sched_unlock();
     */

    void waitForWindowEvent(void);
#endif

#ifdef CONFIG_NXWIDGET_EVENTWAIT
    /**
     * Is external logic awaiting for a window event?
     *
     * @return True if the widget if external logic is waiting.
     */

    inline const bool isWaiting(void) const
    {
      return m_waiting;
    }
#endif

    /**
     * Run all code that needs to take place on a periodic basis.
     * This method normally called externally... either periodically
     * or when a window event is detected.  If CONFIG_NXWIDGET_EVENTWAIT
     * is defined, then external logic want call waitWindow event and
     * when awakened, they should call this function.  As an example:
     *
     *   for (;;)
     *     {
     *       sched_lock(); // Make the sequence atomic
     *       if (!window->pollEvents(0))
     *         {
     *           window->waitWindowEvent();
     *         }
     *       sched_unlock();
     *     }
     *
     * This method is just a wrapper simply calls the followi.
     *
     *   processDeleteQueue()
     *   pollMouseEvents(widget)
     *   pollKeyboardEvents()
     *   pollCursorControlEvents()
     *
     * @param widget.  Specific widget to poll.  Use NULL to run through
     *    of the widgets in the window.
     * @return True means some interesting event occurred
     */

    bool pollEvents(CNxWidget *widget = (CNxWidget *)NULL);

    /**
     * Swaps the depth of the supplied widget.
     * This function presumes that all child widgets are screens.
     *
     * @param widget The widget to be depth-swapped.
     * @return True if the depth swap occurred.
     */

    bool swapWidgetDepth(CNxWidget *widget);

    /**
     * Add another widget to be managed by this control instance
     *
     * @param widget The widget to be controlled.
     */

    inline void addControlledWidget(CNxWidget* widget)
    {
      m_widgets.push_back(widget);
    }

    /**
     * Remove a controlled widget
     *
     * @param widget The widget to be removed
     */

    void removeControlledWidget(CNxWidget* widget);

    /**
     * Get the number of controlled widgets.
     *
     * @return The number of child widgets belonging to this widget.
     */

    inline const int getControlledWidgetCount(void) const
    {
      return m_widgets.size();
    }

    /**
     * Add a widget to the list of widgets to be deleted.
     * Must never be called by anything other than the framework itself.
     *
     * @param widget The widget to add to the delete queue.
     */

    void addToDeleteQueue(CNxWidget *widget);

    /**
     * Set the clicked widget pointer.  Note that this should not be
     * called by code other than within the CWidgetControl library itself.
     *
     * @param widget The new clicked widget.
     */

    void setClickedWidget(CNxWidget *widget);

    /**
     * Get the clicked widget pointer.
     *
     * @return Pointer to the clicked widget.
     */

    inline CNxWidget *getClickedWidget(void)
    {
      return m_clickedWidget;
    }

    /**
     * Set the focused widget that will receive keyboard input.
     *
     * @param widget The new focused widget.
     */

    void setFocusedWidget(CNxWidget *widget);

    /**
     * Reset the focused widget so that it will no longer receive keyboard input.
     *
     * @param widget The new focused widget.
     */

    void clearFocusedWidget(CNxWidget *widget)
    {
      if (widget == m_focusedWidget)
        {
          m_focusedWidget = (CNxWidget *)NULL;
        }
    }

    /**
     * Get the focused widget pointer.
     *
     * @return Pointer to the focused widget.
     */

    inline CNxWidget *getFocusedWidget(void)
    {
      return m_focusedWidget;
    }

    /**
     * Check for the occurrence of a double click.
     *
     * @return Pointer to the clicked widget.
     */

    inline bool doubleClick(void)
    {
#ifdef CONFIG_NX_XYINPUT
      return (bool)m_xyinput.doubleClick;
#else
      return false;
#endif
    }

    /**
     * Get the default widget style for this window.
     *
     * @param style.  The location to return the widget's style
     */

    inline void getWidgetStyle(CWidgetStyle *style)
    {
      copyWidgetStyle(style, &m_style);
    }

    /**
     * Set the default widget style for this window.
     *
     * @param style.  The new widget style to copy.
     */

    inline void setWidgetStyle(const CWidgetStyle *style)
    {
      copyWidgetStyle(&m_style, style);
    }

    /**
     * These remaining methods are used by the CCallback instance to
     * provide notifications of certain events.
     */

    /**
     * This event will occur when the position or size of the underlying
     * window occurs.
     *
     * @param hWindow The window handle that should be used to communicate
     *        with the window
     * @param pos The position of the window in the physical device space.
     * @param size The size of the window.
     * @param bounds The size of the underlying display (pixels x rows)
     */

    void geometryEvent(NXHANDLE hWindow,
                       const struct nxgl_size_s *size,
                       const struct nxgl_point_s *pos,
                       const struct nxgl_rect_s *bounds);

    /**
     * This event will occur when the a portion of the window that was
     * previously obscured is now exposed.
     *
     * @param nxRect The region in the window that must be redrawn.
     * @param more True means that more re-draw requests will follow
     */

    void redrawEvent(FAR const struct nxgl_rect_s *nxRect, bool more);

    /**
     * This event means that new mouse data is available for the window.
     *
     * @param pos The (x,y) position of the mouse.
     * @param buttons See NX_MOUSE_* definitions.
     */

    void newMouseEvent(FAR const struct nxgl_point_s *pos, uint8_t buttons);

#ifdef CONFIG_NX_KBD
   /**
    * This event means that keyboard/keypad data is available for the window.
    *
    * @param nCh The number of characters that are available in pStr[].
    * @param pStr The array of characters.
    */

    void newKeyboardEvent(uint8_t nCh, FAR const uint8_t *pStr);
#endif

   /**
    * This event is the response from nx_block (or nxtk_block). Those
    * blocking interfaces are used to assure that no further messages are
    * directed to the window. Receipt of the blocked callback signifies
    * that (1) there are no further pending events and (2) that the
    * window is now 'defunct' and will receive no further events.
    *
    * This event supports coordinated destruction of a window in multi-
    * user mode.  In multi-use mode, the client window logic must stay
    * intact until all of the queued callbacks are processed.  Then the
    * window may be safely closed.  Closing the window prior with pending
    * callbacks can lead to bad behavior when the callback is executed.
    *
    * @param arg - User provided argument (see nx_block or nxtk_block)
    */

   inline void windowBlocked(FAR void *arg)
   {
     m_eventHandlers.raiseBlockedEvent(arg);
   }

   /**
    * This event means that cursor control data is available for the window.
    *
    * @param cursorControl The cursor control code received.
    */

    void newCursorControlEvent(ECursorControl cursorControl);

    /**
     * Get the window handle reported on the first position callback.
     *
     * @return This function returns the window handle.
     */

    inline NXHANDLE getWindowHandle(void)
    {
      // Verify that we have the window handle

      if (m_hWindow == (NXHANDLE)0)
        {
          // The window handle is saved at the same time as the bounds

          waitBoundsData();
        }

      return m_hWindow;
    }

    /**
     * Get the window bounding box in physical display coordinates.  This
     * method may need to wait until bounds data is available.
     *
     * @return This function returns the window handle.
     */

    inline CRect getWindowBoundingBox(void)
    {
      waitBoundsData();
      return CRect(&m_bounds);
    }

    inline void getWindowBoundingBox(FAR struct nxgl_rect_s *bounds)
    {
      waitBoundsData();
      nxgl_rectcopy(bounds, &m_bounds);
    }

    /**
     * Get the position of the window (as reported by the last NX callback).  This
     * method may need to wait until geometry data is available.
     *
     * @return The position.
     */

    inline bool getWindowPosition(FAR struct nxgl_point_s *pos)
    {
      // Check if we already have geometry data available.  CAREFUL:  This
      // might refer to OLD geometry data if the position was recently
      // changed!

      waitGeoData();
      pos->x = m_pos.x;
      pos->y = m_pos.y;
      return true;
    }

    /**
     * Get the size of the window (as reported by the last NX callback).  This
     * method may need to wait until geometry data is available.
     *
     * @return The size.
     */

    inline bool getWindowSize(FAR struct nxgl_size_s *size)
    {
      // Check if we already have geometry data available.  CAREFUL:  This
      // might refer to OLD geometry data if the position was recently
      // changed!

      waitGeoData();
      size->h = m_size.h;
      size->w = m_size.w;
      return true;
    }

    /**
     * Get the width of the window (as reported by the last NX callback).  This
     * method may need to wait until geometry data is available.
     *
     * @return The size.
     */

    inline nxgl_coord_t getWindowWidth(void)
    {
      // Check if we already have geometry data available.  CAREFUL:  This
      // might refer to OLD geometry data if the position was recently
      // changed!

      waitGeoData();
      return m_size.w;
    }

    /**
     * Get the height of the window (as reported by the last NX callback).  This
     * method may need to wait until geometry data is available.
     *
     * @return The size.
     */

    inline nxgl_coord_t getWindowHeight(void)
    {
      // Check if we already have geometry data available.  CAREFUL:  This
      // might refer to OLD geometry data if the position was recently
      // changed!

      waitGeoData();
      return m_size.h;
    }

   /**
    * The creation sequence is:
    *
    * 1) Create a dumb CWigetControl instance
    * 2) Pass the dumb CWidgetControl instance to the window constructor
    *    that inherits from INxWindow.
    * 3) The call this method with the static_cast to INxWindow to,
    *    finally, create the CGraphicsPort for this window.
    * 4) After that, the fully smartend CWidgetControl instance can
    *    be used to generate additional widgets.
    *
    * @param window The instance of INxWindow needed to construct the
    *   CGraphicsPort instance
    */

   bool createGraphicsPort(INxWindow *window);

   /**
    * Get the CGraphicsPort instance for drawing on this window
    */

   inline CGraphicsPort *getGraphicsPort(void)
   {
     return m_port;
   }

   /**
    * Adds a window event handler.  The window handler will receive
    * notification all NX events received by this window\.
    *
    * @param eventHandler A pointer to the event handler.
    */

   inline void addWindowEventHandler(CWindowEventHandler *eventHandler)
   {
     m_eventHandlers.addWindowEventHandler(eventHandler);
   }

   /**
    * Remove a window event handler.
    *
    * @param eventHandler A pointer to the event handler to remove.
    */

   inline void removeWindowEventHandler(CWindowEventHandler *eventHandler)
   {
     m_eventHandlers.removeWindowEventHandler(eventHandler);
   }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CWIDGETCONTROLT_HXX
