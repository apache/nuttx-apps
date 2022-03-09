/****************************************************************************
 * apps/graphics/nxwidgets/src/cnxserver.cxx
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/boardctl.h>
#include <sys/prctl.h>

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <sched.h>
#include <pthread.h>
#include <debug.h>

#include <nuttx/board.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/singletons.hxx"
#include "graphics/nxwidgets/cnxserver.hxx"

/****************************************************************************
 * Static Data Members
 ****************************************************************************/

using namespace NXWidgets;

uint8_t CNxServer::m_nServers;   /**< The number of NX server instances */

/****************************************************************************
 * Method Implementations
 ****************************************************************************/

/**
 * CNXServer constructor
 */

CNxServer::CNxServer(void)
{
  // Initialize server instance state data

  m_hDevice    = (FAR NX_DRIVERTYPE *)NULL;  // LCD/Framebuffer device handle
  m_hNxServer  = (NXHANDLE)NULL;             // NX server handle
  m_connected  = false;                      // True:  Connected to the server
  sem_init(&m_connsem, 0, 0);                // Wait for server connection

  // Increment the global count of NX servers.  Normally there is only one
  // but we don't want to preclude the case where there might be multiple
  // displays, each with its own NX server instance

  m_nServers++;

  // Create miscellaneous singleton instances.  Why is this done here?
  // Because this needs to be done once before any widgets are created and we
  // don't want to rely on static constructors.

  instantiateSingletons();
}

/**
 * CNXServer destructor
 */

CNxServer::~CNxServer(void)
{
  // Disconnect from the server

  disconnect();

  // Decrement the count of NX servers.  When that count goes to zero,
  // delete all of the fake static instances

  if (--m_nServers == 0)
    {
      freeSingletons();
    }
}

/**
 * Connect to the NX Server
 */

bool CNxServer::connect(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  // Set the client task priority

  param.sched_priority = CONFIG_NXWIDGETS_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      gerr("ERROR: CNxServer::connect: sched_setparam failed: %d\n" , ret);
      return false;
    }

#ifdef CONFIG_NXWIDGET_SERVERINIT
  // Start the NX server kernel thread

  ginfo("CNxServer::connect: Starting NX server\n");
  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      gerr("ERROR: CNxServer::connect: Failed to start the NX server: %d\n", errno);
      return false;
    }
#endif // CONFIG_NXWIDGET_SERVERINIT

  // Connect to the server

  m_hNxServer = nx_connect();
  if (m_hNxServer)
    {
      pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      // Setup the VNC server to support keyboard/mouse inputs

       struct boardioc_vncstart_s vnc =
       {
         0, m_hNxServer
       };

       ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
       if (ret < 0)
         {
           gerr("ERROR: boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
           m_running = false;
           disconnect();
           return false;
         }
#endif

      // Start a separate thread to listen for server events.  This is probably
      // the least efficient way to do this, but it makes this logic flow more
      // smoothly.

      pthread_attr_init(&attr);
      param.sched_priority = CONFIG_NXWIDGETS_LISTENERPRIO;
      pthread_attr_setschedparam(&attr, &param);
      pthread_attr_setstacksize(&attr, CONFIG_NXWIDGETS_LISTENERSTACK);

      m_stop    = false;
      m_running = true;

      ret = pthread_create(&thread, &attr, listener, (FAR void *)this);
      if (ret != 0)
        {
          gerr("ERROR: NxServer::connect: pthread_create failed: %d\n", ret);
          m_running = false;
          disconnect();
          return false;
        }

      // Detach from the thread

      pthread_detach(thread);

      // Don't return until we are connected to the server

      while (!m_connected && m_running)
        {
          // Wait for the listener thread to wake us up when we really
          // are connected.

          sem_wait(&m_connsem);
        }

      // In the successful case, the listener is still running (m_running)
      // and the server is connected (m_connected).  Anything else is a failure.

      if (!m_connected || !m_running)
        {
          disconnect();
          return false;
        }
    }
  else
    {
      gerr("ERROR: NxServer::connect: nx_connect failed: %d\n", errno);
      return false;
    }

  return true;
}

/**
 * Disconnect from the NX Server
 */

void CNxServer::disconnect(void)
{
  // Is the listener running?
  // Hmm.. won't this hang is the listener is in a blocking call?

  while (m_running)
    {
      // Yes.. stop the listener thread

      m_stop = true;
      while (m_running)
        {
          // Wait for the listener thread to stop

          sem_wait(&m_connsem);
        }
    }

  // Disconnect from the server

  if (m_hNxServer)
    {
      nx_disconnect(m_hNxServer);
      m_hNxServer = NULL;
    }
}

/**
 * This is the entry point of a thread that listeners for and dispatches
 * events from the NX server.
 */

FAR void *CNxServer::listener(FAR void *arg)
{
  // The argument must be the CNxServer instance

  CNxServer *This = (CNxServer*)arg;

#if CONFIG_TASK_NAME_SIZE > 0
  prctl(PR_SET_NAME, "CNxServer::listener", 0);
#endif

  // Process events forever

  while (!This->m_stop)
    {
      // Handle the next event.  If we were configured blocking, then
      // we will stay right here until the next event is received.  Since
      // we have dedicated a while thread to servicing events, it would
      // be most natural to also select CONFIG_NX_BLOCKING -- if not, the
      // following would be a tight infinite loop (unless we added addition
      // logic with nx_eventnotify and sigwait to pace it).

      int ret = nx_eventhandler(This->m_hNxServer);
      if (ret < 0)
        {
          // An error occurred... assume that we have lost connection with
          // the server.

          gwarn("WARNING: Lost server connection: %d\n", errno);
          break;
        }

      // If we received a message, we must be connected

      if (!This->m_connected)
        {
          This->m_connected = true;
          sem_post(&This->m_connsem);
          ginfo("Connected\n");
        }
    }

  // We fall out of the loop when either (1) the server has died or
  // we have been requested to stop

  This->m_running   = false;
  This->m_connected = false;
  sem_post(&This->m_connsem);
  return NULL;
}
