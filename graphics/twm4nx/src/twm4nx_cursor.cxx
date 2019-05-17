/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/twm4nx_cursor.cxx
// Cursor images
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <nuttx/nx/nxcursor.h>

#include "graphics/twm4nx/twm4nx_cursor.hxx"

#ifdef CONFIG_NX_SWCURSOR

/////////////////////////////////////////////////////////////////////////////
// Pulbic Data
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
#ifdef CONFIG_TWM4NX_CURSOR_LARGE
#  include "cursor-arrow1-30x30.h"
#  include "cursor-grab-25x30.h"
#  include "cursor-resize-30x30.h"
#  include "cursor-wait-23x30.h"
#else
#  include "cursor-arrow1-16x16.h"
#  include "cursor-grab-14x16.h"
#  include "cursor-resize-16x16.h"
#  include "cursor-wait-13x16.h"
#endif
}

#endif // CONFIG_NX_SWCURSOR
