/********************************************************************************************
 * apps/graphics/nxwm/src/cnxterm.cxx
 *
 *   Copyright (C) 2012. 2014, 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
 *    me be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <cstdio>
#include <cstdlib>
#include <cunistd>
#include <ctime>

#include <sys/boardctl.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sched.h>
#include <assert.h>
#include <debug.h>

#include "nshlib/nshlib.h"

#include "graphics/nxwidgets/cwidgetcontrol.hxx"

#include "graphics/nxwm/nxwmconfig.hxx"
#include "graphics/nxglyphs.hxx"
#include "graphics/nxwm/cnxterm.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/
/* Configuration ****************************************************************************/

#ifdef CONFIG_NSH_USBKBD
#  warning You probably do not really want CONFIG_NSH_USBKBD, try CONFIG_NXWM_KEYBOARD_USBHOST
#endif

/********************************************************************************************
 * Private Types
 ********************************************************************************************/

namespace NxWM
{
  /**
   * This structure is used to pass start up parameters to the NxTerm task and to assure the
   * the NxTerm is successfully started.
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
    bool                   result;  /**< True if successfully initialized */
  };

/********************************************************************************************
 * Private Data
 ********************************************************************************************/

  /**
   * This global data structure is used to pass start parameters to NxTerm task and to
   * assure that the NxTerm is successfully started.
   */

  static struct SNxTerm g_nxtermvars;
}

/********************************************************************************************
 * Private Functions
 ********************************************************************************************/

/********************************************************************************************
 * CNxTerm Method Implementations
 ********************************************************************************************/

using namespace NxWM;

/**
 * CNxTerm constructor
 *
 * @param window.  The application window
 */

CNxTerm::CNxTerm(CTaskbar *taskbar, CApplicationWindow *window)
{
  // Save the constructor data

  m_taskbar = taskbar;
  m_window  = window;

  // The NxTerm is not running

  m_pid    = -1;
  m_nxterm = 0;

  // Add our personalized window label

  NXWidgets::CNxString myName = getName();
  window->setWindowLabel(myName);

  // Add our callbacks with the application window

  window->registerCallbacks(static_cast<IApplicationCallback *>(this));
}

/**
 * CNxTerm destructor
 *
 * @param window.  The application window
 */

CNxTerm::~CNxTerm(void)
{
  // There would be a problem if we were stopped with the NxTerm task
  // running... that should never happen but we'll check anyway:

  stop();

  // Although we didn't create it, we are responsible for deleting the
  // application window

  delete m_window;
}

/**
 * Each implementation of IApplication must provide a method to recover
 * the contained CApplicationWindow instance.
 */

IApplicationWindow *CNxTerm::getWindow(void) const
{
  return static_cast<IApplicationWindow*>(m_window);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CNxTerm::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_NXTERM_ICON);

  return bitmap;
}

/**
 * Get the name string associated with the application
 *
 * @return A copy if CNxString that contains the name of the application.
 */

NXWidgets::CNxString CNxTerm::getName(void)
{
  return NXWidgets::CNxString("NuttShell");
}

/**
 * Start the application (perhaps in the minimized state).
 *
 * @return True if the application was successfully started.
 */

bool CNxTerm::run(void)
{
  // Some sanity checking

  if (m_pid >= 0 || m_nxterm != 0)
    {
      gerr("ERROR: All ready running or connected\n");
      return false;
    }

  // Get exclusive access to the global data structure

  if (sem_wait(&g_nxtermvars.exclSem) != 0)
    {
      // This might fail if a signal is received while we are waiting.

      gerr("ERROR: Failed to get semaphore\n");
      return false;
    }

  // Recover the NXTK window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the widget control associated with the NXTK window

  NXWidgets::CWidgetControl *control =  window->getWidgetControl();

  // Get the window handle from the widget control

  g_nxtermvars.hwnd = control->getWindowHandle();

  // Describe the NxTerm

  g_nxtermvars.wndo.wcolor[0] = CONFIG_NXWM_NXTERM_WCOLOR;
  g_nxtermvars.wndo.fcolor[0] = CONFIG_NXWM_NXTERM_FONTCOLOR;
  g_nxtermvars.wndo.fontid    = CONFIG_NXWM_NXTERM_FONTID;

  // Remember the device minor number (before it is incremented)

  m_minor                     = g_nxtermvars.minor;

  // Get the size of the window

  window->getSize(&g_nxtermvars.wndo.wsize);

  // Start the NxTerm task

  g_nxtermvars.console = (FAR void *)this;
  g_nxtermvars.result  = false;
  g_nxtermvars.nxterm   = 0;

  sched_lock();
  m_pid = task_create("NxTerm", CONFIG_NXWM_NXTERM_PRIO,
                      CONFIG_NXWM_NXTERM_STACKSIZE, nxterm,
                      (FAR char * const *)0);

  // Did we successfully start the NxTerm task?

  bool result = true;
  if (m_pid < 0)
    {
      gerr("ERROR: Failed to create the NxTerm task\n");
      result = false;
    }
  else
    {
      // Wait for up to two seconds for the task to initialize

      struct timespec abstime;
      clock_gettime(CLOCK_REALTIME, &abstime);
      abstime.tv_sec += 2;

      int ret = sem_timedwait(&g_nxtermvars.waitSem, &abstime);
      sched_unlock();

      if (ret == OK && g_nxtermvars.result)
        {
          // Re-direct NX keyboard input to the new NxTerm driver

          DEBUGASSERT(g_nxtermvars.nxterm != 0);
#ifdef CONFIG_NXTERM_NXKBDIN
          window->redirectNxTerm(g_nxtermvars.nxterm);
#endif
          // Save the handle to use in the stop method

          m_nxterm = g_nxtermvars.nxterm;
        }
      else
        {
          // sem_timedwait failed OR the NxTerm task reported a
          // failure.  Stop the application

          gerr("ERROR: Failed start the NxTerm task\n");
          stop();
          result = false;
        }
    }

  sem_post(&g_nxtermvars.exclSem);
  return result;
}

/**
 * Stop the application.
 */

void CNxTerm::stop(void)
{
  // Delete the NxTerm task if it is still running (this could strand
  // resources). If we get here due to CTaskbar::stopApplication() processing
  // initialed by CNxTerm::exitHandler, then do *not* delete the task (it
  // is already being deleted).

  if (m_pid >= 0)
    {
      // Calling task_delete() will also invoke the on_exit() handler.  We se
      // m_pid = -1 before calling task_delete() to let the on_exit() handler,
      // CNxTerm::exitHandler(), know that it should not do anything

      pid_t pid = m_pid;
      m_pid = -1;

      // Then delete the NSH task, possibly stranding resources

      task_delete(pid);
    }

  // Destroy the NX console device

  if (m_nxterm)
    {
      // Re-store NX keyboard input routing

#ifdef CONFIG_NXTERM_NXKBDIN
      NXWidgets::INxWindow *window = m_window->getWindow();
      window->redirectNxTerm((NXTERM)0);
#endif

      // Unlink the NxTerm driver
      // Construct the driver name using this minor number

      char devname[32];
      snprintf(devname, 32, "/dev/nxterm%d", m_minor);

      unlink(devname);
      m_nxterm = 0;
    }
}

/**
 * Destroy the application and free all of its resources.  This method
 * will initiate blocking of messages from the NX server.  The server
 * will flush the window message queue and reply with the blocked
 * message.  When the block message is received by CWindowMessenger,
 * it will send the destroy message to the start window task which
 * will, finally, safely delete the application.
 */

void CNxTerm::destroy(void)
{
  // Block any further window messages

  m_window->block(this);

  // Make sure that the application is stopped

  stop();
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but not at the top of the hierarchy
 */

void CNxTerm::hide(void)
{
  // Disable drawing and events
}

/**
 * Redraw the entire window.  The application has been maximized or
 * otherwise moved to the top of the hierarchy.  This method is call from
 * CTaskbar when the application window must be displayed
 */

void CNxTerm::redraw(void)
{
  // Recover the NXTK window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the size of the window

  struct nxgl_size_s windowSize;
  window->getSize(&windowSize);

  // Redraw the entire NxTerm window

  struct boardioc_nxterm_ioctl_s iocargs;
  struct nxtermioc_redraw_s redraw;

  redraw.handle     = m_nxterm;
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
 * Report of this is a "normal" window or a full screen window.  The
 * primary purpose of this method is so that window manager will know
 * whether or not it show draw the task bar.
 *
 * @return True if this is a full screen window.
 */

bool CNxTerm::isFullScreen(void) const
{
  return m_window->isFullScreen();
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

  // Set up an on_exit() event that will be called when this task exits

  if (on_exit(exitHandler, g_nxtermvars.console) != 0)
    {
      gerr("ERROR: on_exit failed\n");
      goto errout;
    }

  // Use the window handle to create the NX console

  struct boardioc_nxterm_create_s nxcreate;

  nxcreate.nxterm              = (FAR void *)0;
  nxcreate.hwnd                = g_nxtermvars.hwnd;
  nxcreate.wndo                = g_nxtermvars.wndo;
  nxcreate.type                = BOARDIOC_XTERM_FRAMED;
  nxcreate.minor               = g_nxtermvars.minor;

  ret = boardctl(BOARDIOC_NXTERM, (uintptr_t)&nxcreate);
  if (ret < 0)
    {
      gerr("ERROR: boardctl(BOARDIOC_NXTERM) failed: %d\n", errno);
      goto errout;
    }

  g_nxtermvars.nxterm = nxcreate.nxterm;
  DEBUGASSERT(g_nxtermvars.nxterm != NULL);

  // Construct the driver name using this minor number

  char devname[32];
  snprintf(devname, 32, "/dev/nxterm%d", g_nxtermvars.minor);

  // Increment the minor number while it is protect by the semaphore

  g_nxtermvars.minor++;

  // Open the NxTerm driver

#ifdef CONFIG_NXTERM_NXKBDIN
  fd = open(devname, O_RDWR);
#else
  fd = open(devname, O_WRONLY);
#endif
  if (fd < 0)
    {
      gerr("ERROR: Failed open the console device\n");
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

  g_nxtermvars.result = true;
  sem_post(&g_nxtermvars.waitSem);

  // Run the NSH console

#ifdef CONFIG_NSH_CONSOLE
  nsh_consolemain(argc, argv);
#endif

  // We get here if the NSH console should exits.  nsh_consolemain() ALWAYS
  // exits by calling nsh_exit() (which is a pointer to nsh_consoleexit())
  // which, in turn, calls exit()

  return EXIT_SUCCESS;

errout:
  g_nxtermvars.nxterm = 0;
  g_nxtermvars.result = false;
  sem_post(&g_nxtermvars.waitSem);
  return EXIT_FAILURE;
}

/**
 * This is the NxTerm task exit handler.  It registered with on_exit()
 * and called automatically when the nxterm task exits.
 */

void CNxTerm::exitHandler(int code, FAR void *arg)
{
  CNxTerm *This = (CNxTerm *)arg;

  // If we got here because of the task_delete() call in CNxTerm::stop(),
  // then m_pid will be set to -1 to let us know that we do not need to do
  // anything

  if (This->m_pid >= 0)
    {
      // Set m_pid to -1 to prevent calling detlete_task() in CNxTerm::stop().
      // CNxTerm::stop() is called by the processing initiated by the following
      // call to CTaskbar::stopApplication()

      This->m_pid = -1;

      // Remove the NxTerm application from the taskbar

      This->m_taskbar->stopApplication(This);
    }
}

/**
 * Called when the window minimize button is pressed.
 */

void CNxTerm::minimize(void)
{
  m_taskbar->minimizeApplication(static_cast<IApplication*>(this));
}

/**
 * Called when the window close button is pressed.
 */

void CNxTerm::close(void)
{
  m_taskbar->stopApplication(static_cast<IApplication*>(this));
}

/**
 * CNxTermFactory Constructor
 *
 * @param taskbar.  The taskbar instance used to terminate the console
 */

CNxTermFactory::CNxTermFactory(CTaskbar *taskbar)
{
  m_taskbar = taskbar;
}

/**
 * Create a new instance of an CNxTerm (as IApplication).
 */

IApplication *CNxTermFactory::create(void)
{
  // Call CTaskBar::openFullScreenWindow to create a full screen window for
  // the NxTerm application

  CApplicationWindow *window = m_taskbar->openApplicationWindow();
  if (!window)
    {
      gerr("ERROR: Failed to create CApplicationWindow\n");
      return (IApplication *)0;
    }

  // Open the window (it is hot in here)

  if (!window->open())
    {
      gerr("ERROR: Failed to open CApplicationWindow\n");
      delete window;
      return (IApplication *)0;
    }

  // Instantiate the application, providing the window to the application's
  // constructor

  CNxTerm *nxterm = new CNxTerm(m_taskbar, window);
  if (!nxterm)
    {
      gerr("ERROR: Failed to instantiate CNxTerm\n");
      delete window;
      return (IApplication *)0;
    }

  return static_cast<IApplication*>(nxterm);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CNxTermFactory::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_NXTERM_ICON);

  return bitmap;
}

/**
 * One time NSH initialization. This function must be called exactly
 * once during the boot-up sequence to initialize the NSH library.
 *
 * @return True on successful initialization
 */

bool NxWM::nshlibInitialize(void)
{
  // Initialize the global data structure

  sem_init(&g_nxtermvars.exclSem, 0, 1);
  sem_init(&g_nxtermvars.waitSem, 0, 0);

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
