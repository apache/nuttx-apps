/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/ccalibration.hxx
// Perform Touchscreen Calibration
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CCALIBRATION_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CCALIBRATION_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/nx/nxglib.h>

#include <pthread.h>
#include <semaphore.h>
#include <fixedmath.h>

#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/clabel.hxx"
#include "graphics/nxwidgets/cnxfont.hxx"

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/iapplication.hxx"
#include "graphics/twm4nx/cinput.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

// CNxTerm application events
// Window Events

#define EVENT_CALIB_REDRAW      EVENT_SYSTEM_NOP
#define EVENT_CALIB_RESIZE      EVENT_SYSTEM_NOP
#define EVENT_CALIB_XYINPUT     (EVENT_RECIPIENT_APP | 0x0000)
#define EVENT_CALIB_KBDINPUT    EVENT_SYSTEM_NOP
#define EVENT_CALIB_DELETE      (EVENT_RECIPIENT_APP | 0x0001)

// Button events (there are no buttons)

#define EVENT_CALIB_CLOSE       EVENT_SYSTEM_NOP

// Menu Events

#define EVENT_CALIB_START       (EVENT_RECIPIENT_APP | 0x0002)

// Calibration indices

#define CALIB_UPPER_LEFT_INDEX  0
#define CALIB_UPPER_RIGHT_INDEX 1
#define CALIB_LOWER_RIGHT_INDEX 2
#define CALIB_LOWER_LEFT_INDEX  3

#define CALIB_DATA_POINTS       4

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  class CTwm4Nx;                                   // Forward reference

  /**
   * The CCalibration class provides the the calibration window and obtains
   * calibration data.
   */

  class CCalibration : public CTwm4NxEvent
  {
    private:
      // The state of the calibration thread.

      enum ECalThreadState
      {
        CALTHREAD_NOTRUNNING = 0,                  /**< The calibration thread has not yet been started */
        CALTHREAD_STARTED,                         /**< The calibration thread has been started, but is not yet running */
        CALTHREAD_RUNNING,                         /**< The calibration thread is running normally */
        CALTHREAD_STOPREQUESTED,                   /**< The calibration thread has been requested to stop */
        CALTHREAD_TERMINATED                       /**< The calibration thread terminated normally */
      };

      // Identifies the current display state

      enum ECalibrationPhase
      {
        CALPHASE_NOT_STARTED = 0,                  /**< Constructed, but not yet started */
        CALPHASE_UPPER_LEFT,                       /**< Touch point is in the upper left corner */
        CALPHASE_UPPER_RIGHT,                      /**< Touch point is in the upper right corner */
        CALPHASE_LOWER_RIGHT,                      /**< Touch point is in the lower left corner */
        CALPHASE_LOWER_LEFT,                       /**< Touch point is in the lower right corner */
        CALPHASE_COMPLETE                          /**< Calibration is complete */
      };

      // Describes one touchscreen sample.  CInput treats mouse and touchscreen
      // input the same so this reflects the mousey nature of the object.

      struct STouchSample
      {
        bool valid;                                /**< True: Sample is valid */
        uint8_t buttons;                           /**< Left button reflects touch state */
        struct nxgl_point_s pos;                   /**< True: Touch position */
      };

      // Characterizes one calibration screen

      struct SCalibScreenInfo
      {
        struct nxgl_point_s      pos;              /**< The position of the touch point */
        nxgl_mxpixel_t           lineColor;        /**< The color of the cross-hair lines */
        nxgl_mxpixel_t           circleFillColor;  /**< The color of the circle */
      };

      // CCalibration state data

      FAR CTwm4Nx               *m_twm4nx;         /**< Twm4Nx session instance */
      FAR NXWidgets::CNxWindow  *m_nxWin;          /**< The window for the calibration display */
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
      FAR NXWidgets::CLabel     *m_text;           /**< Calibration message */
      FAR NXWidgets::CNxFont    *m_font;           /**< The font used in the message */
#endif
      pthread_t                  m_thread;         /**< The calibration thread ID */
      sem_t                      m_synchSem;       /**< Synchronize calibration thread with events */
      sem_t                      m_exclSem;        /**< For mutually exclusive access to data */
      struct SCalibScreenInfo    m_screenInfo;     /**< Describes the current calibration display */
      struct nxgl_point_s        m_touchPos;       /**< This is the last touch position */
      volatile uint8_t           m_calthread;      /**< Current calibration display state (See ECalibThreadState)*/
      uint8_t                    m_calphase;       /**< Current calibration display state (See ECalibrationPhase)*/
      bool                       m_stop;           /**< True: We have been asked to stop the calibration */
      bool                       m_touched;        /**< True: The screen is touched */
      struct STouchSample        m_sample;         /**< Catches new touch samples */
#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
      uint8_t                    m_nsamples;       /**< Number of samples collected so far at this position */
      struct nxgl_point_s        m_sampleData[CONFIG_TWM4NX_CALIBRATION_NSAMPLES];
#endif
      struct nxgl_point_s        m_calibData[CALIB_DATA_POINTS];

      /**
       * Accept raw touchscreen input.
       *
       * @param sample Touchscreen input sample
       */

      void touchscreenInput(struct STouchSample &sample);

#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
      /**
       * Create widgets need by the calibration thread.
       *
       * @return True if the widgets were successfully created.
       */

      bool createWidgets(void);

      /**
       * Destroy widgets created for the calibration thread.
       */

      void destroyWidgets(void);
#endif

      /**
       * The calibration thread.  This is the entry point of a thread that provides the
       * calibration displays, waits for input, and collects calibration data.
       *
       * @param arg.  The CCalibration 'this' pointer cast to a void*.
       * @return This function always returns NULL when the thread exits
       */

      static FAR void *calibration(FAR void *arg);

      /**
       * Get exclusive access data shared across threads.
       *
       * @return True is returned if the sample data was obtained without error.
       */

      bool exclusiveAccess(void);

      /**
       * Wait for touchscreen next sample data to become available.
       *
       * @param sample Snapshot of last touchscreen sample
       * @return True is returned if the sample data was obtained without error.
       */

      bool waitTouchSample(FAR struct STouchSample &sample);

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
      /**
       * Accumulate and average touch sample data
       *
       * @param average.  When the averaged data is available, return it here
       * @return True: Average data is available; False: Need to collect more samples
       */

      bool averageSamples(struct nxgl_point_s &average);
#endif

      /**
       * This is the calibration state machine.  It is called initially and then
       * as new touchscreen data is received.
       */

       void stateMachine(void);

      /**
       * Presents the next calibration screen
       */

      void showCalibration(void);

      /**
       * Finish calibration steps and provide the calibration data to the
       * touchscreen driver.
       */

      void finishCalibration(void);

      /**
       * Stop the calibration thread.
       */

      void stop(void);

      /**
       * Destroy the application and free all of its resources.  This method
       * will initiate blocking of messages from the NX server.  The server
       * will flush the window message queue and reply with the blocked
       * message.  When the block message is received by CWindowMessenger,
       * it will send the destroy message to the start window task which
       * will, finally, safely delete the application.
       */

      void destroy(void);

      /**
       * Given the raw touch data collected by the calibration thread, create the
       * massaged calibration data needed by CTouchscreen.
       *
       * @param data. A reference to the location to save the calibration data
       * @return True if the calibration data was successfully created.
       */

      bool createCalibrationData(struct SCalibrationData &data);

    public:

      /**
       * CCalibration Constructor
       *
       * @param twm4nx.  The Twm4Nx session instance.
       */

      CCalibration(FAR CTwm4Nx *twm4nx);

      /**
       * CCalibration Destructor
       */

      ~CCalibration(void);

      /**
       * CCalibration Initializer.  Performs parts of the instance
       * construction that may fail.  This function creates the
       * initial calibration display.
       */

      bool initialize(void);

      /**
       * Start the application (perhaps in the minimized state).
       *
       * @return True if the application was successfully started.
       */

      bool run(void);

      /**
       * Handle CCalibration events.  This overrides a method from
       * CTwm4NXEvent.
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

       bool event(FAR struct SEventMsg *eventmsg);
  };

  class CCalibrationFactory : public IApplication,
                              public IApplicationFactory,
                              public CTwm4NxEvent
  {
    private:
      FAR CTwm4Nx *m_twm4nx; /**< Twm4Nx session instance */

      /**
       * Handle CCalibrationFactory events.  This overrides a method from
       * CTwm4NXEvent
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

       bool event(FAR struct SEventMsg *eventmsg);

      /**
       * Create and start a new instance of an CNxTerm.
       */

      bool startFunction(void);

      /**
       * Return the Main Menu item string.  This overrides the method from
       * IApplication
       *
       * @param name The name of the application.
       */

      inline NXWidgets::CNxString getName(void)
      {
        return NXWidgets::CNxString("Calibration");
      }

      /**
       * There is no sub-menu for this Main Menu item.  This overrides
       * the method from IApplication.
       *
       * @return This implementation will always return a null value.
       */

      inline FAR CMenus *getSubMenu(void)
      {
        return (FAR CMenus *)0;
      }

      /**
       * There is no custom event handler.  We use the common event handler.
       *
       * @return.  null is always returned in this implementation.
       */

      inline FAR CTwm4NxEvent *getEventHandler(void)
      {
        return (FAR CTwm4NxEvent *)this;
      }

      /**
       * Return the Twm4Nx event that will be generated when the Main Menu
       * item is selected.
       *
       * @return. This function always returns EVENT_SYSTEM_NOP.
       */

      inline uint16_t getEvent(void)
      {
        return EVENT_CALIB_START;
      }

    public:
      /**
       * CCalibrationFactory Constructor
       */

      inline CCalibrationFactory(void)
      {
        m_twm4nx = (FAR CTwm4Nx *)0;
      }

      /**
       * CCalibrationFactory Destructor
       */

      inline ~CCalibrationFactory(void)
      {
        // REVISIT:  Would need to remove Main Menu item
      }

      /**
       * CCalibrationFactory Initializer.  Performs parts of the instance
       * construction that may fail.  In this implementation, it registers
       * a menu item in the Main Menu and, optionally, bring up a
       * CCalibration instance now in order calibrate unconditionally on
       * start-up
       *
       * @param twm4nx The Twm4Nx session instance
       * @return True if successfully initialized
       */

      bool initialize(FAR CTwm4Nx *twm4nx);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CCALIBRATION_HXX
