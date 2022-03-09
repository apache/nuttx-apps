/********************************************************************************************
 * apps/graphics/nxwm/src/capplicationwindow.cxx
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

#include "graphics/nxglyphs.hxx"
#include "graphics/nxwm/nxwmconfig.hxx"
#include "graphics/nxwm/cwindowmessenger.hxx"
#include "graphics/nxwm/capplicationwindow.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

#ifdef CONFIG_NXWM_STOP_BITMAP
extern const struct NXWidgets::SRlePaletteBitmap CONFIG_NXWM_STOP_BITMAP;
#else
#  define CONFIG_NXWM_STOP_BITMAP NXWidgets::g_stopBitmap
#endif

#ifdef CONFIG_NXWM_MINIMIZE_BITMAP
extern const struct NXWidgets::SRlePaletteBitmap CONFIG_NXWM_MINIMIZE_BITMAP;
#else
#  define CONFIG_NXWM_MINIMIZE_BITMAP NXWidgets::g_minimizeBitmap
#endif

/********************************************************************************************
 * CApplicationWindow Method Implementations
 ********************************************************************************************/

 using namespace NxWM;

/**
 * CApplicationWindow Constructor
 *
 * @param taskbar.  A pointer to the parent task bar instance.
 * @param flags.  Optional flags to control the window configuration (See EWindowFlags).
 */

CApplicationWindow::CApplicationWindow(NXWidgets::CNxTkWindow *window, uint8_t flags)
{
  // Save the window and window flags for later use

  m_window          = window;
  m_flags           = flags;

  // These will be created with the open method is called

  m_toolbar         = (NXWidgets::CNxToolbar        *)0;

  m_minimizeImage   = (NXWidgets::CImage            *)0;
  m_stopImage       = (NXWidgets::CImage            *)0;
  m_windowLabel     = (NXWidgets::CLabel            *)0;

  m_minimizeBitmap  = (NXWidgets::CRlePaletteBitmap *)0;
  m_stopBitmap      = (NXWidgets::CRlePaletteBitmap *)0;

  m_windowFont      = (NXWidgets::CNxFont           *)0;

  // This will be initialized when the registerCallbacks() method is called

  m_callback        = (IApplicationCallback         *)0;
}

/**
 * CApplicationWindow Destructor
 */

CApplicationWindow::~CApplicationWindow(void)
{
  // Free the resources that we are responsible for

  if (m_minimizeImage)
    {
      delete m_minimizeImage;
    }

  if (m_stopImage)
    {
      delete m_stopImage;
    }

  if (m_windowLabel)
    {
      delete m_windowLabel;
    }

  if (m_minimizeBitmap)
    {
      delete m_minimizeBitmap;
    }

  if (m_stopBitmap)
    {
      delete m_stopBitmap;
    }

  if (m_windowFont)
    {
      delete m_windowFont;
    }

  if (m_toolbar)
    {
      delete m_toolbar;
    }

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

bool CApplicationWindow::open(void)
{
  // Create the widget control (with the window messenger) using the default style

  CWindowMessenger *control = new CWindowMessenger();
  if (!control)
    {
      return false;
    }

  // Open the toolbar using the widget control

  m_toolbar = m_window->openToolbar(CONFIG_NXWM_TOOLBAR_HEIGHT, control);
  if (!m_toolbar)
    {
      // We failed to open the toolbar

      return false;
    }

  // Get the width of the display

  struct nxgl_size_s windowSize;
  if (!m_toolbar->getSize(&windowSize))
    {
      return false;
    }

  // Start positioning icons from the right side of the tool bar

  struct nxgl_point_s iconPos;
  struct nxgl_size_s  iconSize;

  iconPos.x = windowSize.w;

  // Create the STOP icon only if this is a non-persistent application

  if (!isPersistent())
    {
      // Create STOP bitmap container

      m_stopBitmap = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_STOP_BITMAP);
      if (!m_stopBitmap)
        {
          return false;
        }

      // Create the STOP application icon at the right of the toolbar
      // Get the height and width of the stop bitmap

      iconSize.w = m_stopBitmap->getWidth();
      iconSize.h = m_stopBitmap->getHeight();

      // The default CImage has borders enabled with thickness of the border
      // width.  Add twice the thickness of the border to the width and height.
      // (We could let CImage do this for us by calling
      // CImage::getPreferredDimensions())

      iconSize.w += 2 * 1;
      iconSize.h += 2 * 1;

      // Pick an X/Y position such that the image will position at the right of
      // the toolbar and centered vertically.

      iconPos.x -= iconSize.w;

      if (iconSize.h >= windowSize.h)
        {
          iconPos.y = 0;
        }
      else
        {
          iconPos.y = (windowSize.h - iconSize.h) >> 1;
        }

      // Now we have enough information to create the image

      m_stopImage = new NXWidgets::CImage(control, iconPos.x, iconPos.y, iconSize.w,
                                          iconSize.h, m_stopBitmap);
      if (!m_stopImage)
        {
          return false;
        }

      // Configure 'this' to receive mouse click inputs from the image

      m_stopImage->setBorderless(true);
      m_stopImage->addWidgetEventHandler(this);
    }

#ifndef CONFIG_NXWM_DISABLE_MINIMIZE
  // Create MINIMIZE application bitmap container

  m_minimizeBitmap = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MINIMIZE_BITMAP);
  if (!m_minimizeBitmap)
    {
      return false;
    }

  // Get the height and width of the stop bitmap

  iconSize.w = m_minimizeBitmap->getWidth();
  iconSize.h = m_minimizeBitmap->getHeight();

  // The default CImage has borders enabled with thickness of the border
  // width.  Add twice the thickness of the border to the width and height.
  // (We could let CImage do this for us by calling
  // CImage::getPreferredDimensions())

  iconSize.w += 2 * 1;
  iconSize.h += 2 * 1;

  // Pick an X/Y position such that the image will position at the right of
  // the toolbar and centered vertically.

  iconPos.x -= iconSize.w;

  if (iconSize.h >= windowSize.h)
    {
      iconPos.y = 0;
    }
  else
    {
      iconPos.y = (windowSize.h - iconSize.h) >> 1;
    }

  // Now we have enough information to create the image

  m_minimizeImage = new NXWidgets::CImage(control, iconPos.x, iconPos.y, iconSize.w,
                                          iconSize.h, m_minimizeBitmap);
  if (!m_minimizeImage)
    {
      return false;
    }

  // Configure 'this' to receive mouse click inputs from the image

  m_minimizeImage->setBorderless(true);
  m_minimizeImage->addWidgetEventHandler(this);
#endif

  // The rest of the toolbar will hold the left-justified application label
  // Create the default font instance

  m_windowFont = new NXWidgets::CNxFont((nx_fontid_e)CONFIG_NXWM_TOOLBAR_FONTID,
                                        CONFIG_NXWM_DEFAULT_FONTCOLOR,
                                        CONFIG_NXWM_TRANSPARENT_COLOR);
  if (!m_windowFont)
    {
      return false;
    }

  // Get the height and width of the text display area

  iconSize.w = iconPos.x;
  iconSize.h = windowSize.h;

  iconPos.x  = 0;
  iconPos.y  = 0;

  // Now we have enough information to create the label

  m_windowLabel = new NXWidgets::CLabel(control, iconPos.x, iconPos.y,
                                        iconSize.w, iconSize.h, "");
  if (!m_windowLabel)
    {
      return false;
    }

  // Configure the label

  m_windowLabel->setFont(m_windowFont);
  m_windowLabel->setBorderless(true);
  m_windowLabel->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_LEFT);
  m_windowLabel->setTextAlignmentVert(NXWidgets::CLabel::TEXT_ALIGNMENT_VERT_CENTER);
  m_windowLabel->setRaisesEvents(false);
  return true;
}

/**
 * Re-draw the application window
 */

void CApplicationWindow::redraw(void)
{
  // Get the widget control from the task bar

  NXWidgets::CWidgetControl *control = m_toolbar->getWidgetControl();

  // Get the graphics port for drawing on the background window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the window

  struct nxgl_size_s windowSize;
  if (!m_toolbar->getSize(&windowSize))
    {
      return;
    }

  // Fill the entire tool bar with the non-shadowed border color

  port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                       CONFIG_NXTK_BORDERCOLOR1);

  // Then draw the stop image (which may not be present if this is a
  // "persistent" application)

  if (m_stopImage)
    {
      m_stopImage->enableDrawing();
      m_stopImage->redraw();
      m_stopImage->setRaisesEvents(true);
    }

  // Draw the minimize image (which may not be present if this is a
  // mimimization is disabled)

  if (m_minimizeImage)
    {
      m_minimizeImage->enableDrawing();
      m_minimizeImage->redraw();
      m_minimizeImage->setRaisesEvents(true);
    }

  // And finally draw the window label

  m_windowLabel->enableDrawing();
  m_windowLabel->redraw();
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but not at the top of the hierarchy)
 */

void CApplicationWindow::hide(void)
{
  // Disable the stop image (which may not be present if this is a
  // "persistent" application)

  if (m_stopImage)
    {
      m_stopImage->disableDrawing();
      m_stopImage->setRaisesEvents(false);
    }

  // Disable the minimize image(which may not be present if this is a
  // mimimization is disabled)

  if (m_minimizeImage)
  {
    m_minimizeImage->disableDrawing();
    m_minimizeImage->setRaisesEvents(false);
  }

  // Disable the window label

  m_windowLabel->disableDrawing();
}

/**
 * Recover the contained NXTK window instance
 *
 * @return.  The window used by this application
 */

NXWidgets::INxWindow *CApplicationWindow::getWindow(void) const
{
  return static_cast<NXWidgets::INxWindow*>(m_window);
}

/**
 * Recover the contained widget control
 *
 * @return.  The widget control used by this application
 */

NXWidgets::CWidgetControl *CApplicationWindow::getWidgetControl(void) const
{
  return m_window->getWidgetControl();
}

/**
 * Block further activity on this window in preparation for window
 * shutdown.
 *
 * @param app. The application to be blocked
 */

void CApplicationWindow::block(IApplication *app)
{
  // Get the widget control from the NXWidgets::CNxWindow instance

  NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

  // And then block further reporting activity on the underlying
  // NX framed window

  nxtk_block(control->getWindowHandle(), (FAR void *)app);
}

/**
 * Set the window label
 *
 * @param appname.  The name of the application to place on the window
 */

void CApplicationWindow::setWindowLabel(NXWidgets::CNxString &appname)
{
  m_windowLabel->setText(appname);
}

/**
 * Report of this is a "normal" window or a full screen window.  The
 * primary purpose of this method is so that window manager will know
 * whether or not it show draw the task bar.
 *
 * @return True if this is a full screen window.
 */

bool CApplicationWindow::isFullScreen(void) const
{
  return false;
}

/**
 * Register to receive callbacks when toolbar icons are selected
 */

void CApplicationWindow::registerCallbacks(IApplicationCallback *callback)
{
  m_callback = callback;
}

/**
 * Handle a widget action event.  For CImage, this is a mouse button pre-release event.
 *
 * @param e The event data.
 */

void CApplicationWindow::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Ignore the event if no callback is registered

  if (m_callback)
    {
      // Check the stop application image

      if (m_stopImage && m_stopImage->isClicked())
        {
          // Notify the controlling logic that the application should be stopped

          m_callback->close();
        }

      // Check the minimize image (only if the stop application image is not pressed)

      else if (m_minimizeImage && m_minimizeImage->isClicked())
        {
          // Notify the controlling logic that the application should be miminsed

          m_callback->minimize();
        }
    }
}
