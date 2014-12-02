/****************************************************************************
 * apps/graphics/traveler/include/trv_input.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_INPUT_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_INPUT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of units player moves forward or backward. */

#define STEP_DISTANCE 15

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct trv_input_s
{
  int16_t fwdrate;    /* Forward motion rate.  Negative is backward */
  int16_t leftrate;   /* Left motion rate.  Negative is right */
  int16_t yawrate;    /* Yaw turn rate.  Positive is to the left */
  int16_t pitchrate;  /* Pitch turn rate.  Positive is upward */
  int16_t stepheight; /* Size a a vertical step, if applicable */
  bool    dooropen;   /* True: Open a door */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Report positional inputs */

extern struct trv_input_s g_trv_input;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void trv_input_initialize(void);
void trv_input_read(void);
void trv_input_terminate(void);
#ifdef CONFIG_GRAPHICS_TRAVELER_NX_XYINPUT
void trv_input_xyinput(trv_coord_t xpos, trv_coord_t xpos, uint8_t buttons);
#endif

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_INPUT_H */
