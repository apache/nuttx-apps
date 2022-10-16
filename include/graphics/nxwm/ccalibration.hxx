/****************************************************************************
 * apps/include/graphics/nxwm/ccalibration.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_CCALIBRATION_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_CCALIBRATION_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/nx/nxglib.h>

#include <pthread.h>
#include <fixedmath.h>

#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/clabel.hxx"
#include "graphics/nxwidgets/cnxfont.hxx"

#include "graphics/nxwm/ctaskbar.hxx"
#include "graphics/nxwm/iapplication.hxx"
#include "graphics/nxwm/cfullscreenwindow.hxx"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * Calibration indices
 */

#define CALIB_UPPER_LEFT_INDEX  0
#define CALIB_UPPER_RIGHT_INDEX 1
#define CALIB_LOWER_RIGHT_INDEX 2
#define CALIB_LOWER_LEFT_INDEX  3

#define CALIB_DATA_POINTS       4

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

namespace NxWM
{
  /**
   * Forward references
   */

  struct CTouchscreen;

  /**
   * Touchscreen calibration data
   */

#ifdef CONFIG_NXWM_CALIBRATION_ANISOTROPIC
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

  /**
   * The CCalibration class provides the the calibration window and obtains
   * calibration data.
   */

  class CCalibration : public IApplication
  {
  private:
    /**
     * The state of the calibration thread.
     */

    enum ECalThreadState
    {
      CALTHREAD_NOTRUNNING = 0,                  /**< The calibration thread has not yet been started */
      CALTHREAD_STARTED,                         /**< The calibration thread has been started, but is not yet running */
      CALTHREAD_RUNNING,                         /**< The calibration thread is running normally */
      CALTHREAD_STOPREQUESTED,                   /**< The calibration thread has been requested to stop */
      CALTHREAD_HIDE,                            /**< The hide() called by calibration thread running */
      CALTHREAD_SHOW,                            /**< The redraw() called by calibration thread running */
      CALTHREAD_TERMINATED                       /**< The calibration thread terminated normally */
    };

    /**
     * Identifies the current display state
     */

    enum ECalibrationPhase
    {
      CALPHASE_NOT_STARTED = 0,                  /**< Constructed, but not yet started */
      CALPHASE_UPPER_LEFT,                       /**< Touch point is in the upper left corner */
      CALPHASE_UPPER_RIGHT,                      /**< Touch point is in the upper right corner */
      CALPHASE_LOWER_RIGHT,                      /**< Touch point is in the lower left corner */
      CALPHASE_LOWER_LEFT,                       /**< Touch point is in the lower right corner */
      CALPHASE_COMPLETE                          /**< Calibration is complete */
    };

    /**
     * Characterizes one calibration screen
     */

    struct SCalibScreenInfo
    {
      struct nxgl_point_s      pos;               /**< The position of the touch point */
      nxgl_mxpixel_t           lineColor;         /**< The color of the cross-hair lines */
      nxgl_mxpixel_t           circleFillColor;   /**< The color of the circle */
    };

    /**
     * CCalibration state data
     */

    CTaskbar                  *m_taskbar;         /**< The taskbar (used to terminate calibration) */
    CFullScreenWindow         *m_window;          /**< The window for the calibration display */
    CTouchscreen              *m_touchscreen;     /**< The touchscreen device */
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
    NXWidgets::CLabel         *m_text;            /**< Calibration message */
    NXWidgets::CNxFont        *m_font;            /**< The font used in the message */
#endif
    pthread_t                  m_thread;          /**< The calibration thread ID */
    struct SCalibScreenInfo    m_screenInfo;      /**< Describes the current calibration display */
    struct nxgl_point_s        m_touchPos;        /**< This is the last touch position */
    volatile uint8_t           m_calthread;       /**< Current calibration display state (See ECalibThreadState)*/
    uint8_t                    m_calphase;        /**< Current calibration display state (See ECalibrationPhase)*/
    bool                       m_stop;            /**< True: We have been asked to stop the calibration */
    bool                       m_touched;         /**< True: The screen is touched */
    uint8_t                    m_touchId;         /**< The ID of the touch */
#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
    uint8_t                    m_nsamples;        /**< Number of samples collected so far at this position */
    struct nxgl_point_s        m_sampleData[CONFIG_NXWM_CALIBRATION_NSAMPLES];
#endif
    struct nxgl_point_s        m_calibData[CALIB_DATA_POINTS];

    /**
     * Accept raw touchscreen input.
     *
     * @param sample Touchscreen input sample
     */

    void touchscreenInput(struct touch_sample_s &sample);

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
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
     * Start the calibration thread.
     *
     * @param initialState.  The initial state of the calibration thread
     * @return True if the thread was successfully started.
     */

    bool startCalibration(enum ECalThreadState initialState);

    /**
     *  Return true if the calibration thread is running normally.  There are
     *  lots of potential race conditions.  There are also two ambiguous
     *  states:
     *
     *  1) The thread may have been started but not yet running
     *     (CALTHREAD_STARTED), or the
     *  2) The thread may been requested to terminate, but has not yet
     *     terminated (CALTHREAD_STOPREQUESTED)
     *
     *  Both of those states will cause isRunning() to return false.
     *
     * @return True if the calibration thread is running normally.
     */

    inline bool isRunning(void) const
      {
        return (m_calthread ==  CALTHREAD_RUNNING ||
                m_calthread ==  CALTHREAD_HIDE ||
                m_calthread ==  CALTHREAD_SHOW);
      }

    /**
     *  Return true if the calibration thread is has been started and has not
     *  yet terminated.  There is a potential race condition here when the
     *  thread has been requested to terminate, but has not yet terminated
     *  (CALTHREAD_STOPREQUESTED).  isStarted() will return false in that case.
     *
     * @return True if the calibration thread has been started and/or is
     *  running normally.
     */

    inline bool isStarted(void) const
      {
        return (m_calthread ==  CALTHREAD_STARTED ||
                m_calthread ==  CALTHREAD_RUNNING ||
                m_calthread ==  CALTHREAD_HIDE ||
                m_calthread ==  CALTHREAD_SHOW);
      }

    /**
     * The calibration thread.  This is the entry point of a thread that provides the
     * calibration displays, waits for input, and collects calibration data.
     *
     * @param arg.  The CCalibration 'this' pointer cast to a void*.
     * @return This function always returns NULL when the thread exits
     */

    static FAR void *calibration(FAR void *arg);

#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
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
     * @param taskbar.  The taskbar instance used to terminate calibration
     * @param window.  The window to use for the calibration display
     * @param touchscreen. An instance of the class that wraps the
     *   touchscreen device.
     */

    CCalibration(CTaskbar *taskbar, CFullScreenWindow *window,
                 CTouchscreen *touchscreen);

    /**
     * CCalibration Destructor
     */

    ~CCalibration(void);

    /**
     * Each implementation of IApplication must provide a method to recover
     * the contained IApplicationWindow instance.
     */

    IApplicationWindow *getWindow(void) const;

    /**
     * Get the icon associated with the application
     *
     * @return An instance if IBitmap that may be used to rend the
     *   application's icon.  This is an new IBitmap instance that must
     *   be deleted by the caller when it is no long needed.
     */

    NXWidgets::IBitmap *getIcon(void);

    /**
     * Get the name string associated with the application
     *
     * @return A copy if CNxString that contains the name of the application.
     */

    NXWidgets::CNxString getName(void);

    /**
     * Start the application (perhaps in the minimized state).
     *
     * @return True if the application was successfully started.
     */

    bool run(void);

    /**
     * Stop the application.
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
     * The application window is hidden (either it is minimized or it is
     * maximized, but not at the top of the hierarchy
     */

    void hide(void);

    /**
     * Redraw the entire window.  The application has been maximized or
     * otherwise moved to the top of the hierarchy.  This method is called from
     * CTaskbar when the application window must be displayed
     */

    void redraw(void);

    /**
     * Report of this is a "normal" window or a full screen window.  The
     * primary purpose of this method is so that window manager will know
     * whether or not it show draw the task bar.
     *
     * @return True if this is a full screen window.
     */

    bool isFullScreen(void) const;
  };

  class CCalibrationFactory : public IApplicationFactory
  {
  private:
    CTaskbar     *m_taskbar;      /**< The taskbar */
    CTouchscreen *m_touchscreen;  /**< The touchscreen device */

  public:
    /**
     * CCalibrationFactory Constructor
     *
     * @param taskbar.  The taskbar instance used to terminate calibration
     * @param touchscreen. An instance of the class that wraps the
     *   touchscreen device.
     */

    CCalibrationFactory(CTaskbar *taskbar, CTouchscreen *touchscreen);

    /**
     * CCalibrationFactory Destructor
     */

    inline ~CCalibrationFactory(void) { }

    /**
     * Create a new instance of an CCalibration (as IApplication).
     */

    IApplication *create(void);

    /**
     * Get the icon associated with the application
     *
     * @return An instance if IBitmap that may be used to rend the
     *   application's icon.  This is an new IBitmap instance that must
     *   be deleted by the caller when it is no long needed.
     */

    NXWidgets::IBitmap *getIcon(void);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_CCALIBRATION_HXX
