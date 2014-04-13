/****************************************************************************
 * NxWidgets/nxwm/include/cmediaplayer.hxx
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
 ****************************************************************************/

#ifndef __INCLUDE_CMEDIAPLAYER_HXX
#define __INCLUDE_CMEDIAPLAYER_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxconsole.h>

#include "cimage.hxx"
#include "clabel.hxx"
#include "cnxfont.hxx"
#include "cglyphsliderhorizontal.hxx"

#include "iapplication.hxx"
#include "capplicationwindow.hxx"
#include "ctaskbar.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

#define NXWM_MEDIAPLAYER_NROWS    6
#define NXWM_MEDIAPLAYER_NCOLUMNS 6

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NxWM
{
  /**
   * This class implements the CMediaPlayer application.
   */

  class CMediaPlayer : public  IApplication,
                       private IApplicationCallback,
                       private NXWidgets::CWidgetEventHandler
  {
  private:
    /**
     * The structure defines a pending operation.
     */

    struct SPendingOperation
    {
      int64_t value;       /**< Accumulated value */
      uint8_t operation;   /**< Identifies the operations */
    };

    /**
     * Media player state data.
     */

    /**
     * Cached constructor parameters.
     */

    CTaskbar                *m_taskbar;    /**< Reference to the "parent" taskbar */
    CApplicationWindow      *m_window;     /**< Reference to the application window */

    /**
     * Widgets
     */

    NXWidgets::CLabel       *m_text;       /**< Some text in the app for now */
    NXWidgets::CNxFont      *m_font;       /**< The font used in the media player */
    NXWidgets::CImage       *m_rew;        /**< Rewind control */
    NXWidgets::CImage       *m_playPause;  /**< Play/Pause control */
    NXWidgets::CImage       *m_fwd;        /**< Forward control */
    NXWidgets::CGlyphSliderHorizontal *m_volume; /**< Volume control */

    /**
     * Calculator geometry.  This stuff does not really have to be retained
     * in memory.  If you are pinched for memory, get rid of these.
     */

    struct nxgl_size_s       m_windowSize; /**< The size of the calculator window */
    struct nxgl_size_s       m_textSize;   /**< The size of the calculator textbox */
    struct nxgl_point_s      m_textPos;    /**< The position of the calculator textbox */

    /**
    * Select the geometry of the media player given the current window size.
    * Only called as part of construction.
    */

   inline void setGeometry(void);

    /**
     * Create the Media Player conrols.  Only start as part of the applicaiton
     * start method.
     */

   inline bool createPlayer(void);

    /**
     * Called when the window minimize button is pressed.
     */

    void minimize(void);

    /**
     * Called when the window minimize close is pressed.
     */

    void close(void);

    /**
     * Handle a widget action event.  For CImage, this is a button pre-
     * release event.
     *
     * @param e The event data.
     */

    void handleActionEvent(const NXWidgets::CWidgetEventArgs &e);

  public:
    /**
     * CMediaPlayer constructor
     *
     * @param window.  The application window
     *
     * @param taskbar.  A pointer to the parent task bar instance
     * @param window.  The window to be used by this application.
     */

    CMediaPlayer(CTaskbar *taskbar, CApplicationWindow *window);

    /**
     * CMediaPlayer destructor
     */

    ~CMediaPlayer(void);

    /**
     * Each implementation of IApplication must provide a method to recover
     * the contained CApplicationWindow instance.
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
     * otherwise moved to the top of the hierarchy.  This method is call from
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

  class CMediaPlayerFactory : public IApplicationFactory
  {
  private:
    CTaskbar *m_taskbar;  /**< The taskbar */

  public:
    /**
     * CMediaPlayerFactory Constructor
     *
     * @param taskbar.  The taskbar instance used to terminate calibration
     */

    CMediaPlayerFactory(CTaskbar *taskbar);

    /**
     * CMediaPlayerFactory Destructor
     */

    inline ~CMediaPlayerFactory(void) { }

    /**
     * Create a new instance of an CMediaPlayer (as IApplication).
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
#endif // __cplusplus

#endif // __INCLUDE_CMEDIAPLAYER_HXX
