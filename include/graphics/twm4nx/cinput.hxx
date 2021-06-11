/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cinput.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CINPUT_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CINPUT_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/nx/nxglib.h>

#include <semaphore.h>
#include <pthread.h>
#include <fixedmath.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/input/mouse.h>

#include "graphics/nxwidgets/cnxserver.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  class CTwm4Nx;    // Forward reference

#ifdef CONFIG_TWM4NX_TOUCHSCREEN
  /**
   * Touchscreen calibration data
   */

#ifdef CONFIG_TWM4NX_TOUCHSCREEN_ANISOTROPIC
  struct SCalibrationLine
  {
    float slope;                     /**< The slope of a line */
    float offset;                    /**< The offset of a line */
  };

  struct SCalibrationData
  {
    struct SCalibrationLine left;    /**< Describes Y values along left edge */
    struct SCalibrationLine right;   /**< Describes Y values along right edge */
    struct SCalibrationLine top;     /**< Describes X values along top */
    struct SCalibrationLine bottom;  /**< Describes X values along bottom edge */
    nxgl_coord_t leftX;              /**< Left X value used in calibration */
    nxgl_coord_t rightX;             /**< Right X value used in calibration */
    nxgl_coord_t topY;               /**< Top Y value used in calibration */
    nxgl_coord_t bottomY;            /**< Bottom Y value used in calibration */
  };
#else
  struct SCalibrationData
  {
    b16_t xSlope;                    /**< X conversion: xSlope*(x) + xOffset */
    b16_t xOffset;
    b16_t ySlope;                    /**< Y conversion: ySlope*(y) + yOffset */
    b16_t yOffset;
  };
#endif
#endif // CONFIG_TWM4NX_TOUCHSCREEN

  /**
   * The CInput class provides receives raw keyboard and mouse inputs and
   * injects that input into NX which it can be properly distributed to the
   * window that has focus (i.e., the window at the top of the display
   * hierarchy, often a modal window).  In additional, the cursor is
   * controlled to track the mouse position.
   */

  class CInput
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
       * CInput state data
       */

      // Session

      CTwm4Nx                     *m_twm4nx;  /**< The Twm4Nx session */
#ifndef CONFIG_TWM4NX_NOKEYBOARD
      int                          m_kbdFd;   /**< File descriptor of the opened keyboard device */
#endif
#ifndef CONFIG_TWM4NX_NOMOUSE
      int                          m_mouseFd; /**< File descriptor of the opened mouse device */
#endif

      // Listener

      pthread_t                    m_thread;  /**< The listener thread ID */
      volatile enum EListenerState m_state;   /**< The state of the listener thread */
      sem_t                        m_waitSem; /**< Used to synchronize with the listener thread */

#ifdef CONFIG_TWM4NX_TOUCHSCREEN
      // Calibration

      bool                         m_calib;   /**< False:  Use raw, uncalibrated touches */
      struct SCalibrationData      m_calData; /**< Touchscreen calibration data */
#endif


#ifndef CONFIG_TWM4NX_NOKEYBOARD
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

      inline int keyboardOpen(void);
#endif

#ifndef CONFIG_TWM4NX_NOMOUSE
     /**
       * Open the mouse/touchscreen input device.  Not very interesting for the
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

      inline int mouseOpen(void);
#endif

#ifndef CONFIG_TWM4NX_NOKEYBOARD
      /**
       * Read data from the keyboard device and inject the keyboard data
       * into NX for proper distribution.
       *
       * @return On success, then method returns a valid file descriptor.  A
       *    negated errno value is returned if an irrecoverable error occurs.
       */

      inline int keyboardInput(void);
#endif

#ifndef CONFIG_TWM4NX_NOMOUSE
#ifdef CONFIG_TWM4NX_TOUCHSCREEN
      /**
       * Calibrate raw touchscreen input.
       *
       * @param raw The raw touch sample
       * @param scaled The location to return the scaled touch position
       * @return On success, this method returns zero (OK).  A negated errno
       *   value is returned if an irrecoverable error occurs.
       */

      int scaleTouchData(FAR const struct touch_point_s &raw,
                         FAR struct nxgl_point_s &scaled);
#endif

      /**
       * Read data from the mouse/touchscreen device.  If the input device
       * is a mouse, then update the cursor position.  And, in either case,
       * inject the mouse data into NX for proper distribution.
       *
       * @return On success, this method returns zero (OK).  A negated errno
       *   value is returned if an irrecoverable error occurs.
       */

      inline int mouseInput(void);
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

      inline int session(void);

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

      static FAR void *listener(FAR void *arg);

    public:

      /**
       * CInput Constructor
       *
       * @param twm4nx. An instance of the Twm4Nx session.
       */

      CInput(CTwm4Nx *twm4nx);

      /**
       * CInput Destructor
       */

      ~CInput(void);

#ifdef CONFIG_TWM4NX_TOUCHSCREEN
      /**
       * Before starting re-calibration, we need to disable touchscreen
       * calibration and provide raw touchscreen input.  Similarly, when
       * valid touchscreen calibration has been provided via
       * CInput::setCalibrationData(), then touchscreen processing must
       * be re-enabled via this method.
       *
       * @param enable True will enable calibration.
       */

       inline void enableCalibration(bool enable)
       {
         m_calib = enable;
       }

      /**
       * Provide touchscreen calibration data.  If calibration data is
       * received (and the touchscreen is enabled), then received
       * touchscreen data will be scaled using the calibration data and
       * forward to the NX layer which dispatches the touchscreen events
       * in window-relative positions to the correct NX window.
       *
       * @param caldata.  A reference to the touchscreen data.
       */

      inline void setCalibrationData(const struct SCalibrationData &caldata)
      {
        m_calData = caldata;
      }
#endif

      /**
       * Start the keyboard/mouse listener thread.
       *
       * @return True if the keyboard listener thread was correctly started.
       */

      bool start(void);
    };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CINPUT_HXX
