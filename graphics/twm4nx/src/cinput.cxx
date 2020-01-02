/****************************************************************************
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <cunistd>
#include <cerrno>
#include <cfcntl>

#include <sched.h>
#include <poll.h>
#include <pthread.h>
#include <assert.h>

#include <nuttx/semaphore.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/input/mouse.h>

#ifdef CONFIG_TWM4NX_MOUSE
#  include <nuttx/nx/nxcursor.h>
#  include "graphics/twm4nx/twm4nx_cursor.hxx"
#else
#  include <nuttx/input/touchscreen.h>
#endif

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cinput.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

#ifndef CONFIG_TWM4NX_NOKEYBOARD
#  define KBD_INDEX         0
#  ifndef CONFIG_TWM4NX_NOMOUSE
#    define MOUSE_INDEX     1
#    define NINPUT_DEVICES  2
#  else
#    define NINPUT_DEVICES  1
#  endif
#else
#  define MOUSE_INDEX       0
#  define NINPUT_DEVICES    1
#endif

/****************************************************************************
 * CInput Method Implementations
 ****************************************************************************/

using namespace Twm4Nx;

/**
 * CInput Constructor
 *
 * @param twm4nx. An instance of the NX server.  This will be needed for
 *   injecting keyboard data.
 */

CInput::CInput(CTwm4Nx *twm4nx)
{
  // Session

  m_twm4nx      = twm4nx;              // Save the NX server
#ifndef CONFIG_TWM4NX_NOKEYBOARD
  m_kbdFd       = -1;                  // Keyboard driver is not opened
#endif
#ifndef CONFIG_TWM4NX_NOMOUSE
  m_mouseFd     = -1;                  // Mouse/touchscreen driver is not opened
#endif

 // Listener

  m_state       = LISTENER_NOTRUNNING; // The listener thread is not running yet

  // Initialize the semaphore used to synchronize with the listener thread

  sem_init(&m_waitSem, 0, 0);
  sem_setprotocol(&m_waitSem, SEM_PRIO_NONE);

#ifdef CONFIG_TWM4NX_TOUCHSCREEN
  // Calibration

  m_calib       = false;               // Use raw touches until calibrated
#endif
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

  pthread_kill(m_thread, CONFIG_TWM4NX_INPUT_SIGNO);

#ifndef CONFIG_TWM4NX_NOKEYBOARD
  // Close the keyboard device

  if (m_kbdFd >= 0)
    {
      std::close(m_kbdFd);
    }
#endif

#ifndef CONFIG_TWM4NX_NOKEYBOARD
  // Close the mouse/touchscreen device

  if (m_mouseFd >= 0)
    {
      std::close(m_mouseFd);
    }
#endif
}

/**
 * Start the keyboard listener thread.
 *
 * @return True if the keyboard listener thread was correctly started.
 */

bool CInput::start(void)
{
  pthread_attr_t attr;

  twminfo("Starting listener\n");

  // Start a separate thread to listen for keyboard events

  pthread_attr_init(&attr);

  struct sched_param param;
  param.sched_priority = CONFIG_TWM4NX_INPUT_LISTENERPRIO;
  pthread_attr_setschedparam(&attr, &param);

  pthread_attr_setstacksize(&attr, CONFIG_TWM4NX_INPUT_LISTENERSTACK);

  m_state  = LISTENER_STARTED; // The listener thread has been started, but is not yet running

  int ret = pthread_create(&m_thread, &attr, listener, (FAR void *)this);
  if (ret != 0)
    {
      twmerr("ERROR: CInput::start: pthread_create failed: %d\n", ret);
      return false;
    }

  // Detach from the thread

  pthread_detach(m_thread);

  // Don't return until we are sure that the listener thread is running
  // (or until it reports an error).

  while (m_state == LISTENER_STARTED)
    {
      // Wait for the listener thread to wake us up when we really
      // are connected.

      sem_wait(&m_waitSem);
    }

  // Then return true only if the listener thread reported successful
  // initialization.

  twminfo("Listener m_state=%d\n", (int)m_state);
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

#ifndef CONFIG_TWM4NX_NOKEYBOARD
int CInput::keyboardOpen(void)
{
  int fd;

  // Loop until we have successfully opened the USB keyboard (or until some
  // irrecoverable error occurs).

  do
    {
      // Try to open the keyboard device non-blocking.

      fd = std::open(CONFIG_TWM4NX_KEYBOARD_DEVPATH, O_RDONLY | O_NONBLOCK);
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

                  twminfo("WAITING for a USB keyboard\n");
                  std::sleep(2);
                }

              // Anything else would be really bad.

              else
#endif
                {
                  // Let the top-level logic decide what it wants to do
                  // about all really bad things

                  twmerr("ERROR: Failed to open %s for reading: %d\n",
                         CONFIG_TWM4NX_KEYBOARD_DEVPATH, errcode);
                  return -errcode;
                }
            }
        }
    }
  while (fd < 0);

  return fd;
}
#endif

/**
 * Open the mouse/touchscreen input devices.  Not very interesting for the
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

#ifndef CONFIG_TWM4NX_NOMOUSE
inline int CInput::mouseOpen(void)
{
  int fd;

  // Loop until we have successfully opened the USB mouse (or until some
  // irrecoverable error occurs).

  do
    {
      // Try to open the mouse device non-blocking

      fd = std::open(CONFIG_TWM4NX_MOUSE_DEVPATH, O_RDONLY | O_NONBLOCK);
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

                  twminfo("WAITING for a USB mouse\n");
                  std::sleep(2);
                }

              // Anything else would be really bad.

              else
#endif
                {
                  // Let the top-level logic decide what it wants to do
                  // about all really bad things

                  twmerr("ERROR: Failed to open %s for reading: %d\n",
                         CONFIG_TWM4NX_MOUSE_DEVPATH, errcode);
                  return -errcode;
                }
            }
        }
    }
  while (fd < 0);

  return fd;
}
#endif

/**
 * Read data from the keyboard device and inject the keyboard data
 * into NX for proper distribution.
 *
 * @return On success, then method returns a valid file descriptor.  A
 *    negated errno value is returned if an irrecoverable error occurs.
 */

#ifndef CONFIG_TWM4NX_NOKEYBOARD
int CInput::keyboardInput(void)
{
  // Read one keyboard sample

  twminfo("Reading keyboard input\n");

  uint8_t rxbuffer[CONFIG_TWM4NX_KEYBOARD_BUFSIZE];
  ssize_t nbytes = read(m_kbdFd, rxbuffer,
                        CONFIG_TWM4NX_KEYBOARD_BUFSIZE);

  // Check for errors

  if (nbytes < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      // EAGAIN and EINTR are not really error.  EAGAIN just indicates that
      // there is nothing available to read now.

      if (errcode == EAGAIN)
        {
          return OK;
        }

      // EINTR it simply means that something is trying to get our attention
      // We need to check m_state to see if we were asked to terminate.
      // Anything else is bad news

      if (errcode != EINTR)
        {
          // Let the top-level listener logic decide what to do about
          // the read failure.

          twmerr("ERROR: read %s failed: %d\n",
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

      NXHANDLE server = m_twm4nx->getServer();
      int ret = nx_kbdin(server, (uint8_t)nbytes, rxbuffer);
      if (ret < 0)
        {
          twmerr("ERROR: nx_kbdin failed: %d\n", ret);

          // Ignore the error
        }
    }

  return OK;
}
#endif

/**
 * Calibrate raw touchscreen input.
 *
 * @param raw The raw touch sample
 * @param scaled The location to return the scaled touch position
 * @return On success, this method returns zero (OK).  A negated errno
 *   value is returned if an irrecoverable error occurs.
 */

#ifdef CONFIG_TWM4NX_TOUCHSCREEN
int CInput::scaleTouchData(FAR const struct touch_point_s &raw,
                           FAR struct nxgl_point_s &scaled)
{
#ifdef CONFIG_NXWM_CALIBRATION_ANISOTROPIC
  // Create a line (varying in X) that have the same matching Y values
  // X lines:
  //
  //   x2 = slope*y1 + offset
  //
  // X value calculated on the left side for the given value of y

  float leftX  = raw.y * m_calData.left.slope + m_calData.left.offset;

  // X value calculated on the right side for the given value of y

  float rightX = raw.y * m_calData.right.slope + m_calData.right.offset;

  // Line of X values between (m_calData.leftX,leftX) and
  // (m_calData.rightX,rightX) the are possible solutions:
  //
  //  x2 = slope * x1 - offset

  struct SCalibrationLine xLine;
  xLine.slope  = (float)((int)m_calData.rightX -
                         (int)m_calData.leftX) / (rightX - leftX);
  xLine.offset = (float)m_calData.leftX - leftX * xLine.slope;

  // Create a line (varying in Y) that have the same matching X value
  // X lines:
  //
  //   y2 = slope*x1 + offset
  //
  // Y value calculated on the top side for a given value of X

  float topY = raw.x * m_calData.top.slope + m_calData.top.offset;

  // Y value calculated on the bottom side for a give value of X

  float bottomY = raw.x * m_calData.bottom.slope +
                  m_calData.bottom.offset;

  // Line of Y values between (topy,m_calData.topY) and
  // (bottomy,m_calData.bottomY) that are possible solutions:
  //
  //  y2 = slope * y1 - offset

  struct SCalibrationLine yLine;
  yLine.slope  = (float)((int)m_calData.bottomY -
                         (int)m_calData.topY) / (bottomY - topY);
  yLine.offset = (float)m_calData.topY - topY * yLine.slope;

  // Then scale the raw x and y positions

  float scaledX = raw.x * xLine.slope + xLine.offset;
  float scaledY = raw.y * yLine.slope + yLine.offset;

  scaled.x = (nxgl_coord_t)scaledX;
  scaled.y = (nxgl_coord_t)scaledY;

  twminfo("raw: (%6.2f, %6.2f) scaled: (%6.2f, %6.2f) (%d, %d)\n",
          raw.x, raw.y, scaledX, scaledY, scaled.x, scaled.y);
  return OK;
#else
  // Get the fixed precision, scaled X and Y values

  b16_t scaledX = raw.x * m_calData.xSlope + m_calData.xOffset;
  b16_t scaledY = raw.y * m_calData.ySlope + m_calData.yOffset;

  // Get integer scaled X and Y positions and clip
  // to fix in the window

  int32_t bigX = b16toi(scaledX + b16HALF);
  int32_t bigY = b16toi(scaledY + b16HALF);

  // Clip to the display

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);


  if (bigX < 0)
    {
      scaled.x = 0;
    }
  else if (bigX >= displaySize.w)
    {
      scaled.x = displaySize.w - 1;
    }
  else
    {
      scaled.x = (nxgl_coord_t)bigX;
    }

  if (bigY < 0)
    {
      scaled.y = 0;
    }
  else if (bigY >= displaySize.h)
    {
      scaled.y = displaySize.h - 1;
    }
  else
    {
      scaled.y = (nxgl_coord_t)bigY;
    }

  twminfo("raw: (%d, %d) scaled: (%d, %d)\n",
          raw.x, raw.y, scaled.x, scaled.y);
  return OK;
#endif
}
#endif

/**
 * Read data from the mouse/touchscreen device.  If the input device is a
 * mouse, then update the cursor position.  And, in either case, inject
 * the mouse data into NX for proper distribution.
 *
 * @return On success, then method returns zero (OK).  A negated errno
 *   value is returned if an irrecoverable error occurs.
 */

#ifndef CONFIG_TWM4NX_NOMOUSE
int CInput::mouseInput(void)
{
  // Read one mouse sample

  twminfo("Reading XY input\n");

  uint8_t rxbuffer[CONFIG_TWM4NX_MOUSE_BUFSIZE];
  ssize_t nbytes = read(m_mouseFd, rxbuffer,
                        CONFIG_TWM4NX_MOUSE_BUFSIZE);

  // Check for errors

  if (nbytes < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      // EAGAIN and EINTR are not really error.  EAGAIN just indicates that
      // there is nothing available to read now.

      if (errcode == EAGAIN)
        {
          return OK;
        }

      // EINTR it simply means that something is trying to get our attention
      // We need to check m_state to see if we were asked to terminate.
      // Anything else is bad news

      if (errcode != EINTR)
        {
          // Let the top-level listener logic decide what to do about
          // the read failure.

          twmerr("ERROR: read %s failed: %d\n",
                 CONFIG_TWM4NX_KEYBOARD_DEVPATH, errcode);
          return -errcode;
        }

      fwarn("WARNING: Awakened with EINTR\n");
    }

#ifdef CONFIG_TWM4NX_MOUSE
  // On a truly successful read, the size of the returned data will
  // be greater than or equal to size of one mouse report.

  else if (nbytes < (ssize_t)sizeof(struct mouse_report_s))
    {
      twmerr("ERROR Unexpected read size=%d, expected=%d\n",
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

      twminfo("Mouse pos: (%d,%d) Buttons: %02x\n",
              rtp->x, rpt->y, rpt->buttons);

      struct nxgl_point_s pos =
      {
        .x = rpt->x,
        .y = rpt->y
      };

      int ret = nxcursor_setposition(m_twm4nx, &pos);
      if (ret < 0)
        {
          twmerr("ERROR: nxcursor_setposition failed: %d\n", ret);

          // Ignore the error
        }

      // Then inject the mouse input into NX

      NXHANDLE server = m_twm4nx->getServer();
      ret = nx_mousein(server, rpt->x, rpt->y, rpt->buttons);
      if (ret < 0)
        {
          twmerr("ERROR: nx_mousein failed: %d\n", ret);

          // Ignore the error
        }
    }
#else
  // On a truly successful read, the size of the returned data will
  // be greater than or equal to size of one touchscreen sample.  It
  // be greater only in the case of a multi-touch touchscreen device
  // when multi-touches are reported.

  else if (nbytes < (ssize_t)SIZEOF_TOUCH_SAMPLE_S(1))
    {
      twmerr("ERROR Unexpected read size=%d, expected=%d\n",
             nbytes, SIZEOF_TOUCH_SAMPLE_S(1));
      return -EIO;
    }
  else
    {
      // Looks like good touchscreen input... process it.
      // NOTE: m_twm4nx inherits from NXWidgets::CNXServer so we all ready
      // have the server instance.

      // Inject the touchscreen as mouse input into NX (with the left
      // button pressed)

      FAR struct touch_sample_s *sample =
        (FAR struct touch_sample_s *)rxbuffer;

      // Have we received calibration data yet?

      struct nxgl_point_s touchPos;
      int ret;

      if (m_calib)
        {
          // Yes.. scale the raw mouse input to fit the display

          ret = scaleTouchData(sample->point[0], touchPos);
          if (ret < 0)
            {
              twmerr("ERROR: scaleTouchData failed: %d\n", ret);
              return ret;
            }
        }
      else
        {
          // Hmm.. It is probably an error if no calibration data is
          // provided.  However, it is also possible that the touch data as
          // received does not require calibration.  We will just have to
          // trust that the user knows what they are doing.

          touchPos.x = sample->point[0].x;
          touchPos.y = sample->point[0].y;
        }

      // Now we will inject the touchscreen into NX as mouse input.  First
      // massage the data a little so that it behaves a little more like a
      // mouse with only a left button
      //
      // Was the touch up or down?

      uint8_t buttons;
      if ((sample->point[0].flags & (TOUCH_DOWN | TOUCH_MOVE)) != 0)
        {
          buttons = MOUSE_BUTTON_1;
        }
      else if ((sample->point[0].flags & TOUCH_UP) != 0)
        {
          buttons = 0;
        }
      else
        {
          // The touch is neither up nor down. This should not happen

          return -EIO;
       }

      twminfo("Touch pos: (%d,%d)->(%d,%d) Buttons: %02x\n",
              sample->point[0].x, sample->point[0].y,
              touchPos.x, touchPos.y, buttons);

      NXHANDLE server = m_twm4nx->getServer();
      ret = nx_mousein(server, touchPos.x, touchPos.y, buttons);
      if (ret < 0)
        {
          twmerr("ERROR: nx_mousein failed: %d\n", ret);

          // Ignore the error
        }
    }
#endif

  return OK;
}
#endif

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
  twminfo("Session started\n");

#ifdef CONFIG_TWM4NX_MOUSE
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
#endif

  // Loop, reading and dispatching keyboard data

  int ret = OK;
  while (m_state == LISTENER_RUNNING)
    {
      // Wait for data availability

      struct pollfd pfd[NINPUT_DEVICES];
#ifndef CONFIG_TWM4NX_NOKEYBOARD
      pfd[KBD_INDEX].fd        = m_kbdFd;
      pfd[KBD_INDEX].events    = POLLIN | POLLERR | POLLHUP;
      pfd[KBD_INDEX].revents   = 0;
#endif
#ifndef CONFIG_TWM4NX_NOMOUSE
      pfd[MOUSE_INDEX].fd      = m_mouseFd;
      pfd[MOUSE_INDEX].events  = POLLIN | POLLERR | POLLHUP;
      pfd[MOUSE_INDEX].revents = 0;
#endif

      ret = poll(pfd, NINPUT_DEVICES, -1);
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
              twmerr("ERROR: poll() failed");
              break;
            }
        }

#ifndef CONFIG_TWM4NX_NOKEYBOARD
      // Check for keyboard input

      if ((pfd[KBD_INDEX].revents & (POLLERR | POLLHUP)) != 0)
        {
          twmerr("ERROR: keyboard poll() failed. revents=%04x\n",
                 pfd[KBD_INDEX].revents);
          ret = -EIO;
          break;
        }

      if ((pfd[KBD_INDEX].revents & POLLIN) != 0)
        {
          ret = keyboardInput();
          if (ret < 0)
            {
              twmerr("ERROR: keyboardInput() failed: %d\n", ret);
              break;
            }
        }
#endif

#ifndef CONFIG_TWM4NX_NOMOUSE
      // Check for mouse input

      if ((pfd[MOUSE_INDEX].revents & (POLLERR | POLLHUP)) != 0)
        {
          twmerr("ERROR: Mouse poll() failed. revents=%04x\n",
                 pfd[MOUSE_INDEX].revents);
          ret = -EIO;
          break;
        }

      if ((pfd[MOUSE_INDEX].revents & POLLIN) != 0)
        {
          ret = mouseInput();
          if (ret < 0)
            {
              twmerr("ERROR: mouseInput() failed: %d\n", ret);
              break;
            }
        }
#endif
    }

#ifdef CONFIG_TWM4NX_MOUSE
  // Disable the cursor

  m_twm4nx->enableCursor(false);
#endif
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

  twminfo("Listener started\n");

#if defined(CONFIG_TWM4NX_KEYBOARD_USBHOST) || defined(CONFIG_TWM4NX_MOUSE_USBHOST)
  // Indicate that we have successfully started.  We might be stuck waiting
  // for a USB keyboard to be connected, but we are technically running

  This->m_state = LISTENER_RUNNING;
  sem_post(&This->m_waitSem);

  // Loop until we are told to quit

  while (This->m_state == LISTENER_RUNNING)
#endif
    {
#ifndef CONFIG_TWM4NX_NOKEYBOARD
      // Open/Re-open the keyboard device

      This->m_kbdFd = This->keyboardOpen();
      if (This->m_kbdFd < 0)
        {
          twmerr("ERROR: open failed: %d\n", This->m_kbdFd);
          This->m_state = LISTENER_FAILED;
          sem_post(&This->m_waitSem);
          return (FAR void *)0;
        }
#endif

#ifndef CONFIG_TWM4NX_NOMOUSE
      // Open/Re-open the mouse device

      This->m_mouseFd = This->mouseOpen();
      if (This->m_mouseFd < 0)
        {
          twmerr("ERROR: open failed: %d\n", This->m_mouseFd);
          This->m_state = LISTENER_FAILED;
          sem_post(&This->m_waitSem);
          return (FAR void *)0;
        }
#endif

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

#ifndef CONFIG_TWM4NX_NOKEYBOARD
      // Close the keyboard device

      std::close(This->m_kbdFd);
      This->m_kbdFd = -1;
#endif

#ifndef CONFIG_TWM4NX_NOMOUSE
      // Close the mouse device

      std::close(This->m_mouseFd);
      This->m_mouseFd = -1;
#endif
    }

  // We should get here only if we were asked to terminate via
  // m_state = LISTENER_STOPREQUESTED (or perhaps if some irrecoverable
  // error has occurred).

  twminfo("Listener exiting\n");
  This->m_state = LISTENER_TERMINATED;
  return (FAR void *)0;
}
