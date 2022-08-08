/****************************************************************************
 * apps/system/uorb/sensor/ppgq.c
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

#include <sensor/ppgq.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static void print_sensor_ppgq_message(FAR const struct orb_metadata *meta,
                                      FAR const void *buffer)
{
  FAR const struct sensor_ppgq *message = buffer;
  const orb_abstime now = orb_absolute_time();

  uorbinfo_raw("%s:\ttimestamp: %" PRIu64 " (%" PRIu64 " us ago) "
               "ppg1: %" PRIu32 " ppg2: %" PRIu32 " ppg3: %" PRIu32 " "
               "ppg4: %" PRIu32 "current: %" PRIu32 " gain1: %" PRIu16 " "
               "gain2: %" PRIu16 " gain3: %" PRIu16 " gain4: %" PRIu16 "",
               meta->o_name, message->timestamp, now - message->timestamp,
               message->ppg[0], message->ppg[1], message->ppg[2],
               message->ppg[3], message->current, message->gain[0],
               message->gain[1], message->gain[2], message->gain[3]);
}
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_ppgq, struct sensor_ppgq, print_sensor_ppgq_message);
