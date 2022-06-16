/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ccalibration.cxx
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cinttypes>
#include <cassert>
#include <cerrno>

#include <limits.h>
#include <semaphore.h>
#include <debug.h>
#include <sched.h>
#include <unistd.h>

#include <nuttx/semaphore.h>
#include <nuttx/nx/nxbe.h>

#ifdef CONFIG_TWM4NX_TOUCHSCREEN_CONFIGDATA
#  include "platform/configdata.h"
#endif

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cinput.hxx"
#include "graphics/twm4nx/cmainmenu.hxx"

#include "graphics/twm4nx/apps/calib_config.hxx"
#include "graphics/twm4nx/apps/ccalibration.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

// Configuration

#ifndef CONFIG_NX
#  error "NX is not enabled (CONFIG_NX)"
#endif

// Positional/size data for the calibration lines and circles

#define CALIBRATION_LEFTX           CONFIG_TWM4NX_CALIBRATION_MARGIN
#define CALIBRATION_RIGHTX          (windowSize.w - CONFIG_TWM4NX_CALIBRATION_MARGIN + 1)
#define CALIBRATION_TOPY            CONFIG_TWM4NX_CALIBRATION_MARGIN
#define CALIBRATION_BOTTOMY         (windowSize.h - CONFIG_TWM4NX_CALIBRATION_MARGIN + 1)

#define CALIBRATION_CIRCLE_RADIUS   16
#define CALIBRATION_LINE_THICKNESS  2

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
static const char GTouchMsg[] = "Touch";
static const char GAgainMsg[] = "Again";
static const char GOkMsg[]    = "OK";
#endif

/////////////////////////////////////////////////////////////////////////////
// CCalibration Method Implementations
/////////////////////////////////////////////////////////////////////////////

/**
 * CCalibration Constructor
 *
 * @param twm4nx.  The Twm4Nx session instance.
 */

CCalibration::CCalibration(FAR CTwm4Nx *twm4nx)
{
  // Initialize state data

  m_twm4nx         = twm4nx;
  m_nxWin          = (FAR NXWidgets::CNxWindow *)0;
  m_thread         = (pthread_t)0;
  m_calthread      = CALTHREAD_NOTRUNNING;
  m_calphase       = CALPHASE_NOT_STARTED;
  m_touched        = false;

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
  m_nsamples       = 0;
#endif

  // Initialize touch sample.

  m_sample.valid   = false;
  m_sample.pos.x   = 0;
  m_sample.pos.y   = 0;
  m_sample.buttons = 0;

#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
  // Nullify widgets that will be instantiated when the calibration thread
  // is started

  m_text           = (NXWidgets::CLabel  *)0;
  m_font           = (NXWidgets::CNxFont *)0;
#endif

  // Set up the semaphores that are used to synchronize the calibration
  // thread with Twm4Nx events

  sem_init(&m_exclSem, 0, 1);
  sem_init(&m_synchSem, 0, 0);
  sem_setprotocol(&m_synchSem, SEM_PRIO_NONE);
}

/**
 * CCalibration Destructor
 */

CCalibration::~CCalibration(void)
{
  // Make sure that the application is not running (it should already
  // have been stopped)

  stop();

  if (m_nxWin != (FAR NXWidgets::CNxWindow *)0)
    {
      delete m_nxWin;
    }
}

/**
 * CCalibration Initializer.  Performs parts of the instance
 * construction that may fail.  This function creates the
 * initial calibration display.
 */

bool CCalibration::initialize(void)
{
  // Create the calibration window
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_CALIB_REDRAW;
  events.mouseEvent  = EVENT_CALIB_XYINPUT;
  events.kbdEvent    = EVENT_CALIB_KBDINPUT;
  events.closeEvent  = EVENT_CALIB_CLOSE;
  events.deleteEvent = EVENT_CALIB_DELETE;

  FAR CWindowEvent *control =
    new CWindowEvent(m_twm4nx, (FAR CWindow *)0, events);

  // 4. Create the calibration window

  uint8_t wflags = NXBE_WINDOW_HIDDEN;

  m_nxWin = m_twm4nx->createRawWindow(control, wflags);
  if (m_nxWin == (FAR NXWidgets::CNxWindow *)0)
    {
      twmerr("ERROR: Failed open raw window\n");
      delete control;
      return false;
    }

  // 5. Open and initialize the main window

  bool success = m_nxWin->open();
  if (!success)
    {
      delete m_nxWin;
      m_nxWin = (FAR NXWidgets::CNxWindow *)0;
      return false;
    }

  // 6. Set the window position to the display origin

  struct nxgl_point_s pos =
  {
    .x = 0,
    .y = 0
  };

  if (!m_nxWin->setPosition(&pos))
    {
      delete m_nxWin;
      m_nxWin = (FAR NXWidgets::CNxWindow *)0;
      return false;
    }

  // 7. Set the window size to fill the whole display

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  if (!m_nxWin->setSize(&displaySize))
    {
      delete m_nxWin;
      m_nxWin = (FAR NXWidgets::CNxWindow *)0;
      return false;
    }

#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
  // Create widgets that will be used in the calibration display

  if (!createWidgets())
    {
      twmerr("ERROR: failed to create widgets\n");
      return (FAR void *)0;
    }
#endif

  // Disable touchscreen processing so that we receive raw, un-calibrated
  // touchscreen input

  FAR CInput *cinput = m_twm4nx->getInput();

  cinput->enableCalibration(false);
  return true;
}

/**
 * Start the application (perhaps in the minimized state).
 *
 * @return True if the application was successfully started.
 */

bool CCalibration::run(void)
{
  // Verify that the thread is not already running

  if (m_calthread == CALTHREAD_RUNNING)
    {
      twmwarn("WARNING: The calibration thread is already running\n");
      return false;
    }

  // Configure the calibration thread

  pthread_attr_t attr;
  pthread_attr_init(&attr);

  struct sched_param param;
  param.sched_priority = CONFIG_TWM4NX_CALIBRATION_LISTENERPRIO;
  pthread_attr_setschedparam(&attr, &param);

  pthread_attr_setstacksize(&attr, CONFIG_TWM4NX_CALIBRATION_LISTENERSTACK);

  // Set the initial state of the thread

  m_calthread = CALTHREAD_STARTED;

  // Start the thread that will perform the calibration process

  int ret = pthread_create(&m_thread, &attr, calibration, (FAR void *)this);
  if (ret != 0)
    {
      twmerr("ERROR: pthread_create failed: %d\n", ret);
      return false;
    }

  twminfo("Calibration thread m_calthread=%d\n", (int)m_calthread);
  return true;
}

/**
 * Handle CCalibration events.  This overrides a method from
 * CTwm4NXEvent.
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CCalibration::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_CALIB_XYINPUT:  // New touchscreen input
        {
          // Make sure that the thread is still running

          if (m_calthread == CALTHREAD_RUNNING)
            {
              // Get exclusive access to the data shared between threads

              success = exclusiveAccess();
              if (success)
                {
                  FAR struct SXyInputEventMsg *xymsg =
                    (FAR struct SXyInputEventMsg *)eventmsg;

                  // Save the new sample data

                  m_sample.valid   = true;
                  m_sample.pos     = xymsg->pos;
                  m_sample.buttons = xymsg->buttons;

                  // And let the let the calibration thread know that new
                  // data is available

                  sem_post(&m_synchSem);
                  sem_post(&m_exclSem);
                }
            }
        }
        break;

      case EVENT_CALIB_DELETE:   // Today is a good day to die.
        DEBUGASSERT(m_calthread == CALTHREAD_TERMINATED);
        delete this;             // This is suicide
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Accept raw touchscreen input.
 *
 * @param sample Touchscreen input sample
 */

void CCalibration::touchscreenInput(struct STouchSample &sample)
{
  // Is this a new touch event?

  if ((sample.buttons & MOUSE_BUTTON_1) != 0)
    {
      // Yes.. save the touch position and wait for the un-touch report

      m_touchPos.x = sample.pos.x;
      m_touchPos.y = sample.pos.y;

      twminfo("Touch buttons: %02x x: %d y: %d\n",
              sample.buttons, sample.pos.x, sample.pos.y);

      // Show calibration screen again, changing the color of the circle to
      // make it clear that the touch has been noticed.

      if (!m_touched)
        {
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_TOUCHEDCOLOR;
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
          m_text->setText(GOkMsg);
#endif
          showCalibration();
          m_touched = true;
        }
    }

  // There is no touch.  Did we see the pen down event?

  else if (m_touched)
    {
      // Yes.. invoke the state machine.

      twminfo("State: %d Screen x: %d y: %d  Touch x: %d y: %d\n",
              m_calphase, m_screenInfo.pos.x, m_screenInfo.pos.y,
              m_touchPos.x, m_touchPos.y);

      stateMachine();

      // The screen is no longer touched

      m_touched = false;
    }
}

#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
/**
 * Create widgets need by the calibration thread.
 *
 * @return True if the widgets were successfully created.
 */

bool CCalibration::createWidgets(void)
{
  // Select a font for the calculator

  m_font = new NXWidgets::
    CNxFont((nx_fontid_e)CONFIG_TWM4NX_CALIBRATION_FONTID,
            CONFIG_TWM4NX_DEFAULT_FONTCOLOR, CONFIG_TWM4NX_TRANSPARENT_COLOR);
  if (!m_font)
    {
      twmerr("ERROR failed to create font\n");
      return false;
    }

  // Get the size of the window

  struct nxgl_size_s windowSize;
  if (!m_nxWin->getSize(&windowSize))
    {
      twmerr("ERROR: Failed to get window size\n");
      delete m_font;
      m_font = (NXWidgets::CNxFont *)0;
      return false;
    }

  // How big is the biggest string we might display?

  nxgl_coord_t maxStringWidth = m_font->getStringWidth(GTouchMsg);
  nxgl_coord_t altStringWidth = m_font->getStringWidth(GAgainMsg);

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

  NXWidgets::CWidgetControl *control = m_nxWin->getWidgetControl();

  // Create a label to show the calibration message.

  m_text = new NXWidgets::
    CLabel(control, labelPos.x, labelPos.y, labelSize.w, labelSize.h, "");

  if (!m_text)
    {
      twmerr("ERROR: Failed to create CLabel\n");
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
 * The calibration thread.  This is the entry point of a thread that provides the
 * calibration displays, waits for input, and collects calibration data.
 *
 * @param arg.  The CCalibration 'this' pointer cast to a void*.
 * @return This function always returns NULL when the thread exits
 */

FAR void *CCalibration::calibration(FAR void *arg)
{
  CCalibration *This = (CCalibration *)arg;

  // The calibration thread is now running

  This->m_calthread = CALTHREAD_RUNNING;
  This->m_calphase  = CALPHASE_NOT_STARTED;

  twminfo("Started: m_calthread=%d\n", (int)This->m_calthread);

  // Make the calibration display visible and show the initial calibration
  // display

  This->m_nxWin->show();
  This->stateMachine();

  // Loop until calibration completes or we have been requested to terminate

  while (This->m_calthread != CALTHREAD_STOPREQUESTED &&
         This->m_calphase != CALPHASE_COMPLETE)
    {
      // Wait for the next raw touchscreen input (or possibly a signal)

      struct STouchSample sample;
      while (!This->waitTouchSample(sample) &&
              This->m_calthread == CALTHREAD_RUNNING);

      // Then process the raw touchscreen input

      if (This->m_calthread == CALTHREAD_RUNNING)
        {
          This->touchscreenInput(sample);
        }
    }

#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
  // Hide the message

  This->m_text->setText("");
  This->m_text->enableDrawing();
  This->m_text->redraw();
  This->m_text->disableDrawing();
#endif

  // Perform the final steps of calibration

  This->finishCalibration();

  // Then destroy the window and free resources

  This->destroy();

  twminfo("Terminated: m_calthread=%d\n", (int)This->m_calthread);
  return (FAR void *)0;
}

/**
 * Get exclusive access data shared across threads.
 *
 * @return True is returned if the sample data was obtained without error.
 */

bool CCalibration::exclusiveAccess(void)
{
  int ret;

  do
    {
      ret = sem_wait(&m_exclSem);
      if (ret < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode == EINTR || errcode == ECANCELED);
          if (errcode != EINTR)
            {
              return false;
            }
        }
    }
  while (ret < 0);

  return true;
}

/**
 * Wait for touchscreen next sample data to become available.
 *
 * @param sample Snapshot of last touchscreen sample
 * @return True is returned if the sample data was obtained without error.
 */

bool CCalibration::waitTouchSample(struct STouchSample &sample)
{
  // Loop until we get the sample (or something really bad happens)

  for (; ; )
    {
      // Is the sample valid?

      if (!m_sample.valid)
        {
          // No, wait for new data to be posted

          int ret = sem_wait(&m_synchSem);
          if (ret < 0)
            {
              // EINTR is not an error, it just means that we were
              // interrupted by a signal.  Everything else is bad news.

              int errcode = errno;
              DEBUGASSERT(errcode == EINTR || errcode == ECANCELED);
              if (errcode != EINTR)
                {
                  return false;
                }
            }
        }

      // The samples is valid!  Get exclusive access and check again

      else if (exclusiveAccess())
        {
          if (m_sample.valid)
            {
              // We have good data.  Return it to the caller

              sample         = m_sample;
              m_sample.valid = false;

              sem_post(&m_exclSem);
              return true;
            }
          else
            {
              // No.. What happened?

              sem_post(&m_exclSem);
            }
        }
    }

  return true;
}

/**
 * Accumulate and average touch sample data
 *
 * @param average.  When the averaged data is available, return it here
 * @return True: Average data is available; False: Need to collect more samples
 */

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
bool CCalibration::averageSamples(struct nxgl_point_s &average)
{
  // Have we started collecting sample data? */

  // Save the sample data

  twminfo("Sample %d: Touch x: %d y: %d\n",
          m_nsamples + 1, m_touchPos.x, m_touchPos.y);

  m_sampleData[m_nsamples].x = m_touchPos.x;
  m_sampleData[m_nsamples].y = m_touchPos.y;
  m_nsamples++;

  // Have all of the samples been collected?

  if (m_nsamples <  CONFIG_TWM4NX_CALIBRATION_NSAMPLES)
    {
      // Not yet... return false

      return false;
    }

  // Yes.. we have all of the samples
#ifdef CONFIG_TWM4NX_CALIBRATION_NSAMPLES
  // Discard the smallest X and Y values

  int xValue = INT_MAX;
  int xIndex = -1;

  int yValue = INT_MAX;
  int yIndex = -1;

  for (int i = 0; i < CONFIG_TWM4NX_CALIBRATION_NSAMPLES; i++)
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

  m_sampleData[xIndex].x = m_sampleData[CONFIG_TWM4NX_CALIBRATION_NSAMPLES-1].x;
  m_sampleData[yIndex].y = m_sampleData[CONFIG_TWM4NX_CALIBRATION_NSAMPLES-1].y;

  // Discard the largest X and Y values

  xValue = -1;
  xIndex = -1;

  yValue = -1;
  yIndex = -1;

  for (int i = 0; i < CONFIG_TWM4NX_CALIBRATION_NSAMPLES-1; i++)
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

  m_sampleData[xIndex].x = m_sampleData[CONFIG_TWM4NX_CALIBRATION_NSAMPLES-2].x;
  m_sampleData[yIndex].y = m_sampleData[CONFIG_TWM4NX_CALIBRATION_NSAMPLES-2].y;
#endif

#if TWM4NX_CALIBRATION_NAVERAGE > 1
  // Calculate the average of the remaining values

  long xaccum = 0;
  long yaccum = 0;

  for (int i = 0; i < TWM4NX_CALIBRATION_NAVERAGE; i++)
    {
      xaccum += m_sampleData[i].x;
      yaccum += m_sampleData[i].y;
    }

  average.x = xaccum / TWM4NX_CALIBRATION_NAVERAGE;
  average.y = yaccum / TWM4NX_CALIBRATION_NAVERAGE;
#else
  average.x = m_sampleData[0].x;
  average.y = m_sampleData[0].y;
#endif

  twminfo("Average: Touch x: %d y: %d\n", average.x, average.y);
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
  twminfo("Old m_calphase=%d\n", m_calphase);

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
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

              m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR;
              m_text->setText(GAgainMsg);
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

  // Get the size of the fullscreen window

  struct nxgl_size_s windowSize;
  if (!m_nxWin->getSize(&windowSize))
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

          NXWidgets::CWidgetControl *control = m_nxWin->getWidgetControl();

          // Get the CCGraphicsPort instance for this window

          NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

          // Fill the entire window with the background color

          port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                               CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR);

          // Then draw the first calibration screen

          m_screenInfo.pos.x           = CALIBRATION_LEFTX;
          m_screenInfo.pos.y           = CALIBRATION_TOPY;
          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
          m_text->setText(GTouchMsg);
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

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
          m_calibData[CALIB_UPPER_LEFT_INDEX].x = average.x;
          m_calibData[CALIB_UPPER_LEFT_INDEX].y = average.y;
#else
          m_calibData[CALIB_UPPER_LEFT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_UPPER_LEFT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the background
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
          showCalibration();

          // Then draw the next calibration screen

          m_screenInfo.pos.x           = CALIBRATION_RIGHTX;
          m_screenInfo.pos.y           = CALIBRATION_TOPY;
          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
          m_text->setText(GTouchMsg);
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

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
          m_calibData[CALIB_UPPER_RIGHT_INDEX].x = average.x;
          m_calibData[CALIB_UPPER_RIGHT_INDEX].y = average.y;
#else
          m_calibData[CALIB_UPPER_RIGHT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_UPPER_RIGHT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the backgro9und
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
          showCalibration();

          // Then draw the next calibration screen

          m_screenInfo.pos.x           = CALIBRATION_RIGHTX;
          m_screenInfo.pos.y           = CALIBRATION_BOTTOMY;
          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
          m_text->setText(GTouchMsg);
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

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
          m_calibData[CALIB_LOWER_RIGHT_INDEX].x = average.x;
          m_calibData[CALIB_LOWER_RIGHT_INDEX].y = average.y;
#else
          m_calibData[CALIB_LOWER_RIGHT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_LOWER_RIGHT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the backgro9und
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
          showCalibration();

          // Then draw the next calibration screen

          m_screenInfo.pos.x           = CALIBRATION_LEFTX;
          m_screenInfo.pos.y           = CALIBRATION_BOTTOMY;
          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_LINECOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_CIRCLECOLOR;
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
          m_text->setText(GTouchMsg);
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

#ifdef CONFIG_TWM4NX_CALIBRATION_AVERAGE
          m_calibData[CALIB_LOWER_LEFT_INDEX].x = average.x;
          m_calibData[CALIB_LOWER_LEFT_INDEX].y = average.y;
#else
          m_calibData[CALIB_LOWER_LEFT_INDEX].x = m_touchPos.x;
          m_calibData[CALIB_LOWER_LEFT_INDEX].y = m_touchPos.y;
#endif

          // Clear the previous screen by re-drawing it using the background
          // color.  That is much faster than clearing the whole display

          m_screenInfo.lineColor       = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
          m_screenInfo.circleFillColor = CONFIG_TWM4NX_CALIBRATION_BACKGROUNDCOLOR;
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
          m_text->setText(GTouchMsg);
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

  twminfo("New m_calphase=%d Screen x: %d y: %d\n",
          m_calphase, m_screenInfo.pos.x, m_screenInfo.pos.y);
}

/**
 * Presents the next calibration screen
 *
 * @param screenInfo Describes the next calibration screen
 */

void CCalibration::showCalibration(void)
{
  // Get the widget control associated with the full screen window

  NXWidgets::CWidgetControl *control =  m_nxWin->getWidgetControl();

  // Get the CCGraphicsPort instance for this window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the full screen window

  struct nxgl_size_s windowSize;
  if (!m_nxWin->getSize(&windowSize))
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

#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
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
  // Hide the window

  m_nxWin->hide();

  // Did we finish calibration successfully?

  if (m_calphase == CALPHASE_COMPLETE)
    {
      // Yes... Get the final Calibration data

      struct SCalibrationData caldata;
      if (createCalibrationData(caldata))
        {
#ifdef CONFIG_TWM4NX_TOUCHSCREEN_CONFIGDATA
          // Save the new calibration data.  The saved calibration
          // data may be used to avoided recalibrating in the future.

          int ret = platform_setconfig(CONFIGDATA_TSCALIBRATION, 0,
                                       (FAR const uint8_t *)&caldata,
                                       sizeof(struct SCalibrationData));
          if (ret != 0)
            {
              twmerr("ERROR: Failed to save calibration data\n");
            }
#endif
          // And provide the calibration data to CInput, enabling
          // touchscreen processing

          FAR CInput *cinput = m_twm4nx->getInput();

          cinput->setCalibrationData(caldata);
          cinput->enableCalibration(true);
        }
    }

  // And set the terminated stated

  m_calthread = CALTHREAD_TERMINATED;
}

/**
 * Stop the application.
 */

void CCalibration::stop(void)
{
  twminfo("Stopping calibration: m_calthread=%d\n", (int)m_calthread);

  // Was the calibration thread created?

  if (m_thread != 0)
    {
      // Is the calibration thread running?

      if (m_calthread == CALTHREAD_RUNNING)
        {
          // The main thread is stuck waiting for the next touchscreen input...
          // We can signal that we would like the thread to stop, but we will be
          // stuck here until the next touch

          m_calthread = CALTHREAD_STOPREQUESTED;

          // Try to wake up the calibration thread so that it will see our
          // termination request

          twminfo("Stopping calibration: m_calthread=%d\n", (int)m_calthread);
          pthread_kill(m_thread, CONFIG_TWM4NX_CALIBRATION_SIGNO);

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
 * message.  When the block message is received by CWindowEvent,
 * it will send the destroy message to the calibration window task which
 * will, finally, safely delete the application.
 */

void CCalibration::destroy(void)
{
#ifdef CONFIG_TWM4NX_CALIBRATION_MESSAGES
  // Destroy widgets

  destroyWidgets();
#endif

  // Close the calibration window... but not yet.  Send the blocked message.
  // The actual termination will no occur until the NX server drains all of
  // the message events.  We will get the EVENT_WINDOW_DELETE event at that
  // point

  NXWidgets::CWidgetControl *control = m_nxWin->getWidgetControl();
  nx_block(control->getWindowHandle(), (FAR void *)m_nxWin);
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
  // Get the size of the fullscreen window

  struct nxgl_size_s windowSize;
  if (!m_nxWin->getSize(&windowSize))
    {
      twmerr("ERROR: NXWidgets::INxWindow::getSize failed\n");
      return false;
    }

#ifdef CONFIG_TWM4NX_CALIBRATION_ANISOTROPIC
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

  twminfo("Left slope: %6.2f offset: %6.2f\n", data.left.slope, data.left.offset);

  topX               = (float)m_calibData[CALIB_UPPER_RIGHT_INDEX].x;
  bottomX            = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].x;

  topY               = (float)m_calibData[CALIB_UPPER_RIGHT_INDEX].y;
  bottomY            = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].y;

  data.right.slope   = (bottomX - topX) / (bottomY - topY);
  data.right.offset  = topX - topY * data.right.slope;

  twminfo("Right slope: %6.2f offset: %6.2f\n", data.right.slope, data.right.offset);

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

  twminfo("Top slope: %6.2f offset: %6.2f\n", data.top.slope, data.top.offset);

  leftX              = (float)m_calibData[CALIB_LOWER_LEFT_INDEX].x;
  rightX             = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].x;

  leftY              = (float)m_calibData[CALIB_LOWER_LEFT_INDEX].y;
  rightY             = (float)m_calibData[CALIB_LOWER_RIGHT_INDEX].y;

  data.bottom.slope  = (rightY - leftY) / (rightX - leftX);
  data.bottom.offset = leftY - leftX * data.bottom.slope;

  twminfo("Bottom slope: %6.2f offset: %6.2f\n", data.bottom.slope, data.bottom.offset);

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

  twminfo("New xSlope: %08" PRIx32 " xOffset: %08" PRIx32 "\n",
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

  twminfo("New ySlope: %08" PRIx32 " yOffset: %08" PRIx32 "\n",
          data.ySlope, data.yOffset);
#endif

  return true;
}

/////////////////////////////////////////////////////////////////////////////
// CCalibrationFactory Method Implementations
/////////////////////////////////////////////////////////////////////////////

/**
 * CCalibrationFactory Initializer.  Performs parts of the instance
 * construction that may fail.  In this implementation, it will
 * initialize the NSH library and register an menu item in the
 * Main Menu.
 */

bool CCalibrationFactory::initialize(FAR CTwm4Nx *twm4nx)
{
  // Save the session instance

  m_twm4nx = twm4nx;

  // Register an entry with the Main menu.  When selected, this will
  // create a new instance of the touchscreen calibration.

  FAR CMainMenu *cmain = twm4nx->getMainMenu();
  bool success = cmain->addApplication(this);

#ifdef CONFIG_TW4NX_STARTUP_CALIB
  if (success)
    {
      // Show the calibration thread at start-up (and later if selected
      // from the Main Menu)

      startFunction();
    }
#endif

  return success;
}

/**
 * Handle CCalibrationFactory events.  This overrides a method from
 * CTwm4NXEvent
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CCalibrationFactory::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_CALIB_START:  // Main menu selection
        startFunction();       // Create a new CCalibration instance
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Create and start a new instance of CCalibration.
 */

bool CCalibrationFactory::startFunction(void)
{
  // Instantiate the Nxterm application, providing only the session session
  // instance to the constructor

  CCalibration *calib = new CCalibration(m_twm4nx);
  if (!calib)
    {
      twmerr("ERROR: Failed to instantiate CCalibration\n");
      return false;
    }

  // Initialize the Calibration application

  if (!calib->initialize())
    {
      twmerr("ERROR: Failed to initialize CCalibration instance\n");
      delete calib;
      return false;
    }

  // Start the Calibration application instance

  if (!calib->run())
    {
      twmerr("ERROR: Failed to start the Calibration application\n");
      delete calib;
      return false;
    }

  return true;
}
