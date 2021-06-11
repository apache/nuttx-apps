/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cwindow.hxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
/////////////////////////////////////////////////////////////////////////////

// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOW_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOW_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <mqueue.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxterm.h>

#include "graphics/nxwidgets/cnxtkwindow.hxx"
#include "graphics/nxwidgets/cnxtoolbar.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/ciconwidget.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

/**
 * Toolbar Icons.  The Title bar contains (from left to right):
 *
 * 1. Menu button
 * 2. Window title (text)
 * 3. Minimize (Iconify) button
 * 4. Resize button
 * 5. Delete button
 *
 * There is no focus indicator
 */

#define MENU_BUTTON               0  // First on left
#define DELETE_BUTTON             1  // First on right
#define RESIZE_BUTTON             2
#define MINIMIZE_BUTTON           3
#define NTOOLBAR_BUTTONS          4

// Windows may be customized with the flags parameter to the constructor.
// These are the bits fields that may be OR'ed together in the flags.

#define NO_TOOLBAR                (NTOOLBAR_BUTTONS + 0)
#define ICONMGR_WINDOW            (NTOOLBAR_BUTTONS + 1)
#define MENU_WINDOW               (NTOOLBAR_BUTTONS + 2)
#define HIDDEN_WINDOW             (NTOOLBAR_BUTTONS + 3)

#define WFLAGS_NO_MENU_BUTTON     (1 << MENU_BUTTON)
#define WFLAGS_NO_DELETE_BUTTON   (1 << DELETE_BUTTON)
#define WFLAGS_NO_RESIZE_BUTTON   (1 << RESIZE_BUTTON)
#define WFLAGS_NO_MINIMIZE_BUTTON (1 << MINIMIZE_BUTTON)
#define WFLAGS_NO_TOOLBAR         (1 << NO_TOOLBAR)
#define WFLAGS_ICONMGR            (1 << ICONMGR_WINDOW)
#define WFLAGS_MENU               (1 << MENU_WINDOW)
#define WFLAGS_HIDDEN             (1 << HIDDEN_WINDOW)

#define WFLAGS_HAVE_MENU_BUTTON(f)     (((f) & WFLAGS_NO_MENU_BUTTON) == 0)
#define WFLAGS_HAVE_DELETE_BUTTON(f)   (((f) & WFLAGS_NO_DELETE_BUTTON) == 0)
#define WFLAGS_HAVE_RESIZE_BUTTON(f)   (((f) & WFLAGS_NO_RESIZE_BUTTON) == 0)
#define WFLAGS_HAVE_MINIMIZE_BUTTON(f) (((f) & WFLAGS_NO_MINIMIZE_BUTTON) == 0)
#define WFLAGS_HAVE_TOOLBAR(f)         (((f) & WFLAGS_NO_TOOLBAR) == 0)
#define WFLAGS_IS_ICONMGR(f)           (((f) & WFLAGS_ICONMGR) != 0)
#define WFLAGS_IS_MENU(f)              (((f) & WFLAGS_MENU) != 0)
#define WFLAGS_IS_HIDDEN(f)            (((f) & WFLAGS_HIDDEN) != 0)

// Buttons can be disabled temporarily while in certain absorbing states
// (such as resizing the window).

#define DISABLE_NO_BUTTONS        (0)
#define DISABLE_MENU_BUTTON       (1 << MENU_BUTTON)
#define DISABLE_DELETE_BUTTON     (1 << DELETE_BUTTON)
#define DISABLE_RESIZE_BUTTON     (1 << RESIZE_BUTTON)
#define DISABLE_MINIMIZE_BUTTON   (1 << MINIMIZE_BUTTON)

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CNxString;                              // Forward reference
  class  CImage;                                 // Forward reference
  class  CLabel;                                 // Forward reference
  struct SRlePaletteBitmap;                      // Forward reference
}

namespace Twm4Nx
{
  class  CIconWidget;                            // Forward reference
  class  CIconMgr;                               // Forward reference
  class  CWindow;                                // Forward reference
  struct SMenuRoot;                              // Forward reference
  struct SMenuItem;                              // Forward reference

  // The CWindow class implements a standard, framed window with a toolbar
  // containing the standard buttons and the window title.

  class CWindow : protected NXWidgets::CWidgetEventHandler,
                  protected IEventTap,
                  public CTwm4NxEvent
  {
    private:
      CTwm4Nx                    *m_twm4nx;      /**< Cached Twm4Nx session */
      mqd_t                       m_eventq;      /**< NxWidget event message queue */

      // Primary Window

      NXWidgets::CNxString        m_name;        /**< Name of the window */
      FAR NXWidgets::CNxTkWindow *m_nxWin;       /**< The contained NX primary window */
      FAR CWindowEvent           *m_windowEvent; /**< Cached window event reference */
      FAR void                   *m_eventObj;    /**< Object reference that accompanies events */
      nxgl_coord_t                m_minWidth;    /**< The minimum width of the window */
      struct SAppEvents           m_appEvents;   /**< Application event information */
      bool                        m_modal;       /**< Window is in modal state */

      // Icon

      FAR NXWidgets::CRlePaletteBitmap *m_iconBitMap; /**< The icon image */

      FAR CIconWidget            *m_iconWidget;  /**< The icon widget */
      FAR CIconMgr               *m_iconMgr;     /**< Pointer to it if this is an icon manager */
      bool                        m_iconified;   /**< Is the window an icon now ? */

      // Toolbar

      FAR NXWidgets::CNxToolbar  *m_toolbar;     /**< The tool bar sub-window */
      FAR NXWidgets::CWidgetStyle m_tbStyle;     /**< The tool bar widget style */
      FAR NXWidgets::CLabel      *m_tbTitle;     /**< Toolbar title widget */
      nxgl_coord_t                m_tbHeight;    /**< Height of the toolbar */
      nxgl_coord_t                m_tbLeftX;     /**< Rightmost position of left buttons */
      nxgl_coord_t                m_tbRightX;    /**< Leftmost position of right buttons */
      uint8_t                     m_tbFlags;     /**< Toolbar button customizations */
      uint8_t                     m_tbDisables;  /**< Toolbar button disables */

      // List of all toolbar button images

      FAR NXWidgets::CImage      *m_tbButtons[NTOOLBAR_BUTTONS];

      // Dragging

      struct nxgl_point_s         m_dragPos;     /**< Last reported mouse position */
      struct nxgl_size_s          m_dragCSize;   /**< The grab cursor size */
      bool                        m_dragging;    /**< True: Drag in-progress */
      volatile bool               m_clicked;     /**< True: Mouse left button is clicked */

      /**
       * Create the main window
       *
       * Initially, the application window will generate no window-related events
       * (redraw, mouse/touchscreen, keyboard input, etc.).  After creating the
       * window, the user may call the configureEvents() method to select the
       * eventIDs of the events to be generated.
       *
       * @param winsize   The initial window size
       * @param winpos    The initial window position
       * @param flags Toolbar customizations see WFLAGS_NO_* definitions
       */

      bool createMainWindow(FAR const nxgl_size_s *winsize,
                            FAR const nxgl_point_s *winpos,
                            uint8_t flags);

      /**
       * Calculate the height of the tool bar
       */

      bool getToolbarHeight(FAR const NXWidgets::CNxString &name);

      /**
       * Create all toolbar buttons
       *
       * @param flags Toolbar customizations see WFLAGS_NO_* definitions
       * @return True if the window was successfully initialize; false on
       *   any failure,
       */

      bool createToolbarButtons(uint8_t flags);

      /**
       * Add buttons and title widgets to the tool bar
       *
       * @param name The name to use for the toolbar title
       */

      bool createToolbarTitle(FAR const NXWidgets::CNxString &name);

      /**
       * Create the tool bar
       */

      bool createToolbar(void);

      /**
       * Fill the toolbar background color
       */

      bool fillToolbar(void);

      /**
       * Update the toolbar layout, resizing the title text window and
       * repositioning all windows on the toolbar.
       */

      bool updateToolbarLayout(void);

      /**
       * Disable toolbar widget drawing and widget events.
       */

      bool disableToolbarWidgets(void);

      /**
       * Enable toolbar widget drawing and widget events.
       */

      bool enableToolbarWidgets(void);

      /**
       * After the toolbar was grabbed, it may be dragged then dropped, or it
       * may be simply "un-grabbed". Both cases are handled here.
       *
       * NOTE: Unlike the other event handlers, this does NOT override any
       * virtual event handling methods.  It just combines some common event-
       * handling logic.
       *
       * @param x The mouse/touch X position.
       * @param y The mouse/touch y position.
       */

      void handleUngrabEvent(nxgl_coord_t x, nxgl_coord_t y);

      /**
       * Handle a mouse click event.
       *
       * @param e The event data.
       */

      void handleClickEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Override the virtual CWidgetEventHandler::handleReleaseEvent.  This
       * event will fire when the title widget is released.  isClicked()
       * will return false for the title widget.
       *
       * @param e The event data.
       */

      void handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Override the virtual CWidgetEventHandler::handleActionEvent.  This
       * event will fire when the image is released but before it has been
       * has been drawn.  isClicked() will return true for the appropriate
       * images.
       *
       * @param e The event data.
       */

      void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * This function is called when there is any movement of the mouse or
       * touch position that would indicate that the object is being moved.
       *
       * This function overrides the virtual IEventTap::moveEvent method.
       *
       * @param pos The current mouse/touch X/Y position.
       * @param arg The user-argument provided that accompanies the callback
       * @return True: if the movement event was processed; false it was
       *   ignored.  The event should be ignored if there is not actually
       *   a movement event in progress
       */

      bool moveEvent(FAR const struct nxgl_point_s &pos,
                     uintptr_t arg);

      /**
       * This function is called if the mouse left button is released or
       * if the touchscreen touch is lost.  This indicates that the
       * movement sequence is complete.
       *
       * This function overrides the virtual IEventTap::dropEvent method.
       *
       * @param pos The last mouse/touch X/Y position.
       * @param arg The user-argument provided that accompanies the callback
       * @return True: if the drop event was processed; false it was
       *   ignored.  The event should be ignored if there is not actually
       *   a movement event in progress
       */

      bool dropEvent(FAR const struct nxgl_point_s &pos,
                     uintptr_t arg);

      /**
       * Is the tap enabled?
       *
       * @param arg The user-argument provided that accompanies the callback
       * @return True: If the the tap is enabled.
       */

      bool isActive(uintptr_t arg);

      /**
       * Enable/disable dragging
       *
       * True is provided when (1) isActive() returns false, but (2) a mouse
       *   report with a left-click is received.
       * False is provided when (1) isActive() returns true, but (2) a mouse
       *   report without a left-click is received.
       *
       * In the latter is redundant since dropEvent() will be called immediately
       * afterward.
       *
       * @param pos.  The mouse position at the time of the click or release
       * @param enable.  True:  Enable dragging
       * @param arg The user-argument provided that accompanies the callback
       */

      void enableMovement(FAR const struct nxgl_point_s &pos,
                          bool enable, uintptr_t arg);

      /**
       * Handle the TOOLBAR_GRAB event.  That corresponds to a left
       * mouse click on the toolbar (other than on an icon)
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool toolbarGrab(FAR struct SEventMsg *eventmsg);

      /**
       * Handle the WINDOW_DRAG event.  That corresponds to a mouse
       * movement when the window is in a grabbed state.
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool windowDrag(FAR struct SEventMsg *eventmsg);

      /**
       * Handle the TOOLBAR_UNGRAB event.  The corresponds to a mouse
       * left button release while in the grabbed
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool toolbarUngrab(FAR struct SEventMsg *eventmsg);

      /**
       * Cleanup on failure or as part of the destructor
       */

      void cleanup(void);

    public:

      /**
       * CWindow Constructor
       *
       * @param twm4nx.  Twm4Nx session
       */

      CWindow(CTwm4Nx *twm4nx);

      /**
       * CWindow Destructor
       */

      ~CWindow(void);

      /**
       * CWindow Initializer (unlike the constructor, this may fail)
       *
       * The window is initialized with all application events disabled.
       * The CWindows::configureEvents() method may be called as a second
       * initialization step in order to enable application events.
       *
       * @param name      The the name of the window (and its icon)
       * @param pos       The initial position of the window
       * @param size      The initial size of the window
       * @param sbitmap   The Icon bitmap image.  null if no icon.
       * @param iconMgr   Pointer to icon manager instance.  To support
       *                  multiple Icon Managers.
       * @param flags     Toolbar customizations see WFLAGS_NO_* definitions
       * @return True if the window was successfully initialize; false on
       *   any failure,
       */

      bool initialize(FAR const NXWidgets::CNxString &name,
                      FAR const struct nxgl_point_s *pos,
                      FAR const struct nxgl_size_s *size,
                      FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                      FAR CIconMgr *iconMgr,  uint8_t flags);

      /**
       * Configure application window events.
       *
       * @param events Describes the application event configuration
       * @return True is returned on success
       */

      bool configureEvents(FAR const struct SAppEvents &events);

      /**
       * Register an IEventTap instance to provide callbacks when mouse
       * movement is received.  A mouse movement with the left button down
       * or a touchscreen touch movement are treated as a drag event.
       * Release of the mouse left button or loss of the touchscreen touch
       * is treated as a drop event.
       *
       * @param tapHandler A reference to the IEventTap callback interface.
       * @param arg The argument returned with the IEventTap callbacks.
       */

      inline void installEventTap(FAR IEventTap *tapHandler, uintptr_t arg)
      {
         m_windowEvent->installEventTap(tapHandler, arg);
      }

      /**
       * Return the installed event tap.  This is useful if you want to
       * install a different event tap, then restore the event tap returned
       * by this method when you are finished.
       *
       * @param tapHandler The location to return IEventTap callback interface.
       * @param arg The loation to return the IEventTap argument
       */

      inline void getEventTap(FAR IEventTap *&tapHandler, uintptr_t &arg)
      {
         m_windowEvent->getEventTap(tapHandler, arg);
      }

      /**
       * Synchronize the window with the NX server.  This function will delay
       * until the the NX server has caught up with all of the queued requests.
       * When this function returns, the state of the NX server will be the
       * same as the state of the application.
       */

      inline void synchronize(void)
      {
        m_nxWin->synchronize();
      }

      /**
       * Get the widget control instance needed to support application drawing
       * into the window.
       */

      inline FAR NXWidgets::CWidgetControl *getWidgetControl()
      {
        return m_nxWin->getWidgetControl();
      }

      /**
       * Get the raw window interface.  This is sometimes needed for connecting
       * the window to external NXWidgets applications.
       *
       * @return The raw window interface (NXWidgets::INxWindow)
       */

      inline NXWidgets::INxWindow *getNxWindow(void)
      {
        return static_cast<NXWidgets::INxWindow *>(m_nxWin);
      }

#ifdef CONFIG_NXTERM_NXKBDIN
    /**
     * By default, NX forwards keyboard input to the various widgets residing
     * in the window.  But NxTerm is a different usage model; In this case,
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
        m_nxWin->redirectNxTerm(handle);
      }
#endif

      /**
       * Get the name of the window
       */

      inline NXWidgets::CNxString getWindowName(void)
      {
        return m_name;
      }

      /**
       * Return true if this is an Icon Manager Window
       */

      inline bool isIconMgr(void)
      {
        return WFLAGS_IS_ICONMGR(m_tbFlags);
      }

      /**
       * Return the Icon Manager Window instance.  Supports multiple
       * Icon Managers.
       *
       * @return The Icon Manager to which this window belongs.
       */

      inline FAR CIconMgr *getIconMgr(void)
      {
        return m_iconMgr;
      }

      /**
       * Get the size of the primary window.  This is useful only
       * for applications that need to know about the drawing area.
       *
       * @param size Location to return the primary window size
       */

      inline bool getWindowSize(FAR struct nxgl_size_s *size)
      {
        return m_nxWin->getSize(size);
      }

      /**
       * Set the size of the primary window.  This is useful only
       * for applications that need to control the drawing area.
       *
       * This method is only usable from windows that have no
       * toolbar.  The the window has a toolbar, then changing the
       * width of the window will mess up the toolbar layout.  In
       * such cases, the better to use is resizeFrame() which will
       * update the toolbar geometry after the resize.
       *
       * @param size New primary window size
       */

      inline bool setWindowSize(FAR const struct nxgl_size_s *size)
      {
        return m_nxWin->setSize(size);
      }

      /**
       * Get the position of the primary window.  This is useful only
       * for applications that need to know about the drawing area.
       *
       * @param pos Location to return the primary window position
       */

      inline bool getWindowPosition(FAR struct nxgl_point_s *pos)
      {
        return m_nxWin->getPosition(pos);
      }

      /**
       * Set the position of the primary window.
       *
       * @param pos The new primary window position
       */

      inline bool setWindowPosition(FAR const struct nxgl_point_s *pos)
      {
        return m_nxWin->setPosition(pos);
      }

      /**
       * Get the height of the tool bar
       */

      inline nxgl_coord_t getToolbarHeight(void)
      {
        return m_tbHeight;
      }

      /**
       * Raise the window to the top of the hierarchy.
       */

      inline bool raiseWindow(void)
      {
        return m_nxWin->raise();
      }

      /**
       * Lower the window to the bottom of the hierarchy.
       */

      inline bool lowerWindow(void)
      {
        return m_nxWin->lower();
      }

      /**
       * Return true if the window is currently being displayed
       *
       * @return True if the window is visible
       */

      inline bool isWindowVisible(void)
      {
        return m_nxWin->isVisible();
      }

      /**
       * Show a hidden window
       *
       * @return True if the operation was successful
       */

      inline bool showWindow(void)
      {
        return m_nxWin->show();
      }

      /**
       * Hide a visible window
       *
       * @return True if the operation was successful
       */

      inline bool hideWindow(void)
      {
        return m_nxWin->hide();
      }

      /**
       * Convert the position of a primary window to the position of
       * the containing frame.
       */

      inline void windowToFramePos(FAR const struct nxgl_point_s *winpos,
                                   FAR struct nxgl_point_s *framepos)
      {
        framepos->x = winpos->x - CONFIG_NXTK_BORDERWIDTH;
        framepos->y = winpos->y - m_tbHeight - CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Convert the position of the containing frame to the position of the
       * primary window.
       */

      inline void frameToWindowPos(FAR const struct nxgl_point_s *framepos,
                                   FAR struct nxgl_point_s *winpos)
      {
        winpos->x = framepos->x + CONFIG_NXTK_BORDERWIDTH;
        winpos->y = framepos->y + m_tbHeight + CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Convert the size of a primary window to the size of the containing
       * frame.
       */

      inline void windowToFrameSize(FAR const struct nxgl_size_s *winsize,
                                    FAR struct nxgl_size_s *framesize)
      {
        framesize->w = winsize->w + 2 * CONFIG_NXTK_BORDERWIDTH;
        framesize->h = winsize->h + m_tbHeight + 2 * CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Convert the size of the containing frame to the size of the primary window to the size of the containing
       * frame.
       */

      inline void frameToWindowSize(FAR const struct nxgl_size_s *framesize,
                                    FAR struct nxgl_size_s *winsize)
      {
        winsize->w = framesize->w - 2 * CONFIG_NXTK_BORDERWIDTH;
        winsize->h = framesize->h - m_tbHeight - 2 * CONFIG_NXTK_BORDERWIDTH;
      }

      /**
       * Get the raw window size (including toolbar and frame)
       *
       * @param framesize Location to return the window frame size
       * @return True if the operation was successful
       */

      bool getFrameSize(FAR struct nxgl_size_s *framesize);

      /**
       * Update the window frame after a resize operation (includes the toolbar
       * and user window)
       *
       * @param frameSize The new window frame size
       * @param framePos  The frame location which may also have changed (NULL
       *   may be used to preserve the current position)
       * @return True if the operation was successful
       */

      bool resizeFrame(FAR const struct nxgl_size_s *frameSize,
                       FAR const struct nxgl_point_s *framePos);

      /**
       * Get the raw frame position (accounting for toolbar and frame)
       *
       * @param framepos Location to return the window frame position
       * @return True if the operation was successful
       */

      bool getFramePosition(FAR struct nxgl_point_s *framepos);

      /**
       * Set the raw frame position (accounting for toolbar and frame)
       *
       * @param framepos The new raw window position
       * @return True if the operation was successful
       */

      bool setFramePosition(FAR const struct nxgl_point_s *framepos);

      /**
       * Minimize (iconify) the window
       *
       * @return True if the operation was successful
       */

      bool iconify(void);

      /**
       * De-iconify the window
       *
       * @return True if the operation was successful
       */

      bool deIconify(void);

      /**
       * Is the window iconified?
       *
       * @return True if the operation was successful
       */

      inline bool isIconified(void)
      {
        return m_iconified;
      }

      /**
       * Check if this window has an Icon.  Menu windows, for examples, have
       * no icons.
       */

      inline bool hasIcon(void)
      {
        return (m_iconWidget != (FAR CIconWidget *)0);
      }

      /**
       * Get the size of the icon window associated with this application
       * window.  This is needed for placement of the icon on the background
       * window and for determining if the icon widget needs to be redraw
       * when the background is redrawn.
       *
       * @param size Location to return the icon window size
       * @return True if the icon size was returned
       */

      inline void getIconWidgetSize(FAR struct nxgl_size_s &size)
      {
        if (m_iconWidget != (FAR CIconWidget *)0)
          {
            m_iconWidget->getSize(size);
          }
        else
          {
            size.w = 0;
            size.h = 0;
          }
      }

      /**
       * Get the current position of the icon window associated with the
       * application window.  This is needed for determining if an icon
       * widgets needs to be redrawn when the background is redrawn.
       *
       * @param pos Location to return the icon window position
       * @return True if the icon position was returned
       */

      inline void getIconWidgetPosition(FAR struct nxgl_point_s &pos)
      {
        if (m_iconWidget != (FAR CIconWidget *)0)
          {
            m_iconWidget->getPos(pos);
          }
        else
          {
            pos.x = 0;
            pos.y = 0;
          }
      }

      /**
       * Set the new position of the icon window associated with the
       * application window.  This is needed for movement of the icon on the
       * background window.
       *
       * @param pos The new location of the icon window
       * @return True if the icon position was correctly set
       */

      inline bool setIconWidgetPosition(FAR const struct nxgl_point_s &pos)
      {
        bool success = false;
        if (m_iconWidget != (FAR CIconWidget *)0)
          {
            success = m_iconWidget->moveTo(pos.x, pos.y);
          }

        return success;
      }

      /**
       * Redraw the icon.
       */

      inline void redrawIcon(void)
      {
        if (m_iconWidget != (FAR CIconWidget *)0)
          {
            // Make sure that the icon is properly enabled, then redraw it

            m_iconWidget->enable();
            m_iconWidget->enableDrawing();
            m_iconWidget->setRaisesEvents(true);
            m_iconWidget->redraw();
          }
      }

      /**
       * Enable/disable toolbar buttons.  Buttons may need to be disabled
       * temporarily while in certain absorbing states (such as resizing the
       * window).
       *
       * @param disables The set of buttons to enable or disable See
       *   DISABLE_* definitions.
       */

      inline void disableToolbarButtons(uint8_t disables)
      {
        m_tbDisables = disables;
      }

      /**
       * Check for widget-related toolbar events, typically button presses.
       * This is called by event handling logic for events that require
       * detection of widget events.
       */

      inline bool pollToolbarEvents(void)
      {
        NXWidgets::CWidgetControl *control = m_toolbar->getWidgetControl();

       // pollEvents() returns true if any interesting event occurred.
       // false is not a failure.

        control->pollEvents();
        return true;
      }

      /**
       * Handle EVENT_WINDOW events.
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);
  };

  ///////////////////////////////////////////////////////////////////////////
  // Public Function Prototypes
  ///////////////////////////////////////////////////////////////////////////

  /**
   * Return the minimum width of the toolbar window.  If the window is
   * resized smaller than this width, then the items in the toolbar will
   * overlap.
   *
   * @param twm4nx The Twm4Nx session object
   * @param title The window title string
   * @param flags Window toolbar properties
   * @return The minimum recommended window width.
   */

  nxgl_coord_t minimumToolbarWidth(FAR CTwm4Nx *twm4nx,
                                   FAR const NXWidgets::CNxString &title,
                                   uint8_t flags);
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOW_HXX
