/****************************************************************************
 * apps/include/graphics/nxwidgets/ccallback.hxx
 *
 *   Copyright (C) 2012, 2019 Gregory Nutt. All rights reserved.
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
 ****************************************************************************/

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CCALLBACK_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CCALLBACK_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nx.h>

#ifdef CONFIG_NXTERM_NXKBDIN
#  include <nuttx/nx/nxterm.h>
#endif

#include "graphics/nxwidgets/crect.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class CWidgetControl;

  /**
   * Callback function proxies.  This class receives and dispatches callbacks
   * from the NX server.  This calls also manages a few lower-level details
   * such as keeping track of the reported window handles and window positions
   * and sizes.
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

  class CCallback
  {
  private:
    CWidgetControl      *m_widgetControl; /**< The widget control instance for this window */
    struct nx_callback_s m_callbacks;     /**< C-callable vtable of callback function pointers */
#ifdef CONFIG_NXTERM_NXKBDIN
    NXTERM               m_nxterm;        /**< The NxTerm handle for redirection of keyboard input */
#endif
    volatile bool       m_synchronized;   /**< True:  Syncrhonized with NX server */
    sem_t               m_semevent;       /**< Event wait semaphore */

    // Methods in the callback vtable

    /**
     * Re-Draw Callback.  The redraw event is handled by CWidgetControl::redrawEvent.
     *
     * NOTE:  This method runs in the context of the NX callback which may
     * either be the context of the owning thread or, in the case of multi-
     * user NX, the context of the NX event listener thread.
     *
     * @param hwnd Handle to a specific NX window.
     * @param rect The rectangle that needs to be re-drawn (in window
     * relative coordinates).
     * @param bMore true: More re-draw requests will follow.
     * @param arg User provided argument (see nx_openwindow, nx_requestbg,
     * nxtk_openwindow, or nxtk_opentoolbar).
     */

    static void redraw(NXHANDLE hwnd, FAR const struct nxgl_rect_s *rect,
                       bool bMore, FAR void *arg);

    /**
     * Position Callback.  The new positional data is handled by
     * CWidgetControl::geometryEvent.
     *
     * NOTE:  This method runs in the context of the NX callback which may
     * either be the context of the owning thread or, in the case of multi-
     * user NX, the context of the NX event listener thread.
     *
     * @param hwnd Handle to a specific NX window.
     * @param size The size of the window.
     * @param pos The position of the upper left hand corner of the window on
     * the overall display.
     * @param bounds The bounding rectangle that describes the entire display.
     * @param arg User provided argument (see nx_openwindow, nx_requestbg,
     * nxtk_openwindow, or nxtk_opentoolbar).
     */

    static void position(NXHANDLE hwnd, FAR const struct nxgl_size_s *size,
                         FAR const struct nxgl_point_s *pos,
                         FAR const struct nxgl_rect_s *bounds,
                         FAR void *arg);

#ifdef CONFIG_NX_XYINPUT
    /**
     * New mouse data is available for the window.  The new mouse
     * data is handled by CWidgetControl::newMouseEvent.
     *
     * NOTE:  This method runs in the context of the NX callback which may
     * either be the context of the NX event listener thread (if multi-
     * user NX), or possibly in the connects of device driver or even a
     * device driver interrupt.
     *
     * The GUI thread is probably sleeping a semaphore, waiting to be
     * awakened by a mouse or keyboard event.
     *
     * @param hwnd Handle to a specific NX window.
     * @param pos The (x,y) position of the mouse.
     * @param buttons See NX_MOUSE_* definitions.
     * @param arg User provided argument (see nx_openwindow, nx_requestbg,
     * nxtk_openwindow, or nxtk_opentoolbar).
     */

    static void newMouseEvent(NXHANDLE hwnd,
                              FAR const struct nxgl_point_s *pos,
                              uint8_t buttons, FAR void *arg);
#endif /* CONFIG_NX_XYINPUT */

#ifdef CONFIG_NX_KBD
    /**
     * New keyboard/keypad data is available for the window.  The new
     * keyboard data is handled by CWidgetControl::newKeyboardEvent.
     *
     * NOTE:  This method runs in the context of the NX callback which may
     * either be the context of the NX event listener thread (if multi-
     * user NX), or possibly in the connects of device driver or even a
     * device driver interrupt.
     *
     * The GUI thread is probably sleeping a semaphore, waiting to be
     * awakened by a mouse or keyboard event.
     *
     * @param hwnd Handle to a specific NX window.
     * @param nCh The number of characters that are available in str[].
     * @param str The array of characters.
     * @param arg User provided argument (see nx_openwindow, nx_requestbg,
     * nxtk_openwindow, or nxtk_opentoolbar).
     */

    static void newKeyboardEvent(NXHANDLE hwnd, uint8_t nCh,
                                 FAR const uint8_t *str, FAR void *arg);
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
     * @param arg2 - User provided argument (see nx_block or nxtk_block)
     */

    static void windowEvent(NXWINDOW hwnd, enum nx_event_e event,
                            FAR void *arg1, FAR void *arg2);

  public:

    /**
     * Enum of window types
     */

    enum WindowType
    {
      NX_RAWWINDOW = 0,
      NXTK_FRAMEDWINDOW
    };

    /**
     * Constructor.
     *
     * @param widgetControl Control object associated with this window
     */

    CCallback(CWidgetControl *widgetControl);

    /**
     * Destructor.
     */

    inline ~CCallback(void) {}

    /**
     * Get the callback vtable.  This is neeed only by the window
     * instance that inherits this class.  The window instance needs the
     * C-callable vtable in order to create the NX window.  Once the
     * window is created, this class will begin to receive callbacks via
     * the C-callable vtable methods.
     *
     * @return This method returns the C-callable vtable needed for
     *         NX window creation.
     */

    inline FAR struct nx_callback_s *getCallbackVTable(void)
    {
      return &m_callbacks;
    }

    /**
     * Synchronize the window with the NX server.  This function will delay
     * until the the NX server has caught up with all of the queued requests.
     * When this function returns, the state of the NX server will be the
     * same as the state of the application.
     *
     * @param hwnd Handle to a specific NX window.
     */

    void synchronize(NXWINDOW hwnd, enum WindowType windowType);

#ifdef CONFIG_NXTERM_NXKBDIN
    /**
     * By default, NX keyboard input is given to the various widgets
     * residing in the window. But NxTerm is a different usage model;
     * In this case, keyboard input needs to be directed to the NxTerm
     * character driver.  This method can be used to enable (or disable)
     * redirection of NX keyboard input from the window widgets to the
     * NxTerm
     *
     * @param handle.  The NXTERM handle.  If non-NULL, NX keyboard
     *    input will be directed to the NxTerm driver using this
     *    handle;  If NULL (the default), NX keyboard input will be
     *    directed to the widgets within the window.
     */

    inline void setNxTerm(NXTERM handle)
    {
      m_nxterm = handle;
    }
#endif
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CCALLBACK_HXX

