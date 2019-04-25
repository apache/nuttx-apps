/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cinput.hxx
// Keyboard injection
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CINPUT_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CINPUT_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/nx/nxglib.h>

#include <semaphore.h>
#include <pthread.h>

#include <nuttx/input/mouse.h>

#include "graphics/nxwidgets/cnxserver.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  class CTwm4Nx;    // Forward reference

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

      CTwm4Nx                     *m_twm4nx;  /**< The Twm4Nx session */
      int                          m_kbdFd;   /**< File descriptor of the opened keyboard device */
      int                          m_mouseFd; /**< File descriptor of the opened mouse device */
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
       * Opens the keyboard device specified by CONFIG_TWM4NX_KEYBOARD_DEVPATH.
       *
       * @return On success, then method returns a valid file descriptor.  A
       *    negated errno value is returned if an irrecoverable error occurs.
       */

      inline int keyboardOpen(void);

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

      inline int mouseOpen(void);

      /**
       * Read data from the keyboard device and inject the keyboard data
       * into NX for proper distribution.
       *
       * @return On success, then method returns a valid file descriptor.  A
       *    negated errno value is returned if an irrecoverable error occurs.
       */

      inline int keyboardInput(void);

      /**
       * Read data from the mouse device, update the cursor position, and
       * inject the mouse data into NX for proper distribution.
       *
       * @return On success, then method returns a valid file descriptor.  A
       *    negated errno value is returned if an irrecoverable error occurs.
       */

      inline int mouseInput(void);

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

      /**
       * Start the keyboard listener thread.
       *
       * @return True if the keyboard listener thread was correctly started.
       */

      bool start(void);
    };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CINPUT_HXX
