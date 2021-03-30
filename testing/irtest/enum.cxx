/****************************************************************************
 * apps/testing/irtest/enum.cxx
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "enum.hpp"
#include <nuttx/lirc.h>
#include <stdio.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

ENUM_START(mode_t)
    ENUM_VALUE(LIRC_MODE_RAW)
    ENUM_VALUE(LIRC_MODE_PULSE)
    ENUM_VALUE(LIRC_MODE_MODE2)
    ENUM_VALUE(LIRC_MODE_SCANCODE)
    ENUM_VALUE(LIRC_MODE_LIRCCODE)
ENUM_END(mode_t, "0x%02x")

ENUM_START(features_t)
    ENUM_VALUE(LIRC_CAN_SEND_RAW)
    ENUM_VALUE(LIRC_CAN_SEND_PULSE)
    ENUM_VALUE(LIRC_CAN_SEND_MODE2)
    ENUM_VALUE(LIRC_CAN_SEND_SCANCODE)
    ENUM_VALUE(LIRC_CAN_SEND_LIRCCODE)
    ENUM_VALUE(LIRC_CAN_SET_SEND_CARRIER)
    ENUM_VALUE(LIRC_CAN_SET_SEND_DUTY_CYCLE)
    ENUM_VALUE(LIRC_CAN_SET_TRANSMITTER_MASK)
    ENUM_VALUE(LIRC_CAN_REC_RAW)
    ENUM_VALUE(LIRC_CAN_REC_PULSE)
    ENUM_VALUE(LIRC_CAN_REC_MODE2)
    ENUM_VALUE(LIRC_CAN_REC_SCANCODE)
    ENUM_VALUE(LIRC_CAN_REC_LIRCCODE)
    ENUM_VALUE(LIRC_CAN_SET_REC_CARRIER)
    ENUM_VALUE(LIRC_CAN_SET_REC_DUTY_CYCLE)
    ENUM_VALUE(LIRC_CAN_SET_REC_DUTY_CYCLE_RANGE)
    ENUM_VALUE(LIRC_CAN_SET_REC_CARRIER_RANGE)
    ENUM_VALUE(LIRC_CAN_GET_REC_RESOLUTION)
    ENUM_VALUE(LIRC_CAN_SET_REC_TIMEOUT)
    ENUM_VALUE(LIRC_CAN_SET_REC_FILTER)
    ENUM_VALUE(LIRC_CAN_MEASURE_CARRIER)
    ENUM_VALUE(LIRC_CAN_USE_WIDEBAND_RECEIVER)
    ENUM_VALUE(LIRC_CAN_NOTIFY_DECODE)
ENUM_END(features_t, "0x%08x")

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct enum_type *g_enum_table[] =
{
  &g_mode_t_type,
  &g_features_t_type,
  NULL,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
