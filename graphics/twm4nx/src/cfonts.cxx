/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cfonts.cxx
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

/////////////////////////////////////////////////////////////////////////////
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
