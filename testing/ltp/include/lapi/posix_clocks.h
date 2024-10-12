/****************************************************************************
 * apps/testing/ltp/include/lapi/posix_clocks.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef _LTP_LAPI_POSIX_CLOCKS_H__
#define _LTP_LAPI_POSIX_CLOCKS_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_CLOCKS 16

#define CLOCK_MONOTONIC_RAW 5

#define CLOCK_REALTIME_COARSE 6

#define CLOCK_MONOTONIC_COARSE 7

#define CLOCK_REALTIME_ALARM 8

#define CLOCK_BOOTTIME_ALARM 9

#define CLOCK_TAI 11

#endif /* _LTP_LAPI_POSIX_CLOCKS_H__ */
