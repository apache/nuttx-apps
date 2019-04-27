/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/ciconwin.hxx
// Icon Windows
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CICONWIN_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CICONWIN_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include "graphics/nxwidgets/cnxwindow.hxx"

#include "graphics/twm4nx/ctwm4nxevent.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  struct SRlePaletteBitmap;                     // Forward reference
}

namespace Twm4Nx
{
  class  CTwm4Nx;                               // Forward reference
  class  CWindow;                               // Forward reference

  /**
   * The CIconWin represents on icon window
   */

  class CIconWin : public CTwm4NxEvent
  {
    private:

      FAR CTwm4Nx                *m_twm4nx;     /**< The Twm4Nx session */
      FAR NXWidgets::CNxWindow   *m_nxwin;      /**< The cursor "raw" window */

      // Dragging

      struct nxgl_point_s         m_dragPos;    /**< Last mouse position */
      struct nxgl_point_s         m_dragOffset; /**< Offset from mouse to window origin */
      struct nxgl_size_s          m_dragCSize;  /**< The grab cursor size */
      bool                        m_drag;       /**< Drag in-progress */

      /**
       * Handle the ICON_GRAB event.  That corresponds to a left
       * mouse click on the icon
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool iconGrab(FAR struct SEventMsg *eventmsg);

      /**
       * Handle the ICON_DRAG event.  That corresponds to a mouse
       * movement when the icon is in a grabbed state.
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool iconDrag(FAR struct SEventMsg *eventmsg);

      /**
       * Handle the ICON_UNGRAB event.  The corresponds to a mouse
       * left button release while in the grabbed
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool iconUngrab(FAR struct SEventMsg *eventmsg);

      /**
       * Cleanup on failure or as part of the destructor
       */

      void cleanup(void);

    public:

      /**
       * CIconWin Constructor
       */

      CIconWin(CTwm4Nx *twm4nx);

      /**
       * CIconWin Destructor
       */

      ~CIconWin(void);

      /**
       * Initialize the icon window
       *
       * @param parent  The parent window
       * @param sbitmap The Icon bitmap image
       * @param pos     The default position
       */

      bool initialize(FAR CWindow *parent,
                      FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap,
                      FAR const struct nxgl_point_s *pos);

      /**
       * Get the size of the icon window on the background
       *
       * @param size The location to return the size of the icon window
       */

      inline bool getSize(FAR struct nxgl_size_s *size)
      {
        return m_nxwin->getSize(size);
      }

      /**
       * Get the icon window position on the background
       *
       * @param size The location to return the position of the icon window
       */

      inline bool getPosition(FAR struct nxgl_point_s *pos)
      {
        return m_nxwin->getPosition(pos);
      }

      /**
       * Set the icon window position on the background
       *
       * @param size The new position of the icon window
       */

      inline bool setPosition(FAR const struct nxgl_point_s *pos)
      {
        return m_nxwin->setPosition(pos);
      }

      /**
       * Raise the icon window.
       */

      inline void raise(void)
      {
        m_nxwin->raise();
      }

      /**
       * Lower the icon window.
       */

      inline void lower(void)
      {
        m_nxwin->lower();
      }

      /**
       * Handle ICON WINDOW events.
       *
       * @param eventmsg.  The received NxWidget ICON event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);
  };
}

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CICONWIN_HXX
