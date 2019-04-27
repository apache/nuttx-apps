/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cresize.hxx
// Resize function externs
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CRESIZE_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CRESIZE_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/twm4nx/ctwm4nxevent.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class  CNxTkWindow;                             // Forward reference
  class  CLabel;                                  // Forward reference
}

namespace Twm4Nx
{
  class  CWindow;                                 // Forward reference
  struct SEventMsg;                               // Forward reference

  class CResize : protected NXWidgets::CWidgetEventHandler, public CTwm4NxEvent
  {
    private:

      CTwm4Nx                    *m_twm4nx;       /**< Cached Twm4Nx session */
      mqd_t                       m_eventq;       /**< NxWidget event message queue */
      FAR NXWidgets::CNxTkWindow *m_sizeWindow;   /**< The resize dimensions window */
      FAR NXWidgets::CLabel      *m_sizeLabel;    /**< Resize dimension label */
      FAR CWindow                *m_resizeWindow; /**< The window being resized */
      struct nxgl_point_s         m_origpos;      /**< Original position */
      struct nxgl_size_s          m_origsize;     /**< Original size */
      struct nxgl_point_s         m_dragpos;      /**< Dragged position */
      struct nxgl_size_s          m_dragsize;     /**< Dragged size */
      struct nxgl_rect_s          m_clamp;
      struct nxgl_point_s         m_delta;
      struct nxgl_size_s          m_last;
      struct nxgl_point_s         m_addingPos;
      struct nxgl_size_s          m_addingSize;
      int                         m_stringWidth;    /**< Size of current size string */

      /**
       * Create the size window
       */

      bool createSizeWindow(void);

      /**
       * Create the size label widget
       */

      bool createSizeLabel(void);

      /**
       * Set the Window Size
       */

      bool setWindowSize(FAR struct nxgl_size_s *size);

      void resizeFromCenter(FAR CWindow *win);

      /**
       * Begin a window resize operation
       * @param ev           the event structure (button press)
       * @param cwin         the TWM window pointer
       */

      void startResize(FAR struct SEventMsg *eventmsg, FAR CWindow *cwin);

      void menuStartResize(FAR CWindow *cwin,
                           FAR struct nxgl_point_s *pos,
                           FAR struct nxgl_size_s *size);

      /**
       * Update the size show in the size dimension label.
       *
       * @param cwin   The current window be resized
       * @param size   The size of the rubber band
       */

      void updateSizeLabel(FAR CWindow *cwin, FAR struct nxgl_size_s *size);

    public:

      /**
       * CResize Constructor
       *
       * @param twm4nx   The Twm4Nx session
       */

      CResize(CTwm4Nx *twm4nx);

      /**
       * CResize Destructor
       */

      ~CResize(void);

      /**
       * CResize Initializer.  Performs the parts of the CResize construction
       * that may fail.
       *
       * @result True is returned on success
       */

      bool initialize(void);

      /**
       * What is this?
       */

      void addingSize(FAR struct nxgl_size_s *size)
      {
        m_addingSize.w = size->w;
        m_addingSize.h = size->h;
      }

      /**
       * Begin a window resize operation
       *
       * @param cwin the Twm4Nx window pointer
       */

      void addStartResize(FAR CWindow *cwin,
                          FAR struct nxgl_point_s *pos,
                          FAR struct nxgl_size_s *size);

      /**
       * @param cwin  The current Twm4Nx window
       * @param root  The X position in the root window
       */

      void menuDoResize(FAR CWindow *cwin,
                        FAR struct nxgl_point_s *root);

      /**
       * Resize the window.  This is called for each motion event while we are
       * resizing
       *
       * @param cwin  The current Twm4Nx window
       * @param root  The X position in the root window
       */

      void doResize(FAR CWindow *cwin,
                    FAR struct nxgl_point_s *root);

      /**
       * Finish the resize operation
       */

      void endResize(FAR CWindow *cwin);

      void menuEndResize(FAR CWindow *cwin);

      /**
       * Adjust the given width and height to account for the constraints imposed
       * by size hints.
       */

      // REVISIT: Only used internally.  Used to be used to handle prompt
      // for window size vs. automatically sizing.

      void constrainSize(FAR CWindow *cwin, FAR nxgl_size_s *size);

      /**
       * Set window sizes.
       *
       * Special Considerations:
       *   This routine will check to make sure the window is not completely off the
       *   display, if it is, it'll bring some of it back on.
       *
       * The cwin->frame_XXX variables should NOT be updated with the values of
       * x,y,w,h prior to calling this routine, since the new values are compared
       * against the old to see whether a synthetic ConfigureNotify event should be
       * sent.  (It should be sent if the window was moved but not resized.)
       *
       * @param cwin The CWiondow instance
       * @param pos  The position of the upper-left outer corner of the frame
       * @param size The size of the frame window
       */

      void setupWindow(FAR CWindow *cwin,
                       FAR struct nxgl_point_s *pos,
                       FAR struct nxgl_size_s *size);

      /**
       * Zooms window to full height of screen or to full height and width of screen.
       * (Toggles so that it can undo the zoom - even when switching between fullZoom
       * and vertical zoom.)
       *
       * @param cwin  the TWM window pointer
       */

      void fullZoom(FAR CWindow *cwin, int flag);

      /**
       * Handle RESIZE events.
       *
       * @param msg.  The received NxWidget RESIZE event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *msg);
  };
}

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CRESIZE_HXX
