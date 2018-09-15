/****************************************************************************
 * apps/graphics/traveler/include/trv_types.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TYPES_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TYPES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifndef __HOST_BUILD__
#  include <nuttx/config.h>
#  include <nuttx/compiler.h>
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The maximum size of a line (for example, in the .INI file) */

#define TRV_MAX_LINE   256

/* Size of one (internal) pixel */

#define TRV_PIXEL_MAX  UINT8_MAX

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef uint8_t trv_pixel_t;  /* Width of one pixel in rendering phase */
typedef int16_t trv_coord_t;  /* Contains one display coordinate */
                              /* Max world size is +/- 65536/64 = 1024 */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TYPES_H */
