/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cbackground.hxx
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

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cnxserver.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"

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

  class CBackground
  {
    protected:
      FAR CTwm4Nx                  *m_twm4nx;     /**< Cached CTwm4Nx instance */
      FAR NXWidgets::CBgWindow     *m_backWindow; /**< The background window */
      FAR NXWidgets::CImage        *m_backImage;  /**< The background image */

      /**
       * Create the background window.
       *
       * @return true on success
       */

      bool createBackgroundWindow(void);

      /**
       * Create the background image.
       *
       * @param sbitmap.  Identifies the bitmap to paint on background
       * @return true on success
       */

      bool createBackgroundImage(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap);

      /**
       * (Re-)draw the background window.
       *
       * @return true on success
       */

       bool redrawBackgroundWindow(void);

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
       * Set the background image
       *
       * @param sbitmap.  Identifies the bitmap to paint on background
       * @return true on success
       */

      bool setBackgroundImage(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap);

      /**
       * Get the size of the physical display device which is equivalent to
       * size of the background window.
       *
       * @return The size of the display
       */

      void getDisplaySize(FAR struct nxgl_size_s &size);
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CBACKGROUND_HXX
