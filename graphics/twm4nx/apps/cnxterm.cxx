/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cnxterm.cxx
// NxTerm window
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
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
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdio>
#include <cstdlib>
#include <cunistd>
#include <cfcntl>
#include <ctime>
#include <cassert>

#include <sys/boardctl.h>
#include <semaphore.h>
#include <debug.h>

#include "nshlib/nshlib.h"

#include "graphics/nxwidgets/cwidgetcontrol.hxx"

#include "graphics/nxglyphs.hxx"
#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cmainmenu.hxx"

#include "graphics/twm4nx/apps/nxterm_config.hxx"
#include "graphics/twm4nx/apps/cnxterm.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-Processor Definitions
/////////////////////////////////////////////////////////////////////////////

// Configuration ////////////////////////////////////////////////////////////

#ifdef CONFIG_NSH_USBKBD
#  warning You probably do not really want CONFIG_NSH_USBKBD, try CONFIG_TWM4NX_KEYBOARD_USBHOST
#endif

/////////////////////////////////////////////////////////////////////////////
// Private Types
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
/////////////////////////////////////////////////////////////////////////////
// Private Types
/////////////////////////////////////////////////////////////////////////////

  /**
   * This structure is used to pass start up parameters to the NxTerm task
   * and to assure the the NxTerm is successfully started.
   */

  struct SNxTerm
  {
    FAR void              *console; /**< The console 'this' pointer use with on_exit() */
    sem_t                  exclSem; /**< Sem that gives exclusive access to this structure */
    sem_t                  waitSem; /**< Sem that posted when the task is initialized */
    NXTKWINDOW             hwnd;    /**< Window handle */
    NXTERM                 nxterm;  /**< NxTerm handle */
    int                    minor;   /**< Next device minor number */
    struct nxterm_window_s wndo;    /**< Describes the NxTerm window */
    bool                   success; /**< True if successfully initialized */
  };

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

  /**
   * This global data structure is used to pass start parameters to NxTerm
   * task and to assure that the NxTerm is successfully started.
   */

  static struct SNxTerm GNxTermVars;
}

/////////////////////////////////////////////////////////////////////////////
// CNxTerm Method Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CNxTerm constructor
 *
 * @param twm4nx.  The Twm4Nx session instance
 */

CNxTerm::CNxTerm(FAR CTwm4Nx *twm4nx)
{
  // Save the context data

  m_twm4nx       = twm4nx;
  m_nxtermWindow = (FAR CWindow *)0;

  // The NxTerm is not running

  m_pid          = (pid_t)-1;
  m_NxTerm       = (NXTERM)0;
}

/**
 * CNxTerm destructor
 */

CNxTerm::~CNxTerm(void)
{
  // There would be a problem if we were stopped with the NxTerm task
  // running... that should never happen but we'll check anyway:

  stop();
}

/**
 * CNxTerm initializers.  Perform miscellaneous post-construction
 * initialization that may fail (and hence is not appropriate to be
 * done in the constructor)
 *
 * @return True if the NxTerm application was successfully initialized.
 */

bool CNxTerm::initialize(void)
{
  // Call CWindowFactory::createWindow() to create a window for the NxTerm
  // application.  Customizations:
  //
  // Flags: WFLAGS_NO_MENU_BUTTON indicates that there is no menu associated
  //   with the NxTerm application window
  // Null Icon manager means to use the system, common Icon Manager

  NXWidgets::CNxString name("NuttShell");

  uint8_t wflags = (WFLAGS_NO_MENU_BUTTON | WFLAGS_HIDDEN);

  FAR CWindowFactory *factory = m_twm4nx->getWindowFactory();
  m_nxtermWindow = factory->createWindow(name, &CONFIG_TWM4NX_NXTERM_ICON,
                                         (FAR CIconMgr *)0, wflags);
  if (m_nxtermWindow == (FAR CWindow *)0)
    {
      twmerr("ERROR: Failed to create CWindow\n");
      return false;
    }

  // Configure events needed by the NxTerm applications

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_NXTERM_REDRAW;
  events.resizeEvent = EVENT_NXTERM_RESIZE;
  events.mouseEvent  = EVENT_NXTERM_XYINPUT;
  events.kbdEvent    = EVENT_NXTERM_KBDINPUT;
  events.closeEvent  = EVENT_NXTERM_CLOSE;
  events.deleteEvent = EVENT_NXTERM_DELETE;

  bool success = m_nxtermWindow->configureEvents(events);
  if (!success)
    {
      delete m_nxtermWindow;
      m_nxtermWindow = (FAR CWindow *)0;
      return false;
    }

  return m_nxtermWindow->showWindow();
}

/**
 * Start the application (perhaps in the minimized state).
 *
 * @return True if the application was successfully started.
 */

bool CNxTerm::run(void)
{
  // Some sanity checking

  if (m_pid >= 0 || m_NxTerm != 0)
    {
      twmerr("ERROR: All ready running or connected\n");
      return false;
    }

  // Get exclusive access to the global data structure

  if (sem_wait(&GNxTermVars.exclSem) != 0)
    {
      // This might fail if a signal is received while we are waiting.

      twmerr("ERROR: Failed to get semaphore\n");
      return false;
    }

  // Get the widget control associated with the NXTK window

  NXWidgets::CWidgetControl *control =  m_nxtermWindow->getWidgetControl();

  // Get the window handle from the widget control

  GNxTermVars.hwnd = control->getWindowHandle();

  // Describe the NxTerm

  GNxTermVars.wndo.wcolor[0] = CONFIG_TWM4NX_NXTERM_WCOLOR;
  GNxTermVars.wndo.fcolor[0] = CONFIG_TWM4NX_NXTERM_FONTCOLOR;
  GNxTermVars.wndo.fontid    = CONFIG_TWM4NX_NXTERM_FONTID;

  // Remember the device minor number (before it is incremented)

  m_minor                    = GNxTermVars.minor;

  // Get the size of the window

  if (!m_nxtermWindow->getWindowSize(&GNxTermVars.wndo.wsize))
    {
      twmerr("ERROR: getWindowSize() failed\n");
      return false;
    }

  // Start the NxTerm task

  GNxTermVars.console = (FAR void *)this;
  GNxTermVars.success = false;
  GNxTermVars.nxterm  = 0;

  sched_lock();
  m_pid = task_create("NxTerm", CONFIG_TWM4NX_NXTERM_PRIO,
                      CONFIG_TWM4NX_NXTERM_STACKSIZE, nxterm,
                      (FAR char * const *)0);

  // Did we successfully start the NxTerm task?

  bool success = true;
  if (m_pid < 0)
    {
      twmerr("ERROR: Failed to create the NxTerm task\n");
      success = false;
    }
  else
    {
      // Wait for up to two seconds for the task to initialize

      struct timespec abstime;
      clock_gettime(CLOCK_REALTIME, &abstime);
      abstime.tv_sec += 2;

      int ret = sem_timedwait(&GNxTermVars.waitSem, &abstime);
      sched_unlock();

      if (ret == OK && GNxTermVars.success)
        {
#ifdef CONFIG_NXTERM_NXKBDIN
          // Re-direct NX keyboard input to the new NxTerm driver

          DEBUGASSERT(GNxTermVars.nxterm != 0);
          m_nxtermWindow->redirectNxTerm(GNxTermVars.nxterm);
#endif
          // Save the handle to use in the stop method

          m_NxTerm = GNxTermVars.nxterm;
        }
      else
        {
          // sem_timedwait failed OR the NxTerm task reported a
          // failure.  Stop the application

          twmerr("ERROR: Failed start the NxTerm task\n");
          stop();
          success = false;
        }
    }

  sem_post(&GNxTermVars.exclSem);
  return success;
}

/**
 * This is the close window event handler.  It will stop the NxTerm
 * application thread.
 */

void CNxTerm::stop(void)
{
  // Delete the NxTerm task if it is still running (this could strand
  // resources).

  if (m_pid >= 0)
    {
      pid_t pid = m_pid;
      m_pid     = (pid_t)-1;

      // Then delete the NSH task, possibly stranding resources

      task_delete(pid);
    }

  // Destroy the NX console device

  if (m_NxTerm)
    {
#ifdef CONFIG_NXTERM_NXKBDIN
      // Re-store NX keyboard input routing

      m_nxtermWindow->redirectNxTerm((NXTERM)0);
#endif

      // Unlink the NxTerm driver
      // Construct the driver name using this minor number

      char devname[32];
      snprintf(devname, 32, "/dev/nxterm%d", m_minor);

      unlink(devname);
      m_NxTerm = 0;
    }
}

/**
 * This is the NxTerm task.  This function first redirects output to the
 * console window then calls to start the NSH logic.
 */

int CNxTerm::nxterm(int argc, char *argv[])
{
  // To stop compiler complaining about "jump to label crosses initialization
  // of 'int fd'

  int fd = -1;
  int ret = OK;

  // Use the window handle to create the NX console

  struct boardioc_nxterm_create_s nxcreate;

  nxcreate.nxterm              = (FAR void *)0;
  nxcreate.hwnd                = GNxTermVars.hwnd;
  nxcreate.wndo                = GNxTermVars.wndo;
  nxcreate.type                = BOARDIOC_XTERM_FRAMED;
  nxcreate.minor               = GNxTermVars.minor;

  ret = boardctl(BOARDIOC_NXTERM, (uintptr_t)&nxcreate);
  if (ret < 0)
    {
      twmerr("ERROR: boardctl(BOARDIOC_NXTERM) failed: %d\n", errno);
      goto errout;
    }

  GNxTermVars.nxterm = nxcreate.nxterm;
  DEBUGASSERT(GNxTermVars.nxterm != NULL);

  // Construct the driver name using this minor number

  char devname[32];
  snprintf(devname, 32, "/dev/nxterm%d", GNxTermVars.minor);

  // Increment the minor number while it is protect by the semaphore

  GNxTermVars.minor++;

  // Open the NxTerm driver

#ifdef CONFIG_NXTERM_NXKBDIN
  fd = open(devname, O_RDWR);
#else
  fd = open(devname, O_WRONLY);
#endif
  if (fd < 0)
    {
      twmerr("ERROR: Failed open the console device\n");
      unlink(devname);
      goto errout;
    }

  // Now re-direct stdout and stderr so that they use the NX console driver.
  // Notes: (1) stdin is retained (file descriptor 0, probably the the serial
  // console).  (2) Don't bother trying to put debug instrumentation in the
  // following because it will end up in the NxTerm window.

  std::fflush(stdout);
  std::fflush(stderr);

#ifdef CONFIG_NXTERM_NXKBDIN
  std::fclose(stdin);
#endif
  std::fclose(stdout);
  std::fclose(stderr);

#ifdef CONFIG_NXTERM_NXKBDIN
  std::dup2(fd, 0);
#endif
  std::dup2(fd, 1);
  std::dup2(fd, 2);

#ifdef CONFIG_NXTERM_NXKBDIN
  std::fdopen(0, "r");
#endif
  std::fdopen(1, "w");
  std::fdopen(2, "w");

  // And we can close our original driver file descriptor

  if (fd > 2)
    {
      std::close(fd);
    }

  // Inform the parent thread that we successfully initialized

  GNxTermVars.success = true;
  sem_post(&GNxTermVars.waitSem);

  // Run the NSH console

#ifdef CONFIG_NSH_CONSOLE
  nsh_consolemain(argc, argv);
#endif

  // We get here if the NSH console should exits.  nsh_consolemain() ALWAYS
  // exits by calling nsh_exit() (which is a pointer to nsh_consoleexit())
  // which, in turn, calls exit()

  return EXIT_SUCCESS;

errout:
  GNxTermVars.nxterm = 0;
  GNxTermVars.success = false;
  sem_post(&GNxTermVars.waitSem);
  return EXIT_FAILURE;
}

/**
 * Handle Twm4Nx events.  This overrides a method from CTwm4NXEvent
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CNxTerm::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_NXTERM_REDRAW:  // Redraw event
        redraw();                // Redraw the whole window
        break;

      case EVENT_NXTERM_RESIZE:  // Resize event
        resize();                // Size the NxTerm window
        break;

      case EVENT_NXTERM_CLOSE:   // Window close event
        stop();                  // Stop the NxTerm thread
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Redraw the entire window.  The application has been maximized, resized or
 * moved to the top of the hierarchy.  This method is call from CTwm4Nx when
 * the application window must be displayed
 */

void CNxTerm::redraw(void)
{
  // Get the size of the window

  struct nxgl_size_s windowSize;
  m_nxtermWindow->getWindowSize(&windowSize);

  // Redraw the entire NxTerm window

  struct boardioc_nxterm_ioctl_s iocargs;
  struct nxtermioc_redraw_s redraw;

  redraw.handle     = m_NxTerm;
  redraw.rect.pt1.x = 0;
  redraw.rect.pt1.y = 0;
  redraw.rect.pt2.x = windowSize.w - 1;
  redraw.rect.pt2.y = windowSize.h - 1;
  redraw.more       = false;

  iocargs.cmd       = NXTERMIOC_NXTERM_REDRAW;
  iocargs.arg       = (uintptr_t)&redraw;

  boardctl(BOARDIOC_NXTERM_IOCTL, (uintptr_t)&iocargs);
}

/**
 * Inform NxTerm of a new window size.
 */

void CNxTerm::resize(void)
{
  struct nxtermioc_resize_s resize;

  // Get the size of the window

  resize.handle     = m_NxTerm;
  m_nxtermWindow->getWindowSize(&resize.size);

  // Inform NxTerm of the new size

  struct boardioc_nxterm_ioctl_s iocargs;
  iocargs.cmd       = NXTERMIOC_NXTERM_RESIZE;
  iocargs.arg       = (uintptr_t)&resize;

  boardctl(BOARDIOC_NXTERM_IOCTL, (uintptr_t)&iocargs);
}

/////////////////////////////////////////////////////////////////////////////
// CNxTermFactory Method Implementations
/////////////////////////////////////////////////////////////////////////////

/**
 * CNxTermFactory Initializer.  Performs parts of the instance
 * construction that may fail.  In this implementation, it will
 * initialize the NSH library and register an menu item in the
 * Main Menu.
 */

bool CNxTermFactory::initialize(FAR CTwm4Nx *twm4nx)
{
  // Save the session instance

  m_twm4nx = twm4nx;

  // Initialize the NSH library

  if (!nshlibInitialize())
    {
      twmerr("ERROR:  Failed to initialize the NSH library\n");
      return false;
    }

  // Register an entry with the Main menu.  When selected, this will
  // Case the start

  FAR CMainMenu *cmain = twm4nx->getMainMenu();
  return cmain->addApplication(this);
}

/**
 * One time NSH initialization. This function must be called exactly
 * once during the boot-up sequence to initialize the NSH library.
 *
 * @return True on successful initialization
 */

bool CNxTermFactory::nshlibInitialize(void)
{
  // Initialize the global data structure

  sem_init(&GNxTermVars.exclSem, 0, 1);
  sem_init(&GNxTermVars.waitSem, 0, 0);

  // Initialize the NSH library

  nsh_initialize();

  // If the Telnet console is selected as a front-end, then start the
  // Telnet daemon.

#ifdef CONFIG_NSH_TELNET
  int ret = nsh_telnetstart(AF_UNSPEC);
  if (ret < 0)
    {
      // The daemon is NOT running!

      return false;
   }
#endif

  return true;
}

/**
 * Handle CNxTermFactory events.  This overrides a method from
 * CTwm4NXEvent
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CNxTermFactory::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_NXTERM_START:  // Main menu selection
        startFunction();        // Create a new NxTerm instance
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Create and start a new instance of CNxTerm.
 */

bool CNxTermFactory::startFunction(void)
{
  // Instantiate the Nxterm application, providing only the session session
  // instance to the constructor

  CNxTerm *nxterm = new CNxTerm(m_twm4nx);
  if (!nxterm)
    {
      twmerr("ERROR: Failed to instantiate CNxTerm\n");
      return false;
    }

  // Initialize the NxTerm application

  if (!nxterm->initialize())
    {
      twmerr("ERROR: Failed to initialize CNxTerm instance\n");
      delete nxterm;
      return false;
    }

  // Start the NxTerm application instance

  if (!nxterm->run())
    {
      twmerr("ERROR: Failed to start the NxTerm application\n");
      delete nxterm;
      return false;
    }

  return true;
}
