/****************************************************************************
 * apps/graphics/nxwidgets/src/ccallback.cxx
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

#include <sys/types.h>
#ifdef CONFIG_NXTERM_NXKBDIN
#  include <sys/boardctl.h>
#endif

#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <debug.h>

#include <cassert>
#include <cerrno>

#include <nuttx/semaphore.h>
#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxtk.h>

#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/ccallback.hxx"

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
  * @param widgetControl Control object associated with this window
  */

CCallback::CCallback(CWidgetControl *widgetControl)
{
  // Save the widgetControl

  m_widgetControl      = widgetControl;

  // Initialize the callback vtable

  m_callbacks.redraw   = redraw;
  m_callbacks.position = position;
#ifdef CONFIG_NX_XYINPUT
  m_callbacks.mousein  = newMouseEvent;
#endif
#ifdef CONFIG_NX_KBD
  m_callbacks.kbdin    = newKeyboardEvent;
#endif
  m_callbacks.event    = windowEvent;

  // Synchronization support

  m_synchronized       = false;

  sem_init(&m_semevent, 0, 0);
  sem_setprotocol(&m_semevent, SEM_PRIO_NONE);

#ifdef CONFIG_NXTERM_NXKBDIN
  // Keyboard input is initially directed to the widgets within the window

  m_nxterm             = (NXTERM)0;
#endif
}

/**
 * Synchronize the window with the NX server.  This function will delay
 * until the the NX server has caught up with all of the queued requests.
 * When this function returns, the state of the NX server will be the
 * same as the state of the application.
 *
 * REVISIT:  An instance of this function is not re-entrant.
 *
 * @param hwnd Handle to a specific NX window.
 */

void CCallback::synchronize(NXWINDOW hwnd, enum WindowType windowType)
{
  m_synchronized = false;

  // Request synchronization.  Window type matters here because the void*
  // window handle will be interpreted differently.

  if (windowType == NX_RAWWINDOW)
    {
      int ret = nx_synch(hwnd, (FAR void *)this);
      if (ret < 0)
        {
          gerr("ERROR: nx_synch() failed: %d\n", errno);
          return;
        }
    }
  else
    {
      DEBUGASSERT(windowType == NXTK_FRAMEDWINDOW);

      int ret = nxtk_synch(hwnd, (FAR void *)this);
      if (ret < 0)
        {
          gerr("ERROR: nxtk_synch() failed: %d\n", errno);
          return;
        }
    }

  while (!m_synchronized)
    {
      int ret = sem_wait(&m_semevent);
      DEBUGASSERT(ret >= 0 || errno == EINTR);
      UNUSED(ret);
    }

  m_synchronized = false;
}

 /**
  * ReDraw Callback.  The redraw action is handled by CWidgetControl:redrawEvent.
  *
  * @param hwnd Handle to a specific NX window.
  * @param nxRect The rectangle that needs to be re-drawn (in window
  * relative coordinates).
  * @param more true: More re-draw requests will follow.
  * @param arg User provided argument (see nx_openwindow, nx_requestbg,
  * nxtk_openwindow, or nxtk_opentoolbar).
  */

void CCallback::redraw(NXHANDLE hwnd,
                       FAR const struct nxgl_rect_s *nxRect,
                       bool more, FAR void *arg)
{
  ginfo("hwnd=%p nxRect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd,
         nxRect->pt1.x, nxRect->pt1.y, nxRect->pt2.x, nxRect->pt2.y,
         more ? "true" : "false");

  // The argument must be the CCallback instance

  CCallback *This = (CCallback *)arg;

  // Just forward the callback to the CWidgetControl::redrawEvent method

  This->m_widgetControl->redrawEvent(nxRect, more);
}

 /**
  * Position Callback. The new positional data is handled by
  * CWidgetControl::geometryEvent.
  *
  * @param hwnd Handle to a specific NX window.
  * @param size The size of the window.
  * @param pos The position of the upper left hand corner of the window on
  * the overall display.
  * @param bounds The bounding rectangle that describes the entire display.
  * @param arg User provided argument (see nx_openwindow, nx_requestbg,
  * nxtk_openwindow, or nxtk_opentoolbar).
  */

void CCallback::position(NXHANDLE hwnd,
                         FAR const struct nxgl_size_s *size,
                         FAR const struct nxgl_point_s *pos,
                         FAR const struct nxgl_rect_s *bounds,
                         FAR void *arg)
{
  ginfo("hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)} arg=%p\n",
        hwnd, size->w, size->h, pos->x, pos->y,
        bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y,
        arg);

  // The argument must be the CCallback instance

  CCallback *This = (CCallback *)arg;

  // Just forward the callback to the CWidgetControl::geometry method

  This->m_widgetControl->geometryEvent(hwnd, size, pos, bounds);
}

#ifdef CONFIG_NX_XYINPUT
 /**
  * New mouse data is available for the window.  The new mouse data is
  * handled by CWidgetControl::newMouseEvent.
  *
  * @param hwnd Handle to a specific NX window.
  * @param pos The (x,y) position of the mouse.
  * @param buttons See NX_MOUSE_* definitions.
  * @param arg User provided argument (see nx_openwindow, nx_requestbg,
  * nxtk_openwindow, or nxtk_opentoolbar).
  */

void CCallback::newMouseEvent(NXHANDLE hwnd,
                              FAR const struct nxgl_point_s *pos,
                              uint8_t buttons, FAR void *arg)
{
  ginfo("hwnd=%p pos=(%d,%d) buttons=%02x arg=%p\n",
        hwnd, pos->x, pos->y, buttons, arg);

  // The argument must be the CCallback instance

  CCallback *This = (CCallback *)arg;

  // Just forward the callback to the CWidgetControl::newMouseEvent method

  This->m_widgetControl->newMouseEvent(pos, buttons);
}
#endif /* CONFIG_NX_XYINPUT */

#ifdef CONFIG_NX_KBD
/**
 * New keyboard/keypad data is available for the window.  The new keyboard
 * data is handled by CWidgetControl::newKeyboardEvent.
 *
 * @param hwnd Handle to a specific NX window.
 * @param nCh The number of characters that are available in str[].
 * @param str The array of characters.
 * @param arg User provided argument (see nx_openwindow, nx_requestbg,
 * nxtk_openwindow, or nxtk_opentoolbar).
 */

void CCallback::newKeyboardEvent(NXHANDLE hwnd, uint8_t nCh,
                                 FAR const uint8_t *str,
                                 FAR void *arg)
{
  ginfo("hwnd=%p nCh=%d arg=%p\n", hwnd, nCh, arg);

  // The argument must be the CCallback instance

  CCallback *This = (CCallback *)arg;

#ifdef CONFIG_NXTERM_NXKBDIN
  // Is NX keyboard input being directed to the widgets within the window
  // (default) OR is NX keyboard input being re-directed to an NxTerm
  // driver?

  if (This->m_nxterm)
    {
      struct boardioc_nxterm_ioctl_s iocargs;
      struct nxtermioc_kbdin_s kbdin;

      // Keyboard input is going to an NxTerm

      kbdin.handle = This->m_nxterm;
      kbdin.buffer = str;
      kbdin.buflen = nCh;

      iocargs.cmd  = NXTERMIOC_NXTERM_KBDIN;
      iocargs.arg  = (uintptr_t)&kbdin;

      boardctl(BOARDIOC_NXTERM_IOCTL, (uintptr_t)&iocargs);
    }
  else
#endif
    {
      // Just forward the callback to the CWidgetControl::newKeyboardEvent method

      This->m_widgetControl->newKeyboardEvent(nCh, str);
    }
}

#endif // CONFIG_NX_KBD

/**
 *   This callback is used to communicate server events to the window
 *   listener.
 *
 *   NXEVENT_BLOCKED - Window messages are blocked.
 *
 *     This callback is the response from nx_block (or nxtk_block). Those
 *     blocking interfaces are used to assure that no further messages are
 *     directed to the window. Receipt of the blocked callback signifies
 *     that (1) there are no further pending callbacks and (2) that the
 *     window is now 'defunct' and will receive no further callbacks.
 *
 *     This callback supports coordinated destruction of a window.  In
 *     the multi-user mode, the client window logic must stay intact until
 *     all of the queued callbacks are processed.  Then the window may be
 *     safely closed.  Closing the window prior with pending callbacks can
 *     lead to bad behavior when the callback is executed.
 *
 *   NXEVENT_SYCNCHED - Synchronization handshake
 *
 *     This completes the handshake started by nx_synch().  nx_synch()
 *     sends a syncrhonization messages to the NX server which responds
 *     with this event.  The sleeping client is awakened and continues
 *     graphics processing, completing the handshake.
 *
 *     Due to the highly asynchronous nature of client-server
 *     communications, nx_synch() is sometimes necessary to assure that
 *     the client and server are fully synchronized.
 *
 * @param hwnd. Window handle of the blocked window
 * @param event. The server event
 * @param arg1. User provided argument (see nx_openwindow, nx_requestbkgd,
 *   nxtk_openwindow, or nxtk_opentoolbar)
 * @param arg2 - User provided argument (see nx[tk]_block or nx[tk]_synch)
 */

void CCallback::windowEvent(NXWINDOW hwnd, enum nx_event_e event,
                            FAR void *arg1, FAR void *arg2)
{
  ginfo("hwnd=%p devent=%d arg1=%p arg2=%p\n", hwnd, event, arg1, arg2);

  switch (event)
    {
      case NXEVENT_SYNCHED:  // Server is synchronized
        {
          // The second argument must be the CCallback instance

          CCallback *This = (CCallback *)arg2;
          DEBUGASSERT(This != NULL);

          // We are now synchronized

          This->m_synchronized = true;
          sem_post(&This->m_semevent);
        }
        break;

      case NXEVENT_BLOCKED:  // Window block, ready to be closed.
        {
          // The first argument must be the CCallback instance

          CCallback *This = (CCallback *)arg1;
          DEBUGASSERT(This != NULL);

          // Let the CWidgetControl::windowBlocked method handle the event.

          This->m_widgetControl->windowBlocked(arg2);
        }
        break;

      default:
        break;
    }
}
