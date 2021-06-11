/****************************************************************************
 * apps/include/graphics/nxwm/cfullscreenwindow.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_CFULLSCREENWINDOW_NXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_CFULLSCREENWINDOW_NXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"

#include "graphics/nxwm/iapplicationwindow.hxx"

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
   * This class represents a generic window.  This bland window is used,
   * for example, to support full screen displays.
   */

  class CFullScreenWindow : public IApplicationWindow
  {
  protected:
    NXWidgets::CNxWindow *m_window; /**< The generic window used by the application */

  public:

    /**
     * CFullScreenWindow Constructor
     *
     * @param window.  The raw window to be used by this full screen application.
     */

    CFullScreenWindow(NXWidgets::CNxWindow *window);

    /**
     * CFullScreenWindow Destructor
     */

    ~CFullScreenWindow(void);

    /**
     * Initialize window.  Window initialization is separate from
     * object instantiation so that failures can be reported.
     *
     * @return True if the window was successfully initialized.
     */

    bool open(void);

    /**
     * Re-draw the application window
     */

    void redraw(void);

    /**
     * The application window is hidden (either it is minimized or it is
     * maximized, but not at the top of the hierarchy)
     */

    void hide(void);

    /**
     * Recover the contained NXTK window instance
     *
     * @return.  The window used by this application
     */

    NXWidgets::INxWindow *getWindow(void) const;

    /**
     * Recover the contained widget control
     *
     * @return.  The widget control used by this application
     */

    NXWidgets::CWidgetControl *getWidgetControl(void) const;

    /**
     * Block further activity on this window in preparation for window
     * shutdown.
     *
     * @param app. The application to be blocked
     */

    void block(IApplication *app);

    /**
     * Set the window label
     *
     * @param appname.  The name of the application to place on the window
     */

    void setWindowLabel(NXWidgets::CNxString &appname);

    /**
     * Report of this is a "normal" window or a full screen window.  The
     * primary purpose of this method is so that window manager will know
     * whether or not it show draw the task bar.
     *
     * @return True if this is a full screen window.
     */

    bool isFullScreen(void) const;

    /**
     * Register to receive callbacks when toolbar icons are selected
     */

    void registerCallbacks(IApplicationCallback *callback);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_CFULLSCREENWINDOW_NXX
