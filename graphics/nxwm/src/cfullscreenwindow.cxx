/********************************************************************************************
 * apps/graphics/nxwm/src/cfullscreenwindow.cxx
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
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"

#include "graphics/nxwm/nxwmconfig.hxx"
#include "graphics/nxglyphs.hxx"
#include "graphics/nxwm/cfullscreenwindow.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * CFullScreenWindow Method Implementations
 ********************************************************************************************/

using namespace NxWM;

/**
 * CFullScreenWindow Constructor
 *
 * @param window.  The raw window to be used by this application.
 */

CFullScreenWindow::CFullScreenWindow(NXWidgets::CNxWindow *window)
{
  // Save the window for later use

  m_window = window;
}

/**
 * CFullScreenWindow Destructor
 */

CFullScreenWindow::~CFullScreenWindow(void)
{
  // We didn't create the window.  That was done by the task bar,
  // But we will handle destruction of with window as a courtesy.

  if (m_window)
    {
      delete m_window;
    }
}

/**
 * Initialize window.  Window initialization is separate from
 * object instantiation so that failures can be reported.
 *
 * @return True if the window was successfully initialized.
 */

bool CFullScreenWindow::open(void)
{
  return true;
}

/**
 * Re-draw the application window
 */

void CFullScreenWindow::redraw(void)
{
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but not at the top of the hierarchy)
 */

void CFullScreenWindow::hide(void)
{
}

/**
 * Recover the contained raw window instance
 *
 * @return.  The window used by this application
 */

NXWidgets::INxWindow *CFullScreenWindow::getWindow(void) const
{
  return static_cast<NXWidgets::INxWindow*>(m_window);
}

/**
 * Recover the contained widget control
 *
 * @return.  The widget control used by this application
 */

NXWidgets::CWidgetControl *CFullScreenWindow::getWidgetControl(void) const
{
  return m_window->getWidgetControl();
}

/**
 * Block further activity on this window in preparation for window
 * shutdown.
 *
 * @param app. The application to be blocked
 */

void CFullScreenWindow::block(IApplication *app)
{
  // Get the widget control from the NXWidgets::CNxWindow instance

  NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

  // And then block further reporting activity on the underlying
  // NX raw window

  nx_block(control->getWindowHandle(), (FAR void *)app);
}

/**
 * Set the window label
 *
 * @param appname.  The name of the application to place on the window
 */

void CFullScreenWindow::setWindowLabel(NXWidgets::CNxString &appname)
{
}

/**
 * Report of this is a "normal" window or a full screen window.  The
 * primary purpose of this method is so that window manager will know
 * whether or not it show draw the task bar.
 *
 * @return True if this is a full screen window.
 */

bool CFullScreenWindow::isFullScreen(void) const
{
  return true;
}

/**
 * Register to receive callbacks when toolbar icons are selected
 */

void CFullScreenWindow::registerCallbacks(IApplicationCallback *callback)
{
}
