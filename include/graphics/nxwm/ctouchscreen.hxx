/****************************************************************************
 * apps/include/graphics/nxwm/ctouchscreen.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_CTOUCHSCREEN_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_CTOUCHSCREEN_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/nx/nxglib.h>

#include <semaphore.h>
#include <pthread.h>

#include <nuttx/input/touchscreen.h>

#include "graphics/nxwidgets/cnxserver.hxx"
#include "graphics/nxwm/ccalibration.hxx"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

namespace NxWM
{
  /**
   * The CTouchscreen class provides the the calibration window and obtains
   * calibration data.
   */

  class CTouchscreen
  {
  private:
    /**
     * The state of the listener thread.
     */

    enum EListenerState
    {
      LISTENER_NOTRUNNING = 0,  /**< The listener thread has not yet been started */
      LISTENER_STARTED,         /**< The listener thread has been started, but is not yet running */
      LISTENER_RUNNING,         /**< The listener thread is running normally */
      LISTENER_STOPREQUESTED,   /**< The listener thread has been requested to stop */
      LISTENER_TERMINATED,      /**< The listener thread terminated normally */
      LISTENER_FAILED           /**< The listener thread terminated abnormally */
    };

    /**
     * CTouchscreen state data
     */

    NXWidgets::CNxServer        *m_server;     /**< The current NX server */
    int                          m_touchFd;    /**< File descriptor of the opened touchscreen device */
    sem_t                        m_waitSem;    /**< Semaphore the supports waits for touchscreen data */
    pthread_t                    m_thread;     /**< The listener thread ID */
    volatile enum EListenerState m_state;      /**< The state of the listener thread */
    volatile bool                m_enabled;    /**< True: Normal touchscreen processing */
    volatile bool                m_capture;    /**< True: There is a thread waiting for raw touch data */
    volatile bool                m_calibrated; /**< True: If have calibration data */
    struct nxgl_size_s           m_windowSize; /**< The size of the physical display */
    struct SCalibrationData      m_calibData;  /**< Calibration data */
    struct touch_sample_s        m_sample;     /**< In normal mode, touch data is collected here */
    struct touch_sample_s       *m_touch;      /**< Points to the current touch data buffer */

    /**
     * The touchscreen listener thread.  This is the entry point of a thread that
     * listeners for and dispatches touchscreens events to the NX server.
     *
     * @param arg.  The CTouchscreen 'this' pointer cast to a void*.
     * @return This function normally does not return but may return NULL on
     *   error conditions.
     */

    static FAR void *listener(FAR void *arg);

    /**
     *  Inject touchscreen data into NX as mouse input
     *
     * @param sample.  The buffer where data was collected.
     */

    void handleMouseInput(struct touch_sample_s *sample);

  public:

    /**
     * CTouchscreen Constructor
     *
     * @param server. An instance of the NX server.  This will be needed for
     *   injecting mouse data.
     * @param windowSize.  The size of the physical window in pixels.  This
     *   is needed for touchscreen scaling.
     */

    CTouchscreen(NXWidgets::CNxServer *server, struct nxgl_size_s *windowSize);

    /**
     * CTouchscreen Destructor
     */

    ~CTouchscreen(void);

    /**
     * Start the touchscreen listener thread.
     *
     * @return True if the touchscreen listener thread was correctly started.
     */

    bool start(void);

    /**
     * Enable/disable touchscreen data processing.  When enabled, touchscreen events
     * are calibrated and forwarded to the NX layer which dispatches the touchscreen
     * events in window-relative positions to the correct NX window.
     *
     * When disabled, touchscreen data is not forwarded to NX, but is instead captured
     * and made available for touchscreen calibration.  The touchscreen driver is
     * initially disabled and must be specifically enabled be begin normal processing.
     * Normal processing also requires calibration data (see method setCalibrationData)
     *
     * @param capture.  True enables capture mode; false disables.
     */

    inline void setEnabled(bool enable)
    {
      // Set the capture flag.  m_calibrated must also be set to get to normal
      // mode where touchscreen data is forwarded to NX.

      m_enabled = enable;
    }

    /**
     * Is the touchscreen calibrated?
     *
     * @return True if the touchscreen has been calibrated.
     */

    inline bool isCalibrated(void) const
    {
      return m_calibrated;
    }

    /**
     * Provide touchscreen calibration data.  If calibration data is received (and
     * the touchscreen is enabled), then received touchscreen data will be scaled
     * using the calibration data and forward to the NX layer which dispatches the
     * touchscreen events in window-relative positions to the correct NX window.
     *
     * @param caldata.  A reference to the touchscreen data.
     */

    void setCalibrationData(const struct SCalibrationData &caldata);

    /**
     * Recover the calibration data so that it can be saved to non-volatile storage.
     *
     * @param data.  A reference to the touchscreen data.
     * @return True if calibration data was successfully returned.
     */

    inline bool getCalibrationData(struct SCalibrationData &caldata) const
    {
      if (m_calibrated)
        {
          caldata = m_calibData;
        }

      return m_calibrated;
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

    bool waitRawTouchData(struct touch_sample_s *touch);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_CTOUCHSCREEN_HXX
