/********************************************************************************************
 * NxWidgets/nxwm/src/cmediaplayer.cxx
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
 *           Gregory Nutt <gnutt@nuttx.org>
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

  m_taskbar        = taskbar;
  m_window         = window;

  // Nullify widgets that will be instantiated when the window is started

  m_listbox        = (NXWidgets::CListBox     *)0;
  m_font           = (NXWidgets::CNxFont      *)0;
  m_play           = (NXWidgets::CImage       *)0;
  m_pause          = (NXWidgets::CImage       *)0;
  m_rewind         = (NXWidgets::CStickyImage *)0;
  m_fforward       = (NXWidgets::CStickyImage *)0;
  m_volume         = (NXWidgets::CGlyphSliderHorizontal *)0;

  // Nullify bitmaps that will be instantiated when the window is started

  m_playBitmap     = (NXWidgets::CRlePaletteBitmap *)0;
  m_pauseBitmap    = (NXWidgets::CRlePaletteBitmap *)0;
  m_rewindBitmap   = (NXWidgets::CRlePaletteBitmap *)0;
  m_fforwardBitmap = (NXWidgets::CRlePaletteBitmap *)0;
  m_volumeBitmap   = (NXWidgets::CRlePaletteBitmap *)0;

  // Initial state is stopped

  m_state          = MPLAYER_STOPPED;
  m_prevState      = MPLAYER_STOPPED;
  m_pending        = PENDING_NONE;
  m_fileIndex      = -1;

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

  if (m_listbox)
    {
      delete m_listbox;
    }

  if (m_font)
    {
      delete m_font;
    }

  if (m_play)
    {
      delete m_play;
    }

  if (m_pause)
    {
      delete m_pause;
    }

  if (m_rewind)
    {
      delete m_rewind;
    }

  if (m_fforward)
    {
      delete m_fforward;
    }

  if (m_volume)
    {
      delete m_volume;
    }

  // Destroy bitmaps

  if (m_playBitmap)
    {
      delete m_playBitmap;
    }

  if (m_pauseBitmap)
    {
      delete m_pauseBitmap;
    }

  if (m_rewindBitmap)
    {
      delete m_rewindBitmap;
    }

  if (m_fforwardBitmap)
    {
      delete m_fforwardBitmap;
    }

  if (m_volumeBitmap)
    {
      delete m_volumeBitmap;
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

  if (!m_listbox)
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
  // Just disable further drawing on all widgets

  m_listbox->disableDrawing();
  m_play->disableDrawing();
  m_pause->disableDrawing();
  m_rewind->disableDrawing();
  m_fforward->disableDrawing();
  m_volume->disableDrawing();
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
 * CTaskbar when the application window must be displayed.
 */

void CMediaPlayer::redraw(void)
{
  // Get the widget control associated with the application window

  NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

  // Get the CCGraphicsPort instance for this window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Fill the entire window with the background color

  port->drawFilledRect(0, 0, m_windowSize.w, m_windowSize.h,
                       CONFIG_NXWM_MEDIAPLAYER_BACKGROUNDCOLOR);

  // Redraw all widgets

  redrawWidgets();
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
 * Open a media file for playing.  Called after a file has been selected
 * from the list box.
 */

bool CMediaPlayer::openMediaFile(const NXWidgets::CListBoxDataItem *item)
{
#warning Missing logic
  return true;
}

/**
 * Close media file.  Called when a new media file is selected, when a media file is de-selected, or when destroying the media player instance.
    */

void CMediaPlayer::closeMediaFile(void)
{
#warning Missing logic
}

/**
 * Select the geometry of the media player given the current window size.
 */

void CMediaPlayer::setGeometry(void)
{
  // Recover the NXTK window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the size of the window

  (void)window->getSize(&m_windowSize);
}

/**
 * Load media files into the list box.
 */

inline bool CMediaPlayer::showMediaFiles(const char *mediaPath)
{
  // Remove any filenames already in the list box

  m_listbox->removeAllOptions();

#if 0
  // Open the media path directory
  // Read each directory entry
  // Add the directory entry to the list box
  // Close the directory
#else
#  warning "Missing Logic"

  // Just add a couple of dummy files for testing

  m_listbox->addOption("File 1", 0,
                      CONFIG_NXWIDGETS_DEFAULT_ENABLEDTEXTCOLOR,
                      CONFIG_NXWIDGETS_DEFAULT_BACKGROUNDCOLOR,
                      CONFIG_NXWIDGETS_DEFAULT_SELECTEDTEXTCOLOR,
                      CONFIG_NXWM_DEFAULT_SELECTEDBACKGROUNDCOLOR);
  m_listbox->addOption("File 2", 1,
                      CONFIG_NXWIDGETS_DEFAULT_ENABLEDTEXTCOLOR,
                      CONFIG_NXWIDGETS_DEFAULT_BACKGROUNDCOLOR,
                      CONFIG_NXWIDGETS_DEFAULT_SELECTEDTEXTCOLOR,
                      CONFIG_NXWM_DEFAULT_SELECTEDBACKGROUNDCOLOR);
#endif

  // Sort the file names in alphabetical order

  m_listbox->sort();
  return true;
}

/**
 * Create the media player widgets.  Only start as part of the application
 * start method.
 */

bool CMediaPlayer::createPlayer(void)
{
  // Select a font for the media player

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

  // Work out all of the vertical placement first.  In order to do that, we
  // will need create all of the bitmaps first so that we an use the bitmap
  // height in the calculation.

  m_playBitmap     = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_PLAY_ICON);
  m_pauseBitmap    = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_PAUSE_ICON);
  m_rewindBitmap   = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_REW_ICON);
  m_fforwardBitmap = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_FWD_ICON);
  m_volumeBitmap   = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_VOL_ICON);

  if (!m_playBitmap || !m_pauseBitmap || !m_rewindBitmap ||
      !m_fforwardBitmap || !m_volumeBitmap)
    {
      gdbg("ERROR: Failed to one or more bitmaps\n");
      return false;
    }

  // Control image height.  Use the same height for all images

  nxgl_coord_t controlH = m_playBitmap->getHeight();

  if (controlH < m_pauseBitmap->getHeight())
    {
      controlH = m_pauseBitmap->getHeight();
    }

  if (controlH < m_rewindBitmap->getHeight())
    {
      controlH = m_rewindBitmap->getHeight();
    }

  if (controlH < m_fforwardBitmap->getHeight())
    {
      controlH = m_fforwardBitmap->getHeight();
    }

  controlH += 8;

  // Place the volume slider at a comfortable distance from the bottom of
  // the display

  nxgl_coord_t volumeTop = m_windowSize.h - m_volumeBitmap->getHeight() -
                           CONFIG_NXWM_MEDIAPLAYER_YSPACING;

  // Place the player controls just above that.  The list box will then end
  // just above the controls.

  nxgl_coord_t controlTop = volumeTop - controlH -
                            CONFIG_NXWM_MEDIAPLAYER_YSPACING;

  // The list box will then end just above the controls.  The end of the
  // list box is the same as its height because the origin is zero.

  nxgl_coord_t listHeight = controlTop - CONFIG_NXWM_MEDIAPLAYER_YSPACING;

  // Create a list box to show media file selections.
  // Note that the list box will extend all of the way to the edges of the
  // display and is only limited at the bottom by the player controls.
  // REVISIT: This should be a scrollable list box

  m_listbox = new NXWidgets::CListBox(control, 0, 0,  m_windowSize.w, listHeight);
  if (!m_listbox)
    {
      gdbg("ERROR: Failed to create CListBox\n");
      return false;
    }

  // Configure the list box

  m_listbox->disableDrawing();
  m_listbox->setAllowMultipleSelections(false);
  m_listbox->setFont(m_font);
  m_listbox->setBorderless(false);

  // Register to get events when a new file is selected from the list box

  m_listbox->addWidgetEventHandler(this);

  // Show the media files that are available for playing

  showMediaFiles(CONFIG_NXWM_MEDIAPLAYER_MEDIAPATH);

  // Control image widths.
  // Image widths will depend on if the images will be bordered or not

  nxgl_coord_t playControlW;
  nxgl_coord_t rewindControlW;
  nxgl_coord_t fforwardControlW;

#ifdef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  // Use the same width for all control images.  Set the width to the width
  // of the widest image

  nxgl_coord_t imageW = m_playBitmap->getWidth();

  if (imageW < m_pauseBitmap->getWidth())
    {
      imageW = m_pauseBitmap->getWidth();
    }

  if (imageW < m_rewindBitmap->getWidth())
    {
      imageW = m_rewindBitmap->getWidth();
    }

  if (imageW < m_fforwardBitmap->getWidth())
    {
      imageW = m_fforwardBitmap->getWidth();
    }

  // Add little space around the bitmap and use this width for all images

  imageW          += 8;
  playControlW     = imageW;
  rewindControlW   = imageW;
  fforwardControlW = imageW;

#else
  // Use the bitmap image widths for the image widths (plus a bit)

  playControlW     = m_playBitmap->getWidth() + 8;
  rewindControlW   = m_rewindBitmap->getWidth()  + 8;
  fforwardControlW = m_fforwardBitmap->getWidth()  + 8;

  // The Play and Pause images should be the same width.  But just
  // in case, pick the larger width.

  nxgl_coord_t pauseControlW = m_pauseBitmap->getWidth() + 8;
  if (playControlW < pauseControlW)
    {
      playControlW = pauseControlW;
    }
#endif

  // Create the Play image

  nxgl_coord_t playControlX = (m_windowSize.w >> 1) - (playControlW >> 1);

  m_play = new NXWidgets::
      CImage(control, playControlX, controlTop, playControlW, controlH,
             m_playBitmap);

  if (!m_play)
    {
      gdbg("ERROR: Failed to create play control\n");
      return false;
    }

  // Configure the Play image

  m_play->disableDrawing();
  m_play->alignHorizontalCenter();
  m_play->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_play->setBorderless(true);
#else
  m_play->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Play image

  m_play->addWidgetEventHandler(this);

  // Create the Pause image (at the same position ans size as the Play image)

  m_pause = new NXWidgets::
      CImage(control, playControlX, controlTop, playControlW, controlH,
             m_pauseBitmap);

  if (!m_pause)
    {
      gdbg("ERROR: Failed to create pause control\n");
      return false;
    }

  // Configure the Pause image (hidden and disabled initially)

  m_pause->disableDrawing();
  m_pause->alignHorizontalCenter();
  m_pause->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_pause->setBorderless(true);
#else
  m_pause->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Pause image

  m_pause->addWidgetEventHandler(this);

  // Create the Rewind image

  nxgl_coord_t rewControlX = playControlX - rewindControlW -
                             CONFIG_NXWM_MEDIAPLAYER_XSPACING;

  m_rewind = new NXWidgets::
      CStickyImage(control, rewControlX, controlTop, rewindControlW,
                   controlH, m_rewindBitmap);

  if (!m_rewind)
    {
      gdbg("ERROR: Failed to create rewind control\n");
      return false;
    }

  // Configure the Rewind image

  m_rewind->disableDrawing();
  m_rewind->alignHorizontalCenter();
  m_rewind->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_rewind->setBorderless(true);
#else
  m_rewind->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Rewind image

  m_rewind->addWidgetEventHandler(this);

  // Create the Forward Image

  nxgl_coord_t fwdControlX = playControlX + playControlW +
                             CONFIG_NXWM_MEDIAPLAYER_XSPACING;

  m_fforward = new NXWidgets::
      CStickyImage(control, fwdControlX, controlTop, fforwardControlW,
                   controlH, m_fforwardBitmap);

  if (!m_fforward)
    {
      gdbg("ERROR: Failed to create fast forward control\n");
      return false;
    }

  // Configure the Forward image

  m_fforward->disableDrawing();
  m_fforward->alignHorizontalCenter();
  m_fforward->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_fforward->setBorderless(true);
#else
  m_fforward->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Forward image

  m_fforward->addWidgetEventHandler(this);

  // Create the Volume control

  uint32_t volumeControlX = (9 * m_windowSize.w) >> 8;

  m_volume = new NXWidgets::
      CGlyphSliderHorizontal(control,
                             (nxgl_coord_t)volumeControlX,
                             volumeTop,
                             (nxgl_coord_t)(m_windowSize.w - 2 * volumeControlX),
                             m_volumeBitmap->getHeight() + 4, m_volumeBitmap,
                             MKRGB(63, 90,192));

  if (!m_volume)
    {
      gdbg("ERROR: Failed to create volume control\n");
      return false;
    }

  // Configure the volume control

  m_volume->disableDrawing();
  m_volume->setMinimumValue(0);
  m_volume->setMaximumValue(100);
  m_volume->setValue(15);

  // Register to get events from the value changes in the volume slider

  m_volume->addWidgetEventHandler(this);

  // Make sure that all widgets are setup for the STOPPED state.  Among other this,
  // this will enable drawing in the play widget (only)

  setMediaPlayerState(MPLAYER_STOPPED);

  // Set the volume level

  setVolumeLevel();

  // Enable drawing in the list box, rewind, fast-forward and drawing widgets.

  m_listbox->enableDrawing();
  m_rewind->enableDrawing();
  m_fforward->enableDrawing();
  m_volume->enableDrawing();

  // And redraw all of the widgets that are enabled

  redraw();
  return true;
}

/**
 * Called when the window minimize image is pressed.
 */

void CMediaPlayer::minimize(void)
{
  m_taskbar->minimizeApplication(static_cast<IApplication*>(this));
}

/**
 * Called when the window close image is pressed.
 */

void CMediaPlayer::close(void)
{
  m_taskbar->stopApplication(static_cast<IApplication*>(this));
}

/**
 * Redraw all widgets.  Called from redraw() and also on any state
 * change.
 *
 * @param state The new state to enter.
 */

void CMediaPlayer::redrawWidgets(void)
{
  // Redraw widgets.  We have to re-enable drawing all all widgets since
  // drawing was disabled by the hide() method.

  m_listbox->enableDrawing();
  m_listbox->redraw();

  // Only one of the Play and Pause images should have drawing enabled.

  if (m_state != MPLAYER_STOPPED && m_prevState == MPLAYER_PLAYING)
    {
      // Playing... show the pause button
      // REVISIT:  Really only available if there is a selected file in the list box

      m_pause->enableDrawing();
      m_pause->redraw();
    }
  else
    {
      // Paused or Stopped... show the play button

      m_play->enableDrawing();
      m_play->redraw();
    }

  // Rewind and play buttons are only shown if we are not STOPPED

  if (m_state != MPLAYER_STOPPED)
    {
      m_rewind->enableDrawing();
      m_rewind->redraw();

      m_fforward->enableDrawing();
      m_fforward->redraw();
    }

  m_volume->enableDrawing();
  m_volume->redraw();
}

/**
 * Transition to a new media player state.
 *
 * @param state The new state to enter.
 */

void CMediaPlayer::setMediaPlayerState(enum EMediaPlayerState state)
{
  // Stop drawing on all widgets

  stop();

  // Handle according to the new state

  switch (state)
    {
    case MPLAYER_STOPPED:    // Initial state.  Also the state after playing completes
      m_state     = MPLAYER_STOPPED;
      m_prevState = MPLAYER_PLAYING;

      // List box is enabled and ready for file selection

      m_listbox->enable();

      // Play image enabled and ready to start playing
      // REVISIT:  Really only available if there is a selected file in the list box

      m_play->enable();
      m_play->show();

      // Pause image is disabled and hidden

      m_pause->disable();
      m_pause->hide();

      // Fast forward image is disabled

      m_fforward->disable();
      m_fforward->setStuckSelection(false);

      // Rewind image is disabled

      m_rewind->disable();
      m_rewind->setStuckSelection(false);

      m_volume->enable();
      break;

    case MPLAYER_PLAYING:    // Playing a media file
      m_state     = MPLAYER_PLAYING;
      m_prevState = MPLAYER_PLAYING;

      // List box is not available while playing

      m_listbox->disable();

      // Play image hidden and disabled

      m_play->disable();
      m_play->hide();

      // Pause image enabled and ready to pause playing

      m_pause->enable();
      m_pause->show();

      // Fast forward image is enabled and ready for use

      m_fforward->enable();
      m_fforward->setStuckSelection(false);

      // Rewind image is enabled and ready for use

      m_rewind->enable();
      m_rewind->setStuckSelection(false);
      break;

    case MPLAYER_PAUSED:     // Playing a media file but paused
      m_state     = MPLAYER_PAUSED;
      m_prevState = MPLAYER_PAUSED;

      // List box is enabled a ready for file selection

      m_listbox->enable();

      // Play image enabled and ready to resume playing

      m_play->enable();
      m_play->show();

      // Pause image is disabled and hidden

      m_pause->disable();
      m_pause->hide();

      // Fast forward image is enabled and ready for use

      m_fforward->setStuckSelection(false);

      // Rewind image is enabled and ready for use

      m_rewind->setStuckSelection(false);
      break;

    case MPLAYER_FFORWARD:   // Fast forwarding through a media file */
      m_state = MPLAYER_FFORWARD;

      // List box is not available while fast forwarding

      m_listbox->disable();

      if (m_prevState == MPLAYER_PLAYING)
        {
          // Play image hidden and disabled

          m_play->disable();
          m_play->hide();

          // Pause image enabled and ready to stop fast forwarding

          m_pause->enable();
          m_pause->show();
        }
      else
        {
          // Play image enabled and ready to stop fast forwarding

          m_play->enable();
          m_play->show();

          // Pause image is hidden and disabled

          m_pause->disable();
          m_pause->hide();
        }

      // Fast forward image is enabled, highlighted and ready for use

      m_fforward->setStuckSelection(true);

      // Rewind is enabled and ready for use

      m_rewind->setStuckSelection(false);
      break;

    case MPLAYER_FREWIND:    // Rewinding a media file
      m_state = MPLAYER_FREWIND;

      // List box is not available while rewinding

      m_listbox->disable();

      if (m_prevState == MPLAYER_PLAYING)
        {
          // Play image hidden and disabled

          m_play->disable();
          m_play->hide();

          // Pause image enabled and ready to stop rewinding

          m_pause->enable();
          m_pause->show();
        }
      else
        {
          // Play image enabled and ready to stop rewinding

          m_play->enable();
          m_play->show();

          // Pause image is hidden and disabled

          m_pause->disable();
          m_pause->hide();
        }

      // Fast forward image is enabled and ready for use

      m_fforward->setStuckSelection(false);

      // Rewind image is enabled, highlighted, and ready for use

      m_rewind->setStuckSelection(true);
      break;

    default:
      break;
    }

  // Re-enable drawing and redraw all widgets for the new state

  redrawWidgets();
}

/**
 * Set the new volume level based on the position of the volume slider.
 */

void CMediaPlayer::setVolumeLevel(void)
{
  // Current volume level values.  This is already pre-scaled in the range 0-100

  m_level = m_volume->getValue();

  // Now, provide the new volume setting to the NX Player
#warning Missing logic
}

/**
 * Handle a widget action event.  For this application, that means image
 * pre-release events.
 *
 * @param e The event data.
 */

void CMediaPlayer::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Check if the Play image was clicked

  if (m_play->isClicked() && m_state != MPLAYER_PLAYING)
    {
      // Just arm the state change now, but don't do anything until the
      // release occurs.  Trying to do the state change before the NxWidgets
      // release processing completes causes issues.

      m_pending = PENDING_PLAY_RELEASE;
    }

  // These only make sense in non-STOPPED states

  if (m_state != MPLAYER_STOPPED)
    {
      // Check if the Pause image was clicked

      if (m_pause->isClicked() && m_state != MPLAYER_PAUSED)
        {
         // Just arm the state change now, but don't do anything until the
         // release occurs.  Trying to do the state change before the NxWidgets
         // release processing completes causes issues.

          m_pending = PENDING_PAUSE_RELEASE;
        }

      // Check if the rewind image was clicked

      if (m_rewind->isClicked())
        {
          // Were we already rewinding?

          if (m_state == MPLAYER_FREWIND)
            {
              // Yes.. then revert to the previous play/pause state
              // REVISIT: Or just increase rewind speed?

              setMediaPlayerState(m_prevState);
            }

          // We should not be stopped here, but let's check anyway

          else if (m_state != MPLAYER_STOPPED)
            {
              // Start rewinding

              setMediaPlayerState(MPLAYER_FREWIND);
            }
        }

      // Check if the fast forward image was clicked

      if (m_fforward->isClicked())
        {
          // Were we already fast forwarding?

          if (m_state == MPLAYER_FFORWARD)
            {
              // Yes.. then revert to the previous play/pause state
              // REVISIT: Or just increase fast forward  speed?

              setMediaPlayerState(m_prevState);
            }

          // We should not be stopped here, but let's check anyway

          else if (m_state != MPLAYER_STOPPED)
            {
              // Start fast forwarding

              setMediaPlayerState(MPLAYER_FFORWARD);
            }
        }
    }

  // Check for new file selections from the list box

  int newFileIndex = m_listbox->getSelectedIndex();

  // Check if anything is selected

  if (newFileIndex < 0)
    {
      // Nothing is selected.. If we are not stopped, then stop now

      if (m_state != MPLAYER_STOPPED)
        {
          closeMediaFile();
          setMediaPlayerState(MPLAYER_STOPPED);
        }
    }

  // A media file is selected.  Were stopped before?

  else if (m_state == MPLAYER_STOPPED)
    {
      // Yes.. open the new media file and go to the PAUSE state

      if (!openMediaFile(m_listbox->getSelectedOption()))
        {
          m_fileIndex = -1;
          gdbg("openMediaFile failed\m");
        }
      else
        {
          m_fileIndex = newFileIndex;
          setMediaPlayerState(MPLAYER_PAUSED);
        }
    }

  // We already have a media file loaded.  Is it the same file?

  else if (m_fileIndex != newFileIndex)
    {
      // No.. It is a new file.  Close that media file, load the newly
      // selected file, and make sure that we are in the paused state
      // (that should already be the case)

      closeMediaFile();
      if (!openMediaFile(m_listbox->getSelectedOption()))
        {
          gdbg("openMediaFile failed\m");
          m_fileIndex = -1;
          setMediaPlayerState(MPLAYER_STOPPED);
        }
      else
        {
          m_fileIndex = newFileIndex;
          setMediaPlayerState(MPLAYER_PAUSED);
        }
    }
}

/**
 * Handle a widget release event.  Only the play and pause image release
 * are of interest.
 */

void CMediaPlayer::handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Check if the Play image was released

  if (m_pending == PENDING_PLAY_RELEASE && !m_play->isClicked())
    {
      // Now perform the delayed state change

      setMediaPlayerState(MPLAYER_PLAYING);
      m_pending = PENDING_NONE;
    }

  // Check if the Pause image was released

  else if (m_pending == PENDING_PAUSE_RELEASE && !m_pause->isClicked())
    {
      // Now perform the delayed state change

      setMediaPlayerState(MPLAYER_PAUSED);
      m_pending = PENDING_NONE;
    }
}

/**
 * Handle a widget release event when the widget WAS dragged outside of
 * its original bounding box.  Only the play and pause image release
 * are of interest.
 */

void CMediaPlayer::handleReleaseOutsideEvent(const NXWidgets::CWidgetEventArgs &e)
{
  handleReleaseEvent(e);
}

/**
 * Handle changes in the volume level.
 */

void CMediaPlayer::handleValueChangeEvent(const NXWidgets::CWidgetEventArgs &e)
{
  setVolumeLevel();
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
