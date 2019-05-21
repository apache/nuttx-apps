/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cfonts.cxx
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "graphics/nxwidgets/cnxfont.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cfonts.hxx"

/////////////////////////////////////////////////////////////////////////////
// CFonts Method Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CFonts Constructor
 *
 * @param twm4nx. An instance of the NX server.
 */

CFonts::CFonts(CTwm4Nx *twm4nx)
{
  m_twm4nx          = twm4nx;                        // Save the NX server
  m_titleFont       = (FAR NXWidgets::CNxFont *)0;   // Title bar font
  m_menuFont        = (FAR NXWidgets::CNxFont *)0;   // Menu font
  m_iconFont        = (FAR NXWidgets::CNxFont *)0;   // Icon font
  m_sizeFont        = (FAR NXWidgets::CNxFont *)0;   // Resize font
  m_iconManagerFont = (FAR NXWidgets::CNxFont *)0;   // Window list font
  m_defaultFont     = (FAR NXWidgets::CNxFont *)0;   // The default found
}

/**
 * CFonts Destructor
 */

CFonts::~CFonts(void)
{
  if (m_titleFont != (FAR NXWidgets::CNxFont *)0)
    {
      delete m_titleFont;
    }

  if (m_menuFont != (FAR NXWidgets::CNxFont *)0)
    {
      delete m_menuFont;
    }

  if (m_iconFont != (FAR NXWidgets::CNxFont *)0)
    {
      delete m_iconFont;
    }

  if (m_sizeFont != (FAR NXWidgets::CNxFont *)0)
    {
      delete m_sizeFont;
    }

  if (m_iconManagerFont != (FAR NXWidgets::CNxFont *)0)
    {
      delete m_iconManagerFont;
    }

  if (m_defaultFont != (FAR NXWidgets::CNxFont *)0)
    {
      delete m_defaultFont;
    }
}

/**
 * Initialize fonts
 */

bool CFonts::initialize(void)
{
  m_titleFont =
    new NXWidgets::CNxFont((enum nx_fontid_e)CONFIG_TWM4NX_TITLE_FONTID,
                           CONFIG_TWM4NX_TITLE_FONTCOLOR,
                           CONFIG_TWM4NX_TRANSPARENT_COLOR);
  if (m_titleFont == (FAR NXWidgets::CNxFont *)0)
    {
      return false;
    }

  m_menuFont =
    new NXWidgets::CNxFont((enum nx_fontid_e)CONFIG_TWM4NX_MENU_FONTID,
                           CONFIG_TWM4NX_MENU_FONTCOLOR,
                           CONFIG_TWM4NX_TRANSPARENT_COLOR);
  if (m_menuFont == (FAR NXWidgets::CNxFont *)0)
    {
      return false;
    }

  m_iconFont =
    new NXWidgets::CNxFont((enum nx_fontid_e)CONFIG_TWM4NX_ICON_FONTID,
                           CONFIG_TWM4NX_ICON_FONTCOLOR,
                           CONFIG_TWM4NX_TRANSPARENT_COLOR);
  if (m_iconFont == (FAR NXWidgets::CNxFont *)0)
    {
      return false;
    }

  m_sizeFont =
    new NXWidgets::CNxFont((enum nx_fontid_e)CONFIG_TWM4NX_SIZE_FONTID,
                           CONFIG_TWM4NX_SIZE_FONTCOLOR,
                           CONFIG_TWM4NX_TRANSPARENT_COLOR);
  if (m_sizeFont == (FAR NXWidgets::CNxFont *)0)
    {
      return false;
    }

  m_iconManagerFont =
    new NXWidgets::CNxFont((enum nx_fontid_e)CONFIG_TWM4NX_ICONMGR_FONTID,
                           CONFIG_TWM4NX_ICONMGR_FONTCOLOR,
                           CONFIG_TWM4NX_TRANSPARENT_COLOR);
  if (m_iconManagerFont == (FAR NXWidgets::CNxFont *)0)
    {
      return false;
    }

  m_defaultFont =
    new NXWidgets::CNxFont((enum nx_fontid_e)NXFONT_DEFAULT,
                           CONFIG_TWM4NX_DEFAULT_FONTCOLOR,
                           CONFIG_TWM4NX_TRANSPARENT_COLOR);
  if (m_defaultFont == (FAR NXWidgets::CNxFont *)0)
    {
      return false;
    }

  return true;
}
