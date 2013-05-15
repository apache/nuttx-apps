/********************************************************************************************
 * NxWidgets/nxwm/src/cmediaplayer.cxx
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
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
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <cstdio>
#include <debug.h>

#include "cwidgetcontrol.hxx"

#include "nxwmconfig.hxx"
#include "nxwmglyphs.hxx"
#include "cmediaplayer.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * Private Types
 ********************************************************************************************/

/********************************************************************************************
 * Private Data
 ********************************************************************************************/

/********************************************************************************************
 * Private Functions
 ********************************************************************************************/

/********************************************************************************************
 * CMediaPlayer Method Implementations
 ********************************************************************************************/

extern const struct NXWidgets::SRlePaletteBitmap CONFIG_NXWM_MEDIAPLAYER_ICON;

using namespace NxWM;

/**
 * CMediaPlayer constructor
 *
 * @param window.  The application window
 */

CMediaPlayer::CMediaPlayer(CTaskbar *taskbar, CApplicationWindow *window)
{
  // Save the constructor data

  m_taskbar = taskbar;
  m_window  = window;

  // Nullify widgets that will be instantiated when the window is started

  m_text    = (NXWidgets::CLabel       *)0;
  m_font    = (NXWidgets::CNxFont      *)0;

  // Add our personalized window label

  NXWidgets::CNxString myName = getName();
  window->setWindowLabel(myName);

  // Add our callbacks with the application window

  window->registerCallbacks(static_cast<IApplicationCallback *>(this));

  // Set the geometry of the media player

  setGeometry();
}

/**
 * CMediaPlayer destructor
 *
 * @param window.  The application window
 */

CMediaPlayer::~CMediaPlayer(void)
{
  // Destroy widgets

  if (m_text)
    {
      delete m_text;
    }

  if (m_font)
    {
      delete m_font;
    }

  // Although we didn't create it, we are responsible for deleting the
  // application window

  delete m_window;
}

/**
 * Each implementation of IApplication must provide a method to recover
 * the contained CApplicationWindow instance.
 */

IApplicationWindow *CMediaPlayer::getWindow(void) const
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

NXWidgets::IBitmap *CMediaPlayer::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MEDIAPLAYER_ICON);

  return bitmap;
}

/**
 * Get the name string associated with the application
 *
 * @return A copy if CNxString that contains the name of the application.
 */

NXWidgets::CNxString CMediaPlayer::getName(void)
{
  return NXWidgets::CNxString("Media Player");
}

/**
 * Start the application (perhaps in the minimized state).
 *
 * @return True if the application was successfully started.
 */

bool CMediaPlayer::run(void)
{
  // Create the widgets (if we have not already done so)

  if (!m_text)
    {
      // Create the widgets

      if (!createPlayer())
        {
          gdbg("ERROR: Failed to create widgets\n");
          return false;
        }
    }

  return true;
}

/**
 * Stop the application.
 */

void CMediaPlayer::stop(void)
{
  // Just disable further drawing

  m_text->disableDrawing();
}

/**
 * Destroy the application and free all of its resources.  This method
 * will initiate blocking of messages from the NX server.  The server
 * will flush the window message queue and reply with the blocked
 * message.  When the block message is received by CWindowMessenger,
 * it will send the destroy message to the start window task which
 * will, finally, safely delete the application.
 */

void CMediaPlayer::destroy(void)
{
  // Make sure that the widgets are stopped

  stop();

  // Block any further window messages

  m_window->block(this);
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but not at the top of the hierarchy
 */

void CMediaPlayer::hide(void)
{
  // Disable drawing and events

  stop();
}

/**
 * Redraw the entire window.  The application has been maximized or
 * otherwise moved to the top of the hierarchy.  This method is call from
 * CTaskbar when the application window must be displayed
 */

void CMediaPlayer::redraw(void)
{
  char buffer[24];

  snprintf(buffer, 24, "Comming soon!");

  // setText will perform the redraw as well

  m_text->setText(buffer);

  // Get the widget control associated with the application window

  NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

  // Get the CCGraphicsPort instance for this window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Fill the entire window with the background color

  port->drawFilledRect(0, 0, m_windowSize.w, m_windowSize.h,
                       CONFIG_NXWM_MEDIAPLAYER_BACKGROUNDCOLOR);

  // Enable and redraw widgets

  m_text->enableDrawing();
  m_text->redraw();

  m_rew->enableDrawing();
  m_rew->redraw();
  m_playPause->enableDrawing();
  m_playPause->redraw();
  m_fwd->enableDrawing();
  m_fwd->redraw();
  m_volume->enableDrawing();
  m_volume->redraw();
}

/**
 * Report of this is a "normal" window or a full screen window.  The
 * primary purpose of this method is so that window manager will know
 * whether or not it show draw the task bar.
 *
 * @return True if this is a full screen window.
 */

bool CMediaPlayer::isFullScreen(void) const
{
  return m_window->isFullScreen();
}

/**
 * Select the geometry of the calculator given the current window size.
 */

void CMediaPlayer::setGeometry(void)
{
  // Recover the NXTK window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the size of the window

  (void)window->getSize(&m_windowSize);

  // Get the size of the text box.  Same width as the m_keypad

  m_textSize.w   = m_windowSize.w - 10;
  m_textSize.h   = 36;

  // Now position the text box

  m_textPos.x = 5;
  m_textPos.y = 5;
}

/**
 * Create the calculator widgets.  Only start as part of the applicaiton
 * start method.
 */

bool CMediaPlayer::createPlayer(void)
{
  int   playControlX, playControlY;

  // Select a font for the calculator

  m_font = new NXWidgets::CNxFont((nx_fontid_e)CONFIG_NXWM_MEDIAPLAYER_FONTID,
                                  CONFIG_NXWM_DEFAULT_FONTCOLOR,
                                  CONFIG_NXWM_TRANSPARENT_COLOR);
  if (!m_font)
    {
      gdbg("ERROR failed to create font\n");
      return false;
    }

  // Get the widget control associated with the application window

  NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

  // Create a label to show some text.  A simple label is used
  // because the power of a text box is un-necessary in this application.

  m_text = new NXWidgets::CLabel(control,
                                 m_textPos.x, m_textPos.y,
                                 m_textSize.w, m_textSize.h,
                                 "0");
  if (!m_text)
    {
      gdbg("ERROR: Failed to create CLabel\n");
      return false;
    }

  // Align text on the left

  m_text->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_RIGHT);

  // Disable drawing and events until we are asked to redraw the window

  m_text->disableDrawing();
  m_text->setRaisesEvents(false);

  // Select the font

  m_text->setFont(m_font);

  // Create button for rewind

  playControlX = (m_windowSize.w >> 1) - 64;
  playControlY = m_windowSize.h - 64;

  // Create the Rewind Image

  NXWidgets::CRlePaletteBitmap *pRewBitmap = new NXWidgets::
      CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_REW_ICON);
  m_rew = new NXWidgets::CImage(control,
                                playControlX, playControlY,
                                32, 32,
                                pRewBitmap, 0);
  m_rew->setBorderless(true);

  // Create the Play Image

  pRewBitmap = new NXWidgets::
      CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_PLAY_ICON);
  m_playPause = new NXWidgets::CImage(control,
                                      playControlX + 32 + 16, playControlY,
                                      32, 32,
                                      pRewBitmap, 0);
  m_playPause->setBorderless(true);

  // Create the Forward Image

  pRewBitmap = new NXWidgets::
      CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_FWD_ICON);
  m_fwd = new NXWidgets::CImage(control,
                                playControlX + 32*3, playControlY,
                                32, 32,
                                pRewBitmap, 0);
  m_fwd->setBorderless(true);

  // Create the Volume control

  pRewBitmap = new NXWidgets::
      CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_VOL_ICON);
  m_volume = new NXWidgets::CGlyphSliderHorizontal(control,
                                                   10, m_windowSize.h - 30,
                                                   m_windowSize.w - 20, 26,
                                                   pRewBitmap, MKRGB( 63, 90,192));
  m_volume->setMinimumValue(0);
  m_volume->setMaximumValue(100);
  m_volume->setValue(15);

  return true;
}

/**
 * Called when the window minimize button is pressed.
 */

void CMediaPlayer::minimize(void)
{
  m_taskbar->minimizeApplication(static_cast<IApplication*>(this));
}

/**
 * Called when the window close button is pressed.
 */

void CMediaPlayer::close(void)
{
  m_taskbar->stopApplication(static_cast<IApplication*>(this));
}

/**
 * Handle a widget action event.  For CButtonArray, this is a button pre-
 * release event.
 *
 * @param e The event data.
 */

void CMediaPlayer::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  /* Nothing here yet!  Comming soon! */
}

/**
 * CMediaPlayerFactory Constructor
 *
 * @param taskbar.  The taskbar instance used to terminate the console
 */

CMediaPlayerFactory::CMediaPlayerFactory(CTaskbar *taskbar)
{
  m_taskbar = taskbar;
}

/**
 * Create a new instance of an CMediaPlayer (as IApplication).
 */

IApplication *CMediaPlayerFactory::create(void)
{
  // Call CTaskBar::openFullScreenWindow to create a application window for
  // the NxConsole application

  CApplicationWindow *window = m_taskbar->openApplicationWindow();
  if (!window)
    {
      gdbg("ERROR: Failed to create CApplicationWindow\n");
      return (IApplication *)0;
    }

  // Open the window (it is hot in here)

  if (!window->open())
    {
      gdbg("ERROR: Failed to open CApplicationWindow\n");
      delete window;
      return (IApplication *)0;
    }

  // Instantiate the application, providing the window to the application's
  // constructor

  CMediaPlayer *mediaPlayer = new CMediaPlayer(m_taskbar, window);
  if (!mediaPlayer)
    {
      gdbg("ERROR: Failed to instantiate CMediaPlayer\n");
      delete window;
      return (IApplication *)0;
    }

  return static_cast<IApplication*>(mediaPlayer);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CMediaPlayerFactory::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MEDIAPLAYER_ICON);

  return bitmap;
}
