/********************************************************************************************
 * NxWidgets/nxwm/src/ckeyboard.cxx
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
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

#include <cunistd>
#include <cerrno>
#include <cfcntl>

#include <sched.h>
#include <pthread.h>
#include <assert.h>
#include <debug.h>

#include "nxwmconfig.hxx"
#include "ckeyboard.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * CKeyboard Method Implementations
 ********************************************************************************************/

using namespace NxWM;

/**
 * CKeyboard Constructor
 *
 * @param server. An instance of the NX server.  This will be needed for
 *   injecting mouse data.
 */

CKeyboard::CKeyboard(NXWidgets::CNxServer *server)
{
  m_server      = server;              // Save the NX server
  m_kbdFd     = -1;                    // Device driver is not opened
  m_state       = LISTENER_NOTRUNNING; // The listener thread is not running yet

  // Initialize the semaphore used to synchronize with the listener thread

  sem_init(&m_waitSem, 0, 0);
}

/**
 * CKeyboard Destructor
 */

CKeyboard::~CKeyboard(void)
{
  // Stop the listener thread

  m_state = LISTENER_STOPREQUESTED;

  // Wake up the listener thread so that it will use our buffer
  // to receive data
  // REVISIT:  Need wait here for the listener thread to terminate

  (void)pthread_kill(m_thread, CONFIG_NXWM_KEYBOARD_SIGNO);

  // Close the keyboard device (or should these be done when the thread exits?)

  if (m_kbdFd >= 0)
    {
      std::close(m_kbdFd);
    }
}

/**
 * Start the keyboard listener thread.
 *
 * @return True if the keyboard listener thread was correctly started.
 */

bool CKeyboard::start(void)
{
  pthread_attr_t attr;

  ginfo("Starting listener\n");

  // Start a separate thread to listen for keyboard events

  (void)pthread_attr_init(&attr);

  struct sched_param param;
  param.sched_priority = CONFIG_NXWM_KEYBOARD_LISTENERPRIO;
  (void)pthread_attr_setschedparam(&attr, &param);

  (void)pthread_attr_setstacksize(&attr, CONFIG_NXWM_KEYBOARD_LISTENERSTACK);

  m_state  = LISTENER_STARTED; // The listener thread has been started, but is not yet running

  int ret = pthread_create(&m_thread, &attr, listener, (FAR void *)this);
  if (ret != 0)
    {
      gerr("ERROR: CKeyboard::start: pthread_create failed: %d\n", ret);
      return false;
    }

  // Detach from the thread

  (void)pthread_detach(m_thread);

  // Don't return until we are sure that the listener thread is running
  // (or until it reports an error).

  while (m_state == LISTENER_STARTED)
    {
      // Wait for the listener thread to wake us up when we really
      // are connected.

      (void)sem_wait(&m_waitSem);
    }

  // Then return true only if the listener thread reported successful
  // initialization.

  ginfo("Listener m_state=%d\n", (int)m_state);
  return m_state == LISTENER_RUNNING;
}

/**
 * Open the keyboard device.  Not very interesting for the case of
 * standard device but much more interesting for a USB keyboard device
 * that may disappear when the keyboard is disconnect but later reappear
 * when the keyboard is reconnected.  In this case, this function will
 * not return until the keyboard device was successfully opened (or
 * until an irrecoverable error occurs.
 *
 * Opens the keyboard device specified by CONFIG_NXWM_KEYBOARD_DEVPATH.
 *
 * @return On success, then method returns a valid file descriptor that
 * can be used to redirect stdin.  A negated errno value is returned
 * if an irrecoverable error occurs.
 */

int CKeyboard::open(void)
{
  int fd;

  // Loop until we have successfully opened the USB keyboard (or until some
  // irrecoverable error occurs).

  do
    {
      // Try to open the keyboard device

      fd = std::open(CONFIG_NXWM_KEYBOARD_DEVPATH, O_RDONLY);
      if (fd < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          // EINTR should be ignored because it is not really an error at
          // all.  We should retry immediately

          if (errcode != EINTR)
            {
#ifdef CONFIG_NXWM_KEYBOARD_USBHOST
              // ENOENT means that the USB device is not yet connected and,
              // hence, has no entry under /dev.  If the USB driver still
              // exists under /dev (because other threads still have the driver
              // open), then we might also get ENODEV.

              if (errcode == ENOENT || errcode == ENODEV)
                {
                  // REVIST: Can we inject a constant string here to let the
                  // user know that we are waiting for a USB keyboard to be
                  // connected?

                  // Sleep a bit and try again

                  ginfo("WAITING for a USB device\n");
                  std::sleep(2);
                }

              // Anything else would be really bad.

              else
#endif
                {
                  // Let the top-level logic decide what it wants to do
                  // about all really bad things

                  gerr("ERROR: Failed to open %s for reading: %d\n",
                       CONFIG_NXWM_KEYBOARD_DEVPATH, errcode);
                  return -errcode;
                }
            }
        }
    }
  while (fd < 0);

  return fd;
}

/**
 * This is the heart of the keyboard listener thread.  It contains the
 * actual logic that listeners for and dispatches keyboard events to the
 * NX server.
 *
 * @return  If the session terminates gracefully (i.e., because >m_state
 * is no longer equal to  LISTENER_RUNNING), then method returns OK.  A
 * negated errno value is returned if an error occurs while reading from
 * the keyboard device.  A read error, depending upon the type of the
 * error, may simply indicate that a USB keyboard was removed and we
 * should wait for the keyboard to be connected.
 */

int CKeyboard::session(void)
{
  ginfo("Session started\n");

  // Loop, reading and dispatching keyboard data

  while (m_state == LISTENER_RUNNING)
    {
      // Read one keyboard sample

      ginfo("Listening for keyboard input\n");

      uint8_t rxbuffer[CONFIG_NXWM_KEYBOARD_BUFSIZE];
      ssize_t nbytes = read(m_kbdFd, rxbuffer,
                            CONFIG_NXWM_KEYBOARD_BUFSIZE);

      // Check for errors

      if (nbytes < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          // EINTR is not really an error, it simply means that something is
          // trying to get our attention.  We need to check m_state to see
          // if we were asked to terminate

          if (errcode != EINTR)
            {
              // Let the top-level listener logic decide what to do about
              // the read failure.

              gerr("ERROR: read %s failed: %d\n",
                   CONFIG_NXWM_KEYBOARD_DEVPATH, errcode);
              return -errcode;
            }

          fwarn("WARNING: Awakened with EINTR\n");
        }

      // Give the keyboard input to NX

      else if (nbytes > 0)
        {
          // Looks like good keyboard input... process it.
          // First, get the server handle

          NXHANDLE handle = m_server->getServer();

          // Then inject the keyboard input into NX

          int ret = nx_kbdin(handle, (uint8_t)nbytes, rxbuffer);
          if (ret < 0)
            {
              gerr("ERROR: nx_kbdin failed: %d\n", ret);
              //break;  ignore the error
            }
        }
    }

  return OK;
}

/**
 * The keyboard listener thread.  This is the entry point of a thread
 * that listeners for and dispatches keyboard events to the NX server.
 * It simply opens the keyboard device (using CKeyboard::open()) and
 * executes the session (via CKeyboard::session()).
 *
 * If an errors while reading from the keyboard device AND we are
 * configured to use a USB keyboard, then this function will wait for
 * the USB keyboard to be re-connected.
 *
 * @param arg.  The CKeyboard 'this' pointer cast to a void*.
 * @return This function normally does not return but may return NULL on
 *   error conditions.
 */

FAR void *CKeyboard::listener(FAR void *arg)
{
  CKeyboard *This = (CKeyboard *)arg;

  ginfo("Listener started\n");

#ifdef CONFIG_NXWM_KEYBOARD_USBHOST
  // Indicate that we have successfully started.  We might be stuck waiting
  // for a USB keyboard to be connected, but we are technically running

  This->m_state = LISTENER_RUNNING;
  sem_post(&This->m_waitSem);

  // Loop until we are told to quit

  while (This->m_state == LISTENER_RUNNING)
#endif
    {
      // Open/Re-open the keyboard device

      This->m_kbdFd = This->open();
      if (This->m_kbdFd < 0)
        {
          gerr("ERROR: open failed: %d\n", This->m_kbdFd);
          This->m_state = LISTENER_FAILED;
          sem_post(&This->m_waitSem);
          return (FAR void *)0;
        }

#ifndef CONFIG_NXWM_KEYBOARD_USBHOST
      // Indicate that we have successfully initialized

      This->m_state = LISTENER_RUNNING;
      sem_post(&This->m_waitSem);
#endif

      // Now execute the session.  The session will run until either (1) we
      // were asked to terminate gracefully (with m_state !=LISTENER_RUNNING),
      // of if an error occurred while reading from the keyboard device.  If
      // we are configured to use a USB keyboard, then this error, depending
      // upon what the error is, may indicate that the USB keyboard has been
      // removed.  In that case, we need to continue looping and, hopefully,
      // the USB keyboard will be reconnected.

      int ret = This->session();
#ifdef CONFIG_NXWM_KEYBOARD_USBHOST
      if (ret < 0)
        {
          ferr("ERROR: CKeyboard::session() returned %d\n", ret);
        }
#else
      // No errors from session() are expected

      DEBUGASSERT(ret == OK);
      UNUSED(ret);
#endif

      // Close the keyboard device

      (void)std::close(This->m_kbdFd);
      This->m_kbdFd = -1;
    }

  // We should get here only if we were asked to terminate via
  // m_state = LISTENER_STOPREQUESTED (or perhaps if some irrecoverable
  // error has occurred).

  ginfo("Listener exiting\n");
  This->m_state = LISTENER_TERMINATED;
  return (FAR void *)0;
}
