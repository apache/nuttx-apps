/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/cfonts.hxx
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

// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.
//

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
