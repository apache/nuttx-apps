/****************************************************************************
 * apps/include/graphics/nxwm/cnxterm.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_CNXTERM_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_CNXTERM_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxterm.h>

#include "graphics/nxwm/iapplication.hxx"
#include "graphics/nxwm/capplicationwindow.hxx"
#include "graphics/nxwm/ctaskbar.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NxWM
{
  /**
   * One time NSH initialization. This function must be called exactly
   * once during the boot-up sequence to initialize the NSH library.
   *
   * @return True on successful initialization
   */

  bool nshlibInitialize(void);

  /**
   * This class implements the NxTerm application.
   */

  class CNxTerm : public IApplication, private IApplicationCallback
  {
  private:
    CTaskbar            *m_taskbar;     /**< Reference to the "parent" taskbar */
    CApplicationWindow  *m_window;      /**< Reference to the application window */
    NXTERM               m_nxterm;      /**< NxTerm handle */
    pid_t                m_pid;         /**< Task ID of the NxTerm thread */
    int                  m_minor;       /**< Terminal device minor number */

    /**
     * This is the NxTerm task.  This function first redirects output to the
     * console window then calls to start the NSH logic.
     */

    static int nxterm(int argc, char *argv[]);

    /**
     * This is the NxTerm task exit handler.  It is registered with on_exit()
     * and called automatically when the nxterm task exits.
     */

    static void exitHandler(int code, FAR void *arg);

    /**
     * Called when the window minimize button is pressed.
     */

    void minimize(void);

    /**
     * Called when the window close button is pressed.
     */

    void close(void);

  public:
    /**
     * CNxTerm constructor
     *
     * @param window.  The application window
     *
     * @param taskbar.  A pointer to the parent task bar instance
     * @param window.  The window to be used by this application.
     */

    CNxTerm(CTaskbar *taskbar, CApplicationWindow *window);

    /**
     * CNxTerm destructor
     */

    ~CNxTerm(void);

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
     * Start the application (perhaps in the minimized state).
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
     * maximized, but not at the top of the hierarchy
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
  };

  class CNxTermFactory : public IApplicationFactory
  {
  private:
    CTaskbar *m_taskbar;  /**< The taskbar */

  public:
    /**
     * CNxTermFactory Constructor
     *
     * @param taskbar.  The taskbar instance used to terminate calibration
     */

    CNxTermFactory(CTaskbar *taskbar);

    /**
     * CNxTermFactory Destructor
     */

    inline ~CNxTermFactory(void) { }

    /**
     * Create a new instance of an CNxTerm (as IApplication).
     */

    IApplication *create(void);

    /**
     * Get the icon associated with the application
     *
     * @return An instance if IBitmap that may be used to rend the
     *   application's icon.  This is an new IBitmap instance that must
     *   be deleted by the caller when it is no long needed.
     */

    NXWidgets::IBitmap *getIcon(void);
  };
}
#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_CNXTERM_HXX
