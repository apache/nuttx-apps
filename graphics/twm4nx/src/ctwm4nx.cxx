/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ctwm4nx.cxx
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
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.
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
#include <cstring>
#include <cassert>
#include <cerrno>

#include <fcntl.h>
#include <semaphore.h>

#include <nuttx/semaphore.h>
#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

// Core Twm4Nx Definitions

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cbackground.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cinput.hxx"
#include "graphics/twm4nx/ciconwidget.hxx"
#include "graphics/twm4nx/ciconmgr.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/cmainmenu.hxx"
#include "graphics/twm4nx/cresize.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"

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
  m_iconmgr              = (FAR CIconMgr *)0;
  m_factory              = (FAR CWindowFactory *)0;
  m_fonts                = (FAR CFonts *)0;
  m_mainMenu             = (FAR CMainMenu *)0;
  m_resize               = (FAR CResize *)0;

#if !defined(CONFIG_TWM4NX_NOKEYBOARD) || !defined(CONFIG_TWM4NX_NOMOUSE)
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
 * Perform initialization additional, post-construction initialization
 * that may fail.  This initialization logic fully initialized the
 * Twm4Nx session.  Upon return, the session is ready for use.
 *
 * After Twm4Nx is initialized, external applications should register
 * themselves into the Main Menu in order to be a part of the desktop.
 *
 * @return True if the Twm4Nx was properly initialized.  false is
 * returned on any failure.
 */

bool CTwm4Nx::initialize(void)
{
  // Open a message queue to receive NxWidget-related events.  We need to
  // do this early so that the messasge queue name will be available to
  //  constructors

  struct mq_attr attr;
  attr.mq_maxmsg  = 32;    // REVISIT:  Should be configurable
  attr.mq_msgsize = MAX_EVENT_MSGSIZE;
  attr.mq_flags   = 0;
  attr.mq_curmsgs = 0;

  genMqName();             // Generate a random message queue name
  m_eventq = mq_open(m_queueName, O_RDONLY | O_CREAT, 0666, &attr);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             m_queueName, errno);
      cleanup();
      return false;
    }

  // Connect to the NX server

  if (!connect())
    {
      twmerr("ERROR: Failed to connect to the NX server\n");
      cleanup();
      return false;
    }

  // Get the background up as soon as possible

  m_background = new CBackground(this);
  if (m_background == (FAR CBackground *)0)
    {
      twmerr("ERROR: Failed to create CBackground\n");
      cleanup();
      return false;
    }

  // Initialize the background instance and paint the background image

  if (!m_background->initialize(&CONFIG_TWM4NX_BACKGROUND_IMAGE))
    {
      twmerr("ERROR: Failed to set background image\n");
      cleanup();
      return false;
    }

  // Get the size of the display (which is equivalent to size of the
  // background window).

  m_background->getDisplaySize(m_displaySize);

  DEBUGASSERT((unsigned int)m_displaySize.w <= INT16_MAX &&
              (unsigned int)m_displaySize.h <= INT16_MAX);

  m_maxWindow.w = INT16_MAX - m_displaySize.w;
  m_maxWindow.h = INT16_MAX - m_displaySize.h;

#if !defined(CONFIG_TWM4NX_NOKEYBOARD) || !defined(CONFIG_TWM4NX_NOMOUSE)
  // Create the keyboard/mouse input device thread

  m_input = new CInput(this);
  if (m_input == (CInput *)0)
    {
      twmerr("ERROR: Failed to create CInput\n");
      cleanup();
      return false;
    }

  if (!m_input->start())
    {
      twmerr("ERROR: Failed start the keyboard/mouse listener\n");
      cleanup();
      return false;
    }
#endif

  // Cache a CWindowFactory instance for use across the session.  The window
  // factory is needed by the Icon Manager which is instantiated below.

  m_factory = new CWindowFactory(this);
  if (m_factory == (FAR CWindowFactory *)0)
    {
      cleanup();
      return false;
    }

  // Cache a CFonts instance for use across the session.  Font support is
  // need by the Icon Manager which is instantiated next.

  m_fonts = new CFonts(this);
  if (m_fonts == (FAR CFonts *)0)
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

  // Create the Icon Manager

  m_iconmgr = new CIconMgr(this, CONFIG_TWM4NX_ICONMGR_NCOLUMNS);
  if (m_iconmgr == (FAR CIconMgr *)0)
    {
      cleanup();
      return false;
    }

  if (!m_iconmgr->initialize("Twm4Nx"))
    {
      cleanup();
      return false;
    }

  // Create and initialize a CMainMenu instance for use across the session

  m_mainMenu = new CMainMenu(this);
  if (m_mainMenu == (FAR CMainMenu *)0)
    {
      cleanup();
      return false;
    }

  if (!m_mainMenu->initialize())
    {
      cleanup();
      return false;
    }

  // Now, complete the initialization of some preceding instances that
  // depend on the Main Menu being in place

  if (!m_factory->addMenuItems())
    {
      cleanup();
      return false;
    }

  if (!m_iconmgr->addMenuItems())
    {
      cleanup();
      return false;
    }

  // Cache a CResize instance for use across the session

  m_resize = new CResize(this);
  if (m_resize == (FAR CResize *)0)
    {
      cleanup();
      return false;
    }

  if (!m_resize->initialize())
    {
      cleanup();
      return false;
    }

  return true;
}

/**
 * This is the main, event loop of the Twm4Nx session.
 *
 * @return True if the Twm4Nxr was terminated noramly.  false is returned
 * on any failure.
 */

bool CTwm4Nx::eventLoop(void)
{
  // Enter the event loop

  twminfo("Entering event loop\n");
  for (; ; )
    {
      // Wait for the next NxWidget event

      union
      {
        struct SEventMsg eventmsg;
        char buffer[MAX_EVENT_MSGSIZE];
      } u;

      int ret = mq_receive(m_eventq, u.buffer, MAX_EVENT_MSGSIZE,
                           (FAR unsigned int *)0);
      if (ret < 0)
        {
          twmerr("ERROR: mq_receive failed: %d\n", errno);
          cleanup();
          return false;
        }

      // If we are resizing, then drop all non-critical events (of course,
      // all resizing events must be critical)

      if (!m_resize->resizing() || EVENT_ISCRITICAL(u.eventmsg.eventID))
        {
          // Dispatch the new event

          if (!dispatchEvent(&u.eventmsg))
            {
              twmerr("ERROR: dispatchEvent() failed, eventID=%u\n",
                     u.eventmsg.eventID);
              cleanup();
              return false;
            }
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

  std::asprintf(&m_queueName, "Twm4Nx%06lu", randvalue);
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
  twminfo("eventID: %u\n", eventmsg->eventID);

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
  twminfo("eventID: %u\n", eventmsg->eventID);

  enum EEventRecipient recipient =
    (enum EEventRecipient)(eventmsg->eventID & EVENT_RECIPIENT_MASK);

  bool ret = false;
  switch (recipient)
    {
      case EVENT_RECIPIENT_SYSTEM:     // Twm4Nx system event
        ret = systemEvent(eventmsg);
        break;

      case EVENT_RECIPIENT_BACKGROUND: // Background window event
        ret = m_background->event(eventmsg);
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

      case EVENT_RECIPIENT_MAINMENU:   // Main menu related event
        {
          ret = m_mainMenu->event(eventmsg);
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

          DEBUGASSERT(eventmsg->handler != (FAR void *)0);
          FAR CTwm4NxEvent *handler = (FAR CTwm4NxEvent *)eventmsg->handler;
          ret = handler->event(eventmsg);
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
      mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

  // Delete the background

  if (m_background != (FAR CBackground *)0)
    {
      delete m_background;
      m_background = (FAR CBackground *)0;
    }

#if !defined(CONFIG_TWM4NX_NOKEYBOARD) || !defined(CONFIG_TWM4NX_NOMOUSE)
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

  // Free the session CMainMenu instance

  if (m_mainMenu != (CMainMenu *)0)
    {
      delete m_mainMenu;
      m_mainMenu = (CMainMenu *)0;
    }

  // Free the session CResize instance

  if (m_resize != (CResize *)0)
    {
      delete m_resize;
      m_resize = (CResize *)0;
    }

  CNxServer::disconnect();
}
