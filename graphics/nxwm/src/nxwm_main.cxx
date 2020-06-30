/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwm/src/nxwm_main.cxx
//
//   Copyright (C) 2012, 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
//    me be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdio>
#include <cstdlib>
#include <cunistd>

#include <sys/boardctl.h>

#ifdef CONFIG_NXWM_TOUCHSCREEN_CONFIGDATA
#  include "platform/configdata.h"
#endif

#include "graphics/nxwm/ctaskbar.hxx"
#include "graphics/nxwm/cstartwindow.hxx"

#ifdef CONFIG_NXWM_NXTERM
#  include "graphics/nxwm/cnxterm.hxx"
#endif

#include "graphics/nxwm/chexcalculator.hxx"

#ifdef CONFIG_NXWM_MEDIAPLAYER
#  include "graphics/nxwm/cmediaplayer.hxx"
#endif

#ifdef CONFIG_NXWM_TOUCHSCREEN
#  include "graphics/nxwm/ctouchscreen.hxx"
#  include "graphics/nxwm/ccalibration.hxx"
#endif

#ifdef CONFIG_NXWM_KEYBOARD
#  include "graphics/nxwm/ckeyboard.hxx"
#endif

/////////////////////////////////////////////////////////////////////////////
// Private Types
/////////////////////////////////////////////////////////////////////////////

struct SNxWmTest
{
  NxWM::CTaskbar     *taskbar;             // The task bar
  NxWM::CStartWindow *startwindow;         // The start window
#ifdef CONFIG_NXWM_TOUCHSCREEN
  NxWM::CTouchscreen *touchscreen;         // The touchscreen
  struct NxWM::SCalibrationData calibData; // Calibration data
  bool                calibrated;          // True: Touchscreen has been calibrated
#endif
};

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

static struct SNxWmTest g_nxwmtest;

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Suppress name-mangling

extern "C" int main(int argc, char *argv[]);

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Name: cleanup
/////////////////////////////////////////////////////////////////////////////

static void testCleanUpAndExit(int exitCode)
{
#ifdef CONFIG_NXWM_TOUCHSCREEN
  if (g_nxwmtest.touchscreen)
    {
      delete g_nxwmtest.touchscreen;
    }
#endif

  // Delete the task bar then the start window.  the order is important because
  // we must bet all of the application references out of the task bar before
  // deleting the start window.  When the start window is deleted, it will
  // also delete of of the resources contained within the start window.

  if (g_nxwmtest.taskbar)
    {
      delete g_nxwmtest.taskbar;
    }

  if (g_nxwmtest.startwindow)
    {
      delete g_nxwmtest.startwindow;
    }

  // And exit

  exit(exitCode);
}

/////////////////////////////////////////////////////////////////////////////
// Name: createTaskbar
/////////////////////////////////////////////////////////////////////////////

static bool createTaskbar(void)
{
  // Create an instance of the Task Bar.
  //
  // The general sequence for initializing the task bar is:
  //
  // 1. Create the CTaskbar instance,
  // 2. Call the CTaskbar::connect() method to connect to the NX server (CTaskbar
  //    inherits the connect method from CNxServer),
  // 3. Call the CTaskbar::initWindowManager() method to initialize the task bar.
  // 3. Call CTaskBar::startApplication repeatedly to add applications to the task bar
  // 4. Call CTaskBar::startWindowManager to start the display with applications in place

  printf("createTaskbar: Create CTaskbar instance\n");
  g_nxwmtest.taskbar = new NxWM::CTaskbar();
  if (!g_nxwmtest.taskbar)
    {
      printf("createTaskbar: ERROR: Failed to instantiate CTaskbar\n");
      return false;
    }

  // Connect to the NX server

  printf("createTaskbar: Connect CTaskbar instance to the NX server\n");
  if (!g_nxwmtest.taskbar->connect())
    {
      printf("createTaskbar: ERROR: Failed to connect CTaskbar instance to the NX server\n");
      return false;
    }

  // Initialize the task bar
  //
  // Taskbar::initWindowManager() prepares the task bar to receive applications.
  // CTaskBar::startWindowManager() brings the window manager up with those applications
  // in place.

  printf("createTaskbar: Initialize CTaskbar instance\n");
  if (!g_nxwmtest.taskbar->initWindowManager())
    {
      printf("createTaskbar: ERROR: Failed to initialize CTaskbar instance\n");
      return false;
    }

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// Name: createStartWindow
/////////////////////////////////////////////////////////////////////////////

static bool createStartWindow(void)
{
  // Create the start window.  The start window is unique among applications
  // because it has no factory.  The general sequence for setting up the
  // start window is:
  //
  // 1. Create and open a CApplicationWindow
  // 2. Use the window to create the CStartWindow the start window application
  // 2. Call Cstartwindow::addApplication numerous times to install applications
  //    in the start window.
  // 3. Call CTaskBar::startApplication (initially minimized) to start the start
  //    window application.
  //
  // NOTE: that the start window should not have a stop button.

  NxWM::CApplicationWindow *window = g_nxwmtest.taskbar->openApplicationWindow(NxWM::CApplicationWindow::WINDOW_PERSISTENT);
  if (!window)
    {
      printf("createStartWindow: ERROR: Failed to create CApplicationWindow\n");
      return false;
    }

  // Open the window (it is hot in here)

  if (!window->open())
    {
      printf("createStartWindow: ERROR: Failed to open CApplicationWindow \n");
      delete window;
      return false;
    }

  // Instantiate the application, providing the window to the application's
  // constructor

  g_nxwmtest.startwindow = new NxWM::CStartWindow(g_nxwmtest.taskbar, window);
  if (!g_nxwmtest.startwindow)
    {
      gerr("ERROR: Failed to instantiate CStartWindow\n");
      delete window;
      return false;
    }

  // Add the CStartWindow application to the task bar (minimized)

  printf("createStartWindow: Start the start window application\n");
  if (!g_nxwmtest.taskbar->startApplication(g_nxwmtest.startwindow, true))
    {
      printf("createStartWindow: ERROR: Failed to start the start window application\n");
      return false;
    }

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// Name: startWindowManager
/////////////////////////////////////////////////////////////////////////////

static bool startWindowManager(void)
{
  // Start the window manager

  printf("startWindowManager: Start the window manager\n");
  if (!g_nxwmtest.taskbar->startWindowManager())
    {
      printf("startWindowManager: ERROR: Failed to start the window manager\n");
      return false;
    }

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// Name: createTouchScreen
/////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_NXWM_TOUCHSCREEN
static bool createTouchScreen(void)
{
  // Get the physical size of the display in pixels

  struct nxgl_size_s displaySize;
  g_nxwmtest.taskbar->getDisplaySize(displaySize);

    // Create the touchscreen device

  printf("createTouchScreen: Creating CTouchscreen\n");
  g_nxwmtest.touchscreen = new NxWM::CTouchscreen(g_nxwmtest.taskbar, &displaySize);
  if (!g_nxwmtest.touchscreen)
    {
      printf("createTouchScreen: ERROR: Failed to create CTouchscreen\n");
      return false;
    }

  printf("createTouchScreen: Start touchscreen listener\n");
  if (!g_nxwmtest.touchscreen->start())
    {
      printf("createTouchScreen: ERROR: Failed start the touchscreen listener\n");
      delete g_nxwmtest.touchscreen;
      return false;
    }

  return true;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Name: createKeyboard
/////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_NXWM_KEYBOARD
static bool createKeyboard(void)
{
  printf("createKeyboard: Creating CKeyboard\n");
  NxWM::CKeyboard *keyboard = new NxWM::CKeyboard(g_nxwmtest.taskbar);
  if (!keyboard)
    {
      printf("createKeyboard: ERROR: Failed to create CKeyboard\n");
      return false;
    }

  printf("createKeyboard: Start keyboard listener\n");
  if (!keyboard->start())
    {
      printf("createKeyboard: ERROR: Failed start the keyboard listener\n");
      delete keyboard;
      return false;
    }

  return true;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Name: createCalibration
/////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_NXWM_TOUCHSCREEN
static bool createCalibration(void)
{
  // 1Create the CCalibrationFactory application factory

  printf("createCalibration: Creating CCalibrationFactory\n");
  NxWM::CCalibrationFactory *factory = new NxWM::CCalibrationFactory(g_nxwmtest.taskbar, g_nxwmtest.touchscreen);
  if (!factory)
    {
      printf("createCalibration: ERROR: Failed to create CCalibrationFactory\n");
      return false;
    }

  // Add the calibration application to the start window.

  printf("createCalibration: Adding CCalibration to the start window\n");
  if (!g_nxwmtest.startwindow->addApplication(factory))
    {
      printf("createCalibration: ERROR: Failed to add CCalibrationto the start window\n");
      delete factory;
      return false;
    }

  // Call StartWindowFactory::create to create the start window application

  printf("createCalibration: Creating CCalibration\n");
  NxWM::IApplication *calibration = factory->create();
  if (!calibration)
    {
      printf("createCalibration: ERROR: Failed to create CCalibration\n");
      return false;
    }

#ifdef CONFIG_NXWM_TOUCHSCREEN_CONFIGDATA
  // Check if we have previously stored calibration data

  int ret = platform_getconfig(CONFIGDATA_TSCALIBRATION, 0,
                               (FAR uint8_t *)&g_nxwmtest.calibData,
                               sizeof(struct NxWM::SCalibrationData));
  if (ret == OK)
    {
      // We successfully got the calibration data from the platform-specific
      // logic.  This might fail if (1) calibration data was never saved, or
      // (2) if some failure occurred while trying to obtain the configuration
      // data.  In either event, the appropriate thing to do is to perform
      // the calibration.

      // Provide the calibration data to the touchscreen thread

      g_nxwmtest.touchscreen->setCalibrationData(g_nxwmtest.calibData);
      g_nxwmtest.touchscreen->setEnabled(true);
      g_nxwmtest.calibrated = true;
    }
  else
#endif
    {
      // Call CTaskBar::startApplication to start the Calibration application.
      // Nothing will be displayed because the window manager has not yet been
      // started.

      printf("createCalibration: Start the calibration application\n");
      g_nxwmtest.calibrated = false;

      if (!g_nxwmtest.taskbar->startApplication(calibration, false))
        {
          printf("createCalibration ERROR: Failed to start the calibration application\n");
          delete calibration;
          return false;
        }
    }

  return true;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Name: createNxTerm
/////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_NXWM_NXTERM
static bool createNxTerm(void)
{
  // Add the NxTerm application to the start window

  printf("createNxTerm: Creating the NxTerm application\n");
  NxWM::CNxTermFactory *console = new  NxWM::CNxTermFactory(g_nxwmtest.taskbar);
  if (!console)
    {
      printf("createNxTerm: ERROR: Failed to instantiate CNxTermFactory\n");
      return false;
    }

  printf("createNxTerm: Adding the NxTerm application to the start window\n");
  if (!g_nxwmtest.startwindow->addApplication(console))
    {
      printf("createNxTerm: ERROR: Failed to add CNxTermFactory to the start window\n");
      delete console;
      return false;
    }

  return true;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Name: createHexCalculator
/////////////////////////////////////////////////////////////////////////////

static bool createHexCalculator(void)
{
  // Add the hex calculator application to the start window

  printf("createHexCalculator: Creating the hex calculator application\n");
  NxWM::CHexCalculatorFactory *calculator = new  NxWM::CHexCalculatorFactory(g_nxwmtest.taskbar);
  if (!calculator)
    {
      printf("createHexCalculator: ERROR: Failed to instantiate CHexCalculatorFactory\n");
      return false;
    }

  printf("createHexCalculator: Adding the hex calculator application to the start window\n");
  if (!g_nxwmtest.startwindow->addApplication(calculator))
    {
      printf("createHexCalculator: ERROR: Failed to add CHexCalculatorFactory to the start window\n");
      delete calculator;
      return false;
    }

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// Name: createMediaPlayer
/////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_NXWM_MEDIAPLAYER
static bool createMediaPlayer(void)
{
  // Add the hex calculator application to the start window

  printf("createHexCalculator: Creating the hex calculator application\n");
  NxWM::CMediaPlayerFactory *mediaplayer = new  NxWM::CMediaPlayerFactory(g_nxwmtest.taskbar);
  if (!mediaplayer)
    {
      printf("createMediaPlayer: ERROR: Failed to instantiate CMediaPlayerFactory\n");
      return false;
    }

  printf("createMediaPlayer: Adding the hex calculator application to the start window\n");
  if (!g_nxwmtest.startwindow->addApplication(mediaplayer))
    {
      printf("createMediaPlayer: ERROR: Failed to add CMediaPlayerFactory to the start window\n");
      delete mediaplayer;
      return false;
    }

  return true;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// nxwm_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
#if defined(CONFIG_LIB_BOARDCTL) && !defined(CONFIG_BOARD_LATE_INITIALIZE)
  // Should we perform board-specific initialization?  There are two ways
  // that board initialization can occur:  1) automatically via
  // board_late_initialize() during bootup if CONFIG_BOARD_LATE_INITIALIZE, or
  // 2) here via a call to boardctl() if the interface is enabled
  // (CONFIG_LIB_BOARDCTL=y).

  boardctl(BOARDIOC_INIT, 0);
#endif

#ifdef CONFIG_NXWM_NXTERM
  // Initialize the NSH library

  printf("nxwm_main: Initialize the NSH library\n");
  if (!NxWM::nshlibInitialize())
    {
      printf("nxwm_main: ERROR: Failed to initialize the NSH library\n");
      return EXIT_FAILURE;
    }
#endif

  // Create the task bar.

  if (!createTaskbar())
    {
      printf("nxwm_main: ERROR: Failed to create the task bar\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }

  // Create the start window.

  if (!createStartWindow())
    {
      printf("nxwm_main: ERROR: Failed to create the start window\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }

#ifdef CONFIG_NXWM_KEYBOARD
  // Create the keyboard device

  if (!createKeyboard())
    {
      printf("nxwm_main: ERROR: Failed to create the keyboard\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }
#endif

#ifdef CONFIG_NXWM_TOUCHSCREEN
  // Create the touchscreen device

  if (!createTouchScreen())
    {
      printf("nxwm_main ERROR: Failed to create the touchscreen\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }
#endif

#ifdef CONFIG_NXWM_TOUCHSCREEN
  // Create the calibration application and add it to the start window

  if (!createCalibration())
    {
      printf("nxwm_main ERROR: Failed to create the calibration application\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }
#endif

#ifdef CONFIG_NXWM_NXTERM
  // Create the NxTerm application and add it to the start window

  if (!createNxTerm())
    {
      printf("nxwm_main: ERROR: Failed to create the NxTerm application\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }
#endif

  // Create the hex calculator application and add it to the start window

  if (!createHexCalculator())
    {
      printf("nxwm_main: ERROR: Failed to create the hex calculator application\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }

#ifdef CONFIG_NXWM_MEDIAPLAYER
  // Create the media player application and add it to the start window

  if (!createMediaPlayer())
    {
      printf("nxwm_main: ERROR: Failed to create the media player application\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }
#endif

  // Call CTaskBar::startWindowManager to start the display with applications in place.

  if (!startWindowManager())
    {
      printf("nxwm_main: ERROR: Failed to start the window manager\n");
      testCleanUpAndExit(EXIT_FAILURE);
    }

#ifdef CONFIG_NXWM_TOUCHSCREEN
#ifdef CONFIG_NXWM_TOUCHSCREEN_CONFIGDATA
  // There are two possibilities:  (1) We started the calibration earlier and now
  // need to obtain the calibration data from the calibration process, or (2)
  // We have already obtained stored calibration data in which case, the calibration
  // process never ran.

  if (!g_nxwmtest.calibrated)
#endif
    {
      // Since we started the touchscreen calibration program maximized, it will
      // run immediately when we start the window manager.  There is no positive
      // handshake to know when the touchscreen has been calibrated.  If we really
      // want to know, we have to poll

      printf("nxwm_main: Waiting for touchscreen calibration\n");
      while (!g_nxwmtest.touchscreen->isCalibrated())
        {
          std::sleep(2);
        }

      // This is how we would then recover the calibration data.  After the
      // calibration application creates the calibration data, it hands it to
      // the touchscreen driver.  After the touchscreen driver gets it, it will
      // report isCalibrated() == true and then we can read the calibration data
      // from the touchscreen driver.

      printf("nxwm_main: Getting calibration data from the touchscreen\n");
      if (!g_nxwmtest.touchscreen->getCalibrationData(g_nxwmtest.calibData))
        {
          printf("nxwm_main: ERROR: Failed to get calibration data from the touchscreen\n");
        }
      else
        {
#if 0 // ifdef CONFIG_NXWM_TOUCHSCREEN_CONFIGDATA.  Done in CCalibration
          // Save the new calibration data so that we do not have to do this
          // again the next time we start up.

          int ret = platform_setconfig(CONFIGDATA_TSCALIBRATION, 0,
                                       (FAR const uint8_t *)&g_nxwmtest.calibData,
                                       sizeof(struct NxWM::SCalibrationData));
          if (ret != 0)
            {
              printf("nxwm_main: ERROR: Failed to save calibration data\n");
            }
#endif
          g_nxwmtest.calibrated = true;
        }
    }
#endif

  return EXIT_SUCCESS;
}
