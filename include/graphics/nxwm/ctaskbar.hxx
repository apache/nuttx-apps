/****************************************************************************
 * apps/include/graphics/nxwm/cnxtaskbar.hxx
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

#ifndef __NXWM_INCLUDE_CTASKBAR_HXX
#define __NXWM_INCLUDE_CTASKBAR_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/tnxarray.hxx"
#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cnxserver.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

#include "graphics/nxglyphs.hxx"

#include "graphics/nxwm/nxwmconfig.hxx"
#include "graphics/nxwm/capplicationwindow.hxx"
#include "graphics/nxwm/cfullscreenwindow.hxx"
#include "graphics/nxwm/iapplication.hxx"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Class Definition
 ****************************************************************************/

#if defined(__cplusplus)

namespace NxWM
{
  /**
   * This class describes the NX window manager's task bar.  That task bar is,
   * of course, used to dock active applications.  But in NxWM, it is also
   * the heart of the window manager:  It inherits for CNxServer and, hence,
   * represents the NX server itself.  It also then serves as the NxWM
   * window factory.
   *
   * Why do it this way?  The only reason is so that if you have an instance
   * of CTaskbar, you have everything you need to manage new applications.
   * It might have been a better decision to separate the window factory into
   * a separate class instead of making the task bar of such central importance
   * (and I may still do that someday)
   */

  class CTaskbar : public  NXWidgets::CNxServer,
                   protected NXWidgets::CWidgetEventHandler
  {
  protected:
    /**
     * This structure represents an application and its associated icon image
     */

    struct STaskbarSlot
    {
      IApplication      *app;    /**< A reference to the application */
      NXWidgets::CImage *image;  /**< The icon image for the application */
    };

    /**
     * Taskbar state
     */

    NXWidgets::CNxWindow         *m_taskbar;    /**< The task bar window */
    NXWidgets::CBgWindow         *m_background; /**< The background window */
    NXWidgets::CImage            *m_backImage;  /**< The background image */
    IApplication                 *m_topApp;     /**< The top application in the hierarchy */
    TNxArray<struct STaskbarSlot> m_slots;      /**< List of application slots in the task bar */
    bool                          m_started;    /**< True if window manager has been started */

    /**
     * Create a raw window.
     *
     * 1) Create a dumb NXWidgets::CWidgetControl instance  (See not).
     * 2) Pass the dumb NXWidgets::CWindowMessenger instance to the window constructor
     *    that inherits from INxWindow.  This will "smarten" the NXWidgets::CWidgetControl
     *    instance with some window knowlede
     * 3) Call the open() method on the window to display the window.
     * 4) After that, the fully smartened NXWidgets::CWidgetControl instance can
     *    be used to generate additional widgets by passing it to the
     *    widget constructor
     *
     * NOTE:  Actually, NxWM uses the CWindowMessenger class that inherits from
     * CWidgetControl.  That class just adds some unrelated messaging capability;
     * It cohabitates with CWidgetControl only because it needs the CWidgetControl
     * this point.
     */

    NXWidgets::CNxWindow *openRawWindow(void);

    /**
     * Create a framed application window
     *
     * This may be used to provide the window parater to the IApplication constructor
     *
     * @return A partially initialized application window instance.
     */

    NXWidgets::CNxTkWindow *openFramedWindow(void);

    /**
     * Set size and position of a window in the application area.
     *
     * @param window.   The window to be resized and repositioned
     * @param fullscreen.  True: Use full screen
     */

    void setApplicationGeometry(NXWidgets::INxWindow *window, bool fullscreen);

    /**
     * Create the task bar window.
     *
     * @return true on success
     */

    virtual bool createTaskbarWindow(void);

    /**
     * Create the background window.
     *
     * @return true on success
     */

    virtual bool createBackgroundWindow(void);

    /**
     * Create the background image.
     *
     * @return true on success
     */

    virtual bool createBackgroundImage(void);

    /**
     * (Re-)draw the task bar window.
     *
     * @return true on success
     */

    virtual bool redrawTaskbarWindow(void);

    /**
     * Redraw the window at the top of the hierarchy.
     *
     * @return true on success
     */

    virtual bool redrawTopApplication(void);

    /**
     * Raise the top window to the top of the NXheirarchy.
     *
     * @return true on success
     */

    void raiseTopApplication(void);

    /**
     * (Re-)draw the background window.
     *
     * @return true on success
     */

    virtual bool redrawBackgroundWindow(void);

    /**
     * Redraw the last application in the list of application maintained by
     * the task bar.
     *
     * @param app. The new top application to draw
     * @return true on success
     */

    bool redrawApplicationWindow(IApplication *app);

    /**
     * The application window is hidden (either it is minimized or it is
     * maximized, but not at the top of the hierarchy)
     *
     * @param app. The application to hide
     */

    void hideApplicationWindow(IApplication *app);

    /**
     * Handle a widget action event.  For CImage, this is a mouse button pre-release event.
     *
     * @param e The event data.
     */

    void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

  public:
    /**
     * CTaskbar Constructor
     *
     * @param hWnd - NX server handle
     */

    CTaskbar(void);

    /**
     * CTaskbar Destructor
     */

    ~CTaskbar(void);

    /**
     * Connect to the server
     */

    bool connect(void);

    /**
     * Disconnect from the server
     */

    void disconnect(void);

    /**
     * Initialize task bar.  Task bar initialization is separate from
     * object instantiation so that failures can be reported.  The window
     * manager start-up sequence is:
     *
     * 1. Create the CTaskbar instance,
     * 2. Call the CTaskbar::connect() method to connect to the NX server (CTaskbar
     *    inherits the connect method from CNxServer),
     * 3. Call the CTaskbar::initWindowManager() method to initialize the task bar.
     * 4. Call CTaskBar::startApplication repeatedly to add applications to the task bar
     * 5. Call CTaskBar::startWindowManager() to start the display with applications in place
     *
     * CTaskbar::initWindowManager() prepares the task bar to receive applications.
     * CTaskBar::startWindowManager() brings the window manager up with those applications
     * in place.
     *
     * @return True if the window was successfully initialized.
     */

    bool initWindowManager(void);

    /**
     * Start the window manager and present the initial displays.  The window
     * manager start-up sequence is:
     *
     * 1. Create the CTaskbar instance,
     * 2. Call the CTaskbar::connect() method to connect to the NX server (CTaskbar
     *    inherits the connect method from CNxServer),
     * 3. Call the CTaskbar::initWindowManager() method to initialize the task bar.
     * 4. Call CTaskBar::startApplication repeatedly to add applications to the task bar
     * 5. Call CTaskBar::startWindowManager to start the display with applications in place
     *
     * CTaskbar::initWindowManager() prepares the task bar to receive applications.
     * CTaskBar::startWindowManager() brings the window manager up with those applications
     * in place.
     *
     * CTaskBar::startWindowManager() will present the task bar and the background image.
     * The The initial taskbar will contain only the start window icon.
     *
     * @return true on success
     */

    bool startWindowManager(void);

    /**
     * Create an normal application window.  Creating a normal application in the
     * start window requires three steps:
     *
     * 1. Call CTaskBar::openApplicationWindow to create a window for the application,
     * 2. Instantiate the application, providing the window to the application's
     *    constructor,
     * 3. Then call CStartWindow::addApplication to add the application to the
     *    start window.
     *
     * When the application is selected from the start window:
     *
     * 4. Call CTaskBar::startApplication start the application and bring its window to
     *    the top.
     *
     * @param flags. CApplicationWindow flugs for window customization.
     */

    CApplicationWindow *openApplicationWindow(uint8_t flags = 0);

    /**
     * Create a full screen application window.  Creating a new full screen application
     * requires three steps:
     *
     * 1. Call CTaskBar::FullScreenWindow to create a window for the application,
     * 2. Instantiate the application, providing the window to the application's
     *    constructor,
     * 3. Then call CStartWindow::addApplication to add the application to the
     *    start window.
     *
     * When the application is selected from the start window:
     *
     * 4. Call CTaskBar::startApplication start the application and bring its window to
     *    the top.
     */

    CFullScreenWindow *openFullScreenWindow(void);

    /**
     * Start an application and add its icon to the taskbar.  The applications's
     * window is brought to the top.  Creating a new application in the start
     * window requires three steps:
     *
     * 1. Call CTaskBar::openApplicationWindow to create a window for the application,
     * 2. Instantiate the application, providing the window to the application's
     *    constructor,
     * 3. Then call CStartWindow::addApplication to add the application to the start window.
     *
     * When the application is selected from the start window:
     *
     * 4. Call CTaskBar::startApplication start the application and bring its window to
     *    the top.
     *
     * @param app.  The new application to add to the task bar
     * @param minimized.  The new application starts in the minimized state
     * @return true on success
     */

    bool startApplication(IApplication *app, bool minimized);

    /**
     * Move window to the top of the hierarchy and re-draw it.  This method
     * does nothing if the application is minimized.
     *
     * @param app.  The new application to show
     * @return true on success
     */

    bool topApplication(IApplication *app);

    /**
     * Maximize an application by moving its window to the top of the hierarchy
     * and re-drawing it.  If the application was already maximized, then this
     * method is equivalent to topApplication().
     *
     * @param app.  The new application to add to the task bar
     * @return true on success
     */

    bool maximizeApplication(IApplication *app);

    /**
     * Minimize an application by moving its window to the bottom of the
     * and redrawing the next visible application.
     *
     * @param app.  The new application to add to the task bar
     * @return true on success
     */

    bool minimizeApplication(IApplication *app);

    /**
     * Destroy an application.  Move its window to the bottom and remove its
     * icon from the task bar.
     *
     * @param app.  The new application to remove from the task bar
     * @return true on success
     */

    bool stopApplication(IApplication *app);

    /**
     * Get the size of the physical display device as it is known to the task
     * bar.
     *
     * @return The size of the display
     */

    void getDisplaySize(FAR struct nxgl_size_s &size);

    /**
     * Force a redraw of the taskbar and current application.
     * This should only be necessary if the display loses state due to e.g. powerdown
     * or other manual intervention.
     */

    inline void redraw() { redrawTopApplication(); }
  };
}

#endif // __cplusplus
#endif // __NXWM_INCLUDE_CTASKBAR_HXX
