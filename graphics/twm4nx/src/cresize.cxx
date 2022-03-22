/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cresize.cxx
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

/////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <cerrno>

#include <nuttx/nx/nxbe.h>

#include "graphics/nxwidgets/cnxfont.hxx"
#include "graphics/nxwidgets/clabel.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/twm4nx_cursor.hxx"
#include "graphics/twm4nx/cresize.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

#define MINHEIGHT     0             // had been 32
#define MINWIDTH      0             // had been 60

#define makemult(a,b) ((b == 1) ? (a) : (((int)((a) / (b))) * (b)))

#ifndef MIN
#  define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif

/////////////////////////////////////////////////////////////////////////////
// Class Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CResize Constructor
 */

CResize::CResize(CTwm4Nx *twm4nx)
{
  m_twm4nx       = twm4nx;                           // Save the Twm4Nx session
  m_eventq       = (mqd_t)-1;                        // No widget message queue yet
  m_sizeWindow   = (FAR NXWidgets::CNxTkWindow *)0;  // No resize dimension windows yet
  m_sizeLabel    = (FAR NXWidgets::CLabel *)0;       // No resize dismsion label
  m_resizeWindow = (FAR CWindow *)0;                 // The window being resized
  m_savedTap     = (FAR IEventTap *)0;               // Saved IEventTap
  m_savedTapArg  = 0;                                // Saved IEventTap argument
  m_lastPos.x    = 0;                                // Last window position
  m_lastPos.y    = 0;
  m_lastSize.w   = 0;                                // Last window size
  m_lastSize.h   = 0;
  m_mousePos.x   = 0;                                // Last mouse position
  m_resizing     = false;                            // No resize in progress
  m_resized      = false;                            // The size has not changed
  m_mouseValid   = false;                            // The mouse position is not valid
  m_paused       = false;                            // The window was not un-clicked
}

/**
 * CResize Destructor
 */

CResize::~CResize(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Delete the resize dimension label

  if (m_sizeLabel != (FAR NXWidgets::CLabel *)0)
    {
      delete m_sizeLabel;
      m_sizeLabel = (FAR NXWidgets::CLabel *)0;
    }

  // Delete the resize dimensions window

  if (m_sizeWindow != (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
    }
}

/**
 * CResize Initializer.  Performs the parts of the CResize construction
 * that may fail.
 *
 * @result True is returned on success
 */

bool CResize::initialize(void)
{
  // Open a message queue to NX events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();
  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             mqname, errno);
      return false;
    }

  // Create the size window

  if (!createSizeWindow())
    {
      twmerr("ERROR:  Failed to create menu window\n");
      return false;
    }

  // Create the size label widget

  if (!createSizeLabel())
    {
      twmerr("ERROR:  Failed to recreate size label\n");

      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  return true;
}

/**
 * Handle RESIZE events.
 *
 * @param eventmsg.  The received NxWidget RESIZE event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CResize::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_RESIZE_XYINPUT:
        {
          // Poll for XY input -- None is needed at present
        }
        break;

      case EVENT_RESIZE_BUTTON:
        {
          // Start the resize operation on the first resize button press
          // (with m_resizing == false); Stop the resize operation on the
          // second resize button press (with m_resizing == true)

          if (m_resizing)
            {
              success = endResize(eventmsg);
            }
          else
            {
              success = startResize(eventmsg);
            }
        }
        break;

      case EVENT_RESIZE_MOVE:
        {
         // Update window size when a new mouse position is available

          success = updateSize(eventmsg);
        }
        break;

      case EVENT_RESIZE_PAUSE:
        {
          // Pause the resizee operation when the window is unclicked

          success = pauseResize(eventmsg);
        }
        break;

      case EVENT_RESIZE_RESUME:
        {
          // Resume the resize operation if the window is re-clicked

          success = resumeResize(eventmsg);
        }
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Create the size window
 */

bool CResize::createSizeWindow(void)
{
  // Create the main window
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_SYSTEM_NOP;
  events.mouseEvent  = EVENT_RESIZE_XYINPUT;
  events.kbdEvent    = EVENT_SYSTEM_NOP;
  events.closeEvent  = EVENT_SYSTEM_NOP;
  events.deleteEvent = EVENT_WINDOW_DELETE;

  FAR CWindowEvent *control =
    new CWindowEvent(m_twm4nx, (FAR void *)0, events);

  // 4. Create the main window

  uint8_t wflags = (NXBE_WINDOW_RAMBACKED | NXBE_WINDOW_HIDDEN);

  m_sizeWindow = m_twm4nx->createFramedWindow(control, wflags);
  if (m_sizeWindow == (FAR NXWidgets::CNxTkWindow *)0)
    {
      delete control;
      return false;
    }

  // 5. Open and initialize the main window

  bool success = m_sizeWindow->open();
  if (!success)
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  // 6. Set the initial window size

  // Create the resize dimension window

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  struct nxgl_size_s size;
  size.w = sizeFont->getStringWidth(" 8888 x 8888 ");
  size.h = sizeFont->getHeight() + CONFIG_TWM4NX_ICONMGR_VSPACING * 2;

  if (!m_sizeWindow->setSize(&size))
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  // 7. Set the initial window position

  struct nxgl_point_s pos =
  {
    .x = 0,
    .y = 0
  };

  if (!m_sizeWindow->setPosition(&pos))
    {
      delete m_sizeWindow;
      m_sizeWindow = (FAR NXWidgets::CNxTkWindow *)0;
      return false;
    }

  return true;
}

/**
 * Create the size label widget
 */

bool CResize::createSizeLabel(void)
{
  // The size of label is selected to fill the entire size window

  struct nxgl_size_s labelSize;
  if (!m_sizeWindow->getSize(&labelSize))
    {
      twmerr("ERROR: Failed to get window size\n");
      return false;
    }

  // Position the label at the origin of the window.

  struct nxgl_point_s labelPos;
  labelPos.x = 0;
  labelPos.y = 0;

  // Get the Widget control instance from the size window.  This
  // will force all widget drawing to go to the size window.

  FAR NXWidgets:: CWidgetControl *control =
    m_sizeWindow->getWidgetControl();

  if (control == (FAR NXWidgets:: CWidgetControl *)0)
    {
      // Should not fail

      return false;
    }

  // Create the size label widget

  m_sizeLabel = new NXWidgets::CLabel(control, labelPos.x, labelPos.y,
                                      labelSize.w, labelSize.h,
                                      " 8888 x 8888 ");
  if (m_sizeLabel == (FAR NXWidgets::CLabel *)0)
    {
      twmerr("ERROR: Failed to construct size label widget\n");
      return false;
    }

  // Configure the size label

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *sizeFont = fonts->getSizeFont();

  m_sizeLabel->setFont(sizeFont);
  m_sizeLabel->setBorderless(true);
  m_sizeLabel->disableDrawing();
  m_sizeLabel->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_CENTER);
  m_sizeLabel->setTextAlignmentVert(NXWidgets::CLabel::TEXT_ALIGNMENT_VERT_CENTER);
  m_sizeLabel->setRaisesEvents(false);

  // Register to get events from the mouse clicks on the image

  m_sizeLabel->addWidgetEventHandler(this);
  return true;
}

/**
 * Set the Window Size
 */

bool CResize::setWindowSize(FAR struct nxgl_size_s *size)
{
  // Set the window size

  if (!m_sizeWindow->setSize(size))
    {
      return false;
    }

  // Set the label size to match

  if (!m_sizeLabel->resize(size->w, size->h))
    {
      return false;
    }

  return true;
}

/**
 * Update the size show in the size dimension label.
 *
 * @param windowSize The new size of the window
 */

void CResize::updateSizeLabel(FAR struct nxgl_size_s &windowSize)
{
  // Do nothing if the size has not changed

  struct nxgl_size_s labelSize;
  m_sizeLabel->getSize(labelSize);

  if (labelSize.w == windowSize.w && labelSize.h == windowSize.h)
    {
      return;
    }

  FAR char *str;
  asprintf(&str, " %4d x %-4d ", windowSize.w, windowSize.h);
  if (str == (FAR char *)0)
    {
      twmerr("ERROR: Failed to get size string\n");
      return;
    }

  // Un-hide the window and bring the window to the top of the hierarchy

  m_sizeWindow->show();

  // Add the string to the label widget

  m_sizeLabel->disableDrawing();
  m_sizeLabel->setRaisesEvents(false);

  m_sizeLabel->setText(str);
  std::free(str);

  // Re-enable and redraw the label widget

  m_sizeLabel->enableDrawing();
  m_sizeLabel->setRaisesEvents(true);
  m_sizeLabel->redraw();
}

/**
 * This function handles the EVENT_RESIZE_BUTTON event.  It will start a new
 * resize sequence.  This occurs the first time that the toolbar resize
 * icon  resize icon is clicked.
 *
 * @param msg.  The received NxWidget RESIZE event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CResize::startResize(FAR struct SEventMsg *eventmsg)
{
  // Save the window that we are operating on

  m_resizeWindow = (FAR CWindow *)eventmsg->obj;

  // Get the window current position and size

  if (!m_resizeWindow->getFramePosition(&m_lastPos))
    {
      twmerr("ERROR: Failed to get frame position");
      return false;
    }

  if (!m_resizeWindow->getFrameSize(&m_lastSize))
    {
      twmerr("ERROR: Failed to get frame size");
      return false;
    }

  // Save the current touch position

  m_mousePos.x = eventmsg->pos.x;
  m_mousePos.y = eventmsg->pos.y;
  m_mouseValid = false;            // RESVISIT:  Why is position invalid?

  // Update the size label

  updateSizeLabel(m_lastSize);

  // Disable all toolbar buttons except for the RESIZE button which is
  // needed to exit the resize mode.
  // NOTE: This is really unnecessary since all non-critical events are
  // ignored during resizing.

  m_resizeWindow->disableToolbarButtons(DISABLE_MENU_BUTTON |
                                        DISABLE_DELETE_BUTTON |
                                        DISABLE_MINIMIZE_BUTTON);

  // Unhide the size window

  m_sizeWindow->show();

  // Set up the temporary event tap

  m_resizeWindow->getEventTap(m_savedTap, m_savedTapArg);
  m_resizeWindow->installEventTap(this, (uintptr_t)0);

  // Make the size window modal so that it will stay at the top of the
  // hierarchy

  m_sizeWindow->modal(true);
  m_resizing = true;
  m_resized  = false;

#ifdef CONFIG_TWM4NX_MOUSE
  // Select the resize cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_RZCURSOR_IMAGE);
#endif

  return true;
}

/**
 * This function handles the EVENT_RESIZE_MOVE event.  It will update
 * the resize information based on the new mouse position.
 *
 * @param msg.  The received NxWidget RESIZE event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CResize::updateSize(FAR struct SEventMsg *eventmsg)
{
  // REVISIT:  This is a kludge.  It appears that in startResize(), the
  // received mouse position is always (0,0).  So we have to waste one update
  // to get a valid size.

  if (!m_mouseValid)
    {
      m_mousePos.x = eventmsg->pos.x;
      m_mousePos.y = eventmsg->pos.y;
      m_mouseValid = true;
      return true;
    }

  // The unconstrained, new window size is the old window size plus the
  // change in the mouse position.

  struct nxgl_size_s newSize;
  newSize.w = m_lastSize.w + eventmsg->pos.x - m_mousePos.x;
  newSize.h = m_lastSize.h + eventmsg->pos.y - m_mousePos.y;

  // Create a bounding box for the new window

  struct nxgl_rect_s windowBounds;
  windowBounds.pt1.x = m_lastPos.x;
  windowBounds.pt1.y = m_lastPos.y;
  windowBounds.pt2.x = m_lastPos.x + newSize.w - 1;
  windowBounds.pt2.y = m_lastPos.y + newSize.h - 1;

  // If the new size could exceed the right limit of the window, then
  // move the left position to keep the window within the display.

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  if (windowBounds.pt2.x >= displaySize.w)
    {
      // Stretch the left side and truncate the right side

      windowBounds.pt1.x = displaySize.w - newSize.w;
      windowBounds.pt2.x = displaySize.w - 1;

      // Clip the left side if necessary

      if (windowBounds.pt1.x < 0)
        {
          windowBounds.pt1.x = 0;
        }
    }

  // If the new size could exceed the left limit of the window, then
  // move the right position to keep the window within the display.

  else if (windowBounds.pt1.x < 0)
    {
      windowBounds.pt1.x = 0;
      windowBounds.pt2.x = newSize.w - 1;

      // Clip the right side if necessary

      if (windowBounds.pt2.x >= displaySize.w)
        {
          windowBounds.pt2.x = displaySize.w - 1;
        }
    }

  // If the new size could exceed the bottom limit of the window, then
  // move the top position to keep the window within the display.

  if (windowBounds.pt2.y >= displaySize.h)
    {
      // Stretch the left side and truncate the right side

      windowBounds.pt1.y = displaySize.h - newSize.h;
      windowBounds.pt2.y = displaySize.h - 1;

      // Clip the top side if necessary

      if (windowBounds.pt1.y < 0)
        {
          windowBounds.pt1.y = 0;
        }
    }

  // If the new size could exceed the top limit of the window, then
  // move the bottom position to keep the window within the display.

  else if (windowBounds.pt1.y < 0)
    {
      windowBounds.pt1.y = 0;
      windowBounds.pt2.y = newSize.h - 1;

      // Clip the bottom side if necessary

      if (windowBounds.pt2.x > displaySize.h)
        {
          windowBounds.pt2.x = displaySize.h - 1;
        }
    }

  // Check if the size changed

  newSize.w = windowBounds.pt2.x - windowBounds.pt1.x + 1;
  newSize.h = windowBounds.pt2.y - windowBounds.pt1.y + 1;

  // Don't do more unless the size has actually changed

  if (m_lastSize.w != newSize.h || m_lastSize.h != newSize.h)
    {
      // Do we want to try a continuous resize?   If so, we should call
      // m_resizeWindow->resizeFrame() here.  This probably a bit much for the
      // typical embedded MCU to handle.  So we, instead, just display the
      // updated size.  The window will not be resized until the resize button
      // is pressed a second time.

      updateSizeLabel(newSize);
      m_resized = true;
    }

  // Reset every thing in preparation for the next mouse report

  m_lastSize.w = newSize.w;
  m_lastSize.h = newSize.h;
  m_lastPos.x  = windowBounds.pt1.x;
  m_lastPos.y  = windowBounds.pt1.y;
  m_mousePos.x = eventmsg->pos.x;
  m_mousePos.y = eventmsg->pos.y;

  return true;
}

/**
 * This function handles the EVENT_RESIZE_PAUSE event.  This occurs
 * when the window is un-clicked.  Another click in the window
 * will resume the resize operation.
 *
 * @param msg.  The received NxWidget RESIZE event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CResize::pauseResize(FAR struct SEventMsg *eventmsg)
{
  // m_paused should have already been set asynchronously

  bool success = false;
  if (m_paused)
    {
      // One last update to the mouse release position

      if (!updateSize(eventmsg))
        {
          twmerr("ERROR: Failed to update the window size\n");
        }

      // Set the new frame position and size if the size has changed

      else if (m_resized && !m_resizeWindow->resizeFrame(&m_lastSize, &m_lastPos))
        {
          twmerr("ERROR: Failed to resize frame\n");
        }
      else
        {
          m_resized = false;
          success   = true;
        }
    }

  return success;
}

/**
 * This function handles the EVENT_RESIZE_RESUME event.  This occurs
 * when the window is clicked while paused.
 *
 * @param msg.  The received NxWidget RESIZE event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CResize::resumeResize(FAR struct SEventMsg *eventmsg)
{
  // Make sure the we are in the resizing state

  if (!m_resizing)
   {
      return false;
   }

#ifdef CONFIG_TWM4NX_MOUSE
  // Restore the normal cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_CURSOR_IMAGE);
#endif

  // Reset the the window position and size

  if (!m_resizeWindow->getFramePosition(&m_lastPos))
    {
      twmerr("ERROR: Failed to get frame position");
      return false;
    }

  if (!m_resizeWindow->getFrameSize(&m_lastSize))
    {
      twmerr("ERROR: Failed to get frame size");
      return false;
    }

  // Save the current touch position

  m_mousePos.x = eventmsg->pos.x;
  m_mousePos.y = eventmsg->pos.y;

  // Update the size label

  updateSizeLabel(m_lastSize);
  return true;
}

/**
 * This function handles the EVENT_RESIZE_STOP event.  It will terminate a
 * resize sequence.
 *
 * @param msg.  The received NxWidget RESIZE event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CResize::endResize(FAR struct SEventMsg *eventmsg)
{
  // Make sure the we are in the resizing state

  if (!m_resizing)
   {
      return false;
   }

  // Restore the event tap

  m_resizeWindow->installEventTap(m_savedTap, m_savedTapArg);

  // Re-enable toolbar buttons

  m_resizeWindow->disableToolbarButtons(DISABLE_NO_BUTTONS);

  // The size window should no longer be modal

  m_sizeWindow->modal(false);

  // Hide the size window

  m_sizeWindow->hide();

  // Has the size changed?

  bool success = true;
  if (m_resized)
  {
    // If the window was the Icon manager, then we need to deal with the
    // contained widget.

    if (m_resizeWindow->isIconMgr())
      {
        // Adjust the Icon Manager button array to account for the resize

        CIconMgr *iconMgr = m_resizeWindow->getIconMgr();
        iconMgr->resizeIconManager();
      }

    // Set the new window frame position and size

     if (!m_resizeWindow->resizeFrame(&m_lastSize, &m_lastPos))
      {
        twmerr("ERROR: Failed to resize frame\n");
        success = false;
      }
    else
      {
        m_resized = false;
      }
  }

  m_resizing = false;
  return success;
}

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

bool CResize::moveEvent(FAR const struct nxgl_point_s &pos,
                        uintptr_t arg)
{
  twminfo("MOVE event...\n");

  // Send the aEVENT_RESIZE_MOVE event

  struct SEventMsg outmsg;
  outmsg.eventID  = EVENT_RESIZE_MOVE;
  outmsg.obj      = (FAR void *)this;
  outmsg.pos.x    = pos.x;
  outmsg.pos.y    = pos.y;
  outmsg.context  = EVENT_CONTEXT_RESIZE;
  outmsg.handler  = (FAR void *)0;

  int ret = mq_send(m_eventq, (FAR const char *)&outmsg,
                    sizeof(struct SEventMsg), 100);
  if (ret < 0)
   {
     twmerr("ERROR: mq_send failed: %d\n", errno);
     return false;
   }

  return true;
}

/**
 * This function is called if the mouse left button is released or
 * if the touchscreen touch is lost.  This indicates that the
 * movement sequence is complete.
 *
 * In this usage, this indicates the end of a resize sequence.
 *
 * This function overrides the virtual IEventTap::dropEvent method.
 *
 * @param pos The last mouse/touch X/Y position.
 * @param arg The user-argument provided that accompanies the callback
 * @return True: if the drop event was processed; false it was
 *   ignored.  The event should be ignored if there is not actually
 *   a movement event in progress
 */

bool CResize::dropEvent(FAR const struct nxgl_point_s &pos,
                        uintptr_t arg)
{
  twminfo("Drop event...\n");

  // Send the aEVENT_RESIZE_PAUSE event

  struct SEventMsg outmsg;
  outmsg.eventID  = EVENT_RESIZE_PAUSE;
  outmsg.obj      = (FAR void *)this;
  outmsg.pos.x    = pos.x;
  outmsg.pos.y    = pos.y;
  outmsg.context  = EVENT_CONTEXT_RESIZE;
  outmsg.handler  = (FAR void *)0;

  int ret = mq_send(m_eventq, (FAR const char *)&outmsg,
                    sizeof(struct SEventMsg), 100);
  if (ret < 0)
   {
     twmerr("ERROR: mq_send failed: %d\n", errno);
     return false;
   }

  return true;
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
 * @param enable.  True:  Enable the tap
 * @param arg The user-argument provided that accompanies the callback
 */

void CResize::enableMovement(FAR const struct nxgl_point_s &pos,
                             bool enable, uintptr_t arg)
{
  // If we are already dragging, but paused, then send the resume event.
  // NOTE that we don't have to do anything in pause case because we
  // will get the redundant dropEvent

  if (m_paused && m_resizing)
    {
      // Send the aEVENT_RESIZE_RESUME event

      struct SEventMsg outmsg;
      outmsg.eventID  = EVENT_RESIZE_RESUME;
      outmsg.obj      = (FAR void *)this;
      outmsg.pos.x    = pos.x;
      outmsg.pos.y    = pos.y;
      outmsg.context  = EVENT_CONTEXT_RESIZE;
      outmsg.handler  = (FAR void *)0;

      int ret = mq_send(m_eventq, (FAR const char *)&outmsg,
                        sizeof(struct SEventMsg), 100);
      if (ret < 0)
       {
         twmerr("ERROR: mq_send failed: %d\n", errno);
       }
    }

  // Otherwise, just track the state

  m_paused = !enable;
}
