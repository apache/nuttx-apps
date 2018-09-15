/****************************************************************************
 * apps/graphics/traveler/include/trv_debug.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_DEBUG_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_DEBUG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

#include <stdio.h>
#include <debug.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Feature Selection Switches
 *
 * CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL == 3 turns off sound and video and
 *   enables verbose debug messages on stdout.
 * CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL == 2 turns off sound and video and
 *   enables normal debug output
 * CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL == 1 turns off sound and enables
 *   normal debug output
 * OTHERWISE, all debugging features are disabled.
 */

#define ENABLE_SOUND 1
#define ENABLE_VIDEO 1
#undef  TRV_VERBOSE

#ifndef CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL
#  define CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL 0
#elif (CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL == 3)
#  undef ENABLE_SOUND
#  undef ENABLE_VIDEO
#  define TRV_VERBOSE  1
#elif (CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL == 2)
#  undef ENABLE_SOUND
#  undef ENABLE_VIDEO
#elif (CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL == 1)
#  undef ENABLE_SOUND
#endif

/* Sound is not yet supported */

#undef ENABLE_SOUND

/* Debug output macros */

#ifdef CONFIG_CPP_HAVE_VARARGS
#  if (CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL > 0)

#    define trv_debug(format, ...) \
       printf(EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)

#    ifdef TRV_VERBOSE
#      define trv_vdebug(format, ...) \
       printf(EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)

#    else
#      define trv_vdebug(x...)
#    endif

#  else

#    define trv_debug(x...)
#    define trv_vdebug(x...)

#  endif
#else

  /* Variadic macros NOT supported */

#  if (CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL > 0)
#    ifdef TRV_VERBOSE
#      define trv_vdebug trv_debug
#    else
#      define trv_vdebug (void)
#    endif
#  else
#    define trv_debug  (void)
#    define trv_vdebug (void)
#  endif

#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* The debugging interfaces are normally accessed via the macros provided
 * above.  If the cross-compiler's C pre-processor supports a variable
 * number of macro arguments, then those macros below will map all
 * debug statements to printf.
 *
 * If the cross-compiler's pre-processor does not support variable length
 * arguments, then this additional interface will be built.
 */

#if !defined(CONFIG_CPP_HAVE_VARARGS) && CONFIG_GRAPHICS_TRAVELER_DEBUG_LEVEL > 0
int trv_debug(FAR const char *format, ...);
#endif

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_DEBUG_H */
