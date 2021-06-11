/****************************************************************************
 * apps/include/graphics/nxwm/keyboard.hxx
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

#ifndef __NXWM_INCLUDE_CKEYBOARD_HXX
#define __NXWM_INCLUDE_CKEYBOARD_HXX

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
   * The CKeyboard class provides the the calibration window and obtains
   * calibration data.
   */

  class CKeyboard
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
     * CKeyboard state data
     */

    NXWidgets::CNxServer        *m_server;  /**< The current NX server */
    int                          m_kbdFd;   /**< File descriptor of the opened keyboard device */
    pthread_t                    m_thread;  /**< The listener thread ID */
    volatile enum EListenerState m_state;   /**< The state of the listener thread */
    sem_t                        m_waitSem; /**< Used to synchronize with the listener thread */

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

    inline int open(void);

    /**
     * This is the heart of the keyboard listener thread.  It contains the
     * actual logic that listeners for and dispatches keyboard events to the
     * NX server.
     *
     * @return  If the session terminates gracefully (i.e., because >m_state
     * is no longer equal to  LISTENER_RUNNING, then method returns OK.  A
     * negated errno value is returned if an error occurs while reading from
     * the keyboard device.  A read error, depending upon the type of the
     * error, may simply indicate that a USB keyboard was removed and we
     * should wait for the keyboard to be connected.
     */

    inline int session(void);

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

    static FAR void *listener(FAR void *arg);

  public:

    /**
     * CKeyboard Constructor
     *
     * @param server. An instance of the NX server.  This will be needed for
     *   injecting mouse data.
     */

    CKeyboard(NXWidgets::CNxServer *server);

    /**
     * CKeyboard Destructor
     */

    ~CKeyboard(void);

    /**
     * Start the keyboard listener thread.
     *
     * @return True if the keyboard listener thread was correctly started.
     */

    bool start(void);
  };
}

#endif // __NXWM_INCLUDE_CKEYBOARD_HXX
