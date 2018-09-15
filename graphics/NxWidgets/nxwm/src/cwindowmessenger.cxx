/********************************************************************************************
 * NxWidgets/nxwm/src/cwindowmessenger.cxx
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
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <cfcntl>
#include <cerrno>

#include <debug.h>

#include "nxwmconfig.hxx"
#include "cstartwindow.hxx"
#include "cwindowmessenger.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * CWindowMessenger Method Implementations
 ********************************************************************************************/

using namespace NxWM;

/**
 * CWindowMessenger Constructor
 *
 * @param style The default style that all widgets on this display
 *   should use.  If this is not specified, the widget will use the
 *   values stored in the defaultCWidgetStyle object.
 */

CWindowMessenger::CWindowMessenger(FAR const NXWidgets::CWidgetStyle *style)
: NXWidgets::CWidgetControl(style)
{
  // Add ourself to the list of window event handlers

  addWindowEventHandler(this);
}

/**
 * CWindowMessenger Destructor.
 */

CWindowMessenger::~CWindowMessenger(void)
{
  // Remove ourself from the list of the window event handlers

  removeWindowEventHandler(this);
}

/**
 * Handle an NX window mouse input event.
 *
 * @param e The event data.
 */

#ifdef CONFIG_NX_XYINPUT
void CWindowMessenger::handleMouseEvent(void)
{
  // The logic path here is tortuous but flexible:
  //
  //  1. A listener thread receives mouse or touchscreen input and injects
  //     that into NX via nx_mousein
  //  2. In the multi-user mode, this will send a message to the NX server
  //  3. The NX server will determine which window gets the mouse input
  //     and send a window event message to the NX listener thread.
  //  4. The NX listener thread receives a windows event.  The NX listener thread
  //     is part of CTaskBar and was created when NX server connection was
  //     established.  This event may be a positional change notification, a
  //     redraw request, or mouse or keyboard input.  In this case, mouse input.
  //  5. The NX listener thread handles the message by calling nx_eventhandler().
  //     nx_eventhandler() dispatches the message by calling a method in the
  //     NXWidgets::CCallback instance associated with the window.
  //     NXWidgets::CCallback is a part of the CWidgetControl.
  //  6. NXWidgets::CCallback calls into NXWidgets::CWidgetControl to process
  //     the event.
  //  7. NXWidgets::CWidgetControl records the new state data and raises a
  //     window event.
  //  8. NXWidgets::CWindowEventHandlerList will give the event to this method
  //     NxWM::CWindowMessenger.
  //  9. This NxWM::CWindowMessenger method will schedule an entry on the work
  //     queue.
  // 10. The work queue callback will finally call pollEvents() to execute whatever
  //     actions the input event should trigger.

  work_state_t *state = new work_state_t;
  state->windowMessenger = this;
  int ret = work_queue(USRWORK, &state->work, &inputWorkCallback, state, 0);
  if (ret < 0)
    {
      gerr("ERROR: work_queue failed: %d\n", ret);
    }
}
#endif

/**
 * Handle a NX window keyboard input event.
 */

#ifdef CONFIG_NX_KBD
void CWindowMessenger::handleKeyboardEvent(void)
{
  work_state_t *state = new work_state_t;
  state->windowMessenger = this;

  int ret = work_queue(USRWORK, &state->work, &inputWorkCallback, state, 0);
  if (ret < 0)
    {
      gerr("ERROR: work_queue failed: %d\n", ret);
    }
}
#endif

/**
 * Handle a NX window blocked event.  This handler is called when we
 * receive the BLOCKED message meaning that there are no further pending
 * actions on the window.  It is now safe to delete the window.
 *
 * This is handled by sending a message to the start window thread (vs just
 * calling the destructors) because in the case where an application
 * destroys itself (because of pressing the stop button), then we need to
 * unwind and get out of the application logic before destroying all of its
 * objects.
 *
 * @param arg - User provided argument (see nx_block or nxtk_block)
 */

void CWindowMessenger::handleBlockedEvent(FAR void *arg)
{
  // Send a message to destroy the window instance.

  work_state_t *state = new work_state_t;
  state->windowMessenger = this;
  state->instance = arg;

  int ret = work_queue(USRWORK, &state->work, &destroyWorkCallback, state, 0);
  if (ret < 0)
    {
      gerr("ERROR: work_queue failed: %d\n", ret);
    }
}

/** Work queue callback functions */

void CWindowMessenger::inputWorkCallback(FAR void *arg)
{
  work_state_t *state = (work_state_t*)arg;
  state->windowMessenger->pollEvents();
  delete state;
}

void CWindowMessenger::destroyWorkCallback(FAR void *arg)
{
  work_state_t *state = (work_state_t*)arg;

  // First make sure any pending input events have been handled.

  state->windowMessenger->pollEvents();

  // Then release the memory.

  ginfo("Deleting app=%p\n", state->instance);
  IApplication *app = (IApplication *)state->instance;
  delete app;
  delete state;
}
