/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cwindow.hxx
// Represents one window instance
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOW_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOW_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <mqueue.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cnxtoolbar.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

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

#define MENU_BUTTON      0  // First on left
#define DELETE_BUTTON    1  // First on right
#define RESIZE_BUTTON    2
#define MINIMIZE_BUTTON  3
#define NTOOLBAR_BUTTONS 4

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CImage;                                // Forward reference
  class  CLabel;                                // Forward reference
  struct SRlePaletteBitmap;                     // Forward reference
}

namespace Twm4Nx
{
  class  CIconWidget;                           // Forward reference
  class  CIconMgr;                              // Forward reference
  class  CWindow;                               // Forward reference
  struct SMenuRoot;                             // Forward reference
  struct SMenuItem;                             // Forward reference

  // The CWindow class implements a standard, framed window with a toolbar
  // containing the standard buttons and the window title.

  class CWindow : protected NXWidgets::CWidgetEventHandler, public CTwm4NxEvent
  {
    private:
      CTwm4Nx                    *m_twm4nx;     /**< Cached Twm4Nx session */
      mqd_t                       m_eventq;     /**< NxWidget event message queue */

      // Primary Window

      FAR char                   *m_name;       /**< Name of the window */
      FAR NXWidgets::CNxTkWindow *m_nxWin;      /**< The contained NX primary window */
      uint16_t                    m_zoom;       /**< Window zoom: ZOOM_NONE or EVENT_RESIZE_* */
      bool                        m_modal;      /**< Window zoom: ZOOM_NONE or EVENT_RESIZE_* */

      // Icon

      FAR NXWidgets::CRlePaletteBitmap *m_iconBitMap; /**< The icon image */

      FAR CIconWidget            *m_iconWidget; /**< The icon widget */
      FAR CIconMgr               *m_iconMgr;    /**< Pointer to it if this is an icon manager */
      bool                        m_isIconMgr;  /**< This is an icon manager window */
      bool                        m_iconMoved;  /**< User explicitly moved the icon. */
      bool                        m_iconOn;     /**< Icon is visible. */
      bool                        m_iconified;  /**< Is the window an icon now ? */

      // Toolbar

      FAR NXWidgets::CNxToolbar  *m_toolbar;    /**< The tool bar sub-window */
      FAR NXWidgets::CLabel      *m_tbTitle;    /**< Toolbar title widget */
      nxgl_coord_t                m_tbHeight;   /**< Height of the toolbar */
      nxgl_coord_t                m_tbLeftX;    /**< Rightmost position of left buttons */
      nxgl_coord_t                m_tbRightX;   /**< Leftmost position of right buttons */

      // List of all toolbar button images

      FAR NXWidgets::CImage      *m_tbButtons[NTOOLBAR_BUTTONS];

      // Dragging

      struct nxgl_point_s         m_dragOffset; /**< Offset from mouse to window origin */
      struct nxgl_size_s          m_dragCSize;  /**< The grab cursor size */
      bool                        m_drag;       /**< Drag in-progress */

      /**
       * Create the main window
       *
       * @param winsize   The initial window size
       * @param winpos    The initial window position
       */

      bool createMainWindow(FAR const nxgl_size_s *winsize,
                            FAR const nxgl_point_s *winpos);

      /**
       * Calculate the height of the tool bar
       */

      bool getToolbarHeight(FAR const char *name);

      /**
       * Create all toolbar buttons
       */

      bool createToolbarButtons(void);

      /**
       * Add buttons and title widgets to the tool bar
       *
       * @param name The name to use for the toolbar title
       */

      bool createToolbarTitle(FAR const char *name);

      /**
       * Create the tool bar
       */

      bool createToolbar(void);

      /**
       * Update the toolbar layout, resizing the title text window and
       * repositioning all windows on the toolbar.
       */

      bool updateToolbarLayout(void);

      /**
       * Disable widget drawing and widget events.
       */

      bool disableWidgets(void);

      /**
       * Enable widget drawing and widget events.
       */

      bool enableWidgets(void);

      /**
       * After the toolbar was grabbed, it may be dragged then dropped, or it
       * may be simply "un-grabbed". Both cases are handled here.
       *
       * NOTE: Unlike the other event handlers, this does NOT override any
       * virtual event handling methods.  It just combines some common event-
       * handling logic.
       *
       * @param e The event data.
       */

      void handleUngrabEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Override the mouse button drag event.
       *
       * @param e The event data.
       */

      void handleDragEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Override a drop event, triggered when the widget has been dragged-and-dropped.
       *
       * @param e The event data.
       */

      void handleDropEvent(const NXWidgets::CWidgetEventArgs &e);

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
       * @param name      The the name of the window (and its icon)
       * @param pos       The initial position of the window
       * @param size      The initial size of the window
       * @param sbitmap   The Icon bitmap image
       * @param isIconMgr Flag to tell if this is an icon manager window
       * @param iconMgr   Pointer to icon manager instance
       * @param noToolbar True: Don't add Title Bar
       * @return True if the window was successfully initialize; false on
       *   any failure,
       */

      bool initialize(FAR const char *name,
                      FAR const struct nxgl_point_s *pos,
                      FAR const struct nxgl_size_s *size,
                      FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                      bool isIconMgr, FAR CIconMgr *iconMgr, bool noToolbar);

      /**
       * Get the widget control instance needed to support application drawing
       * into the window.
       */

      inline FAR NXWidgets::CWidgetControl *getWidgetControl()
      {
        return m_nxWin->getWidgetControl();
      }

      /**
       * Get the name of the window
       */

      inline FAR const char *getWindowName(void)
      {
        return m_name;
      }

      /**
       * Return true if this is an Icon Manager Window
       */

      inline bool isIconMgr(void)
      {
        return m_isIconMgr;
      }

      /**
       * Return the Icon Manager Window instance
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
       * Set the size of the primary window.  This is useful only
       * for oapplications that need to control the drawing area.
       *
       * @param size New primary window size
       */

      inline bool setWindowSize(FAR const struct nxgl_size_s *size)
      {
        return m_nxWin->setSize(size);
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
       */

      bool getFrameSize(FAR struct nxgl_size_s *framesize);

      /**
       * Update the window frame after a resize operation (includes the toolbar
       * and user window)
       *
       * @param size The new window frame size
       * @param pos  The frame location which may also have changed
       */

      bool resizeFrame(FAR const struct nxgl_size_s *framesize,
                       FAR struct nxgl_point_s *framepos);

      /**
       * Get the raw frame position (accounting for toolbar and frame)
       *
       * @param framepos Location to return the window frame position
       */

      bool getFramePosition(FAR struct nxgl_point_s *framepos);

      /**
       * Set the raw frame position (accounting for toolbar and frame)
       *
       * @param framepos The new raw window position
       */

      bool setFramePosition(FAR const struct nxgl_point_s *framepos);

      /* Minimize (iconify) the window */

      void iconify(void);

      /**
       * De-iconify the window
       */

      void deIconify(void);

      /**
       * Is the window iconified?
       */

      inline bool isIconified(void)
      {
        return m_iconified;
      }

      /**
       * Has the Icon moved?
       */

      inline bool hasIconMoved(void)
      {
        return m_iconMoved;
      }

      /**
       * Set Icon moved
       */

      inline void setIconMoved(bool moved)
      {
        m_iconMoved = moved;
      }

      /**
       * Get the size of the icon window associated with this application
       * window.  This is needed for placement of the icon on the background
       * window.
       *
       * @param size Location to return the icon window size
       */

      inline void getIconWidgetSize(FAR struct nxgl_size_s &size)
      {
        m_iconWidget->getSize(size);
      }

      /**
       * Get the current position of the icon window associated with the
       * application window. This is needed for placement of the icon on
       * the background window.
       *
       * @param pos Location to return the icon window position
       */

      inline void getIconWidgetPosition(FAR struct nxgl_point_s &pos)
      {
        m_iconWidget->getPos(pos);
      }

      /**
       * Set the new position of the icon window associated with the
       * application window. This is needed for placement of the icon on the
       * background window.
       *
       * @param pos The new location of the icon window
       */

      inline bool setIconWindowPosition(FAR const struct nxgl_point_s &pos)
      {
        return m_iconWidget->resize(pos.x, pos.y);
      }

      /**
       * Get zoom
       */

      inline uint16_t getZoom(void)
      {
        return m_zoom;
      }

      /**
       * Set zoom
       *
       * @param zoom The new zoom setting
       */

      inline void setZoom(uint16_t zoom)
      {
        m_zoom = zoom;
      }

      /**
       * Check for widget-related toolbar events, typically button presses.
       * This is called by event handling logic for events that require
       * detection of widget events.
       */

      inline bool pollToolbarEvents(void)
      {
        NXWidgets::CWidgetControl *control = m_toolbar->getWidgetControl();
        return control->pollEvents();
      }

      /**
       * Handle Twm4Nx events.
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);
  };
}

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CWINDOW_HXX
