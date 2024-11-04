/****************************************************************************
 * apps/system/uorb/sensor/eng.c
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

#include <sensor/eng.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static const char sensor_eng_format[] =
  "timestamp:%" PRIu64 ",eng0:%hf,eng1:%hf,eng2:%hf,eng3:%hf,"
  "status:0x%" PRIx32 "";
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(sensor_eng, struct sensor_eng, sensor_eng_format);
