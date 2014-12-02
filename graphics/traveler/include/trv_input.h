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

/****************************************************************************
 * Public Types
 ****************************************************************************/
/* INPUT */

enum trv_move_event_e
{
  INPUT_MOVE_NONE = 0,
  INPUT_MOVE_FORWARD,
  INPUT_MOVE_BACKWARD,
  INPUT_MOVE_LEFT,
  INPUT_MOVE_RIGHT
};

enum trv_turn_event_e
{
  INPUT_TURN_NONE = 0,
  INPUT_TURN_UP,
  INPUT_TURN_DOWN,
  INPUT_TURN_LEFT,
  INPUT_TURN_RIGHT,
};

enum trv_door_event_e
{
  INPUT_DOOR_NONE = 0,
  INPUT_DOOR_OPEN,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern enum trv_move_event_e g_move_event;
extern enum trv_turn_event_e g_turn_event;
extern enum trv_door_event_e g_door_event;
extern trv_coord_t g_trv_move_rate;
extern trv_coord_t g_trv_turn_rate;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void trv_input_initialize(void);
void trv_input_read(void);
void trv_input_terminate(void);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_INPUT_H */
