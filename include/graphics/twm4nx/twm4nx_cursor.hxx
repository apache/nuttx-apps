/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/twm4nx_cursor.hxx
// Cursor-related definitions
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
