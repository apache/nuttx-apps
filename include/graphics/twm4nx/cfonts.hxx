/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cfonts.hxx
// Font support for twm4nx
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
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CFONTS_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CFONTS_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class CNxFont;                                    // Forward reference
}

namespace Twm4Nx
{
  class CTwm4Nx;                                    // Forward reference

  /**
   * The CFonts class encapsulates font support.
   */

  class CFonts
  {
    private:
      FAR CTwm4Nx            *m_twm4nx;             /**< The Twm4Nx session */
      FAR NXWidgets::CNxFont *m_titleFont;          /**< Title bar font */
      FAR NXWidgets::CNxFont *m_menuFont;           /**< Menu font */
      FAR NXWidgets::CNxFont *m_iconFont;           /**< Icon font */
      FAR NXWidgets::CNxFont *m_sizeFont;           /**< Resize font */
      FAR NXWidgets::CNxFont *m_iconManagerFont;    /**< Window list font */
      FAR NXWidgets::CNxFont *m_defaultFont;        /**< The default found */

    public:

      /**
       * CFonts Constructor
       *
       * @param twm4nx. An instance of the Twm4Nx session.
       */

      CFonts(FAR CTwm4Nx *twm4nx);

      /**
       * CFonts Destructor
       */

      ~CFonts(void);

      /**
       * Initialize fonts
       *
       * @return True is returned if the fonts were correctly initialized;
       *   false is returned in the event of an error.
       */

      bool initialize(void);

      /**
       * Return the Title font
       *
       * @return A reference to the initialized title font.
       */

      FAR NXWidgets::CNxFont *getTitleFont(void)
      {
        return m_titleFont;
      }

      /**
       * Return the Menu font
       *
       * @return A reference to the initialized menu font.
       */

      FAR NXWidgets::CNxFont *getMenuFont(void)
      {
        return m_menuFont;
      }

      /**
       * Return the Icon font
       *
       * @return A reference to the initialized menu font.
       */

      FAR NXWidgets::CNxFont *getIconFont(void)
      {
        return m_iconFont;
      }

      /**
       * Return the Size font
       *
       * @return A reference to the initialized menu font.
       */

      FAR NXWidgets::CNxFont *getSizeFont(void)
      {
        return m_sizeFont;
      }

      /**
       * Return the IconManager font
       *
       * @return A reference to the initialized menu font.
       */

      FAR NXWidgets::CNxFont *getIconManagerFont(void)
      {
        return m_iconManagerFont;
      }

      /**
       * Return the Default font
       *
       * @return A reference to the initialized menu font.
       */

      FAR NXWidgets::CNxFont *getDefaultFont(void)
      {
        return m_defaultFont;
      }
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CFONTS_HXX
