/********************************************************************************************
 * apps/graphics/NxWidgets/nxwm/src/cinput.cxx
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
#include <poll.h>
#include <pthread.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/twm4nx_cursor.hxx"
#include "graphics/twm4nx/cinput.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * CInput Method Implementations
 ********************************************************************************************/

using namespace Twm4Nx;

/**
 * CInput Constructor
 *
 * @param twm4nx. An instance of the NX server.  This will be needed for
 *   injecting keyboard data.
 */

CInput::CInput(CTwm4Nx *twm4nx)
{
  m_twm4nx      = twm4nx;              // Save the NX server
  m_kbdFd      = -1;                   // Device driver is not opened
  m_state       = LISTENER_NOTRUNNING; // The listener thread is not running yet

  // Initialize the semaphore used to synchronize with the listener thread

  sem_init(&m_waitSem, 0, 0);
}

/**
 * CInput Destructor
 */

CInput::~CInput(void)
{
  // Stop the listener thread

  m_state = LISTENER_STOPREQUESTED;

  // Wake up the listener thread so that it will use our buffer
  // to receive data
  // REVISIT:  Need wait here for the listener thread to terminate

  (void)pthread_kill(m_thread, CONFIG_TWM4NX_INPUT_SIGNO);

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

bool CInput::start(void)
{
  pthread_attr_t attr;

  ginfo("Starting listener\n");

  // Start a separate thread to listen for keyboard events

  (void)pthread_attr_init(&attr);

  struct sched_param param;
  param.sched_priority = CONFIG_TWM4NX_INPUT_LISTENERPRIO;
  (void)pthread_attr_setschedparam(&attr, &param);

  (void)pthread_attr_setstacksize(&attr, CONFIG_TWM4NX_INPUT_LISTENERSTACK);

  m_state  = LISTENER_STARTED; // The listener thread has been started, but is not yet running

  int ret = pthread_create(&m_thread, &attr, listener, (FAR void *)this);
  if (ret != 0)
    {
      gerr("ERROR: CInput::start: pthread_create failed: %d\n", ret);
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
 * Opens the keyboard device specified by CONFIG_TWM4NX_KEYBOARD_DEVPATH.
 *
 * @return On success, then method returns a valid file descriptor.  A
 *    negated errno value is returned if an irrecoverable error occurs.
 */

int CInput::keyboardOpen(void)
{
  int fd;

  // Loop until we have successfully opened the USB keyboard (or until some
  // irrecoverable error occurs).

  do
    {
      // Try to open the keyboard device

      fd = std::open(CONFIG_TWM4NX_KEYBOARD_DEVPATH, O_RDONLY);
      if (fd < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          // EINTR should be ignored because it is not really an error at
          // all.  We should retry immediately

          if (errcode != EINTR)
            {
#ifdef CONFIG_TWM4NX_KEYBOARD_USBHOST
              // ENOENT means that the USB keyboard is not yet connected and,
              // hence, has no entry under /dev.  If the USB driver still
              // exists under /dev (because other threads still have the driver
              // open), then we might also get ENODEV.

              if (errcode == ENOENT || errcode == ENODEV)
                {
                  // REVIST: Can we inject a constant string here to let the
                  // user know that we are waiting for a USB keyboard to be
                  // connected?

                  // Sleep a bit and try again

                  ginfo("WAITING for a USB keyboard\n");
                  std::sleep(2);
                }

              // Anything else would be really bad.

              else
#endif
                {
                  // Let the top-level logic decide what it wants to do
                  // about all really bad things

                  gerr("ERROR: Failed to open %s for reading: %d\n",
                       CONFIG_TWM4NX_KEYBOARD_DEVPATH, errcode);
                  return -errcode;
                }
            }
        }
    }
  while (fd < 0);

  return fd;
}

/**
 * Open the mouse input devices.  Not very interesting for the
 * case of standard character device but much more interesting for
 * USB mouse devices that may disappear when disconnected but later
 * reappear when reconnected.  In this case, this function will
 * not return until the input device was successfully opened (or
 * until an irrecoverable error occurs).
 *
 * Opens the mouse input device specified by CONFIG_TWM4NX_MOUSE_DEVPATH.
 *
 * @return On success, then method returns a valid file descriptor.  A
 *    negated errno value is returned if an irrecoverable error occurs.
 */

inline int CInput::mouseOpen(void)
{
  int fd;

  // Loop until we have successfully opened the USB mouse (or until some
  // irrecoverable error occurs).

  do
    {
      // Try to open the mouse device

      fd = std::open(CONFIG_TWM4NX_MOUSE_DEVPATH, O_RDONLY);
      if (fd < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          // EINTR should be ignored because it is not really an error at
          // all.  We should retry immediately

          if (errcode != EINTR)
            {
#ifdef CONFIG_TWM4NX_MOUSE_USBHOST
              // ENOENT means that the USB mouse is not yet connected and,
              // hence, has no entry under /dev.  If the USB driver still
              // exists under /dev (because other threads still have the driver
              // open), then we might also get ENODEV.

              if (errcode == ENOENT || errcode == ENODEV)
                {
                  // REVIST: Can we inject a constant string here to let the
                  // user know that we are waiting for a USB mouse to be
                  // connected?

                  // Sleep a bit and try again

                  ginfo("WAITING for a USB mouse\n");
                  std::sleep(2);
                }

              // Anything else would be really bad.

              else
#endif
                {
                  // Let the top-level logic decide what it wants to do
                  // about all really bad things

                  gerr("ERROR: Failed to open %s for reading: %d\n",
                       CONFIG_TWM4NX_MOUSE_DEVPATH, errcode);
                  return -errcode;
                }
            }
        }
    }
  while (fd < 0);

  return fd;
}

/**
 * Read data from the keyboard device and inject the keyboard data
 * into NX for proper distribution.
 *
 * @return On success, then method returns a valid file descriptor.  A
 *    negated errno value is returned if an irrecoverable error occurs.
 */

int CInput::keyboardInput(void)
{
  // Read one keyboard sample

  ginfo("Reading keyboard input\n");

  uint8_t rxbuffer[CONFIG_TWM4NX_KEYBOARD_BUFSIZE];
  ssize_t nbytes = read(m_kbdFd, rxbuffer,
                        CONFIG_TWM4NX_KEYBOARD_BUFSIZE);

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
               CONFIG_TWM4NX_KEYBOARD_DEVPATH, errcode);
          return -errcode;
        }

      fwarn("WARNING: Awakened with EINTR\n");
    }

  // Give the keyboard input to NX

  else if (nbytes > 0)
    {
      // Looks like good keyboard input... process it.
      // NOTE: m_twm4nx inherits from NXWidgets::CNXServer so we all ready
      // have the server instance.

      // Inject the keyboard input into NX

      int ret = nx_kbdin(m_twm4nx, (uint8_t)nbytes, rxbuffer);
      if (ret < 0)
        {
          gerr("ERROR: nx_kbdin failed: %d\n", ret);

          // Ignore the error
        }
    }

  return OK;
}

/**
 * Read data from the mouse device, update the cursor position, and
 * inject the mouse data into NX for proper distribution.
 *
 * @return On success, then method returns a valid file descriptor.  A
 *    negated errno value is returned if an irrecoverable error occurs.
 */

int CInput::mouseInput(void)
{
  // Read one mouse sample

  ginfo("Reading mouse input\n");

  uint8_t rxbuffer[CONFIG_TWM4NX_MOUSE_BUFSIZE];
  ssize_t nbytes = read(m_mouseFd, rxbuffer,
                        CONFIG_TWM4NX_MOUSE_BUFSIZE);

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
               CONFIG_TWM4NX_KEYBOARD_DEVPATH, errcode);
          return -errcode;
        }

      fwarn("WARNING: Awakened with EINTR\n");
    }

  // On a truly successful read, the size of the returned data will
  // be greater than or equal to size of one touchscreen sample.  It
  // be greater only in the case of a multi-touch touchscreen device
  // when multi-touches are reported.

  else if (nbytes < (ssize_t)sizeof(struct mouse_report_s))
    {
      gerr("ERROR Unexpected read size=%d, expected=%d\n",
           nbytes, sizeof(struct mouse_report_s));
      return -EIO;
    }
  else
    {
      // Looks like good mouse input... process it.
      // NOTE: m_twm4nx inherits from NXWidgets::CNXServer so we all ready
      // have the server instance.

      // Update the cursor position

      FAR struct mouse_report_s *rpt =
        (FAR struct mouse_report_s *)rxbuffer;

      struct nxgl_point_s pos =
      {
        .x = rpt->x,
        .y = rpt->y
      };

      int ret = nxcursor_setposition(m_twm4nx, &pos);
      if (ret < 0)
        {
          gerr("ERROR: nxcursor_setposition failed: %d\n", ret);

          // Ignore the error
        }

      // Then inject the mouse input into NX

      ret = nx_mousein(m_twm4nx, rpt->x, rpt->y, rpt->buttons);
      if (ret < 0)
        {
          gerr("ERROR: nx_mousein failed: %d\n", ret);

          // Ignore the error
        }
    }

  return OK;
}

/**
 * This is the heart of the keyboard/mouse listener thread.  It
 * contains the actual logic that listeners for and dispatches input
 * events to the NX server.
 *
 * @return  If the session terminates gracefully (i.e., because >m_state
 *   is no longer equal to LISTENER_RUNNING, then method returns OK.  A
 *   negated errno value is returned if an error occurs while reading from
 *   the input device.  A read error, depending upon the type of the
 *   error, may simply indicate that a USB device was removed and we
 *   should wait for the device to be connected.
 */

int CInput::session(void)
{
  ginfo("Session started\n");

  // Center the cursor

  struct nxgl_size_s size;
  m_twm4nx->getDisplaySize(&size);

  struct nxgl_point_s pos;
  pos.x = size.w / 2,
  pos.y = size.h / 2,

  m_twm4nx->setCursorPosition(&pos);

  // Set the default cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_CURSOR_IMAGE);

  // Enable the cursor

  m_twm4nx->enableCursor(true);

  // Loop, reading and dispatching keyboard data

  int ret = OK;
  while (m_state == LISTENER_RUNNING)
    {
      // Wait for data availability

      struct pollfd pfd[2] =
      {
        {
          .fd      = m_kbdFd,
          .events  = POLLIN | POLLERR | POLLHUP,
          .revents = 0
        },
        {
          .fd      = m_mouseFd,
          .events  = POLLIN | POLLERR | POLLHUP,
          .revents = 0
        },
      };

      ret = poll(pfd, 2, -1);
      if (ret < 0)
        {
          int errcode = errno;

          /* Ignore signal interruptions */

          if (errcode == EINTR)
            {
              continue;
            }
          else
            {
              gerr("ERROR: poll() failed");
              break;
            }
        }

      // Check for keyboard input

      if ((pfd[0].revents & (POLLERR | POLLHUP)) != 0)
        {
          gerr("ERROR: keyboard poll() failed. revents=%04x\n",
               pfd[0].revents);
          ret = -EIO;
          break;
        }

      if ((pfd[0].revents & POLLIN) != 0)
        {
          ret = keyboardInput();
          if (ret < 0)
            {
              gerr("ERROR: keyboardInput() failed: %d\n", ret);
              break;
            }
        }

      // Check for mouse input

      if ((pfd[1].revents & (POLLERR | POLLHUP)) != 0)
        {
          gerr("ERROR: Mouse poll() failed. revents=%04x\n",
               pfd[1].revents);
          ret = -EIO;
          break;
        }

      if ((pfd[1].revents & POLLIN) != 0)
        {
          ret = mouseInput();
          if (ret < 0)
            {
              gerr("ERROR: mouseInput() failed: %d\n", ret);
              break;
            }
        }
    }

  // Disable the cursor

  m_twm4nx->enableCursor(false);
  return ret;
}

/**
 * The keyboard/mouse listener thread.  This is the entry point of a
 * thread that listeners for and dispatches keyboard and mouse events
 * to the NX server. It simply opens the input devices (using
 * CInput::keyboardOpen() and CInput::mouseOpen()) and executes the
 * session (via CInput::session()).
 *
 * If an errors while reading from the input device AND that device is
 * configured to use a USB connection, then this function will wait for
 * the USB device to be re-connected.
 *
 * @param arg.  The CInput 'this' pointer cast to a void*.
 * @return This function normally does not return but may return NULL
 *   on error conditions.
 */

FAR void *CInput::listener(FAR void *arg)
{
  CInput *This = (CInput *)arg;

  ginfo("Listener started\n");

#if defined(CONFIG_TWM4NX_KEYBOARD_USBHOST) || defined(CONFIG_TWM4NX_MOUSE_USBHOST)
  // Indicate that we have successfully started.  We might be stuck waiting
  // for a USB keyboard to be connected, but we are technically running

  This->m_state = LISTENER_RUNNING;
  sem_post(&This->m_waitSem);

  // Loop until we are told to quit

  while (This->m_state == LISTENER_RUNNING)
#endif
    {
      // Open/Re-open the keyboard device

      This->m_kbdFd = This->keyboardOpen();
      if (This->m_kbdFd < 0)
        {
          gerr("ERROR: open failed: %d\n", This->m_kbdFd);
          This->m_state = LISTENER_FAILED;
          sem_post(&This->m_waitSem);
          return (FAR void *)0;
        }

      // Open/Re-open the mouse device

      This->m_mouseFd = This->mouseOpen();
      if (This->m_mouseFd < 0)
        {
          gerr("ERROR: open failed: %d\n", This->m_mouseFd);
          This->m_state = LISTENER_FAILED;
          sem_post(&This->m_waitSem);
          return (FAR void *)0;
        }

#if !defined(CONFIG_TWM4NX_KEYBOARD_USBHOST) && !defined(CONFIG_TWM4NX_MOUSE_USBHOST)
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
#if defined(CONFIG_TWM4NX_KEYBOARD_USBHOST) || defined(CONFIG_TWM4NX_MOUSE_USBHOST)
      if (ret < 0)
        {
          ferr("ERROR: CInput::session() returned %d\n", ret);
        }
#else
      // No errors from session() are expected

      DEBUGASSERT(ret == OK);
      UNUSED(ret);
#endif

      // Close the keyboard device

      (void)std::close(This->m_kbdFd);
      This->m_kbdFd = -1;

      // Close the mouse device

      (void)std::close(This->m_mouseFd);
      This->m_mouseFd = -1;
    }

  // We should get here only if we were asked to terminate via
  // m_state = LISTENER_STOPREQUESTED (or perhaps if some irrecoverable
  // error has occurred).

  ginfo("Listener exiting\n");
  This->m_state = LISTENER_TERMINATED;
  return (FAR void *)0;
}
