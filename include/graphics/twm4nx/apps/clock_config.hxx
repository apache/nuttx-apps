/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/clock_config.hxx
// Clock configuration settings
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CLOCK_CONFIG_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CLOCK_CONFIG_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <debug.h>

#include "graphics/nxglyphs.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/twm4nx/twm4nx_config.hxx"

#ifdef CONFIG_TWM4NX_CLOCK

/////////////////////////////////////////////////////////////////////////////
// Pre-Processor Definitions
/////////////////////////////////////////////////////////////////////////////

// General Configuration ////////////////////////////////////////////////////

/**
 * Required settings:
 *
 * CONFIG_HAVE_CXX            : C++ support is required
 * CONFIG_NX                  : NX must enabled
 * CONFIG_GRAPHICS_SLCD       : For segment LCD emulation
 */

#ifndef CONFIG_HAVE_CXX
#  error C++ support is required (CONFIG_HAVE_CXX)
#endif

#ifndef CONFIG_NX
#  error NX support is required (CONFIG_NX)
#endif

#ifndef CONFIG_GRAPHICS_SLCD
#  warning Segment LCD emulation is required (CONFIG_GRAPHICS_SLCD)
#endif

// Clock Window /////////////////////////////////////////////////////////////

/**
 * Clock Window Configuration
 *
 * CONFIG_TWM4NX_CLOCK_PRIO - Priority of the Clock task.  Default:
 *   SCHED_PRIORITY_DEFAULT.  NOTE:  This priority should be less than
 *   CONFIG_NXSTART_SERVERPRIO or else there may be data overrun errors.
 *   Such errors would most likely appear as duplicated rows of data on the
 *   display.
 * CONFIG_TWM4NX_CLOCK_STACKSIZE - The stack size to use when starting the
 *   Clock task.  Default: 1024 bytes.
 * CONFIG_TWM4NX_CLOCK_ICON - The glyph to use as the Clock icon
 * CONFIG_TWM4NX_CLOCK_HEIGHT - The fixed height of the clock
 */

// Tasking

#ifndef CONFIG_TWM4NX_CLOCK_PRIO
#  define CONFIG_TWM4NX_CLOCK_PRIO       SCHED_PRIORITY_DEFAULT
#endif

#if CONFIG_NXSTART_SERVERPRIO <= CONFIG_TWM4NX_CLOCK_PRIO
#  warning "CONFIG_NXSTART_SERVERPRIO <= CONFIG_TWM4NX_CLOCK_PRIO"
#  warning" -- This can result in data overrun errors"
#endif

#ifndef CONFIG_TWM4NX_CLOCK_STACKSIZE
#  define CONFIG_TWM4NX_CLOCK_STACKSIZE  1024
#endif

// Colors -- Controlled by the CSLcd configuration

// The Clock window glyph

#ifndef CONFIG_TWM4NX_CLOCK_ICON
#  define CONFIG_TWM4NX_CLOCK_ICON       NXWidgets::g_lcdClockBitmap
#endif

// The fixed clock window height

#ifndef CONFIG_TWM4NX_CLOCK_HEIGHT
#  define CONFIG_TWM4NX_CLOCK_HEIGHT     64
#endif

// Spacing

#ifndef CONFIG_TWM4NX_CLOCK_HSPACING
#  define CONFIG_TWM4NX_CLOCK_HSPACING   2
#endif

#ifndef CONFIG_TWM4NX_CLOCK_VSPACING
#  define CONFIG_TWM4NX_CLOCK_VSPACING   2
#endif

#endif // CONFIG_TWM4NX_CLOCK
#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_APPS_CLOCK_CONFIG_HXX
