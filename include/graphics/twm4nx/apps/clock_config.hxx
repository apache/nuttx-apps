/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/apps/clock_config.hxx
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
