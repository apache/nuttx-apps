/****************************************************************************
 * apps/include/graphics/nxwm/cnxstart.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_CSTARTWINDOW_NXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_CSTARTWINDOW_NXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>

#include "graphics/nxwidgets/tnxarray.hxx"

#include "graphics/nxwm/iapplication.hxx"
#include "graphics/nxwm/capplicationwindow.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Abstract Base Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NxWM
{
  /**
   * Forward references
   */

  class CTaskbar;

  /**
   * Start window message opcodes and format
   */

  enum EStartWindowMessageOpcodes
  {
    MSGID_POSITIONAL_CHANGE = 1, /**< Change in window positional data (not used) */
    MSGID_REDRAW_REQUEST,        /**< Request to redraw a portion of the window (not used) */
    MSGID_MOUSE_INPUT,           /**< New mouse input is available */
    MSGID_KEYBOARD_INPUT,        /**< New keyboard input is available */
    MSGID_DESTROY_APP            /**< Destroy the application */
  };

  struct SStartWindowMessage
  {
    enum EStartWindowMessageOpcodes msg_id;   /**< The message opcode */
    FAR void                       *instance; /**< Object instance. */
  };

  /**
   * The well-known name for the Start Window's message queue.
   */

  extern FAR const char *g_startWindowMqName;

  /**
   * This class is the the start window application.
   */

  class CStartWindow : public IApplication,
                       protected IApplicationCallback,
                       protected NXWidgets::CWidgetEventHandler
  {
  protected:
    /**
     * This structure represents an application and its associated icon image
     */

    struct SStartWindowSlot
    {
      IApplicationFactory            *app;      /**< A reference to the icon */
      NXWidgets::CImage              *image;    /**< The icon image that goes with the application */
    };

    /**
     * CStartWindow state data
     */

    CTaskbar                         *m_taskbar;   /**< Reference to the "parent" taskbar */
    CApplicationWindow               *m_window;    /**< Reference to the application window */
    TNxArray<struct SStartWindowSlot> m_slots;     /**< List of apps in the start window */
    struct nxgl_size_s                m_iconSize;  /**< A box big enough to hold the largest icon */

    /**
     * This is the start window task.  This function receives window events from
     * the NX listener threads indirectly through this sequence:
     *
     * 1. The NX listener thread receives a windows event.  The NX listener thread
     *    which is part of CTaskBar and was created when NX server connection was
     *    established).  This event may be a positional change notification, a
     *    redraw request, or mouse or keyboard input.
     * 2. The NX listener thread handles the message by calling nx_eventhandler().
     *    nx_eventhandler() dispatches the message by calling a method in the
     *    NXWidgets::CCallback instance associated with the window.
     *    NXWidgets::CCallback is a part of the CWidgetControl.
     * 3. NXWidgets::CCallback calls into NXWidgets::CWidgetControl to process
     *    the event.
     * 4. NXWidgets::CWidgetControl records the new state data and raises a
     *    window event.
     * 5. NXWidgets::CWindowEventHandlerList will give the event to
     *    NxWM::CWindowMessenger.
     * 6. NxWM::CWindowMessenger will send the a message on a well-known message
     *    queue.
     * 7. This CStartWindow::startWindow task will receive and process that
     *    message.
     */

    static int startWindow(int argc, char *argv[]);

    /**
     * Called when the window minimize button is pressed.
     */

    void minimize(void);

    /**
     * Called when the window close button is pressed.
     */

    void close(void);

    /**
     * Calculate the icon bounding box
     */

     void getIconBounds(void);

    /**
     * Stop all applications
     */

    void removeAllApplications(void);

    /**
     * Handle a widget action event.  For CButtonArray, this is a mouse
     * button pre-release event.
     *
     * @param e The event data.
     */

    void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

  public:

    /**
     * CStartWindow Constructor
     *
     * @param taskbar.  A pointer to the parent task bar instance
     * @param window.  The window to be used by this application.
     */

    CStartWindow(CTaskbar *taskbar, CApplicationWindow *window);

    /**
     * CStartWindow Constructor
     */

    ~CStartWindow(void);

    /**
     * Each implementation of IApplication must provide a method to recover
     * the contained CApplicationWindow instance.
     */

    IApplicationWindow *getWindow(void) const;

    /**
     * Get the icon associated with the application
     *
     * @return An instance if IBitmap that may be used to rend the
     *   application's icon.  This is an new IBitmap instance that must
     *   be deleted by the caller when it is no long needed.
     */

    NXWidgets::IBitmap *getIcon(void);

    /**
     * Get the name string associated with the application
     *
     * @return A copy if CNxString that contains the name of the application.
     */

    NXWidgets::CNxString getName(void);

    /**
     * Start the application.
     *
     * @return True if the application was successfully started.
     */

    bool run(void);

    /**
     * Stop the application.
     */

    void stop(void);

    /**
     * Destroy the application and free all of its resources.  This method
     * will initiate blocking of messages from the NX server.  The server
     * will flush the window message queue and reply with the blocked
     * message.  When the block message is received by CWindowMessenger,
     * it will send the destroy message to the start window task which
     * will, finally, safely delete the application.
     */

    void destroy(void);

    /**
     * The application window is hidden (either it is minimized or it is
     * maximized, but not at the top of the hierarchy)
     */

    void hide(void);

    /**
     * Redraw the entire window.  The application has been maximized or
     * otherwise moved to the top of the hierarchy.  This method is call from
     * CTaskbar when the application window must be displayed
     */

    void redraw(void);

    /**
     * Report of this is a "normal" window or a full screen window.  The
     * primary purpose of this method is so that window manager will know
     * whether or not it show draw the task bar.
     *
     * @return True if this is a full screen window.
     */

    bool isFullScreen(void) const;

    /**
     * Add the application to the start window.  The general sequence for
     * setting up the start window is:
     *
     * 1. Call IAppicationFactory::create to a new instance of the application
     * 2. Call CStartWindow::addApplication to add the application to the
     *    start window.
     *
     * @param app.  The new application to add to the start window
     * @return true on success
     */

    bool addApplication(IApplicationFactory *app);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_CSTARTWINDOW_NXX
