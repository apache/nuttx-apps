/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cresize.hxx
// Resize function externs
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
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CRESIZE_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CRESIZE_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CNxTkWindow;                             // Forward reference
  class  CLabel;                                  // Forward reference
}

namespace Twm4Nx
{
  class  CWindow;                                 // Forward reference
  struct SEventMsg;                               // Forward reference

  // The CResize class is, logically, a part of CWindow.  It is brought out
  // separately only to minimize the complexity of CWindows (and because it
  // matches the original partitioning of TWM).  The downside is that (1) CWindow
  // instances have to be passed un-necessarily, and (2) the precludes the
  // possibility of resizing two window simultaneous.  That latter is not
  // currently supported anyway.

  class CResize : protected NXWidgets::CWidgetEventHandler,
                  protected IEventTap,
                  public CTwm4NxEvent
  {
    private:

      CTwm4Nx                    *m_twm4nx;       /**< Cached Twm4Nx session */
      mqd_t                       m_eventq;       /**< NxWidget event message queue */
      FAR NXWidgets::CNxTkWindow *m_sizeWindow;   /**< The resize dimensions window */
      FAR NXWidgets::CLabel      *m_sizeLabel;    /**< Resize dimension label */
      FAR CWindow                *m_resizeWindow; /**< The window being resized */
      FAR IEventTap              *m_savedTap;     /**< Saved IEventTap */
      uintptr_t                   m_savedTapArg;  /**< Saved IEventTap argument */
      struct nxgl_point_s         m_lastPos;      /**< Last window position */
      struct nxgl_size_s          m_lastSize;     /**< Last window size */
      struct nxgl_point_s         m_mousePos;     /**< Last mouse position */
      bool                        m_resizing;     /**< Resize in progress */
      bool                        m_resized;      /**< The size has changed */
      bool                        m_mouseValid;   /**< True: m_mousePos is valid */
      volatile bool               m_paused;       /**< The window was un-clicked */

      /**
       * Create the size window
       */

      bool createSizeWindow(void);

      /**
       * Create the size label widget
       */

      bool createSizeLabel(void);

      /**
       * Set the Window Size
       */

      bool setWindowSize(FAR struct nxgl_size_s *size);

      /**
       * Update the size show in the size dimension label.
       *
       * @param size   The size of the rubber band
       */

      void updateSizeLabel(FAR struct nxgl_size_s &size);

      /**
       * This function handles the EVENT_RESIZE_BUTTON event.  It will start a
       * new resize sequence.  This occurs the first time that the toolbar
       * resize icon is clicked.
       *
       * @param msg.  The received NxWidget RESIZE event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool startResize(FAR struct SEventMsg *eventmsg);

      /**
       * This function handles the EVENT_RESIZE_MOVE event.  It will update
       * the resize information based on the new mouse position.
       *
       * @param msg.  The received NxWidget RESIZE event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool updateSize(FAR struct SEventMsg *eventmsg);

      /**
       * This function handles the EVENT_RESIZE_PAUSE event.  This occurs
       * when the window is un-clicked.  Another click in the window
       * will resume the resize operation.
       *
       * @param msg.  The received NxWidget RESIZE event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool pauseResize(FAR struct SEventMsg *eventmsg);

      /**
       * This function handles the EVENT_RESIZE_RESUME event.  This occurs
       * when the window is clicked while paused.
       *
       * @param msg.  The received NxWidget RESIZE event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool resumeResize(FAR struct SEventMsg *eventmsg);

      /**
       * This function handles the EVENT_RESIZE_STOP event.  It will
       * terminate a resize sequence.  This occurs when the resize button
       * is pressed a second time.
       *
       * @param msg.  The received NxWidget RESIZE event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool endResize(FAR struct SEventMsg *eventmsg);

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
       * if the touchscrreen touch is lost.  This indicates that the
       * resize sequence is complete.
       *
       * This function overrides the virtual IEventTap::dropEvent method.
       *
       * @param pos The last mouse/touch X/Y position.
       * @param arg The user-argument provided that accompanies the callback
       * @return True: if the drop event was processed; false it was
       *   ignored.  The event should be ignored if there is not actually
       *   a resize event in progress
       */

      bool dropEvent(FAR const struct nxgl_point_s &pos,
                     uintptr_t arg);

      /**
       * Is the tap enabled?
       *
       * @param arg The user-argument provided that accompanies the callback
       * @return True: If the the tap is enabled.
       */

      inline bool isActive(uintptr_t arg)
      {
        return (!m_paused && m_resizing);
      }

      /**
       * Enable/disable the resizing.  The disable event will cause resizing
       * to be paused.
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
       * @param enable.  True:  Enable movement
       * @param arg The user-argument provided that accompanies the callback
       */

      void enableMovement(FAR const struct nxgl_point_s &pos,
                          bool enable, uintptr_t arg);

    public:

      /**
       * CResize Constructor
       *
       * @param twm4nx   The Twm4Nx session
       */

      CResize(CTwm4Nx *twm4nx);

      /**
       * CResize Destructor
       */

      ~CResize(void);

      /**
       * @return True if resize is in-progress
       */

      inline bool resizing(void)
      {
        return m_resizing;
      }

      /**
       * CResize Initializer.  Performs the parts of the CResize construction
       * that may fail.
       *
       * @result True is returned on success
       */

      bool initialize(void);

      /**
       * Handle RESIZE events.
       *
       * @param msg.  The received NxWidget RESIZE event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *msg);
  };
}

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CRESIZE_HXX
