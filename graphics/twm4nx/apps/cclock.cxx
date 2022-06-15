/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cnxterm.cxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <ctime>
#include <cstring>
#include <cassert>

#include <semaphore.h>
#include <debug.h>
#include <sched.h>
#include <unistd.h>

#include <nuttx/semaphore.h>

#include "nshlib/nshlib.h"

#include "graphics/nxwidgets/cwidgetcontrol.hxx"

#include "graphics/nxglyphs.hxx"
#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cmainmenu.hxx"

#include "graphics/slcd.hxx"
#include "graphics/twm4nx/apps/clock_config.hxx"
#include "graphics/twm4nx/apps/cclock.hxx"

/////////////////////////////////////////////////////////////////////////////
// Private Types
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
/////////////////////////////////////////////////////////////////////////////
// Private Types
/////////////////////////////////////////////////////////////////////////////

  /**
   * This structure is used to pass start up parameters to the Clock task
   * and to assure the the Clock is successfully started.
   */

  struct SClock
  {
    FAR CClock *that;    /**< The CClock 'this' pointer */
    sem_t       exclSem; /**< Sem that gives exclusive access to this structure */
    sem_t       waitSem; /**< Sem that posted when the task is initialized */
    bool        success; /**< True if successfully initialized */
  };

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

  /**
   * This global data structure is used to pass start parameters to Clock
   * task and to assure that the Clock is successfully started.
   */

  static struct SClock GClockVars;
}

/////////////////////////////////////////////////////////////////////////////
// CClock Method Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CClock constructor
 *
 * @param twm4nx.  The Twm4Nx session instance
 */

CClock::CClock(FAR CTwm4Nx *twm4nx)
{
  // Save/initialize the context data

  m_twm4nx  = twm4nx;
  m_window  = (FAR CWindow *)0;
  m_slcd    = (FAR SLcd::CSLcd *)0;

  // The clock task is not running

  m_pid     = (pid_t)-1;

  // This is un-necessary but helpful in debugging to have a known value

  std::memset(m_digits, 0, CLOCK_NDIGITS * sizeof(struct SClockDigit));
}

/**
 * CClock destructor
 */

CClock::~CClock(void)
{
  // There would be a problem if we were stopped with the Clock task
  // running... that should never happen but we'll check anyway:

  stop();

  // Destroy the CSLcd instance

  if (m_window != (FAR CWindow *)0)
    {
      delete m_window;
    }

  // Destroy the CSLcd instance

  if (m_slcd != (FAR SLcd::CSLcd *)0)
    {
      delete m_slcd;
    }
}

/**
 * CClock initializers.  Perform miscellaneous post-construction
 * initialization that may fail (and hence is not appropriate to be
 * done in the constructor)
 *
 * @return True if the Clock application was successfully initialized.
 */

bool CClock::initialize(void)
{
  // Call CWindowFactory::createWindow() to create a window for the Clock
  // application.  Customizations:
  //
  // Flags:
  //   WFLAGS_NO_MENU_BUTTON     No menu button in the toolbar
  //   WFLAGS_NO_RESIZE_BUTTON   No resize button in the toolbar
  //   WFLAGS_HIDDEN             Window is initially hidden
  //
  // Null Icon manager means to use the system, common Icon Manager

  NXWidgets::CNxString name("Clock");

  uint8_t wflags = (WFLAGS_NO_MENU_BUTTON | WFLAGS_NO_RESIZE_BUTTON |
                    WFLAGS_HIDDEN);

  FAR CWindowFactory *factory = m_twm4nx->getWindowFactory();
  m_window = factory->createWindow(name, &CONFIG_TWM4NX_CLOCK_ICON,
                                   (FAR CIconMgr *)0, wflags);
  if (m_window == (FAR CWindow *)0)
    {
      twmerr("ERROR: Failed to create CWindow\n");
      return false;
    }

  // Get the minimum tool bar

  // Configure events needed by the Clock applications

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_CLOCK_REDRAW;
  events.resizeEvent = EVENT_CLOCK_RESIZE;
  events.mouseEvent  = EVENT_CLOCK_XYINPUT;
  events.kbdEvent    = EVENT_CLOCK_KBDINPUT;
  events.closeEvent  = EVENT_CLOCK_CLOSE;
  events.deleteEvent = EVENT_CLOCK_DELETE;

  bool success = m_window->configureEvents(events);
  if (!success)
    {
      delete m_window;
      m_window = (FAR CWindow *)0;
      return false;
    }

  // Create an instance of the segment LCD emulation

  m_slcd = new SLcd::CSLcd(m_window->getNxWindow(),
                           CONFIG_TWM4NX_CLOCK_HEIGHT);
  if (m_slcd == (FAR SLcd::CSLcd *)0)
    {
      twmerr("ERROR: Failed to create SLcd::CSLcd instance\n");
      delete m_window;
      m_window = (FAR CWindow *)0;
      return false;
    }

  // Set the correct size and position of the window based on the SLCD
  // character height.

  nxgl_coord_t segmentWidth = m_slcd->getWidth();
  nxgl_coord_t gapWidth     = segmentWidth / 2;

  struct nxgl_size_s windowSize;
  windowSize.w = 4 * (segmentWidth + CONFIG_TWM4NX_CLOCK_HSPACING) + gapWidth;
  windowSize.h = m_slcd->getHeight() + 2 * CONFIG_TWM4NX_CLOCK_HSPACING;

  // Check against the minimum toolbar width

  nxgl_coord_t minWidth = minimumToolbarWidth(m_twm4nx, name, wflags);

  nxgl_coord_t xOffset = CONFIG_TWM4NX_CLOCK_HSPACING;
  if (windowSize.w < minWidth)
    {
      xOffset      += (minWidth - windowSize.w) / 2;
      windowSize.w  = minWidth;
    }

  // Resize and position the frame.  The window is initially position in
  // the upper left hand corner of the display.

  struct nxgl_size_s frameSize;
  m_window->windowToFrameSize(&windowSize, &frameSize);

  struct nxgl_point_s framePos;
  framePos.x = 0;
  framePos.y = 0;

  if (!m_window->resizeFrame(&frameSize, &framePos))
    {
      delete m_window;
      m_window = (FAR CWindow *)0;
      delete m_slcd;
      m_slcd = (FAR SLcd::CSLcd *)0;
      return false;
    }

  // Initialize the SLCD digit horizontal positions

  m_digits[0].xOffset = xOffset;
  m_digits[1].xOffset = m_digits[0].xOffset + segmentWidth +
                        CONFIG_TWM4NX_CLOCK_HSPACING;
  m_digits[2].xOffset = m_digits[1].xOffset + segmentWidth +
                        gapWidth;
  m_digits[3].xOffset = m_digits[2].xOffset + segmentWidth +
                        CONFIG_TWM4NX_CLOCK_HSPACING;

  // Display the initial time

  redraw();

  // Now we can show the completed window

  return m_window->showWindow();
}

/**
 * Start the application (perhaps in the minimized state).
 *
 * @return True if the application was successfully started.
 */

bool CClock::run(void)
{
  // Some sanity checking

  if (m_pid >= 0)
    {
      twmerr("ERROR: All ready running?\n");
      return false;
    }

  // Get exclusive access to the global data structure

  if (sem_wait(&GClockVars.exclSem) != 0)
    {
      // This might fail if a signal is received while we are waiting. Or,
      // perhaps if the task is canceled.

      twmerr("ERROR: Failed to get semaphore\n");
      return false;
    }

  // Initialize the rest of parameter passing area for temporary use to
  // start the new Clock task

  sem_init(&GClockVars.waitSem, 0, 0);
  sem_setprotocol(&GClockVars.waitSem, SEM_PRIO_NONE);

  GClockVars.that    = this;
  GClockVars.success = false;

  // Start the Clock task

  sched_lock();
  m_pid = task_create("Clock", CONFIG_TWM4NX_CLOCK_PRIO,
                      CONFIG_TWM4NX_CLOCK_STACKSIZE, clock,
                      (FAR char * const *)0);

  // Did we successfully start the Clock task?

  bool success = true;
  if (m_pid < 0)
    {
      twmerr("ERROR: Failed to create the Clock task\n");
      success = false;
    }
  else
    {
      // Wait for up to two seconds for the task to initialize

      struct timespec abstime;
      clock_gettime(CLOCK_REALTIME, &abstime);
      abstime.tv_sec += 2;

      int ret = sem_timedwait(&GClockVars.waitSem, &abstime);
      sched_unlock();

      if (ret < 0 || !GClockVars.success)
        {
          // sem_timedwait failed OR the Clock task reported a
          // failure.  Stop the application

          twmerr("ERROR: Failed start the Clock task\n");
          stop();
          success = false;
        }
    }

  sem_destroy(&GClockVars.waitSem);
  sem_post(&GClockVars.exclSem);
  return success;
}

/**
 * This is the close window event handler.  It will stop the Clock
 * application thread.
 */

void CClock::stop(void)
{
  // Delete the Clock task if it is still running (this could strand
  // resources).

  if (m_pid >= 0)
    {
      pid_t pid = m_pid;
      m_pid     = (pid_t)-1;

      // Then delete the NSH task, possibly stranding resources

      task_delete(pid);
    }
}

/**
 * This is the Clock task.
 */

int CClock::clock(int argc, char *argv[])
{
  // Get the 'this' pointer

  FAR CClock *This = GClockVars.that;

  // Inform the parent thread that we successfully initialized

  GClockVars.success = true;
  sem_post(&GClockVars.waitSem);

  // Loop forever.  When the window is deleted, this task will be brutally
  // terminated via task_delete()

  for (; ; )
    {
      // Update the clock

      This->update();

      // Then sleep for a minute.  REVISIT:  It might make sense to check
      // the time at a much higher rate?

      sleep(60);
    }

  return EXIT_FAILURE; // Never get here
}

/**
 * Handle Twm4Nx events.  This overrides a method from CTwm4NXEvent
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CClock::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_CLOCK_REDRAW:  // Redraw event (doesn't happen)
        redraw();               // Redraw the whole window
        break;

      case EVENT_CLOCK_CLOSE:   // Window close event
        stop();                 // Stop the Clock thread
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Update the Clock.
 */

void CClock::update(void)
{
  // Get the current time.

  FAR struct std::timespec ts;
  int ret = std::clock_gettime(CLOCK_REALTIME, &ts);
  if (ret < 0)
    {
      twmerr("ERROR: clock_gettime() failed\n");
      return;
    }

  // Break out the time
  //
  //   tm_hour - The number of hours past midnight, in the range 0 to 23.
  //   tm_min  - The number of minutes after the hour, in the range 0 to 59.

  struct std::tm tm;
  if (std::gmtime_r(&ts.tv_sec, &tm) == (FAR struct std::tm *)0)
    {
      twmerr("ERROR: gmtime_r() failed\n");
      return;
    }

  // Convert hours and minutes into SLCD segment codes
  //
  // Conversion of 24 hour clock (0-23) to twelve hour (1-12)
  // 0 -> 12  6 -> 6  12 -> 12 18 -> 6
  // 1 -> 1   7 -> 7  13 -> 1  19 -> 7
  // 2 -> 2   8 -> 8  14 -> 2  20 -> 8
  // 3 -> 3   9 -> 9  15 -> 3  21 -> 9
  // 4 -> 4  10 -> 10 16 -> 4  22 -> 10
  // 5 -> 5  11 -> 11 17 -> 5  23 -> 11

  unsigned int hour = tm.tm_hour;  /* Range 0-23 */
  if (hour == 0)
    {
      hour = 12;
    }
  else if (hour > 12)              /* Range 1-23 */
    {
      hour -= 12;                  /* Range 1-11 */
    }

  uint8_t codes[CLOCK_NDIGITS];

  char ascii = (hour > 9) ? '1' : ' ';
  m_slcd->convert(ascii, codes[0]);

  ascii = (hour % 10) + '0';
  m_slcd->convert(ascii, codes[1]);

  ascii = (tm.tm_min / 10) + '0';
  m_slcd->convert(ascii, codes[2]);

  ascii = (tm.tm_min % 10) + '0';
  m_slcd->convert(ascii, codes[3]);

  struct nxgl_point_s pos;
  pos.y = CONFIG_TWM4NX_CLOCK_VSPACING;

  // Then show each segment LCD
  // There might be a more efficient way to do this than to erase the entire
  // SLCD on each update.

  for (int i = 0; i < CLOCK_NDIGITS; i++)
    {
      if (m_digits[i].segments != codes[i])
        {
          pos.x = m_digits[i].xOffset;

          m_slcd->erase(pos);
          m_slcd->show(codes[i], pos);
          m_digits[i].segments = codes[i];
        }
    }
}

/**
 * Redraw the entire clock.
 */

void CClock::redraw(void)
{
  // Get the size of the window

  struct nxgl_size_s windowSize;
  m_window->getWindowSize(&windowSize);

  // Create a bounding box

  struct nxgl_rect_s windowRect;
  windowRect.pt1.x = 0;
  windowRect.pt1.y = 0;
  windowRect.pt2.x = windowSize.w - 1;
  windowRect.pt2.y = windowSize.h - 1;

  // Fill the entire window with the clock background color

  FAR NXWidgets::INxWindow *nxWindow = m_window->getNxWindow();
  if (!nxWindow->fill(&windowRect, SLCD_BACKGROUND))
    {
      twmerr("ERROR: Failed to fill window");
      return;
    }

  // Reset the saved codes to force a complete update

  for (int i = 0; i < CLOCK_NDIGITS; i++)
    {
      m_digits[i].segments = 0;
    }

  // Then update the Clock window

  update();
}

/////////////////////////////////////////////////////////////////////////////
// CClockFactory Method Implementations
/////////////////////////////////////////////////////////////////////////////

/**
 * CClockFactory Initializer.  Performs parts of the instance
 * construction that may fail.  In this implementation, it will
 * initialize the NSH library and register an menu item in the
 * Main Menu.
 */

bool CClockFactory::initialize(FAR CTwm4Nx *twm4nx)
{
  // Save the session instance

  m_twm4nx = twm4nx;

  // Initialize the parameter passing area.  Other fields are initialized
  // by CClock thread-specific logic.

  sem_init(&GClockVars.exclSem, 0, 1);

  // Register an entry with the Main menu.  When selected, this will
  // Case the start

  FAR CMainMenu *cmain = twm4nx->getMainMenu();
  return cmain->addApplication(this);
}

/**
 * Handle CClockFactory events.  This overrides a method from
 * CTwm4NXEvent
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CClockFactory::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_CLOCK_START:  // Main menu selection
        startFunction();        // Create a new Clock instance
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Create and start a new instance of CClock.
 */

bool CClockFactory::startFunction(void)
{
  // Instantiate the Nxterm application, providing only the session session
  // instance to the constructor

  FAR CClock *clock = new CClock(m_twm4nx);
  if (clock == (FAR CClock *)0)
    {
      twmerr("ERROR: Failed to instantiate CClock\n");
      return false;
    }

  // Initialize the Clock application

  if (!clock->initialize())
    {
      twmerr("ERROR: Failed to initialize CClock instance\n");
      delete clock;
      return false;
    }

  // Start the Clock application instance

  if (!clock->run())
    {
      twmerr("ERROR: Failed to start the Clock application\n");
      delete clock;
      return false;
    }

  return true;
}
