/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cbackground.hxx
// Manage background image
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CBACKGROUND_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CBACKGROUND_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <mqueue.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cnxserver.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

#include "graphics/twm4nx/ctwm4nxevent.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Class Definition
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CBgWindow;                               // Forward reference
  class  CImage;                                  // Forward reference
  class  CWidgetControl;                          // Forward reference
  struct SRlePaletteBitmap;                       // Forward reference
}

namespace Twm4Nx
{
  class CTwm4Nx;                                  // Forward reference

  /**
   * Background management
   */

  class CBackground : protected NXWidgets::CWidgetEventHandler, public CTwm4NxEvent
  {
    protected:
      FAR CTwm4Nx                  *m_twm4nx;     /**< Cached CTwm4Nx instance */
      mqd_t                         m_eventq;     /**< NxWidget event message queue */
      FAR NXWidgets::CBgWindow     *m_backWindow; /**< The background window */
#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
      FAR NXWidgets::CImage        *m_backImage;  /**< The background image */
#endif

      /**
       * Create the background window.
       *
       * @return true on success
       */

      bool createBackgroundWindow(void);

      /**
       * Create the background widget.  The background widget is simply a
       * container for all of the widgets on the background.
       */

      bool createBackgroundWidget(void);

#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
      /**
       * Create the background image.
       *
       * @param sbitmap.  Identifies the bitmap to paint on background
       * @return true on success
       */

      bool createBackgroundImage(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap);
#endif

      /**
       * Bring up the main menu (if it is not already up).
       *
       * @param pos The window click position.
       * @param buttons The set of mouse button presses.
       */

      void showMainMenu(FAR struct nxgl_point_s &pos, uint8_t buttons);

      /**
       * Release resources held by the background.
       */

      void cleanup(void);

    public:
      /**
       * CBackground Constructor
       *
       * @param twm4nx The Twm4Nx session object
       */

      CBackground(FAR CTwm4Nx *twm4nx);

      /**
       * CBackground Destructor
       */

      ~CBackground(void);

      /**
       * Get the widget control instance needed to support application drawing
       * into the background.
       */

      inline FAR NXWidgets::CWidgetControl *getWidgetControl()
      {
        return m_backWindow->getWidgetControl();
      }

      /**
       * Finish construction of the background instance.  This performs
       * That are not appropriate for the constructor because they may
       * fail.
       *
       * @param sbitmap.  Identifies the bitmap to paint on background
       * @return true on success
       */

      bool initialize(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap);

      /**
       * Get the size of the physical display device which is equivalent to
       * size of the background window.
       *
       * @return The size of the display
       */

      void getDisplaySize(FAR struct nxgl_size_s &size);

      /**
       * Handle the background window redraw.
       *
       * @param nxRect The region in the window to be redrawn
       * @param more More redraw requests will follow
       * @return true on success
       */

      bool redrawBackgroundWindow(FAR const struct nxgl_rect_s *rect, bool more);

      /**
       * Check if the region within 'bounds' collides with any other reserved
       * region on the desktop.  This is used for icon placement.
       *
       * @param iconBounds The candidate bounding box
       * @param collision The bounding box of the reserved region that the
       *   candidate collides with
       * @return Returns true if there is a collision
       */

      bool checkCollision(FAR const struct nxgl_rect_s &bounds,
                          FAR struct nxgl_rect_s &collision);

      /**
       * Handle EVENT_BACKGROUND events.
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CBACKGROUND_HXX
