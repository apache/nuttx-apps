/********************************************************************************************
 * NxWidgets/nxwm/src/cmediaplayer.cxx
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#include <sys/stat.h>

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <strings.h>
#include <dirent.h>
#include <debug.h>

#include "system/nxplayer.h"
#include <nuttx/audio/audio.h>

#include "cwidgetcontrol.hxx"

#include "nxwmconfig.hxx"
#include "nxwmglyphs.hxx"
#include "cmediaplayer.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/* We want debug output from this file if either audio or graphics debug is enabled. */

#if !defined(CONFIG_DEBUG_AUDIO) && !defined(CONFIG_DEBUG_GRAPHICS)
#  undef gerr
#  undef _info
#  ifdef CONFIG_CPP_HAVE_VARARGS
#    define gerr(x...)
#    define _info(x...)
#  else
#    define gerr  (void)
#    define _info (void)
#  endif
#endif

#define AUDIO_NSUBSAMPLES 4

/********************************************************************************************
 * Private Types
 ********************************************************************************************/

/********************************************************************************************
 * Private Data
 ********************************************************************************************/

static const uint8_t g_motionSteps[AUDIO_NSUBSAMPLES] =
{
  AUDIO_SUBSAMPLE_2X, AUDIO_SUBSAMPLE_4X, AUDIO_SUBSAMPLE_8X, AUDIO_SUBSAMPLE_16X
};

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
#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
  m_speed          = (NXWidgets::CLabel       *)0;
#endif

  // Nullify bitmaps that will be instantiated when the window is started

  m_playBitmap     = (NXWidgets::CRlePaletteBitmap *)0;
  m_pauseBitmap    = (NXWidgets::CRlePaletteBitmap *)0;
  m_rewindBitmap   = (NXWidgets::CRlePaletteBitmap *)0;
  m_fforwardBitmap = (NXWidgets::CRlePaletteBitmap *)0;
  m_volumeBitmap   = (NXWidgets::CRlePaletteBitmap *)0;

  // Initial state is stopped

  m_player         = (FAR struct nxplayer_s   *)0;
  m_state          = MPLAYER_STOPPED;
  m_prevState      = MPLAYER_STOPPED;
  m_pending        = PENDING_NONE;
  m_fileIndex      = -1;
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  m_level          = 0;
#endif
#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
  m_subSample      = 0;
#endif
  m_fileReady      = false;

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

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
  if (m_speed)
    {
      delete m_speed;
    }
#endif

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

  // Release the NxPlayer

  if (m_player)
    {
      nxplayer_release(m_player);
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
  // Configure the NxPlayer and create the widgets (if we have not already
  // done so)

  if (!m_player)
    {
      // Configure the NxPlayer library and player thread

      if (!configureNxPlayer())
        {
          gerr("ERROR: Failed to configure NxPlayer\n");
          return false;
        }

      // Create the widgets

      if (!createPlayer())
        {
          gerr("ERROR: Failed to create widgets\n");
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
#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
  m_speed->disableDrawing();
#endif
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
 * Get the full media file path and make ready for playing.  Called
 * after a file has been selected from the list box.
 */

bool CMediaPlayer::getMediaFile(const NXWidgets::CListBoxDataItem *item)
{
  // Get the full path to the file

  NXWidgets::CNxString newFilePath(CONFIG_NXWM_MEDIAPLAYER_MEDIAPATH "/");
  newFilePath.append(item->getText());

  // Make sure that this is a different name than the one we already have

  if (!m_fileReady || m_filePath.compareTo(newFilePath) != 0)
    {
      // It is a new file name
      // Get the path to the file as a regular C-style string

      NXWidgets::nxwidget_char_t *filePath =
        new NXWidgets::nxwidget_char_t[newFilePath.getAllocSize()];

      if (!filePath)
        {
          gerr("ERROR: Failed to allocate file path\n");
          return false;
        }

      newFilePath.copyToCharArray(filePath);

      // Verify that the file of this name exists and that it is a not
      // something weird (like a directory or a block device).
      //
      // REVISIT: Should check if read-able as well.

      struct stat buf;
      int ret = stat((FAR const char *)filePath, &buf);
      delete[] filePath;

      if (ret < 0)
        {
          int errcode = errno;
          gerr("ERROR: Could not stat file %s: %d\n", filePath, errcode);
          UNUSED(errcode);

          // Make sure there is no previous file information

          m_fileReady = false;
          m_filePath.remove(0);
          return false;
        }

      if (S_ISDIR(buf.st_mode) || S_ISBLK(buf.st_mode))
        {
          gerr("ERROR: Not a regular file\n");

          // Make sure there is no previous file information

          m_fileReady = false;
          m_filePath.remove(0);
          return false;
        }
    }

  // Save the new file path and mark ready to play

  m_filePath  = newFilePath;
  m_fileReady = true;
  return true;
}

/**
 * Stop playing the current file.  Called when a new media file is selected,
 * when a media file is de-selected, or when destroying the media player
 * instance.
 */

void CMediaPlayer::stopPlaying(void)
{
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  // Stop playing the file

  int ret = nxplayer_stop(m_player);
  if (ret < 0)
    {
      auderr("ERROR: nxplayer_stop failed: %d\n", ret);
    }
#endif

  // Clear information about the selected file

  m_fileReady = false;
  m_filePath.remove(0);
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

  // Open the media path directory

  FAR DIR *dirp = opendir(CONFIG_NXWM_MEDIAPLAYER_MEDIAPATH);
  if (!dirp)
    {
       m_listbox->addOption("Media volume is not mounted", 0,
                            MKRGB(192, 0, 0),
                            CONFIG_NXWIDGETS_DEFAULT_BACKGROUNDCOLOR,
                            MKRGB(255, 0, 0),
                            CONFIG_NXWM_DEFAULT_SELECTEDBACKGROUNDCOLOR);
       return false;
    }

  // Read each directory entry

  FAR struct dirent *direntry;
  int index;

  for (direntry = readdir(dirp), index = 0;
       direntry;
       direntry = readdir(dirp))
    {
      // TODO: Recursively examine files in all sub-directories
      // Ignore directory entries beginning with '.'

      if (direntry->d_name[0] == '.')
        {
          continue;
        }

#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER)
      // Filter files by extension

      FAR const char *extension = std::strchr(direntry->d_name, '.');
      if (!extension || (true
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_AC3)
          && std::strcasecmp(extension, ".ac3") != 0
#endif
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_DTS)
          && std::strcasecmp(extension, ".dts") != 0
#endif
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_WAV)
          && std::strcasecmp(extension, ".wav") != 0
#endif
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_PCM)
          && std::strcasecmp(extension, ".pcm") != 0
#endif
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_MP3)
          && std::strcasecmp(extension, ".mp3") != 0
#endif
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_MIDI)
          && std::strcasecmp(extension, ".mid") != 0
#endif
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_WMA)
          && std::strcasecmp(extension, ".wma") != 0
#endif
#if defined(CONFIG_NXWM_MEDIAPLAYER_FILTER_OGGVORBIS)
          && std::strcasecmp(extension, ".ogg") != 0
#endif
         ))
        {
          // File does not match any configured extension

          continue;
        }
#endif

      // Add the directory entry to the list box

       m_listbox->addOption(direntry->d_name, index,
                            CONFIG_NXWIDGETS_DEFAULT_ENABLEDTEXTCOLOR,
                            CONFIG_NXWIDGETS_DEFAULT_BACKGROUNDCOLOR,
                            CONFIG_NXWIDGETS_DEFAULT_SELECTEDTEXTCOLOR,
                            CONFIG_NXWM_DEFAULT_SELECTEDBACKGROUNDCOLOR);
       index++;
    }

  // Close the directory

  (void)closedir(dirp);

  // Sort the file names in alphabetical order

  m_listbox->sort();
  return true;
}

/**
 * Set the preferred audio device for playback
 */

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
bool CMediaPlayer::setDevice(FAR const char *devPath)
{
  // First try to open the file using the device name as provided

  int ret = nxplayer_setdevice(m_player, devPath);
  if (ret == -ENOENT)
    {
      char path[32];

      // Append the device path and try again

#ifdef CONFIG_AUDIO_CUSTOM_DEV_PATH
#ifdef CONFIG_AUDIO_DEV_ROOT
      std::snprintf(path, sizeof(path), "/dev/%s", devPath);
#else
      std::snprintf(path, sizeof(path), CONFIG_AUDIO_DEV_PATH "/%s", devPath);
#endif
#else
      std::snprintf(path, sizeof(path), "/dev/audio/%s", devPath);
#endif
      ret = nxplayer_setdevice(m_player, path);
    }

  // Test if the device file exists

  if (ret == -ENOENT)
    {
      // Device doesn't exit.  Report an error

      gerr("ERROR: Device %s not found\n", devPath);
      return false;
    }

  // Test if is is an audio device

  if (ret == -ENODEV)
    {
      gerr("ERROR: Device %s is not an audio device\n", devPath);
      return false;
    }

  if (ret < 0)
    {
      gerr("ERROR: Error selecting device %s\n", devPath);
      return false;
    }

  // Device set successfully

  return true;
}
#endif

/**
 * Configure the NxPlayer.
 */

bool CMediaPlayer::configureNxPlayer(void)
{
  // Get the NxPlayer handle

  m_player = nxplayer_create();
  if (!m_player)
    {
      gerr("ERROR: Failed get NxPlayer handle\n");
      return false;
    }

#ifdef CONFIG_NXPLAYER_INCLUDE_PREFERRED_DEVICE
  // Set the NxPlayer audio device

  if (!setDevice(CONFIG_NXWM_MEDIAPLAYER_PREFERRED_DEVICE))
    {
      gerr("ERROR: Failed select NxPlayer audio device\n");
      return false;
    }
#endif

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
      gerr("ERROR: Failed to create font\n");
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
      gerr("ERROR: Failed to one or more bitmaps\n");
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
      gerr("ERROR: Failed to create CListBox\n");
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

  (void)showMediaFiles(CONFIG_NXWM_MEDIAPLAYER_MEDIAPATH);

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
      gerr("ERROR: Failed to create play control\n");
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
      gerr("ERROR: Failed to create pause control\n");
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
      gerr("ERROR: Failed to create rewind control\n");
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

#ifndef CONFIG_AUDIO_EXCLUDE_REWIND
  // Register to get events from the mouse clicks on the Rewind image

  m_rewind->addWidgetEventHandler(this);
#endif

  // Create the Forward Image

  nxgl_coord_t fwdControlX = playControlX + playControlW +
                             CONFIG_NXWM_MEDIAPLAYER_XSPACING;

  m_fforward = new NXWidgets::
      CStickyImage(control, fwdControlX, controlTop, fforwardControlW,
                   controlH, m_fforwardBitmap);

  if (!m_fforward)
    {
      gerr("ERROR: Failed to create fast forward control\n");
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

#ifndef CONFIG_AUDIO_EXCLUDE_FFORWARD
  // Register to get events from the mouse clicks on the Forward image

  m_fforward->addWidgetEventHandler(this);
#endif

  // Create the Volume control

  uint32_t volumeControlX     = (9 * (uint32_t)m_windowSize.w) >> 8;
  nxgl_coord_t volumeControlW = (nxgl_coord_t)(m_windowSize.w - 2 * volumeControlX);
  nxgl_coord_t volumeControlH = m_volumeBitmap->getHeight() - 4;

  // Don't let the height of the volume control get too small

  if (volumeControlH < CONFIG_NXWM_MEDIAPLAYER_MINVOLUMEHEIGHT)
    {
      volumeControlH = CONFIG_NXWM_MEDIAPLAYER_MINVOLUMEHEIGHT;
    }

  m_volume = new NXWidgets::
      CGlyphSliderHorizontal(control, (nxgl_coord_t)volumeControlX, volumeTop,
                             volumeControlW, volumeControlH, m_volumeBitmap,
                             CONFIG_NXWM_MEDIAPLAYER_VOLUMECOLOR);

  if (!m_volume)
    {
      gerr("ERROR: Failed to create volume control\n");
      return false;
    }

  // Configure the volume control

  m_volume->disableDrawing();
  m_volume->setMinimumValue(0);
  m_volume->setMaximumValue(100);
  m_volume->setValue(15);
  m_volume->setPageSize(CONFIG_NXWM_MEDIAPLAYER_VOLUMESTEP);

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  // Register to get events from the value changes in the volume slider

  m_volume->addWidgetEventHandler(this);
#endif

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
  // Create the speed of motion indicator
  // The bounding box is determined by the font size

  nxgl_coord_t motionW = (nxgl_coord_t)(3 * m_font->getMaxWidth());
  nxgl_coord_t motionH = (nxgl_coord_t)(m_font->getHeight());

  // Horizontal position: aligned with the right size of volume slider
  // Vertical postion: same as the motion controls

  m_speed = new NXWidgets::CLabel(control,
                                  volumeControlX + volumeControlW - motionW,
                                  controlTop + (controlH - motionH) / 2,
                                  motionW, motionH, "");

  // Configure the speed indicator

  m_speed->disableDrawing();
  m_speed->setBorderless(true);
  m_speed->setRaisesEvents(false);
  m_speed->setFont(m_font);
  m_speed->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_RIGHT);
  m_speed->setTextAlignmentVert(NXWidgets::CLabel::TEXT_ALIGNMENT_VERT_CENTER);
  m_speed->hide();
#endif

  // Make sure that all widgets are setup for the STOPPED state.  Among other this,
  // this will enable drawing in the play widget (only)

  setMediaPlayerState(MPLAYER_STOPPED);

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  // Set the volume level

  setVolumeLevel();
#endif

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
  // Play should be visible if we are in any STOPPED state of if we
  // are fast forward or rewind state that came from the PAUSED state

  if (m_state != MPLAYER_STOPPED &&    // Stopped states
      m_state != MPLAYER_STAGED &&
      m_prevState == MPLAYER_PLAYING)  // Previously playing
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

  // Rewind and play buttons

  m_rewind->enableDrawing();
  m_rewind->redraw();

  m_fforward->enableDrawing();
  m_fforward->redraw();

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
  m_speed->enableDrawing();
  m_speed->redraw();
#endif

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
      m_prevState = MPLAYER_STOPPED;

      // List box is enabled and ready for file selection

      m_listbox->enable();

      // Play image is visible, but disabled.  It will not enabled until
      // we enter the MPLAYER_STAGED state after a file is selected

      m_play->disable();
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

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
      // Volume slider is available

      m_volume->enable();
#endif
      break;

    case MPLAYER_STAGED:     // Media file selected, not playing
      m_state     = MPLAYER_STAGED;
      m_prevState = MPLAYER_STOPPED;

      // List box is still enabled a ready for file selection

      m_listbox->enable();

      // Play image enabled and ready to start playing

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

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
      // Volume slider is available

      m_volume->enable();
#endif
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

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
      m_pause->enable();
#endif
      m_pause->show();

#ifndef CONFIG_AUDIO_EXCLUDE_FFORWARD
      // Fast forward image is enabled and ready for use

      m_fforward->enable();
      m_fforward->setStuckSelection(false);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_REWIND
      // Rewind image is enabled and ready for use

      m_rewind->enable();
      m_rewind->setStuckSelection(false);
#endif
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

#ifndef CONFIG_AUDIO_EXCLUDE_FFORWARD
      // Fast forward image is enabled and ready for use

      m_fforward->setStuckSelection(false);
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_REWIND
      // Rewind image is enabled and ready for use

      m_rewind->setStuckSelection(false);
#endif
      break;

    case MPLAYER_FFORWARD:   // Fast forwarding through a media file */
#ifndef CONFIG_AUDIO_EXCLUDE_FFORWARD
      m_state = MPLAYER_FFORWARD;

      // List box is not available while fast forwarding

      m_listbox->disable();

      if (m_prevState == MPLAYER_PLAYING)
        {
          // Play image hidden and disabled

          m_play->disable();
          m_play->hide();

          // Pause image enabled and ready to stop fast forwarding

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
          m_pause->enable();
#endif
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

#ifndef CONFIG_AUDIO_EXCLUDE_REWIND
      // Rewind is enabled and ready for use

      m_rewind->setStuckSelection(false);
#endif
#endif
      break;

    case MPLAYER_FREWIND:    // Rewinding a media file
#ifndef CONFIG_AUDIO_EXCLUDE_REWIND
      m_state = MPLAYER_FREWIND;

      // List box is not available while rewinding

      m_listbox->disable();

      if (m_prevState == MPLAYER_PLAYING)
        {
          // Play image hidden and disabled

          m_play->disable();
          m_play->hide();

          // Pause image enabled and ready to stop rewinding

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
          m_pause->enable();
#endif
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

#ifndef CONFIG_AUDIO_EXCLUDE_FFORWARD
      // Fast forward image is enabled and ready for use

      m_fforward->setStuckSelection(false);
#endif

      // Rewind image is enabled, highlighted, and ready for use

      m_rewind->setStuckSelection(true);
#endif
      break;

    default:
      break;
    }

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
  // Update the motion indicator for the new state

  updateMotionIndicator();
#endif

  // Re-enable drawing and redraw all widgets for the new state

  redrawWidgets();
}

/**
 * Set the new volume level based on the position of the volume slider.
 */

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
void CMediaPlayer::setVolumeLevel(void)
{
  // Get the current volume level value.  This is already pre-scaled in the
  // range 0-100

  int newLevel =  m_volume->getValue();
  if (newLevel < 0 || newLevel > 100)
    {
      gerr("ERROR: volume is out of range: %d\n", newLevel);
    }

  // Has the volume level changed?

  else if ((int)m_level != newLevel)
    {
      // Yes.. provide the new volume setting to the NX Player

      int ret = nxplayer_setvolume(m_player, (uint16_t)newLevel);
      if (ret < OK)
        {
          gerr("ERROR: nxplayer_setvolume failed: %d\n", ret);
        }
      else
        {
          // New volume set successfully.. save the new setting

          m_level = (uint8_t)newLevel;
        }
    }
}
#endif

/**
 * Check if a new file has been selected (or de-selected) in the list box
 */

void CMediaPlayer::checkFileSelection(void)
{
  // Check for new file selections from the list box

  int newFileIndex = m_listbox->getSelectedIndex();

  // Check if anything is selected

  if (newFileIndex < 0)
    {
      // No file is selected

      m_fileReady = false;
      m_fileIndex = -1;

      // Nothing is selected.. If we are not stopped, then stop now

      if (m_state != MPLAYER_STOPPED)
        {
          // Stop playing.  Should be okay if m_state == MPLAYER_STAGED.
          // We are not really playing yet, but NxPlayer should be able
          // to handle that.

          stopPlaying();

          // Then go to the STOPPED state

          setMediaPlayerState(MPLAYER_STOPPED);
        }
    }

  // Ignore the file selection if it is the same file that was selected
  // last time.

  else if (newFileIndex != m_fileIndex)
    {
      // Remember the file selection

      m_fileIndex = newFileIndex;

      // A media file is selected.  Were we in a STOPPED state before?
      // Make sure that we are not already playing.  Should be okay if
      // are in a STOPPED or STAGED state. We are not really playing
      // yet those cases, but NxPlayer should be able to handle any
      // spurious stops.

      stopPlaying();

      // Get the path to the newly selected file

      if (!getMediaFile(m_listbox->getSelectedOption()))
        {
          // Go to the STOPPED state on a failure to open the media file
          // The play button will be disabled because m_fileReady is false.
          // No harm done if we were already STOPPED.

          gerr("ERROR: getMediaFile failed\n");
          setMediaPlayerState(MPLAYER_STOPPED);
        }
      else
        {
          // We have the file.  Go to the STAGED state (enabling the PLAY
          // button).  NOTE that if for some reason this is the same file
          // that we were already and playing, then playing will be
          // restarted.

          setMediaPlayerState(MPLAYER_STAGED);
        }
    }
}

/**
 * Update fast forward/rewind speed indicator.  Called on each state change
 * and after each change in the speed of motion.
 */

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
void CMediaPlayer::updateMotionIndicator(void)
{
  m_speed->disableDrawing();
  if (m_state == MPLAYER_FFORWARD || m_state == MPLAYER_FREWIND)
    {
      // Set the new speed string

      char buffer[8];
      (void)std::snprintf(buffer, 8, "%dX", g_motionSteps[m_subSample]);

      NXWidgets::CNxString speed(buffer);
      m_speed->setText(speed);

      // Show (un-hide) the speed indicator

      m_speed->show();

      // Redraw the speed indicator

      m_speed->enableDrawing();
      m_speed->redraw();
    }

  // No, then the speed indicator should be hidden.  But don't redraw if
  // it is already hidden

  else if (!m_speed->isHidden())
    {
      // Clear the new speed string

      NXWidgets::CNxString nospeed("");
      m_speed->setText(nospeed);

      // Redraw the empty speed indicator

      m_speed->enableDrawing();
      m_speed->redraw();

      // Hide the speed indicator

      m_speed->hide();
    }
}
#endif

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

  if (m_state != MPLAYER_STOPPED && m_state != MPLAYER_STAGED)
    {
      // Check if the Pause image was clicked

      if (m_pause->isClicked() && m_state != MPLAYER_PAUSED)
        {
         // Just arm the state change now, but don't do anything until the
         // release occurs.  Trying to do the state change before the NxWidgets
         // release processing completes causes issues.

          m_pending = PENDING_PAUSE_RELEASE;
        }

#ifndef CONFIG_AUDIO_EXCLUDE_REWIND
      // Check if the rewind image was clicked

      if (m_rewind->isClicked())
        {
          // Were we already rewinding?

          if (m_state == MPLAYER_FREWIND)
            {
              // Yes.. then just increase rewind rate (by specifying a
              // higher level of sub-sampling)

              m_subSample++;
              if (m_subSample >= AUDIO_NSUBSAMPLES)
                {
                  m_subSample = 0;
                }

              int ret = nxplayer_rewind(m_player, g_motionSteps[m_subSample]);
              if (ret < 0)
                {
                  gerr("ERROR: nxplayer_rewind failed: %d\n", ret);
                }

              // Update the speed indicator

              updateMotionIndicator();
            }

          // We should not be in a STOPPED state here, but let's check anyway

          else if (m_state != MPLAYER_STOPPED && m_state != MPLAYER_STAGED)
            {
              // Start rewinding at the minimum rate

              m_subSample = 0;

              int ret = nxplayer_rewind(m_player, g_motionSteps[m_subSample]);
              if (ret < 0)
                {
                  gerr("ERROR: nxplayer_rewind failed: %d\n", ret);
                }
              else
                {
                  setMediaPlayerState(MPLAYER_FREWIND);
                }
            }
        }
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_FFORWARD
      // Check if the fast forward image was clicked

      if (m_fforward->isClicked())
        {
          // Were we already fast forwarding?

          if (m_state == MPLAYER_FFORWARD)
            {
              // Yes.. then just increase fast forward rate (by specifying a
              // level level of sub-sampling)

              m_subSample++;
              if (m_subSample >= AUDIO_NSUBSAMPLES)
                {
                  m_subSample = 0;
                }

              int ret = nxplayer_fforward(m_player, g_motionSteps[m_subSample]);
              if (ret < 0)
                {
                  gerr("ERROR: nxplayer_fforward failed: %d\n", ret);
                }

              // Update the speed indicator

              updateMotionIndicator();
            }

          // We should not be in a STOPPED state here, but let's check anyway

          else if (m_state != MPLAYER_STOPPED && m_state != MPLAYER_STAGED)
            {
              // Start fast forwarding at the minimum rate

              m_subSample = 0;

              int ret = nxplayer_fforward(m_player, g_motionSteps[m_subSample]);
              if (ret < 0)
                {
                  gerr("ERROR: nxplayer_fforward failed: %d\n", ret);
                }
              else
                {
                  setMediaPlayerState(MPLAYER_FFORWARD);
                }
            }
        }
#endif
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
      // Yes.. Now perform the delayed state change.
      //
      // If we were previously STOPPED or PAUSED, then enter the PLAYING
      // state.
      // If there is no selected file, then the play button does nothing

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
      if (m_state == MPLAYER_PAUSED)
        {
          // Resume playing

          int ret = nxplayer_resume(m_player);
          if (ret < 0)
            {
              gerr("ERROR: nxplayer_resume() failed: %d\n", ret);
            }
          else
            {
              setMediaPlayerState(MPLAYER_PLAYING);
            }
        }
      else if (m_state == MPLAYER_STAGED)
#else
      if (m_state == MPLAYER_STAGED || m_state == MPLAYER_PAUSED)
#endif
        {
          // Has a file been selected?  If not the ignore the event

          if (m_fileReady)
            {
              // Get the path to the file as a regular C-style string

              NXWidgets::nxwidget_char_t *filePath =
                new NXWidgets::nxwidget_char_t[m_filePath.getAllocSize()];

              if (!filePath)
                {
                  gerr("ERROR: Failed to allocate file path\n");
                  return;
                }

              m_filePath.copyToCharArray(filePath);

              // And start playing

              int ret = nxplayer_playfile(m_player, (FAR const char *)filePath,
                                          AUDIO_FMT_UNDEF, AUDIO_FMT_UNDEF);
              if (ret < 0)
                {
                  gerr("ERROR: nxplayer_playfile %s failed: %d\n", filePath, ret);
                }
              else
                {
                  setMediaPlayerState(MPLAYER_PLAYING);
                }

              delete[] filePath;
            }
        }

      // Ignore the event if (1) we are already in the PLAYING state, or (2)
      // we are still in the STOPPED state (with no file selected).

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
      // The remaining cases include only the FFORWARD and REWIND states

      else if (m_state == MPLAYER_FFORWARD || m_state == MPLAYER_FREWIND)
        {
          // In these states, stop the fast motion action and return to the
          // previous state

          m_subSample = 0;

          int ret = nxplayer_cancel_motion(m_player, m_prevState == MPLAYER_PAUSED);
          if (ret < 0)
            {
              gerr("ERROR: nxplayer_cancel_motion failed: %d\n", ret);
            }
          else
            {
              setMediaPlayerState(m_prevState);
            }
        }
#endif

      // No longer any action pending the PLAY image release

      m_pending = PENDING_NONE;
    }

#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  // Check if the Pause image was released

  else if (m_pending == PENDING_PAUSE_RELEASE && !m_pause->isClicked())
    {
      // Yes.. Now perform the delayed state change
      //
      // If we were previously PLAYING, then enter the PAUSED state.

      if (m_state == MPLAYER_PLAYING)
        {
          // Pause playing

          int ret = nxplayer_pause(m_player);
          if (ret < 0)
            {
              gerr("ERROR: nxplayer_pause() failed: %d\n", ret);
            }
          else
            {
              setMediaPlayerState(MPLAYER_PAUSED);
            }
        }

#if !defined(CONFIG_AUDIO_EXCLUDE_FFORWARD) || !defined(CONFIG_AUDIO_EXCLUDE_REWIND)
      // Ignore the event if we are already in the PAUSED or STOPPED states

      else if (m_state != MPLAYER_STOPPED &&
               m_state != MPLAYER_STAGED &&
               m_state != MPLAYER_PAUSED)
        {
          // Otherwise, we must be fast forwarding or rewinding.  In these
          // cases, stop the action and return to the previous state

          m_subSample = 0;

          int ret = nxplayer_cancel_motion(m_player, m_prevState == MPLAYER_PAUSED);
          if (ret < 0)
            {
              gerr("ERROR: nxplayer_cancel_motion failed: %d\n", ret);
            }
          else
            {
              setMediaPlayerState(m_prevState);
            }
        }
#endif

      // No longer any action pending the PAUSE image release

      m_pending = PENDING_NONE;
    }
#endif
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
 * Handle value changes.  This will get events when there is a change in the
 * volume level or a file is selected or deselected.
 */

void CMediaPlayer::handleValueChangeEvent(const NXWidgets::CWidgetEventArgs &e)
{
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
  setVolumeLevel();
#endif
  checkFileSelection();
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
  // the NxTerm application

  CApplicationWindow *window = m_taskbar->openApplicationWindow();
  if (!window)
    {
      gerr("ERROR: Failed to create CApplicationWindow\n");
      return (IApplication *)0;
    }

  // Open the window (it is hot in here)

  if (!window->open())
    {
      gerr("ERROR: Failed to open CApplicationWindow\n");
      delete window;
      return (IApplication *)0;
    }

  // Instantiate the application, providing the window to the application's
  // constructor

  CMediaPlayer *mediaPlayer = new CMediaPlayer(m_taskbar, window);
  if (!mediaPlayer)
    {
      gerr("ERROR: Failed to instantiate CMediaPlayer\n");
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
