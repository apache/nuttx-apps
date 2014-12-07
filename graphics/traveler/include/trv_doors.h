/****************************************************************************
 * apps/graphics/traveler/include/trv_doors.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_DOORS_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_DOORS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* This structure describes the characteristics of the door which currently
 * being opened.
 */

struct trv_opendoor_s
{
  FAR struct trv_rect_data_s *rect; /* Points to the current door rectangle */
  uint8_t       state;              /* State of the door being opened */
  trv_coord_t   zbottom;            /* Z-Coordinate of the bottom of the door */
  trv_coord_t   zdist;              /* Distance which the door has moved */
  int16_t       clock;              /* This is clock which counts down the time
                                     * remaining to keep the door open */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This structure describes the door which is currently opening */

struct trv_opendoor_s g_opendoor;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void trv_door_initialize(void);
void trv_door_animate(void);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_DOORS_H */
