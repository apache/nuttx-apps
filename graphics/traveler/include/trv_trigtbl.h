/****************************************************************************
 * apps/graphics/traveler/include/trv_trigtbl.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TRIGTBL_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TRIGTBL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* These are definitions of commonly used angles. */

#define TWOPI       1920
#define PI           960
#define HALFPI       480
#define QTRPI        240

/* Here are definitions for those who prefer degrees */
/* NOTE:  ANGLE_60 and ANGLE_30 are special values.  They were */
/* chosen to match the horizontal screen resolution of 320 pixels. */
/* These, in fact, drive the entire angular measurement system */

#define ANGLE_0        0
#define ANGLE_6       32
#define ANGLE_9       48
#define ANGLE_30     160
#define ANGLE_60     320
#define ANGLE_90     480
#define ANGLE_180    960
#define ANGLE_270   1440
#define ANGLE_360   1920

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TRIGTBL_H */
