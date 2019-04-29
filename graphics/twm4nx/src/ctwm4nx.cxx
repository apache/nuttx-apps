/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ctwm4nx.cxx
// Twm4Nx - "Tom's Window Manager" for the NuttX NX Server
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
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

#include <cstdlib>
#include <cstdbool>
#include <csignal>
#include <cstdio>
#include <cfcntl>
#include <cstring>
#include <cerrno>

#include <sys/boardctl.h>
#include <semaphore.h>
#include <debug.h>

#include <nuttx/semaphore.h>
#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

#include "platform/cxxinitialize.h"
#include "netutils/netinit.h"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cbackground.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cinput.hxx"
#include "graphics/twm4nx/cicon.hxx"
#include "graphics/twm4nx/ciconwidget.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/cresize.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

#define DEFAULT_NICE_FONT "variable"
#define DEFAULT_FAST_FONT "fixed"

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Suppress name-mangling

#ifdef BUILD_MODULE
extern "C" int main(int argc, FAR char *argv[]);
#else
extern "C" int twm4nx_main(int argc, char *argv[]);
#endif

/////////////////////////////////////////////////////////////////////////////
// Public Data
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

const char Twm4Nx::GNoName[] = "Untitled";  // Name if no name is specified

/////////////////////////////////////////////////////////////////////////////
// CTwm4Nx Implementation
/////////////////////////////////////////////////////////////////////////////

/**
 * CTwm4Nx Constructor
 *
 * @param display.  Indicates which display will be used.  Usually zero
 * except in the case wehre there of multiple displays.
 */

CTwm4Nx::CTwm4Nx(int display)
{
  m_display              = display;
  m_eventq               = (mqd_t)-1;
  m_background           = (FAR CBackground *)0;
  m_icon                 = (FAR CIcon *)0;
  m_iconmgr              = (FAR CIconMgr *)0;
  m_factory              = (FAR CWindowFactory *)0;
  m_fonts                = (FAR CFonts *)0;
  m_resize               = (FAR CResize *)0;

#ifndef CONFIG_VNCSERVER
  m_input                = (FAR CInput *)0;
#endif
}

/**
 * CTwm4Nx Destructor
 */

CTwm4Nx::~CTwm4Nx(void)
{
  cleanup();
}

/**
 * This is the main, controlling thread of the window manager.  It is
 * called only from the extern "C" main() entry point.
 *
 * NOTE: In the event of truly abnormal conditions, this function will
 * not return.  It will exit via the abort() method.
 *
 * @return True if the window manager was terminated properly.  false is
 *   return on any failure.
 */

bool CTwm4Nx::run(void)
{
  // Open a message queue to receive NxWidget-related events.  We need to
  // do this early so that the messasge queue name will be available to
  //  constructors

  struct mq_attr attr;
  attr.mq_maxmsg  = 32;    // REVISIT:  Should be configurable
  attr.mq_msgsize = CONFIG_MQ_MAXMSGSIZE;
  attr.mq_flags   = 0;
  attr.mq_curmsgs = 0;

  genMqName();             // Generate a random message queue name
  m_eventq = mq_open(m_queueName, O_RDONLY | O_CREAT, 0666, &attr);
  if (m_eventq == (mqd_t)-1)
    {
      gerr("ERROR: Failed open message queue '%s': %d\n",
           m_queueName, errno);
      cleanup();
      return false;
    }

#if defined(CONFIG_HAVE_CXX) && defined(CONFIG_HAVE_CXXINITIALIZE)
  // Call all C++ static constructors

  up_cxxinitialize();
#endif

  // Connect to the NX server

  if (!connect())
    {
      gerr("ERROR: Failed to connect to the NX server\n");
      cleanup();
      return false;
    }

  // Get the background up as soon as possible

  m_background = new CBackground(this);
  if (m_background == (FAR CBackground *)0)
    {
      gerr("ERROR: Failed to create CBackground\n");
      cleanup();
      return false;
    }

  // Paint the background image

  if (!m_background->setBackgroundImage(&CONFIG_TWM4NX_BACKGROUND_IMAGE))
    {
      gerr("ERROR: Failed to set backgournd image\n");
      cleanup();
      return false;
    }

  // Get the size of the display (which is equivalent to size of the
  // background window).

  m_background->getDisplaySize(m_displaySize);

  DEBUGASSERT((unsigned int)m_displaySize.w <= INT16_MAX &&
              (unsigned int)m_displaySize.h <= INT16_MAX);

  m_maxWindow.w = INT16_MAX - m_displaySize.w;
  m_maxWindow.h = INT16_MAX - m_displaySize.w;

#ifndef CONFIG_VNCSERVER
  // Create the keyboard/mouse input device thread

  m_input = new CInput(this);
  if (m_input == (CInput *)0)
    {
      gerr("ERROR: Failed to create CInput\n");
      cleanup();
      return false;
    }

  if (!m_input->start())
    {
      gerr("ERROR: Failed start the keyboard/mouse listener\n");
      cleanup();
      return false;
    }
#endif

  // Create the Icon Manager

  m_iconmgr = new CIconMgr(this, 4);
  if (m_iconmgr == (CIconMgr *)0)
    {
      cleanup();
      return false;
    }

  if (!m_iconmgr->initialize("Twm4Nx"))
    {
      cleanup();
      return false;
    }

  // Cache a CIcon instance for use across the session

  m_icon = new CIcon(this);
  if (m_iconmgr == (CIconMgr *)0)
    {
      cleanup();
      return false;
    }

  // Cache a CFonts instance for use across the session

  m_fonts = new CFonts(this);
  if (m_fonts == (CFonts *)0)
    {
      cleanup();
      return false;
    }

  // Create all fonts

  if (!m_fonts->initialize())
    {
      cleanup();
      return false;
    }

  // Cache a CWindowFactory instance for use across the session

  m_factory = new CWindowFactory(this);
  if (m_factory == (CWindowFactory *)0)
    {
      cleanup();
      return false;
    }

  // Cache a CResize instance for use across the session

  m_resize = new CResize(this);
  if (m_resize == (CResize *)0)
    {
      cleanup();
      return false;
    }

  if (!m_resize->initialize())
    {
      cleanup();
      return false;
    }

  // Enter the event loop

  for (; ; )
    {
      // Wait for the next NxWidget event

      struct SEventMsg eventmsg;
      int ret = mq_receive(m_eventq, (FAR char *)&eventmsg,
                           sizeof(struct SEventMsg), (FAR unsigned int *)0);
      if (ret < 0)
        {
          gerr("ERROR: mq_receive failed: %d\n", errno);
          cleanup();
          return false;
        }

      // Dispatch the new event

      if (!dispatchEvent(&eventmsg))
        {
          gerr("ERROR: dispatchEvent failed\n");
          cleanup();
          return false;
        }
    }

  return true;  // Not reachable
}

/**
 * Connect to the NX server
 *
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CTwm4Nx::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Set the background color

      if (!setBackgroundColor(CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR))
        {
          // Failed
        }
    }

  return nxConnected;
}

/**
 * Generate a random message queue name.  Different message queue
 * names are required for each instance of Twm4Nx that is started.
 */

void CTwm4Nx::genMqName(void)
{
  unsigned long randvalue =
    (unsigned long)std::random() & 0x00fffffful;

  (void)std::asprintf(&m_queueName, "Twm4Nx%06ul", randvalue);
}

/**
 * Handle SYSTEM events.
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CTwm4Nx::systemEvent(FAR struct SEventMsg *eventmsg)
{
  switch (eventmsg->eventID)
    {
      case EVENT_SYSTEM_NOP:    // Null event
        break;

      case EVENT_SYSTEM_ERROR:  // Report system error
                                // REVISIT: An audible tone should be generated
        break;

      case EVENT_SYSTEM_EXIT:   // Terminate the Twm4Nx session
        abort();
        break;                  // Does not return

      default:
        return false;
    }

  return true;
};

/**
 * Dispatch NxWidget-related events.
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly dispatched.  false is
 *   return on any failure.
 */

bool CTwm4Nx::dispatchEvent(FAR struct SEventMsg *eventmsg)
{
  enum EEventRecipient recipient =
    (enum EEventRecipient)(eventmsg->eventID & EVENT_RECIPIENT_MASK);

  bool ret = false;
  switch (recipient)
    {
      case EVENT_RECIPIENT_MSG:        // NX message event
        ret = CWindowEvent::event(eventmsg);
        break;

        case EVENT_RECIPIENT_SYSTEM:   // Twm4Nx system event
        ret = systemEvent(eventmsg);
        break;

      case EVENT_RECIPIENT_ICONWIDGET: // Icon widget event
        {
          FAR CIconWidget *iconWidget = (FAR CIconWidget *)eventmsg->obj;
          DEBUGASSERT(iconWidget != (FAR CIconWidget *)0);
          ret = iconWidget->event(eventmsg);
        }
        break;

      case EVENT_RECIPIENT_ICONMGR:    // Icon Manager event
        ret = m_iconmgr->event(eventmsg);
        break;

      case EVENT_RECIPIENT_MENU:       // Menu related event
        {
          FAR CMenus *menus = (FAR CMenus *)eventmsg->obj;
          DEBUGASSERT(menus != (FAR CMenus *)0);
          ret = menus->event(eventmsg);
        }
        break;

      case EVENT_RECIPIENT_WINDOW:     // Window related event
      case EVENT_RECIPIENT_TOOLBAR:    // Toolbar related event
      case EVENT_RECIPIENT_BORDER:     // Window border related event
        ret = m_factory->event(eventmsg);
        break;

      case EVENT_RECIPIENT_RESIZE:     // Wind0w resize event
        ret = m_resize->event(eventmsg);
        break;

      case EVENT_RECIPIENT_APP:        // Application menu event
        {
          // Application events are unique in that they do not have any
          // fixed, a priori endpoint.  Rather, the endpoint must be
          // provided in the 'handler' field of the message

          DEBUGASSERT(eventmsg->handler != (FAR CTwm4NxEvent *)0);
          ret = eventmsg->handler->event(eventmsg);
        }
        break;

      case EVENT_RECIPIENT_MASK:       // Used to isolate recipient
      default:
        break;
    }

  return ret;
}

/**
 * Cleanup and exit Twm4Nx abnormally.
 */

void CTwm4Nx::abort()
{
  cleanup();
  std::exit(EXIT_FAILURE);
}

/**
 * Cleanup in preparation for termination.
 */

void CTwm4Nx::cleanup()
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      (void)mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Delete the background

  if (m_background != (FAR CBackground *)0)
    {
      delete m_background;
      m_background = (FAR CBackground *)0;
    }

#ifndef CONFIG_VNCSERVER
  // Halt the keyboard/mouse listener and destroy the CInput class

  if (m_input != (CInput *)0)
    {
      delete m_input;
      m_input = (CInput *)0;
    }
#endif

  // Free the Icon Manager

  if (m_iconmgr != (CIconMgr *)0)
    {
      delete m_iconmgr;
      m_iconmgr = (CIconMgr *)0;
    }

  // Free the session CWindowFactory instance

  if (m_factory != (CWindowFactory *)0)
    {
      delete m_factory;
      m_factory = (CWindowFactory *)0;
    }

  // Free the session CResize instance

  if (m_resize != (CResize *)0)
    {
      delete m_resize;
      m_resize = (CResize *)0;
    }

  CNxServer::disconnect();
}

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Name: main/twm4nx_main
//
// Description:
//    Start of TWM
//
/////////////////////////////////////////////////////////////////////////////

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int twm4nx_main(int argc, char *argv[])
#endif
{
  int display = 0;

  for (int i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-')
        {
          switch (argv[i][1])
            {
            case 'd':          // -display <number>
              if (std::strcmp(&argv[i][1], "display"))
                {
                  goto usage;
                }

              if (++i >= argc)
                {
                  goto usage;
                }

              display = atoi(argv[i]);
              continue;
            }
        }

    usage:
      gerr("Usage:  %s [-display <number>]\n", argv[0]);
      return EXIT_FAILURE;
    }

  int ret;

#if defined(CONFIG_TWM4NX_ARCHINIT) && defined(CONFIG_LIB_BOARDCTL) && \
   !defined(CONFIG_BOARD_LATE_INITIALIZE)
  // Should we perform board-specific initialization?  There are two ways
  // that board initialization can occur:  1) automatically via
  // board_late_initialize() during bootup if CONFIG_BOARD_LATE_INITIALIZE, or
  // 2) here via a call to boardctl() if the interface is enabled
  // (CONFIG_LIB_BOARDCTL=y).  board_early_initialize() is also possibility,
  // although less likely.

  ret = boardctl(BOARDIOC_INIT, 0);
  if (ret < 0)
    {
      gerr("ERROR: boardctl(BOARDIOC_INIT) failed: %d\n", errno);
      return EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_TWM4NX_NETINIT
  /* Bring up the network */

  ret = netinit_bringup();
  if (ret < 0)
    {
      gerr("ERROR: netinit_bringup() failed: %d\n", ret);
      return EXIT_FAILURE;
    }
#endif

  UNUSED(ret);

  /* Create an instance of CTwm4Nx and and run it */

  FAR CTwm4Nx *twm4nx = new CTwm4Nx(display);
  if (twm4nx == (FAR CTwm4Nx *)0)
    {
      gerr("ERROR: Failed to instantiate CTwm4Nx\n");
      return EXIT_FAILURE;
    }

  // Start the window manager

  bool success = twm4nx->run();
  if (!success)
    {
      gerr(" ERROR: Terminating due to failure\n");
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
