/****************************************************************************
 * apps/include/graphics/nxwm/capplicationwindow.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_CAPPLICATIONWINDOW_NXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_CAPPLICATIONWINDOW_NXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "graphics/nxwidgets/cnxtkwindow.hxx"
#include "graphics/nxwidgets/cnxtoolbar.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cimage.hxx"
#include "graphics/nxwidgets/clabel.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"

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
   * This class represents that application window.  This class contains that the
   * framed window and its toolbar.  It manages callbacks from the toolbar minimize
   * and close buttions and passes these to the application via callbacks.
   */

  class CApplicationWindow : public IApplicationWindow,
                             private NXWidgets::CWidgetEventHandler
  {
  public:
    /**
     * Enumeration describing the bit settings for each window flag
     */

    enum EWindowFlags
    {
      WINDOW_PERSISTENT = 0x01  /**< Persistent windows have no stop button */
    };

  protected:
    NXWidgets::CNxTkWindow       *m_window;         /**< The framed window used by the application */
    NXWidgets::CNxToolbar        *m_toolbar;        /**< The toolbar */
    NXWidgets::CImage            *m_minimizeImage;  /**< The minimize icon */
    NXWidgets::CImage            *m_stopImage;      /**< The close icon */
    NXWidgets::CLabel            *m_windowLabel;    /**< The window title */
    NXWidgets::CRlePaletteBitmap *m_minimizeBitmap; /**< The minimize icon bitmap */
    NXWidgets::CRlePaletteBitmap *m_stopBitmap;     /**< The stop icon bitmap */
    NXWidgets::CNxFont           *m_windowFont;     /**< The font used to rend the window label */
    IApplicationCallback         *m_callback;       /**< Toolbar action callbacks */
    uint8_t                       m_flags;          /**< Window flags */

    /**
     * Handle a widget action event.  For CImage, this is a mouse button pre-release event.
     *
     * @param e The event data.
     */

    void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

  public:

    /**
     * CApplicationWindow Constructor
     *
     * @param window.  The window to be used by this application.
     * @param flags.  Optional flags to control the window configuration (See EWindowFlags).
     */

    CApplicationWindow(NXWidgets::CNxTkWindow *window, uint8_t flags = 0);

    /**
     * CApplicationWindow Destructor
     */

    ~CApplicationWindow(void);

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

    /**
     * Check if this window is configured for a persistent application (i.e.,
     * an application that has no STOP icon
     *
     * @return True if the window is configured for a persistent application.
     */

    inline bool isPersistent(void) const
    {
      return (m_flags & WINDOW_PERSISTENT) != 0;
    }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_CAPPLICATIONWINDOW_NXX
