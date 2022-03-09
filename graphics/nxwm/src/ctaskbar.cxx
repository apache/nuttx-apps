/********************************************************************************************
 * apps/graphics/nxwm/src/ctaskbar.cxx
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

#include <debug.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/crect.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cnxtkwindow.hxx"
#include "graphics/nxwidgets/cscaledbitmap.hxx"

#include "graphics/nxwm/cwindowmessenger.hxx"
#include "graphics/nxwm/ctaskbar.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * CTaskbar Method Implementations
 ********************************************************************************************/

using namespace NxWM;

/**
 * CTaskbar Constructor
 *
 * @param hWnd - NX server handle
 */

CTaskbar::CTaskbar(void)
{
  m_taskbar     = (NXWidgets::CNxWindow *)0;
  m_background  = (NXWidgets::CBgWindow *)0;
  m_backImage   = (NXWidgets::CImage    *)0;
  m_topApp      = (IApplication         *)0;
  m_started     = false;
}

/**
 * CTaskbar Destructor
 */

CTaskbar::~CTaskbar(void)
{
  // The disconnect,putting the instance back in the state that it
  // was before it was constructed.

  disconnect();
}

/**
 * Connect to the server
 */

bool CTaskbar::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Set the background color

      if (!setBackgroundColor(CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR))
        {
          // Failed
        }
    }

  return nxConnected;
}

/**
 * Disconnect from the server.  This method restores the taskbar to the
 * same state that it was in when it was constructed.
 */

void CTaskbar::disconnect(void)
{
  // Stop all applications and remove them from the task bar.  Clearly, there
  // are some ordering issues here... On an orderly system shutdown, disconnection
  // should really occur priority to deleting instances

  while (!m_slots.empty())
    {
       IApplication *app = m_slots.at(0).app;
       stopApplication(app);
    }

  // Close the windows

  NXWidgets::CWidgetControl *control;
  if (m_taskbar)
    {
      // Get the contained widget control

      control = m_taskbar->getWidgetControl();

      // Delete the widget control.  We are responsible for it because we created it

      if (control)
        {
          delete control;
        }

      // Then delete the task bar window

      delete m_taskbar;
      m_taskbar = (NXWidgets::CNxWindow *)0;
    }

  if (m_background)
    {
      // Delete the contained widget control.  We are responsible for it
      // because we created it

      control = m_background->getWidgetControl();
      if (control)
        {
          delete control;
        }

      // Then delete the background

      delete m_background;
      m_background = (NXWidgets::CBgWindow *)0;
    }

  // Delete the background image

  if (m_backImage)
    {
      delete m_backImage;
      m_backImage = (NXWidgets::CImage *)0;
    }

  // Reset other variables

  m_topApp  = (IApplication *)0;
  m_started = false;

  // And disconnect from the server

  CNxServer::disconnect();
}

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
 * 5. Call CTaskBar::startWindowManager to start the display with applications in place
 *
 * CTaskbar::initWindowManager() prepares the task bar to receive applications.
 * CTaskBar::startWindowManager() brings the window manager up with those applications
 * in place.
 *
 * @return True if the window was successfully initialized.
 */

bool CTaskbar::initWindowManager(void)
{
  // Create the taskbar window

  if (!createTaskbarWindow())
    {
      return false;
    }

  // Create the background window

  if (!createBackgroundWindow())
    {
      return false;
    }

  // Create the background image

  if (!createBackgroundImage())
    {
      return false;
    }

  return true;
}

/**
 * Start the window manager and present the initial displays.  The window
 * manager start-up sequence is:
 *
 * 1. Create the CTaskbar instance,
 * 2. Call startApplication repeatedly to add applications to the task bar
 * 3. Call startWindowManager to start the display with applications in place
 *
 * CTaskbar::initWindowManager() prepares the task bar to receive applications.
 * CTaskBar::startWindowManager() brings the window manager up with those applications
 * in place.
 *
 * startWindowManager will present the task bar and the background image.  The
 * initial taskbar will contain only the start window icon.
 *
 * @return true on success
 */

bool CTaskbar::startWindowManager(void)
{
  // Have we already been started

 if (!m_started)
    {
      // We are now started

      m_started = true;

      // Decide which application will be the initial 'top' application

      m_topApp = (IApplication *)0;
      int topIndex = -1;

      // No.. Search for that last, non-minimized application (there might not be one)

      for (int i = m_slots.size() - 1; i >= 0; i--)
        {
          IApplication *candidate = m_slots.at(i).app;
          if (!candidate->isMinimized())
            {
              m_topApp = candidate;
              topIndex = i;
              break;
            }
        }
      ginfo("m_topApp=%p topIndex=%d\n", m_topApp, topIndex);

      // Now start each application (whatever that means to the application)

      for (int i = 0; i < m_slots.size(); )
        {
          IApplication *app = m_slots.at(i).app;

          ginfo("Starting app[%d]\n", i);
          if (!app->run())
            {
              // Call stopApplication on a failure to start.  This will call
              // app->stop() (which is probably not necessary for the application
              //  but it should be prepared/ to handle it).  stopApplication()
              // will also removed the icon image from the list and delete it.

              stopApplication(app);

              // Then continue with the next application.  Notice that i is
              // not incremented in this case and we will continue with the
              // next application which will not be at this same index

              continue;
            }

          // Hide all applications except for the top application.  NOTE:
          // topIndex may still be -1, meaning that there is no application
          // and that all applications should be hidden.

          if (i != topIndex)
            {
              // Bring the application up in the non-visible state (the
              // application may or may not be minimized, but it is not
              // visible now).

              ginfo("Hiding app[%d]\n", i);
              hideApplicationWindow(app);
            }
          else
            {
              // Bring up the application as the new top application

              ginfo("Showing app[%d]\n", i);
              topApplication(app);
            }

          // The application was successfully initialized.. index to the next application

          i++;
        }

      // If there is no top application (i.e., no applications or all applications
      // are minimized), then draw the background image

      if (!m_topApp)
        {
          if (!redrawBackgroundWindow())
            {
              return false;
            }
        }

      // Draw the taskbar.  It will be draw at a higher level than the application.

      return redrawTaskbarWindow();
    }

  return false;
}

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

CApplicationWindow *CTaskbar::openApplicationWindow(uint8_t flags)
{
  // Get a framed window for the application

  NXWidgets::CNxTkWindow *window = openFramedWindow();
  if (!window)
    {
      return (CApplicationWindow *)0;
    }

  // Set size and position of a window in the application area.

  setApplicationGeometry(window, false);

  // Use this window to instantiate the application window

  CApplicationWindow *appWindow = new CApplicationWindow(window, flags);
  if (!appWindow)
    {
      delete window;
    }

  return appWindow;
}

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

CFullScreenWindow *CTaskbar::openFullScreenWindow(void)
{
  // Get a raw window for the application

  NXWidgets::CNxWindow *window = openRawWindow();
  if (!window)
    {
      return (CFullScreenWindow *)0;
    }

  // Set size and position of a window in the application area.

  setApplicationGeometry(window, true);

  // Use this window to instantiate the generia application window

  CFullScreenWindow *appWindow = new CFullScreenWindow(window);
  if (!appWindow)
    {
      delete window;
    }

  return appWindow;
}

/**
 * Start an application and add its icon to the taskbar.  The applications's
 * window is brought to the top.  Creating a new application in the start
 * window requires three steps:
 *
 * 1. Create the CTaskbar instance,
 * 2. Call startApplication repeatedly to add applications to the task bar
 * 3. Call startWindowManager to start the display with applications in place
 *
 * When the application is selected from the start window:
 *
 * 4. Call startApplication start the application and bring its window to
 *    the top.
 *
 * @param app.  The new application to add to the task bar
 * @param minimized.  The new application starts in the minimized state
 * @return true on success
 */

bool CTaskbar::startApplication(IApplication *app, bool minimized)
{
  // Get the widget control associated with the task bar window

  NXWidgets::CWidgetControl *control = m_taskbar->getWidgetControl();

  // Get the bitmap icon that goes with this application

  NXWidgets::IBitmap *bitmap = app->getIcon();

#ifdef CONFIG_NXWM_TASKBAR_ICONSCALE
  // Create a CScaledBitmap to scale the bitmap icon

  NXWidgets::CScaledBitmap *scaler = (NXWidgets::CScaledBitmap *)0;
  if (bitmap)
    {
      // Create a CScaledBitmap to scale the bitmap icon

      struct nxgl_size_s iconSize;
      iconSize.w = CONFIG_NXWM_TASKBAR_ICONWIDTH;
      iconSize.h = CONFIG_NXWM_TASKBAR_ICONHEIGHT;

      scaler = new NXWidgets::CScaledBitmap(bitmap, iconSize);
      if (!scaler)
        {
          return false;
        }
    }
#endif

  // Create a CImage instance to manage the applications icon.  Assume the
  // minimum size in case no bitmap is provided (bitmap == NULL)

  int w = 1;
  int h = 1;

#ifdef CONFIG_NXWM_TASKBAR_ICONSCALE
  if (scaler)
    {
      w = scaler->getWidth();
      h = scaler->getHeight();
    }

  NXWidgets::CImage *image =
    new NXWidgets::CImage(control, 0, 0, w, h, scaler, 0);

#else
  if (bitmap)
    {
      w = bitmap->getWidth();
      h = bitmap->getHeight();
    }

  NXWidgets::CImage *image =
    new NXWidgets::CImage(control, 0, 0, w, h, bitmap, 0);

#endif

  if (!image)
    {
      return false;
    }

  // Configure the image, disabling drawing for now

  image->setBorderless(true);
  image->disableDrawing();
  image->setRaisesEvents(false);

  // Register to get events from the mouse clicks on the image

  image->addWidgetEventHandler(this);

  // Add the application to end of the list of applications managed by
  // the task bar

  struct STaskbarSlot slot;
  slot.app    = app;
  slot.image  = image;
  m_slots.push_back(slot);

  // Initialize the application states

  app->setTopApplication(false);
  app->setMinimized(minimized);

  // Has the window manager been started?

  if (m_started)
    {
      // Yes.. Start the application (whatever that means).

      if (!app->run())
        {
          // Call stopApplication on a failure to start.  This will call
          // app->stop() (which is probably not necessary for the application
          //  but it should be prepared/ to handle it).  stopApplication()
          // will also removed the icon image from the list and delete it.

          stopApplication(app);
          return false;
        }

      // Were we ask to start the application minimized?

      if (minimized)
        {
          // Bring the minimized application up in non-visible state

          hideApplicationWindow(app);
        }
      else
        {
          // Bring up the application as the new top application if we are running
          // If not, we will select and bring up the top application when the
          // window manager is started

          topApplication(app);
        }

      // Redraw the task bar with the new icon (if we have been started)

      redrawTaskbarWindow();
    }

  return true;
}

/**
 * Move window to the top of the hierarchy and re-draw it.  This method
 * does nothing if the application is minimized.
 *
 * @param app.  The new application to show
 * @return true on success
 */

bool CTaskbar::topApplication(IApplication *app)
{
  // Verify that the application is not minimized and is not already the top application

  if (!app->isMinimized() && !app->isTopApplication())
    {
      // It is not minimized.  We are going to bring it to the top of the display.
      // Is there already a top application?

      if (m_topApp)
        {
          // Yes.. make the application non-visible (it is still maximized and
          // will reappear when the window above it is minimized or closed).

          hideApplicationWindow(m_topApp);
        }

      // Make the application the top application and redraw it

      return redrawApplicationWindow(app);
    }

  return false;
}

/**
 * Maximize an application by moving its window to the top of the hierarchy
 * and re-drawing it.  If the application was already maximized, then this
 * method is equivalent to topApplication().
 *
 * @param app.  The new application to add to the task bar
 * @return true on success
 */

bool CTaskbar::maximizeApplication(IApplication *app)
{
  // Mark that the application is no longer minimized

  app->setMinimized(false);

  // Then bring the application to the top of the hierarchy

  return topApplication(app);
}

/**
 * Minimize an application by moving its window to the bottom of the and
 * redrawing the next visible appliation.
 *
 * @param app.  The new application to add to the task bar
 * @return true on success
 */

bool CTaskbar::minimizeApplication(IApplication *app)
{
  // Verify that the application is not already minimized

  if (!app->isMinimized())
    {
      // No, then we are going to minimize it but disabling its components,
      // marking it as minimized, then raising a new window to the top window.

      app->setMinimized(true);
      hideApplicationWindow(app);

      // Re-draw the new top, non-minimized application

      return redrawTopApplication();
    }

  return false;
}

/**
 * Destroy an application.  Move its window to the bottom and remove its
 * icon from the task bar.
 *
 * @param app.  The new application to remove from the task bar
 * @return true on success
 */

bool CTaskbar::stopApplication(IApplication *app)
{
  // Make the application minimized and make sure that it is not the top
  // application.

  app->setMinimized(true);
  if (app->isTopApplication())
    {
      app->setTopApplication(false);
      m_topApp = (IApplication *)0;
    }

  // Hide the application window.  That will move the application to the
  // bottom of the hiearachy.

  hideApplicationWindow(app);

  // Stop the application (whatever this means to the application).  We
  // separate stopping from destroying to get the application a chance
  // to put things in order before being destroyed.

  app->stop();

  // Find the application in the list of applications

  for (int i = 0; i < m_slots.size(); i++)
    {
      // Is this it?

      IApplication *candidate = m_slots.at(i).app;
      if (candidate == app)
        {
          // Yes.. found it.  Delete the icon image and remove the entry
          // from the list of applications

          delete m_slots.at(i).image->getBitmap();
          delete m_slots.at(i).image;
          m_slots.erase(i);
          break;
        }
    }

  // Destroy the application (actually, this just sets up the application for
  // later destruction.

  app->destroy();

  // Re-draw the new top, non-minimized application

  bool ret = redrawTopApplication();
  if (ret)
    {
      // And redraw the task bar (without the icon for this task)

      ret = redrawTaskbarWindow();
    }
  return ret;
}

/**
 * Get the size of the physical display device as it is known to the task
 * bar.
 *
 * @return The size of the display
 */

void CTaskbar::getDisplaySize(FAR struct nxgl_size_s &size)
{
  // Get the widget control from the task bar window.  The physical window geometry
  // should be the same for all windows.

  NXWidgets::CWidgetControl *control = m_taskbar->getWidgetControl();

  // Get the window bounding box from the widget control

  NXWidgets::CRect rect = control->getWindowBoundingBox();

  // And return the size of the window

  rect.getSize(size);
}

/**
 * Create a raw window.
 *
 * 1) Create a dumb CWigetControl instance (see note below)
 * 2) Pass the dumb CWidgetControl instance to the window constructor
 *    that inherits from INxWindow.  This will "smarten" the CWidgetControl
 *    instance with some window knowlede
 * 3) Call the open() method on the window to display the window.
 * 4) After that, the fully smartened CWidgetControl instance can
 *    be used to generate additional widgets by passing it to the
 *    widget constructor
 *
 * NOTE:  Actually, NxWM uses the CWindowMessenger class that inherits from
 * CWidgetControl.  That class just adds some unrelated messaging capability;
 * It cohabitates with CWidgetControl only because it needs the CWidgetControl
 * this point.
 */

NXWidgets::CNxWindow *CTaskbar::openRawWindow(void)
{
  // Create the widget control (with the window messenger) using the default style

  CWindowMessenger *control = new CWindowMessenger((NXWidgets::CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  NXWidgets::CNxWindow *window = createRawWindow(control);
  if (!window)
    {
      delete control;
      return NULL;
    }

  // Open (and initialize) the window

  bool success = window->open();
  if (!success)
    {
      delete window;
      window = (NXWidgets::CNxWindow *)0;
      return NULL;
    }

  return window;
}

/**
 * Create a framed application window
 *
 * This may be used to provide the window parater to the IApplication constructor
 *
 * @return A partially initialized application window instance.
 */

NXWidgets::CNxTkWindow *CTaskbar::openFramedWindow(void)
{
  // Create the widget control (with the window messenger) using the default style

  CWindowMessenger *control = new CWindowMessenger((NXWidgets::CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the framed window as a class
  // that derives from INxWindow.

  NXWidgets::CNxTkWindow *window = createFramedWindow(control);
  if (!window)
    {
      delete control;
      return (NXWidgets::CNxTkWindow *)0;
    }

  // Open (and initialize) the window

  bool success = window->open();
  if (!success)
    {
      delete window;
      window = (NXWidgets::CNxTkWindow *)0;
    }

  return window;
}

/**
 * Set size and position of a window in the application area.
 *
 * @param window.   The window to be resized and repositioned
 * @param fullscreen.  True: Use full screen
 *
 * @return true on success
 */

void CTaskbar::setApplicationGeometry(NXWidgets::INxWindow *window, bool fullscreen)
{
  // Get the physical size of the display

  struct nxgl_size_s displaySize;
  getDisplaySize(displaySize);

  // Now position and size the application.  This will depend on the position and
  // orientation of the task bar.

  struct nxgl_point_s pos;
  struct nxgl_size_s  size;

  // In fullscreen mode, the application window gets everything

  if (fullscreen)
    {
      pos.x = 0;
      pos.y = 0;

      size.w = displaySize.w;
      size.h = displaySize.h;
    }
  else
    {
#if defined(CONFIG_NXWM_TASKBAR_TOP)
      pos.x = 0;
      pos.y = CONFIG_NXWM_TASKBAR_WIDTH;

      size.w = displaySize.w;
      size.h = displaySize.h - CONFIG_NXWM_TASKBAR_WIDTH;
#elif defined(CONFIG_NXWM_TASKBAR_BOTTOM)
      pos.x = 0;
      pos.y = 0;

      size.w = displaySize.w;
      size.h = displaySize.h - CONFIG_NXWM_TASKBAR_WIDTH;
#elif defined(CONFIG_NXWM_TASKBAR_LEFT)
      pos.x = CONFIG_NXWM_TASKBAR_WIDTH;
      pos.y = 0;

      size.w = displaySize.w - CONFIG_NXWM_TASKBAR_WIDTH;
      size.h = displaySize.h;
#else
      pos.x = 0;
      pos.y = 0;

      size.w = displaySize.w - CONFIG_NXWM_TASKBAR_WIDTH;
      size.h = displaySize.h;
#endif
    }

  /* Set the size and position the window.
   *
   * @param pPos The new position of the window.
   * @return True on success, false on failure.
   */

  window->setPosition(&pos);
  window->setSize(&size);
}

/**
 * Create the task bar window.
 *
 * @return true on success
 */

bool CTaskbar::createTaskbarWindow(void)
{
  // Create a raw window to present the task bar

  m_taskbar = openRawWindow();
  if (!m_taskbar)
    {
      return false;
    }

  // Get the size of the physical display

  struct nxgl_size_s displaySize;
  getDisplaySize(displaySize);

  // Now position and size the task bar.  This will depend on the position and
  // orientation of the task bar.

  struct nxgl_point_s pos;
  struct nxgl_size_s  size;

#if defined(CONFIG_NXWM_TASKBAR_TOP)
  pos.x = 0;
  pos.y = 0;

  size.w = displaySize.w;
  size.h = CONFIG_NXWM_TASKBAR_WIDTH;
#elif defined(CONFIG_NXWM_TASKBAR_BOTTOM)
  pos.x = 0;
  pos.y = displaySize.h - CONFIG_NXWM_TASKBAR_WIDTH;

  size.w = displaySize.w;
  size.h = CONFIG_NXWM_TASKBAR_WIDTH;
#elif defined(CONFIG_NXWM_TASKBAR_LEFT)
  pos.x = 0;
  pos.y = 0;

  size.w = CONFIG_NXWM_TASKBAR_WIDTH;
  size.h = displaySize.h;
#else
  pos.x = rect.getWidgth() - CONFIG_NXWM_TASKBAR_WIDTH;
  pos.y = 0;

  size.w = CONFIG_NXWM_TASKBAR_WIDTH;
  size.h = displaySize.h;
#endif

  /* Set the size and position the window.
   *
   * @param pPos The new position of the window.
   * @return True on success, false on failure.
   */

  m_taskbar->setPosition(&pos);
  m_taskbar->setSize(&size);
  return true;
}

/**
 * Create the background window.
 *
 * @return true on success
 */

bool CTaskbar::createBackgroundWindow(void)
{
  CWindowMessenger *control = new CWindowMessenger((NXWidgets::CWidgetStyle *)NULL);

  // Create a raw window to present the background image

  NXWidgets::CBgWindow *background = getBgWindow(control);
  if (!background)
    {
      delete control;
      return false;
    }

  // Open (and initialize) the BG window

  bool success = background->open();
  if (!success)
    {
      delete background;
      return false;
    }

  m_background = background;
  if (!m_background)
    {
      return false;
    }

  // Set the geometry to fit in the application window space

  setApplicationGeometry(static_cast<NXWidgets::INxWindow*>(m_background), false);
  return true;
}

/**
 * Create the background image.
 *
 * @return true on success
 */

bool CTaskbar::createBackgroundImage(void)
{
#ifndef CONFIG_NXWM_DISABLE_BACKGROUND_IMAGE
 // Get the size of the display

  struct nxgl_size_s windowSize;
  if (!m_background->getSize(&windowSize))
    {
      return false;
    }

  // Get the widget control from the background window

  NXWidgets::CWidgetControl *control = m_background->getWidgetControl();

  // Create the bitmap object

  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_BACKGROUND_IMAGE);

  if (!bitmap)
    {
      return false;
    }

  // Get the size of the bitmap image

  struct nxgl_size_s imageSize;
  imageSize.w  = bitmap->getWidth();
  imageSize.h = (nxgl_coord_t)bitmap->getHeight();

  // Pick an X/Y position such that the image will be centered in the display

  struct nxgl_point_s imagePos;
  if (imageSize.w >= windowSize.w)
    {
      imagePos.x = 0;
    }
  else
    {
      imagePos.x = (windowSize.w - imageSize.w) >> 1;
    }

  if (imageSize.h >= windowSize.h)
    {
      imagePos.y = 0;
    }
  else
    {
      imagePos.y = (windowSize.h - imageSize.h) >> 1;
    }

  // Now we have enough information to create the image

  m_backImage = new NXWidgets::CImage(control, imagePos.x, imagePos.y,
                                      imageSize.w, imageSize.h, bitmap);
  if (!m_backImage)
    {
      delete bitmap;
      return false;
    }

  // Configure the background image

  m_backImage->setBorderless(true);
  m_backImage->setRaisesEvents(false);
#endif

  return true;
}

/**
 * (Re-)draw the task bar window.
 *
 * @return true on success
 */

bool CTaskbar::redrawTaskbarWindow(void)
{
  // Only redraw the task bar if (1) the window manager has been started, AND
  // (2) there is no top window (i.e., we are showing the background image), OR
  // (3) there is a top window, but it is not full screen

  if (m_started && (!m_topApp || !m_topApp->isFullScreen()))
    {
      // Get the widget control from the task bar

      NXWidgets::CWidgetControl *control = m_taskbar->getWidgetControl();

      // Get the graphics port for drawing on the background window

      NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

      // Get the size of the window

      struct nxgl_size_s windowSize;
      if (!m_taskbar->getSize(&windowSize))
        {
          return false;
        }

      // Raise the task bar to the top of the display.  This is only necessary
      // after stopping a full screen application.  Other applications do not
      // overlap the task bar and, hence, do not interfere.

      m_taskbar->raise();

      // Fill the entire window with the background color

      port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                           CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR);

#ifndef CONFIG_NXWM_TASKBAR_NO_BORDER
      // Add a border to the task bar to delineate it from the background window

      port->drawBevelledRect(0, 0,  windowSize.w, windowSize.h,
                             CONFIG_NXWM_DEFAULT_SHINEEDGECOLOR,
                             CONFIG_NXWM_DEFAULT_SHADOWEDGECOLOR);
#endif

      // Begin adding icons in the upper left corner

      struct nxgl_point_s taskbarPos;
#if defined(CONFIG_NXWM_TASKBAR_TOP) || defined(CONFIG_NXWM_TASKBAR_BOTTOM)
      taskbarPos.x = CONFIG_NXWM_TASKBAR_HSPACING;
      taskbarPos.y = 0;
#else
      taskbarPos.x = 0;
      taskbarPos.y = CONFIG_NXWM_TASKBAR_VSPACING;
#endif

      // Add each icon in the list of applications

      for (int i = 0; i < m_slots.size(); i++)
        {
          // Get the icon associated with this application

          NXWidgets::CImage *image = m_slots.at(i).image;

          // Disable drawing of the icon image; disable events from the icon

          image->disableDrawing();
          image->setRaisesEvents(false);

          // Highlight the icon for the top-most window

          image->highlight(m_slots.at(i).app == m_topApp);

          // Get the size of the icon image

          NXWidgets::CRect rect;
          image->getPreferredDimensions(rect);

          // Position the icon

          struct nxgl_point_s iconPos;

#if defined(CONFIG_NXWM_TASKBAR_TOP) || defined(CONFIG_NXWM_TASKBAR_BOTTOM)
          // For horizontal task bars, the icons will be aligned along the top of
          // the task bar

          iconPos.x = taskbarPos.x;
          iconPos.y = taskbarPos.y + CONFIG_NXWM_TASKBAR_VSPACING;
#else
          // For vertical task bars, the icons will be centered horizontally

          iconPos.x = (windowSize.w - rect.getWidth()) >> 1;
          iconPos.y = taskbarPos.y;
#endif

          // Set the position of the icon bitmap

          image->moveTo(iconPos.x, iconPos.y);

          // Then re-draw the icon at the new position

          image->enableDrawing();
          image->redraw();
          image->setRaisesEvents(true);

          // Do we add icons left-to-right?  Or top-to-bottom?

#if defined(CONFIG_NXWM_TASKBAR_TOP) || defined(CONFIG_NXWM_TASKBAR_BOTTOM)
          // left-to-right ... increment the X display position

          taskbarPos.x += rect.getWidth() + CONFIG_NXWM_TASKBAR_HSPACING;
          if (taskbarPos.x > windowSize.w)
            {
              break;
            }
#else
          // top-to-bottom ... increment the Y display position

          taskbarPos.y += rect.getHeight() + CONFIG_NXWM_TASKBAR_VSPACING;
          if (taskbarPos.y > windowSize.h)
            {
              break;
            }
#endif
        }

      // If there is a top application then we must now raise it above the task
      // bar so that itwill get the keyboard input.

      raiseTopApplication();
    }

  // Return success (it is not a failure if the window manager is not started
  // or the task bar is occluded by a full screen window.

  return true;
}

/**
 * Redraw the window at the top of the hierarchy.
 *
 * @return true on success
 */

bool CTaskbar::redrawTopApplication(void)
{
  // Check if there is already a top application

  IApplication *app = m_topApp;
  if (!app)
    {
      // No.. Search for that last, non-minimized application

      for (int i = m_slots.size() - 1; i >= 0; i--)
        {
          IApplication *candidate = m_slots.at(i).app;
          if (!candidate->isMinimized())
            {
              app = candidate;
              break;
            }
        }
    }

  // Did we find one?

  if (app)
    {
      // Yes.. make it the top application window and redraw it

      return redrawApplicationWindow(app);
      return true;
    }
  else
    {
      // Otherwise, there is no top application.  Re-draw the background image.

      m_topApp = (IApplication *)0;
      return redrawBackgroundWindow();
    }
}

/**
 * Raise the top window to the top of the NXheirarchy.
 *
 * @return true on success
 */

void CTaskbar::raiseTopApplication(void)
{
  if (m_topApp)
    {
      // Every application provides a method to obtain its application window

      IApplicationWindow *appWindow = m_topApp->getWindow();

      // Each application window provides a method to get the underlying NX window

      NXWidgets::INxWindow *window = appWindow->getWindow();

      // Raise the application window to the top of the hierarchy

      window->raise();
    }
}

/**
 * (Re-)draw the background window.
 *
 * @return true on success
 */

bool CTaskbar::redrawBackgroundWindow(void)
{
  // Get the widget control from the background window

  NXWidgets::CWidgetControl *control = m_background->getWidgetControl();

  // Get the graphics port for drawing on the background window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the window

  struct nxgl_size_s windowSize;
  if (!m_background->getSize(&windowSize))
    {
      return false;
    }

  // Raise the background window to the top of the hierarchy

  m_background->raise();

  // Fill the entire window with the background color

  port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                       CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR);

  // Add a border to the task bar to delineate it from the task bar

  port->drawBevelledRect(0, 0,  windowSize.w, windowSize.h,
                         CONFIG_NXWM_DEFAULT_SHINEEDGECOLOR,
                         CONFIG_NXWM_DEFAULT_SHADOWEDGECOLOR);

  // Then re-draw the background image on the window

  if (m_backImage)
    {
      m_backImage->enableDrawing();
      m_backImage->redraw();
    }

  return true;
}

/**
 * Redraw the last application in the list of application maintained by
 * the task bar.
 *
 * @param app. The new top application to draw
 * @return true on success
 */

bool CTaskbar::redrawApplicationWindow(IApplication *app)
{
  // Mark the window as the top application

  m_topApp = app;
  app->setTopApplication(true);

  // Disable drawing of the background image.

  m_backImage->disableDrawing();

  // Raise to top application to the top of the NX window hierarchy

  raiseTopApplication();

  // Redraw taskbar

  redrawTaskbarWindow();

  // Every application provides a method to obtain its application window

  IApplicationWindow *appWindow = app->getWindow();

  // Re-draw the application window toolbar

  appWindow->redraw();

  // And re-draw the application window itself

  app->redraw();
  return true;
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but not at the top of the hierarchy)
 *
 * @param app. The application to hide
 */

void CTaskbar::hideApplicationWindow(IApplication *app)
{
  // The hidden window is certainly not the top application any longer
  // If it was before then redrawTopApplication() will pick a new one (rather
  // arbitrarily).

  if (app->isTopApplication())
    {
      m_topApp = (IApplication *)0;
      app->setTopApplication(false);
    }

  // We do not need to lower the application to the back.. the new top
  // window will be raised instead.
  //
  // So all that we really have to do is to make sure that all of the
  // components of the hidden window are inactive.

  // Every application provides a method to obtain its application window

  IApplicationWindow *appWindow = app->getWindow();

  // Hide the application window toolbar

  appWindow->hide();

  // The hide the application window itself

  app->hide();
}

/**
 * Handle a widget action event.  For CImage, this is a mouse button pre-release event.
 *
 * @param e The event data.
 */

void CTaskbar::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Was a n ICON clicked?

  for (int i = 0; i < m_slots.size(); i++)
    {
      // Is this it?

      NXWidgets::CImage *image = m_slots.at(i).image;
      if (image->isClicked())
        {
          // Was the icon minimized

          IApplication *app = m_slots.at(i).app;
          if (app->isMinimized())
            {
              // Maximize the application by moving its window to the top of
              // the hierarchy and re-drawing it.

              maximizeApplication(app);
            }

          // No, it is not minimized.  Is it already the top application?

          else if (!app->isTopApplication())
            {
              /* Move window to the top of the hierarchy and re-draw it. */

              topApplication(app);
            }

          // Then break out of the loop

          break;
        }
    }
}
