/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cbackground.hxx
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
