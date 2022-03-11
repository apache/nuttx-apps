/********************************************************************************************
 * apps/graphics/nxwm/src/ctouchscreen.cxx
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
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <cinttypes>
#include <cunistd>
#include <cerrno>
#include <cfcntl>

#include <sys/prctl.h>

#include <sched.h>
#include <pthread.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"

#include "graphics/nxwm/nxwmconfig.hxx"
#include "graphics/nxglyphs.hxx"
#include "graphics/nxwm/ctouchscreen.hxx"

/********************************************************************************************
 * CTouchscreen Method Implementations
 ********************************************************************************************/

using namespace NxWM;

/**
 * CTouchscreen Constructor
 *
 * @param server. An instance of the NX server.  This will be needed for
 *   injecting touchscreen data.
 * @param windowSize.  The size of the physical window in pixels.  This
 *   is needed for touchscreen scaling.
 */

CTouchscreen::CTouchscreen(NXWidgets::CNxServer *server, struct nxgl_size_s *windowSize)
{
  m_server      = server;              // Save the NX server
  m_touchFd     = -1;                  // Device driver is not opened
  m_state       = LISTENER_NOTRUNNING; // The listener thread is not running yet
  m_enabled     = false;               // Normal forwarding is not enabled
  m_capture     = false;               // There is no thread waiting for touchscreen data
  m_calibrated  = false;               // We have no calibration data

  // Save the window size

  m_windowSize = *windowSize;

  // Use the default touch data buffer

  m_touch       = &m_sample;

  // Initialize the m_waitSem semaphore so that any waits for data will block

  sem_init(&m_waitSem, 0, 0);
}

/**
 * CTouchscreen Destructor
 */

CTouchscreen::~CTouchscreen(void)
{
  // Stop the listener thread

  m_state = LISTENER_STOPREQUESTED;

  // Wake up the listener thread so that it will use our buffer
  // to receive data
  // REVISIT:  Need wait here for the listener thread to terminate

  pthread_kill(m_thread, CONFIG_NXWM_TOUCHSCREEN_SIGNO);

  // Close the touchscreen device (or should these be done when the thread exits?)

  if (m_touchFd >= 0)
    {
      std::close(m_touchFd);
    }

   // Destroy the semaphores that we created.

   sem_destroy(&m_waitSem);
}

/**
 * Start the touchscreen listener thread.
 *
 * @return True if the touchscreen listener thread was correctly started.
 */

bool CTouchscreen::start(void)
{
  pthread_attr_t attr;

  _info("Starting listener\n");

  // Start a separate thread to listen for touchscreen events

  pthread_attr_init(&attr);

  struct sched_param param;
  param.sched_priority = CONFIG_NXWM_TOUCHSCREEN_LISTENERPRIO;
  pthread_attr_setschedparam(&attr, &param);

  pthread_attr_setstacksize(&attr, CONFIG_NXWM_TOUCHSCREEN_LISTENERSTACK);

  m_state  = LISTENER_STARTED; // The listener thread has been started, but is not yet running

  int ret = pthread_create(&m_thread, &attr, listener, (FAR void *)this);
  if (ret != 0)
    {
      ginfo("CTouchscreen::start: pthread_create failed: %d\n", ret);
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

  _info("Listener m_state=%d\n", (int)m_state);
  return m_state == LISTENER_RUNNING;
}

/**
 * Provide touchscreen calibration data.  If calibration data is received (and
 * the touchscreen is enabled), then received touchscreen data will be scaled
 * using the calibration data and forward to the NX layer which dispatches the
 * touchscreen events in window-relative positions to the correct NX window.
 *
 * @param data.  A reference to the touchscreen data.
 */

void CTouchscreen::setCalibrationData(const struct SCalibrationData &caldata)
{
  // Save a copy of the calibration data

  m_calibData = caldata;

  // Note that we have calibration data.  Data will now be scaled and forwarded
  // to NX (unless we are still in cpature mode)

   m_calibrated = true;

  // Wake up the listener thread so that it will use our buffer
  // to receive data

  pthread_kill(m_thread, CONFIG_NXWM_TOUCHSCREEN_SIGNO);
}

/**
 * Capture raw driver data.  This method will capture mode one raw touchscreen
 * input.  The normal use of this method is for touchscreen calibration.
 *
 * This function is not re-entrant:  There may be only one thread waiting for
 * raw touchscreen data.
 *
 * @return True if the raw touchscreen data was successfully obtained
 */

bool CTouchscreen::waitRawTouchData(struct touch_sample_s *touch)
{
  _info("Capturing touch input\n");

  // Setup to cpature raw data into the user provided buffer

  sched_lock();
  m_touch   = touch;
  m_capture = true;

  // Wake up the listener thread so that it will use our buffer
  // to receive data

  pthread_kill(m_thread, CONFIG_NXWM_TOUCHSCREEN_SIGNO);

  // And wait for touch data

  int ret = OK;
  while (m_capture)
    {
      ret = sem_wait(&m_waitSem);
      DEBUGASSERT(ret == 0 || errno == EINTR);
    }
  sched_unlock();

  // And return success.  The listener thread will have (1) reset both
  // m_touch and m_capture and (2) posted m_waitSem

  _info("Returning touch input: %d\n", ret);
  return ret == OK;
}

/**
 * The touchscreen listener thread.  This is the entry point of a thread that
 * listeners for and dispatches touchscreen events to the NX server.
 *
 * @param arg.  The CTouchscreen 'this' pointer cast to a void*.
 * @return This function normally does not return but may return NULL on
 *   error conditions.
 */

FAR void *CTouchscreen::listener(FAR void *arg)
{
  CTouchscreen *This = (CTouchscreen *)arg;

#if CONFIG_TASK_NAME_SIZE > 0
  prctl(PR_SET_NAME, "CTouchScreen::listener", 0);
#endif

  _info("Listener started\n");

  // Open the touchscreen device that we just created.

  This->m_touchFd = std::open(CONFIG_NXWM_TOUCHSCREEN_DEVPATH, O_RDONLY);
  if (This->m_touchFd < 0)
    {
      gerr("ERROR Failed to open %s for reading: %d\n",
           CONFIG_NXWM_TOUCHSCREEN_DEVPATH, errno);
      This->m_state = LISTENER_FAILED;
      sem_post(&This->m_waitSem);
      return (FAR void *)0;
    }

  // Indicate that we have successfully initialized

  This->m_state = LISTENER_RUNNING;
  sem_post(&This->m_waitSem);

  // Now loop, reading and dispatching touchscreen data

  while (This->m_state == LISTENER_RUNNING)
    {
      // We may be running in one of three states
      //
      // 1. Disabled or no calibration data:  In this case, just wait for a signal
      //    indicating that the state has changed.
      // 2. Performing calibration and reporting raw touchscreen data
      // 3. Normal operation, reading touchscreen data and forwarding it to NX

      // Check if we need to collect touchscreen data.  That is, that we are enabled,
      // AND have calibration data OR if we need to collect data for the calibration
      // process.

      while ((!This->m_enabled || !This->m_calibrated) && !This->m_capture)
        {
          // No.. just sleep.  This sleep will be awakened by a signal if there
          // is anything for this thread to do

          sleep(1);

          // We woke up here either because the one second elapsed or because we
          // were signalled.  In either case we need to check the conditions and
          // determine what to do next.
        }

      // We are going to collect a sample..
      //
      // The sample pointer can change dynamically let's sample it once
      // and stick with that pointer.

      struct touch_sample_s *sample = This->m_touch;

      // Read one touchscreen sample

      _info("Listening for sample %p\n", sample);
      DEBUGASSERT(sample);
      ssize_t nbytes = read(This->m_touchFd, sample,
                            sizeof(struct touch_sample_s));

      // Check for errors

      if (nbytes < 0)
        {
          // The only expect error is to be interrupt by a signal
#if defined(CONFIG_DEBUG_GRAPHICS_ERROR) || defined(CONFIG_DEBUG_ASSERTIONS)
          int errval = errno;

          gerr("ERROR: read %s failed: %d\n",
              CONFIG_NXWM_TOUCHSCREEN_DEVPATH, errval);
          DEBUGASSERT(errval == EINTR);
#endif
        }

      // On a truly successful read, the size of the returned data will
      // be greater than or equal to size of one touchscreen sample.  It
      // be greater only in the case of a multi-touch touchscreen device
      // when multi-touches are reported.

      else if (nbytes >= (ssize_t)sizeof(struct touch_sample_s))
        {
          // Looks like good touchscreen input... process it

          This->handleMouseInput(sample);
        }
      else
        {
          gerr("ERROR Unexpected read size=%d, expected=%d\n",
              nbytes, sizeof(struct touch_sample_s));
        }
    }

  // We should get here only if we were asked to terminate via
  // m_state = LISTENER_STOPREQUESTED

  _info("Listener exiting\n");
  This->m_state = LISTENER_TERMINATED;
  return (FAR void *)0;
}

/**
 *  Inject touchscreen data into NX as mouse input
 */

void CTouchscreen::handleMouseInput(struct touch_sample_s *sample)
{
  _info("Touch id: %d flags: %02x x: %d y: %d h: %d w: %d pressure: %d\n",
       sample->point[0].id, sample->point[0].flags, sample->point[0].x,
       sample->point[0].y,  sample->point[0].h,     sample->point[0].w,
       sample->point[0].pressure);

  // Verify the touchscreen data

  if (sample->npoints < 1 ||
      ((sample->point[0].flags & TOUCH_POS_VALID) == 0 &&
       (sample->point[0].flags & TOUCH_UP) == 0))
    {
      // The pen is (probably) down, but we have do not have valid
      // X/Y position data to report.  This should not happen.

      return;
    }

  // Was this data captured by some external logic? (probably the
  // touchscreen calibration logic)

  if (m_capture && sample != &m_sample)
    {
      // Yes.. let waitRawTouchData know that the data is available
      // and restore normal buffering

      m_touch   = &m_sample;
      m_capture = false;
      sem_post(&m_waitSem);
      return;
    }

  // Sanity checks.  Re-directed touch data should never reach this point.
  // After posting m_waitSem, m_touch might change asynchronously.

  DEBUGASSERT(sample == &m_sample);

  // Check if normal processing of touchscreen data is enabled.  Check if
  // we have been given calibration data.

  if (!m_enabled || !m_calibrated)
    {
      // No.. we are not yet ready to process touchscreen data (We don't
      // really every get to this condition.

      return;
    }

  // Now we will inject the touchscreen into NX as mouse input.  First
  // massage the data a little so that it behaves a little more like a
  // mouse with only a left button
  //
  // Was the button up or down?

  uint8_t buttons;
  if ((sample->point[0].flags & (TOUCH_DOWN | TOUCH_MOVE)) != 0)
    {
      buttons = NX_MOUSE_LEFTBUTTON;
    }
  else if ((sample->point[0].flags & TOUCH_UP) != 0)
    {
      buttons = NX_MOUSE_NOBUTTONS;
    }
  else
    {
      // The pen is neither up nor down. This should not happen

      return;
    }

  // Get the "raw" touch coordinates (if they are valid)

  nxgl_coord_t x;
  nxgl_coord_t y;

  if ((sample->point[0].flags & TOUCH_POS_VALID) == 0)
    {
       x = 0;
       y = 0;
    }
  else
    {
#ifdef CONFIG_NXWM_CALIBRATION_ANISOTROPIC
      // We have valid coordinates.  Get the raw touch
      // position from the sample

      float rawX = (float)sample->point[0].x;
      float rawY = (float)sample->point[0].y;

      // Create a line (varying in X) that have the same matching Y values
      // X lines:
      //
      //   x2 = slope*y1 + offset
      //
      // X value calculated on the left side for the given value of y

      float leftX  = rawY * m_calibData.left.slope + m_calibData.left.offset;

      // X value calculated on the right side for the given value of y

      float rightX = rawY * m_calibData.right.slope + m_calibData.right.offset;

      // Line of X values between (m_calibData.leftX,leftX) and (m_calibData.rightX,rightX) the
      // are possible solutions:
      //
      //  x2 = slope * x1 - offset

      struct SCalibrationLine xLine;
      xLine.slope  = (float)((int)m_calibData.rightX - (int)m_calibData.leftX) / (rightX - leftX);
      xLine.offset = (float)m_calibData.leftX - leftX * xLine.slope;

      // Create a line (varying in Y) that have the same matching X value
      // X lines:
      //
      //   y2 = slope*x1 + offset
      //
      // Y value calculated on the top side for a given value of X

      float topY = rawX * m_calibData.top.slope + m_calibData.top.offset;

      // Y value calculated on the bottom side for a give value of X

      float bottomY = rawX * m_calibData.bottom.slope + m_calibData.bottom.offset;

      // Line of Y values between (topy,m_calibData.topY) and (bottomy,m_calibData.bottomY) that
      // are possible solutions:
      //
      //  y2 = slope * y1 - offset

      struct SCalibrationLine yLine;
      yLine.slope  = (float)((int)m_calibData.bottomY - (int)m_calibData.topY) / (bottomY - topY);
      yLine.offset = (float)m_calibData.topY - topY * yLine.slope;

      // Then scale the raw x and y positions

      float scaledX = rawX * xLine.slope + xLine.offset;
      float scaledY = rawY * yLine.slope + yLine.offset;

      x = (nxgl_coord_t)scaledX;
      y = (nxgl_coord_t)scaledY;

      _info("raw: (%6.2f, %6.2f) scaled: (%6.2f, %6.2f) (%d, %d)\n",
           rawX, rawY, scaledX, scaledY, x, y);
#else
      // We have valid coordinates.  Get the raw touch
      // position from the sample

      uint32_t rawX = (uint32_t)sample->point[0].x;
      uint32_t rawY = (uint32_t)sample->point[0].y;

      // Get the fixed precision, scaled X and Y values

      b16_t scaledX = rawX * m_calibData.xSlope + m_calibData.xOffset;
      b16_t scaledY = rawY * m_calibData.ySlope + m_calibData.yOffset;

      // Get integer scaled X and Y positions and clip
      // to fix in the window

      int32_t bigX = b16toi(scaledX + b16HALF);
      int32_t bigY = b16toi(scaledY + b16HALF);

      // Clip to the display

      if (bigX < 0)
        {
          x = 0;
        }
      else if (bigX >= m_windowSize.w)
        {
          x = m_windowSize.w - 1;
        }
      else
        {
          x = (nxgl_coord_t)bigX;
        }

      if (bigY < 0)
        {
          y = 0;
        }
      else if (bigY >= m_windowSize.h)
        {
          y = m_windowSize.h - 1;
        }
      else
        {
          y = (nxgl_coord_t)bigY;
        }

      _info("raw: (%" PRId32 ", %" PRId32 ") scaled: (%d, %d)\n",
            rawX, rawY, x, y);
#endif
    }

  // Get the server handle and "inject the mouse data

  NXHANDLE handle = m_server->getServer();
  nx_mousein(handle, x, y, buttons);
}
