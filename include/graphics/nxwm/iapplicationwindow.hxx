/****************************************************************************
 * apps/include/graphics/nxwm/iapplicationwindow.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_IAPPLICATIONWINDOW_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_IAPPLICATIONWINDOW_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "graphics/nxwidgets/inxwindow.hxx"
#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"

#include "graphics/nxwm/iapplication.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Abstract Base Class
 ****************************************************************************/

#if defined(__cplusplus)

namespace NxWM
{
  /**
   * Forward references
   */

  class IApplication;

  /**
   * This callback class is used by the application to get notification of toolbar
   * related events.
   */

  class IApplicationCallback
  {
  public:
    /**
     * Called when the window minimize button is pressed.
     */

    virtual void minimize(void) = 0;

    /**
     * Called when the window minimize close is pressed.
     */

    virtual void close(void) = 0;
  };

  /**
   * This class represents the general application window.  The actual window
   * may be a contained, framed window or and unframed, fullscreen window.
   */

  class IApplicationWindow
  {
  public:
    /**
     * A virtual destructor is required in order to override the IApplicationWindow
     * destructor.  We do this because if we delete IApplicationWindow, we want the
     * destructor of the class that inherits from IApplicationWindow to run, not this
     * one.
     */

      virtual ~IApplicationWindow(void) { }

    /**
     * Initialize window.  Window initialization is separate from
     * object instantiation so that failures can be reported.
     *
     * @return True if the window was successfully initialized.
     */

    virtual bool open(void) = 0;

    /**
     * Re-draw the application window
     */

    virtual void redraw(void) = 0;

    /**
     * The application window is hidden (either it is minimized or it is
     * maximized, but not at the top of the hierarchy)
     */

    virtual void hide(void) = 0;

    /**
     * Recover the contained window instance
     *
     * @return.  The window used by this application
     */

    virtual NXWidgets::INxWindow *getWindow(void) const = 0;

    /**
     * Recover the contained widget control
     *
     * @return.  The widget control used by this application
     */

    virtual NXWidgets::CWidgetControl *getWidgetControl(void) const = 0;

    /**
     * Block further activity on this window in preparation for window
     * shutdown.
     *
     * @param app. The application to be blocked
     */

    virtual void block(IApplication *app) = 0;

    /**
     * Set the window label
     *
     * @param appname.  The name of the application to place on the window
     */

    virtual void setWindowLabel(NXWidgets::CNxString &appname) = 0;

    /**
     * Report of this is a "normal" window or a full screen window.  The
     * primary purpose of this method is so that window manager will know
     * whether or not it show draw the task bar.
     *
     * @return True if this is a full screen window.
     */

    virtual bool isFullScreen(void) const = 0;

    /**
     * Register to receive callbacks when toolbar icons are selected
     */

    virtual void registerCallbacks(IApplicationCallback *callback) = 0;
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_IAPPLICATIONWINDOW_HXX
