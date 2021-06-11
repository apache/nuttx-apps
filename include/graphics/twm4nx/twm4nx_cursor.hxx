/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/twm4nx_cursor.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CURSOR_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CURSOR_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <nuttx/nx/nxcursor.h>

#include "graphics/twm4nx/twm4nx_config.hxx"

#ifdef CONFIG_NX_SWCURSOR

/////////////////////////////////////////////////////////////////////////////
// Public Constant Data
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  extern const struct nx_cursorimage_s   // Normal cursor image
    CONFIG_TWM4NX_CURSOR_IMAGE;
  extern const struct nx_cursorimage_s   // Grab cursor image
    CONFIG_TWM4NX_GBCURSOR_IMAGE;
  extern const struct nx_cursorimage_s   // Resize cursor image
    CONFIG_TWM4NX_RZCURSOR_IMAGE;
  extern const struct nx_cursorimage_s   // Wait cursor image
    CONFIG_TWM4NX_WTCURSOR_IMAGE;
}

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

#endif // CONFIG_NX_SWCURSOR
#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_CURSOR_HXX
