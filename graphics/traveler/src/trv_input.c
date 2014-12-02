/****************************************************************************
 * apps/graphics/traveler/trv_input.c
 * This file contains the main logic for the NuttX version of Traveler
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_input.h"

/****************************************************************************
 * Public Data
 *************************************************************************/

extern enum trv_move_event_e g_move_event;
extern enum trv_turn_event_e g_turn_event;
extern enum trv_door_event_e g_door_event;
extern trv_coord_t g_trv_move_rate;
extern trv_coord_t g_trv_turn_rate;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 *************************************************************************/

static const char g_default_worldfile[] = "transfrm.wld";
static FAR struct trv_graphics_info_s g_trv_ginfo;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_input_initialize
 *
 * Description:
 *   Initialize the selected input device
 *
 ****************************************************************************/

void trv_input_initialize(void)
{
#warning Missing Logic
}

/****************************************************************************
 * Name: trv_input_read
 *
 * Description:
 *   Read the next input froom the input device
 *
 ****************************************************************************/

void trv_input_read(void)
{
#warning Missing Logic
}

/****************************************************************************
 * Name: trv_input_terminate
 *
 * Description:
 *   Terminate input and free resources
 *
 ****************************************************************************/

void trv_input_terminate(void)
{
#warning Missing Logic
}
