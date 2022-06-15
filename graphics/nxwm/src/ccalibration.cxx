/****************************************************************************
 * apps/graphics/nxwm/src/ccalibration.cxx
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <cinttypes>
#include <cerrno>

#include <sched.h>
#include <limits.h>
#include <assert.h>
#include <debug.h>
#include <unistd.h>

#ifdef CONFIG_NXWM_TOUCHSCREEN_CONFIGDATA
#  include "platform/configdata.h"
#endif

#include "graphics/nxwm/nxwmconfig.hxx"
#include "graphics/nxglyphs.hxx"
#include "graphics/nxwm/ctouchscreen.hxx"
#include "graphics/nxwm/ccalibration.hxx"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Configuration
 */

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

/**
 * Positional/size data for the calibration lines and circles
 */

#define CALIBRATION_LEFTX              CONFIG_NXWM_CALIBRATION_MARGIN
#define CALIBRATION_RIGHTX             (windowSize.w - CONFIG_NXWM_CALIBRATION_MARGIN + 1)
#define CALIBRATION_TOPY               CONFIG_NXWM_CALIBRATION_MARGIN
#define CALIBRATION_BOTTOMY            (windowSize.h - CONFIG_NXWM_CALIBRATION_MARGIN + 1)

#define CALIBRATION_CIRCLE_RADIUS      16
#define CALIBRATION_LINE_THICKNESS     2

/* We want debug output from some logic in this file if either input/touchscreen
 * or graphics debug is enabled.
 */

#ifndef CONFIG_DEBUG_INPUT
#  undef  ierr
#  define ierr gerr
#  undef  iinfo
#  define iinfo ginfo
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

using namespace NxWM;

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
static const char g_touchMsg[] = "Touch";
static const char g_againMsg[] = "Again";
static const char g_okMsg[]    = "OK";
#endif

/****************************************************************************
 * CCalibration Implementation Classes
 ****************************************************************************/

/**
 * CCalibration Constructor
 *
 * @param taskbar.  The taskbar instance used to terminate calibration
 * @param window.  The window to use for the calibration display
 * @param touchscreen. An instance of the class that wraps the touchscreen
 *   device.
 */

CCalibration::CCalibration(CTaskbar *taskbar, CFullScreenWindow *window,
                           CTouchscreen *touchscreen)
{
  // Initialize state data

  m_taskbar       = taskbar;
  m_window        = window;
  m_touchscreen   = touchscreen;
  m_thread        = 0;
  m_calthread     = CALTHREAD_NOTRUNNING;
  m_calphase      = CALPHASE_NOT_STARTED;
  m_touched       = false;

  // Nullify widgets that will be instantiated when the calibration thread
  // is started

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
  m_text          = (NXWidgets::CLabel  *)0;
  m_font          = (NXWidgets::CNxFont *)0;
#endif
}

/**
 * CCalibration Destructor
 */

CCalibration::~CCalibration(void)
{
  // Make sure that the application is not running (it should already
  // have been stopped)

  stop();

  // Although we did not create the window, the rule is that I have to dispose
  // of it

  delete m_window;
}

/**
 * Each implementation of IApplication must provide a method to recover
 * the contained IApplicationWindow instance.
 */

IApplicationWindow *CCalibration::getWindow(void) const
{
  return static_cast<IApplicationWindow*>(m_window);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CCalibration::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_CALIBRATION_ICON);

  return bitmap;
}

/**
 * Get the name string associated with the application
 *
 * @return A copy if CNxString that contains the name of the application.
 */

NXWidgets::CNxString CCalibration::getName(void)
{
  return NXWidgets::CNxString("Touchscreen Calibration");
}

/**
 * Start the application (perhaps in the minimized state).
 *
 * @return True if the application was successfully started.
 */

bool CCalibration::run(void)
{
  ginfo("Starting calibration: m_calthread=%d\n", (int)m_calthread);

  return startCalibration(CALTHREAD_STARTED);
}

/**
 * Stop the application.
 */

void CCalibration::stop(void)
{
  ginfo("Stopping calibration: m_calthread=%d\n", (int)m_calthread);

  // Was the calibration thread created?

  if (m_thread != 0)
    {
      // Is the calibration thread running?

      if (isRunning())
        {
          // The main thread is stuck waiting for the next touchscreen input...
          // We can signal that we would like the thread to stop, but we will be
          // stuck here until the next touch

          m_calthread = CALTHREAD_STOPREQUESTED;

          // Try to wake up the calibration thread so that it will see our
          // termination request

          ginfo("Stopping calibration: m_calthread=%d\n", (int)m_calthread);
          pthread_kill(m_thread, CONFIG_NXWM_CALIBRATION_SIGNO);

          // Wait for the calibration thread to exit

          FAR pthread_addr_t value;
          pthread_join(m_thread, &value);
        }
    }
}

/**
 * Destroy the application and free all of its resources.  This method
 * will initiate blocking of messages from the NX server.  The server
 * will flush the window message queue and reply with the blocked
 * message.  When the block message is received by CWindowMessenger,
 * it will send the destroy message to the start window task which
 * will, finally, safely delete the application.
 */

void CCalibration::destroy(void)
{
  // Make sure that the application is stopped (should already be stopped)

  stop();

  // Block any further window messages

  m_window->block(this);
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but it is not at the top of the hierarchy)
 */

void CCalibration::hide(void)
{
  ginfo("Entry\n");

  // Is the calibration thread running?

  if (m_calthread == CALTHREAD_RUNNING || m_calthread == CALTHREAD_SHOW)
    {
      // Ask the calibration thread to hide the display

      m_calthread = CALTHREAD_HIDE;
      pthread_kill(m_thread, CONFIG_NXWM_CALIBRATION_SIGNO);
    }
}

/**
 * Redraw the entire window.  The application has been maximized or
 * otherwise moved to the top of the hierarchy.  This method is called from
 * CTaskbar when the application window must be displayed
 */

void CCalibration::redraw(void)
{
  uint8_t waitcount = 0;

  ginfo("Entry\n");

  // Is the calibration thread still running?  We might have to restart
  // it if we have completed the calibration early but are being brought
  // to top of the display again

  if (!isStarted())
    {
      ginfo("Starting calibration: m_calthread=%d\n", (int)m_calthread);
      startCalibration(CALTHREAD_SHOW);
    }

  // Is the calibration thread running? If not, then wait until it is.

  while (!isRunning() && (++waitcount < 10))
    {
      usleep(500);
    }

  // The calibration thread is running.  Make sure that is is not
  // already processing a redraw

  if (m_calthread != CALTHREAD_SHOW)
    {
      // Ask the calibration thread to restart the calibration and redraw
      // the display

      m_calthread = CALTHREAD_SHOW;
      pthread_kill(m_thread, CONFIG_NXWM_CALIBRATION_SIGNO);
    }
}

/**
 * Report of this is a "normal" window or a full screen window.  The
 * primary purpose of this method is so that window manager will know
 * whether or not it show draw the task bar.
 *
 * @return True if this is a full screen window.
 */

bool CCalibration::isFullScreen(void) const
{
  return m_window->isFullScreen();
}

/**
 * Accept raw touchscreen input.
 *
 * @param sample Touchscreen input sample
 */

void CCalibration::touchscreenInput(struct touch_sample_s &sample)
{
  // Is this a new touch event?  Or is it a drag event?

  if ((sample.point[0].flags & (TOUCH_DOWN|TOUCH_MOVE)) != 0)
    {
      // Yes.. but ignore drag events if we did not see the matching
      // touch down event

      if ((sample.point[0].flags & TOUCH_DOWN) != 0 ||
          (m_touched && sample.point[0].id == m_touchId))
        {
          // Yes.. save the touch position and wait for the TOUCH_UP report

          m_touchPos.x = sample.point[0].x;
          m_touchPos.y = sample.point[0].y;

          iinfo("Touch id: %d flags: %02x x: %d y: %d h: %d w: %d pressure: %d\n",
                sample.point[0].id, sample.point[0].flags, sample.point[0].x,
                sample.point[0].y,  sample.point[0].h,     sample.point[0].w,
                sample.point[0].pressure);

          // Show calibration screen again, changing the color of the circle to
          // make it clear that the touch has been noticed.

          if (!m_touched)
            {
              m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_TOUCHEDCOLOR;
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
              m_text->setText(g_okMsg);
#endif
              showCalibration();
              m_touched = true;
            }

          // Remember the ID of the touch down event

          m_touchId = sample.point[0].id;
        }
    }

  // Was the touch released?

  else if ((sample.point[0].flags & TOUCH_UP) != 0)
    {
      // Yes.. did we see the pen down event?

      if (m_touched)
        {
          // Yes.. For the matching touch ID?

          if (sample.point[0].id == m_touchId)
            {
              // Yes.. invoke the state machine.

              iinfo("State: %d Screen x: %d y: %d  Touch x: %d y: %d\n",
                    m_calphase, m_screenInfo.pos.x, m_screenInfo.pos.y,
                    m_touchPos.x, m_touchPos.y);

              stateMachine();
            }
          else
            {
              // No... restore the un-highlighted circle

              m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
              m_text->setText("");
#endif
              showCalibration();
            }
        }

      // In any event, the screen is not touched

      m_touched = false;
    }
}

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
/**
 * Create widgets need by the calibration thread.
 *
 * @return True if the widgets were successfully created.
 */

bool CCalibration::createWidgets(void)
{
  // Select a font for the calculator

  m_font = new NXWidgets::
    CNxFont((nx_fontid_e)CONFIG_NXWM_CALIBRATION_FONTID,
            CONFIG_NXWM_DEFAULT_FONTCOLOR, CONFIG_NXWM_TRANSPARENT_COLOR);
  if (!m_font)
    {
      gerr("ERROR failed to create font\n");
      return false;
    }

  // Recover the window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the size of the window

  struct nxgl_size_s windowSize;
  if (!window->getSize(&windowSize))
    {
      gerr("ERROR: Failed to get window size\n");
      delete m_font;
      m_font = (NXWidgets::CNxFont *)0;
      return false;
    }

  // How big is the biggest string we might display?

  nxgl_coord_t maxStringWidth = m_font->getStringWidth(g_touchMsg);
  nxgl_coord_t altStringWidth = m_font->getStringWidth(g_againMsg);

  if (altStringWidth > maxStringWidth)
    {
      maxStringWidth = altStringWidth;
    }

  // How big can the label be?

  struct nxgl_size_s labelSize;
  labelSize.w = maxStringWidth + 2*4;
  labelSize.h = m_font->getHeight() + 2*4;

  // Where should the label be?

  struct nxgl_point_s labelPos;
  labelPos.x = ((windowSize.w - labelSize.w) / 2);
  labelPos.y = ((windowSize.h - labelSize.h) / 2);

  // Get the widget control associated with the application window

  NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

  // Create a label to show the calibration message.

  m_text = new NXWidgets::
    CLabel(control, labelPos.x, labelPos.y, labelSize.w, labelSize.h, "");

  if (!m_text)
    {
      gerr("ERROR: Failed to create CLabel\n");
      delete m_font;
      m_font = (NXWidgets::CNxFont *)0;
      return false;
    }

  // No border

  m_text->setBorderless(true);

  // Center text

  m_text->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_CENTER);

  // Disable drawing and events until we are asked to redraw the window

  m_text->disableDrawing();
  m_text->setRaisesEvents(false);

  // Select the font

  m_text->setFont(m_font);
  return true;
}

/**
 * Destroy widgets created for the calibration thread.
 */

void CCalibration::destroyWidgets(void)
{
  delete m_text;
  m_text = (NXWidgets::CLabel *)0;

  delete m_font;
  m_font = (NXWidgets::CNxFont *)0;
}
#endif

/**
 * Start the calibration thread.
 *
 * @param initialState.  The initial state of the calibration thread
 * @return True if the thread was successfully started.
 */

bool CCalibration::startCalibration(enum ECalThreadState initialState)
{

  // Verify that the thread is not already running

  if (isRunning())
    {
      gwarn("WARNING: The calibration thread is already running\n");
      return false;
    }

  // Configure the calibration thread

  pthread_attr_t attr;
  pthread_attr_init(&attr);

  struct sched_param param;
  param.sched_priority = CONFIG_NXWM_CALIBRATION_LISTENERPRIO;
  pthread_attr_setschedparam(&attr, &param);

  pthread_attr_setstacksize(&attr, CONFIG_NXWM_CALIBRATION_LISTENERSTACK);

  // Set the initial state of the thread

  m_calthread = initialState;

  // Start the thread that will perform the calibration process

  int ret = pthread_create(&m_thread, &attr, calibration, (FAR void *)this);
  if (ret != 0)
    {
      gerr("ERROR: pthread_create failed: %d\n", ret);
      return false;
    }

  ginfo("Calibration thread m_calthread=%d\n", (int)m_calthread);
  return true;
}

/**
 * The calibration thread.  This is the entry point of a thread that provides the
 * calibration displays, waits for input, and collects calibration data.
 *
 * @param arg.  The CCalibration 'this' pointer cast to a void*.
 * @return This function always returns NULL when the thread exits
 */

FAR void *CCalibration::calibration(FAR void *arg)
{
  CCalibration *This = (CCalibration *)arg;
  bool stalled = true;

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
  // Create widgets that will be used in the calibration display

  if (!This->createWidgets())
    {
      gerr("ERROR: failed to create widgets\n");
      return (FAR void *)0;
    }

  // No samples have yet been collected

  This->m_nsamples = 0;
#endif

  // The calibration thread is now running

  This->m_calthread = CALTHREAD_RUNNING;
  This->m_calphase  = CALPHASE_NOT_STARTED;
  ginfo("Started: m_calthread=%d\n", (int)This->m_calthread);

  // Loop until calibration completes or we have been requested to terminate

  while (This->m_calthread != CALTHREAD_STOPREQUESTED &&
         This->m_calphase != CALPHASE_COMPLETE)
    {
      // Check for state changes due to display order changes

      if (This->m_calthread == CALTHREAD_HIDE)
        {
          // This state is set by hide() when our display is no longer visible

          This->m_calthread = CALTHREAD_RUNNING;
          This->m_calphase  = CALPHASE_NOT_STARTED;
          stalled           = true;
        }
      else if (This->m_calthread == CALTHREAD_SHOW)
        {
          // This state is set by redraw() when our display has become visible

          This->m_calthread = CALTHREAD_RUNNING;
          This->m_calphase  = CALPHASE_NOT_STARTED;
          stalled           = false;
          This->stateMachine();
        }

      // The calibration thread will stall if has been asked to hide the
      // display.  While stalled, we will just sleep for a bit and test
      // the state again.  If we are re-awakened by a redraw(), then we
      // will be given a signal which will wake us up immediately.
      //
      // Note that stalled is also initially true so we have to receive
      // redraw() before we attempt to draw anything

      if (stalled)
        {
          // Sleep for a while (or until we receive a signal)

          usleep(500*1000);
        }
      else
        {
          // Wait for the next raw touchscreen input (or possibly a signal)

          struct touch_sample_s sample;
          while (!This->m_touchscreen->waitRawTouchData(&sample) &&
                  This->m_calthread == CALTHREAD_RUNNING);

          // Then process the raw touchscreen input

          if (This->m_calthread == CALTHREAD_RUNNING)
            {
              This->touchscreenInput(sample);
            }
        }
    }

  // Hide the message

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
  This->m_text->setText("");
  This->m_text->enableDrawing();
  This->m_text->redraw();
  This->m_text->disableDrawing();
#endif

  // Perform the final steps of calibration

  This->finishCalibration();

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
  // Destroy widgets

  This->destroyWidgets();
#endif

  ginfo("Terminated: m_calthread=%d\n", (int)This->m_calthread);
  return (FAR void *)0;
}

/**
 * Accumulate and average touch sample data
 *
 * @param average.  When the averaged data is available, return it here
 * @return True: Average data is available; False: Need to collect more samples
 */

#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
bool CCalibration::averageSamples(struct nxgl_point_s &average)
{
  // Have we started collecting sample data? */

  // Save the sample data

  iinfo("Sample %d: Touch x: %d y: %d\n", m_nsamples+1, m_touchPos.x, m_touchPos.y);

  m_sampleData[m_nsamples].x = m_touchPos.x;
  m_sampleData[m_nsamples].y = m_touchPos.y;
  m_nsamples++;

  // Have all of the samples been collected?

  if (m_nsamples <  CONFIG_NXWM_CALIBRATION_NSAMPLES)
    {
      // Not yet... return false

      return false;
    }

  // Yes.. we have all of the samples
#ifdef CONFIG_NXWM_CALIBRATION_NSAMPLES
  // Discard the smallest X and Y values

  int xValue = INT_MAX;
  int xIndex = -1;

  int yValue = INT_MAX;
  int yIndex = -1;

  for (int i = 0; i < CONFIG_NXWM_CALIBRATION_NSAMPLES; i++)
    {
      if ((int)m_sampleData[i].x < xValue)
        {
          xValue = (int)m_sampleData[i].x;
          xIndex = i;
        }

      if ((int)m_sampleData[i].y < yValue)
        {
          yValue = (int)m_sampleData[i].y;
          yIndex = i;
        }
    }

  m_sampleData[xIndex].x = m_sampleData[CONFIG_NXWM_CALIBRATION_NSAMPLES-1].x;
  m_sampleData[yIndex].y = m_sampleData[CONFIG_NXWM_CALIBRATION_NSAMPLES-1].y;

  // Discard the largest X and Y values

  xValue = -1;
  xIndex = -1;

  yValue = -1;
  yIndex = -1;

  for (int i = 0; i < CONFIG_NXWM_CALIBRATION_NSAMPLES-1; i++)
    {
      if ((int)m_sampleData[i].x > xValue)
        {
          xValue = (int)m_sampleData[i].x;
          xIndex = i;
        }

      if ((int)m_sampleData[i].y > yValue)
        {
          yValue = (int)m_sampleData[i].y;
          yIndex = i;
        }
    }

  m_sampleData[xIndex].x = m_sampleData[CONFIG_NXWM_CALIBRATION_NSAMPLES-2].x;
  m_sampleData[yIndex].y = m_sampleData[CONFIG_NXWM_CALIBRATION_NSAMPLES-2].y;
#endif

#if NXWM_CALIBRATION_NAVERAGE > 1
  // Calculate the average of the remaining values

  long xaccum = 0;
  long yaccum = 0;

  for (int i = 0; i < NXWM_CALIBRATION_NAVERAGE; i++)
    {
      xaccum += m_sampleData[i].x;
      yaccum += m_sampleData[i].y;
    }

  average.x = xaccum / NXWM_CALIBRATION_NAVERAGE;
  average.y = yaccum / NXWM_CALIBRATION_NAVERAGE;
#else
  average.x = m_sampleData[0].x;
  average.y = m_sampleData[0].y;
#endif

  iinfo("Average: Touch x: %d y: %d\n", average.x, average.y);
  m_nsamples = 0;
  return true;
}
#endif

/**
 * This is the calibration state machine.  It is called initially and then
 * as new touchscreen data is received.
 */

void CCalibration::stateMachine(void)
{
  ginfo("Old m_calphase=%d\n", m_calphase);

#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
  // Are we collecting samples?

  struct nxgl_point_s average;

  switch (m_calphase)
    {
      case CALPHASE_UPPER_LEFT:
      case CALPHASE_UPPER_RIGHT:
      case CALPHASE_LOWER_RIGHT:
      case CALPHASE_LOWER_LEFT:
        {
          // Yes... Have all of the samples been collected?

         if (!averageSamples(average))
            {
              // Not yet...  Show the calibration screen again with the circle
              // in the normal color and an instruction to touch the circle again.

              m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_CIRCLECOLOR;
              m_text->setText(g_againMsg);
              showCalibration();

              // And wait for the next touch

              return;
            }
        }
        break;

      // No... we are not collecting data now

      default:
        break;
    }
#endif

  // Recover the window instance contained in the full screen window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the size of the fullscreen window

  struct nxgl_size_s windowSize;
  if (!window->getSize(&windowSize))
    {
      return;
    }

  switch (m_calphase)
    {
      default:
      case CALPHASE_NOT_STARTED:
        {
          // Clear the entire screen
          // Get the widget control associated with the full screen window

          NXWidgets::CWidgetControl *control =  window->getWidgetControl();

          // Get the CCGraphicsPort instance for this window

          NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

          // Fill the entire window with the background color

          port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                               CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR);

          // Then draw the first calibration screen

          m_screenInfo.pos.x           = CALIBRATION_LEFTX;
          m_screenInfo.pos.y           = CALIBRATION_TOPY;
          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
          m_text->setText(g_touchMsg);
#endif
          showCalibration();

          // Then set up the current state

          m_calphase = CALPHASE_UPPER_LEFT;
        }
        break;

      case CALPHASE_UPPER_LEFT:
        {
          // A touch has been received while in the CALPHASE_UPPER_LEFT state.
          // Save the touch data and set up the next calibration display

#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
          m_calibData[CALIB_UPPER_LEFT_INDEX].x = average.x;
          m_calibData[CALIB_UPPER_LEFT_INDEX].y = average.y;
#else
          m_calibData[CALIB_UPPER_LEFT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_UPPER_LEFT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the background
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
          showCalibration();

          // Then draw the next calibration screen

          m_screenInfo.pos.x           = CALIBRATION_RIGHTX;
          m_screenInfo.pos.y           = CALIBRATION_TOPY;
          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
          m_text->setText(g_touchMsg);
#endif
          showCalibration();

          // Then set up the current state

          m_calphase = CALPHASE_UPPER_RIGHT;
        }
        break;

      case CALPHASE_UPPER_RIGHT:
        {
          // A touch has been received while in the CALPHASE_UPPER_RIGHT state.
          // Save the touch data and set up the next calibration display

#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
          m_calibData[CALIB_UPPER_RIGHT_INDEX].x = average.x;
          m_calibData[CALIB_UPPER_RIGHT_INDEX].y = average.y;
#else
          m_calibData[CALIB_UPPER_RIGHT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_UPPER_RIGHT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the backgro9und
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
          showCalibration();

          // Then draw the next calibration screen

          m_screenInfo.pos.x           = CALIBRATION_RIGHTX;
          m_screenInfo.pos.y           = CALIBRATION_BOTTOMY;
          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
          m_text->setText(g_touchMsg);
#endif
          showCalibration();

          // Then set up the current state

          m_calphase = CALPHASE_LOWER_RIGHT;
        }
        break;

      case CALPHASE_LOWER_RIGHT:
        {
          // A touch has been received while in the CALPHASE_LOWER_RIGHT state.
          // Save the touch data and set up the next calibration display

#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
          m_calibData[CALIB_LOWER_RIGHT_INDEX].x = average.x;
          m_calibData[CALIB_LOWER_RIGHT_INDEX].y = average.y;
#else
          m_calibData[CALIB_LOWER_RIGHT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_LOWER_RIGHT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the backgro9und
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
          showCalibration();

          // Then draw the next calibration screen

          m_screenInfo.pos.x           = CALIBRATION_LEFTX;
          m_screenInfo.pos.y           = CALIBRATION_BOTTOMY;
          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
          m_text->setText(g_touchMsg);
#endif
          showCalibration();

          // Then set up the current state

          m_calphase = CALPHASE_LOWER_LEFT;
        }
        break;

      case CALPHASE_LOWER_LEFT:
        {
          // A touch has been received while in the CALPHASE_LOWER_LEFT state.
          // Save the touch data and set up the next calibration display

#ifdef CONFIG_NXWM_CALIBRATION_AVERAGE
          m_calibData[CALIB_LOWER_LEFT_INDEX].x = average.x;
          m_calibData[CALIB_LOWER_LEFT_INDEX].y = average.y;
#else
          m_calibData[CALIB_LOWER_LEFT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_LOWER_LEFT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the background
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_NXWM_CALIBRATION_BACKGROUNDCOLOR;
#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
          m_text->setText(g_touchMsg);
#endif
          showCalibration();

          // Inform any waiter that calibration is complete

          m_calphase = CALPHASE_COMPLETE;
        }
        break;

      case CALPHASE_COMPLETE:
        // Might happen... do nothing if it does
        break;
    }

  iinfo("New m_calphase=%d Screen x: %d y: %d\n",
        m_calphase, m_screenInfo.pos.x, m_screenInfo.pos.y);
}

/**
 * Presents the next calibration screen
 *
 * @param screenInfo Describes the next calibration screen
 */

void CCalibration::showCalibration(void)
{
  // Recover the window instance contained in the full screen window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the widget control associated with the full screen window

  NXWidgets::CWidgetControl *control =  window->getWidgetControl();

  // Get the CCGraphicsPort instance for this window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the fullscreen window

  struct nxgl_size_s windowSize;
  if (!window->getSize(&windowSize))
    {
      return;
    }

  // Draw the circle at the center of the touch position

  port->drawFilledCircle(&m_screenInfo.pos, CALIBRATION_CIRCLE_RADIUS,
                          m_screenInfo.circleFillColor);

  // Draw horizontal line

  port->drawFilledRect(0, m_screenInfo.pos.y, windowSize.w, CALIBRATION_LINE_THICKNESS,
                       m_screenInfo.lineColor);

  // Draw vertical line

  port->drawFilledRect(m_screenInfo.pos.x, 0, CALIBRATION_LINE_THICKNESS, windowSize.h,
                       m_screenInfo.lineColor);

  // Show the touchscreen message

#ifdef CONFIG_NXWM_CALIBRATION_MESSAGES
  m_text->enableDrawing();
  m_text->redraw();
  m_text->disableDrawing();
#endif
}

/**
 * Finish calibration steps and provide the calibration data to the
 * touchscreen driver.
 */

void CCalibration::finishCalibration(void)
{
  // Did we finish calibration successfully?

  if (m_calphase == CALPHASE_COMPLETE)
    {
      // Yes... Get the final Calibration data

      struct SCalibrationData caldata;
      if (createCalibrationData(caldata))
        {
#ifdef CONFIG_NXWM_TOUCHSCREEN_CONFIGDATA
          // Save the new calibration data.  The saved calibration
          // data may be used to avoided recalibrating in the future.

          int ret = platform_setconfig(CONFIGDATA_TSCALIBRATION, 0,
                                       (FAR const uint8_t *)&caldata,
                                       sizeof(struct SCalibrationData));
          if (ret != 0)
            {
              gerr("ERROR: Failed to save calibration data\n");
            }
#endif
          // And provide the calibration data to the touchscreen, enabling
          // touchscreen processing

          m_touchscreen->setEnabled(false);
          m_touchscreen->setCalibrationData(caldata);
          m_touchscreen->setEnabled(true);
        }
    }

  // Remove the touchscreen application from the taskbar

  m_taskbar->stopApplication(this);

  // And set the terminated stated

  m_calthread = CALTHREAD_TERMINATED;
}

/**
 * Given the raw touch data collected by the calibration thread, create the
 * massaged calibration data needed by CTouchscreen.
 *
 * @param data. A reference to the location to save the calibration data
 * @return True if the calibration data was successfully created.
 */

bool CCalibration::createCalibrationData(struct SCalibrationData &data)
{
  // Recover the window instance contained in the full screen window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the size of the fullscreen window

  struct nxgl_size_s windowSize;
  if (!window->getSize(&windowSize))
    {
      gerr("ERROR: NXWidgets::INxWindow::getSize failed\n");
      return false;
    }

#ifdef CONFIG_NXWM_CALIBRATION_ANISOTROPIC
  // X lines:
  //
  //   x2 = slope*y1 + offset
  //
  // slope  = (bottomY - topY) / (bottomX - topX)
  // offset = (topY - topX * slope)

  float topX         = (float)m_calibData[CALIB_UPPER_LEFT_INDEX].x;
  float bottomX      = (float)m_calibData[CALIB_LOWER_LEFT_INDEX].x;

  float topY         = (float)m_calibData[CALIB_UPPER_LEFT_INDEX].y;
  float bottomY      = (float)m_calibData[CALIB_LOWER_LEFT_INDEX].y;

  data.left.slope    = (bottomX - topX) / (bottomY - topY);
  data.left.offset   = topX - topY * data.left.slope;

  iinfo("Left slope: %6.2f offset: %6.2f\n", data.left.slope, data.left.offset);

  topX               = (float)m_calibData[CALIB_UPPER_RIGHT_INDEX].x;
  bottomX            = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].x;

  topY               = (float)m_calibData[CALIB_UPPER_RIGHT_INDEX].y;
  bottomY            = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].y;

  data.right.slope   = (bottomX - topX) / (bottomY - topY);
  data.right.offset  = topX - topY * data.right.slope;

  iinfo("Right slope: %6.2f offset: %6.2f\n", data.right.slope, data.right.offset);

  // Y lines:
  //
  //   y2 = slope*x1 + offset
  //
  // slope  = (rightX - topX) / (rightY - leftY)
  // offset = (topX - leftY * slope)

  float leftX        = (float)m_calibData[CALIB_UPPER_LEFT_INDEX].x;
  float rightX       = (float)m_calibData[CALIB_UPPER_RIGHT_INDEX].x;

  float leftY        = (float)m_calibData[CALIB_UPPER_LEFT_INDEX].y;
  float rightY       = (float)m_calibData[CALIB_UPPER_RIGHT_INDEX].y;

  data.top.slope     = (rightY - leftY) / (rightX - leftX);
  data.top.offset    = leftY - leftX * data.top.slope;

  iinfo("Top slope: %6.2f offset: %6.2f\n", data.top.slope, data.top.offset);

  leftX              = (float)m_calibData[CALIB_LOWER_LEFT_INDEX].x;
  rightX             = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].x;

  leftY              = (float)m_calibData[CALIB_LOWER_LEFT_INDEX].y;
  rightY             = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].y;

  data.bottom.slope  = (rightY - leftY) / (rightX - leftX);
  data.bottom.offset = leftY - leftX * data.bottom.slope;

  iinfo("Bottom slope: %6.2f offset: %6.2f\n", data.bottom.slope, data.bottom.offset);

  // Save also the calibration screen positions

  data.leftX         = CALIBRATION_LEFTX;
  data.rightX        = CALIBRATION_RIGHTX;
  data.topY          = CALIBRATION_TOPY;
  data.bottomY       = CALIBRATION_BOTTOMY;
#else
  // Calculate the calibration parameters
  //
  // (scaledX - LEFTX) / (rawX - leftX) = (RIGHTX - LEFTX) / (rightX - leftX)
  // scaledX = (rawX - leftX) * (RIGHTX - LEFTX) / (rightX - leftX) + LEFTX
  //         = rawX * xSlope + (LEFTX - leftX * xSlope)
  //         = rawX * xSlope + xOffset
  //
  // where:
  // xSlope  = (RIGHTX - LEFTX) / (rightX - leftX)
  // xOffset = (LEFTX - leftX * xSlope)

  b16_t leftX  = (m_calibData[CALIB_UPPER_LEFT_INDEX].x +
                  m_calibData[CALIB_LOWER_LEFT_INDEX].x) << 15;
  b16_t rightX = (m_calibData[CALIB_UPPER_RIGHT_INDEX].x +
                  m_calibData[CALIB_LOWER_RIGHT_INDEX].x) << 15;

  data.xSlope  = b16divb16(itob16(CALIBRATION_RIGHTX - CALIBRATION_LEFTX), (rightX - leftX));
  data.xOffset = itob16(CALIBRATION_LEFTX) - b16mulb16(leftX, data.xSlope);

  iinfo("New xSlope: %08" PRIx32 " xOffset: %08" PRIx32 "\n",
        data.xSlope, data.xOffset);

  // Similarly for Y
  //
  // (scaledY - TOPY) / (rawY - topY) = (BOTTOMY - TOPY) / (bottomY - topY)
  // scaledY = (rawY - topY) * (BOTTOMY - TOPY) / (bottomY - topY) + TOPY
  //         = rawY * ySlope + (TOPY - topY * ySlope)
  //         = rawY * ySlope + yOffset
  //
  // where:
  // ySlope  = (BOTTOMY - TOPY) / (bottomY - topY)
  // yOffset = (TOPY - topY * ySlope)

  b16_t topY    = (m_calibData[CALIB_UPPER_LEFT_INDEX].y +
                   m_calibData[CALIB_UPPER_RIGHT_INDEX].y) << 15;
  b16_t bottomY = (m_calibData[CALIB_LOWER_LEFT_INDEX].y +
                   m_calibData[CALIB_LOWER_RIGHT_INDEX].y) << 15;

  data.ySlope  = b16divb16(itob16(CALIBRATION_BOTTOMY - CALIBRATION_TOPY), (bottomY - topY));
  data.yOffset = itob16(CALIBRATION_TOPY) - b16mulb16(topY, data.ySlope);

  iinfo("New ySlope: %08" PRIx32 " yOffset: %08" PRIx32 "\n",
        data.ySlope, data.yOffset);
#endif

  return true;
}

/**
 * CCalibrationFactory Constructor
 *
 * @param taskbar.  The taskbar instance used to terminate calibration
 * @param touchscreen. An instance of the class that wraps the
 *   touchscreen device.
 */

CCalibrationFactory::CCalibrationFactory(CTaskbar *taskbar, CTouchscreen *touchscreen)
{
  m_taskbar     = taskbar;
  m_touchscreen = touchscreen;
}

/**
 * Create a new instance of an CCalibration (as IApplication).
 */

IApplication *CCalibrationFactory::create(void)
{
  // Call CTaskBar::openFullScreenWindow to create a full screen window for
  // the calibration application

  CFullScreenWindow *window = m_taskbar->openFullScreenWindow();
  if (!window)
    {
      gerr("ERROR: Failed to create CFullScreenWindow\n");
      return (IApplication *)0;
    }

  // Open the window (it is hot in here)

  if (!window->open())
    {
      gerr("ERROR: Failed to open CFullScreenWindow\n");
      delete window;
      return (IApplication *)0;
    }

  // Instantiate the application, providing the window to the application's
  // constructor

  CCalibration *calibration = new CCalibration(m_taskbar, window, m_touchscreen);
  if (!calibration)
    {
      gerr("ERROR: Failed to instantiate CCalibration\n");
      delete window;
      return (IApplication *)0;
    }

  return static_cast<IApplication*>(calibration);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CCalibrationFactory::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_CALIBRATION_ICON);

  return bitmap;
}
