/****************************************************************************
 * apps/system/uorb/test/utility.h
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

#ifndef __APP_SYSTEM_UORB_UORB_TEST_UTILITY_H
#define __APP_SYSTEM_UORB_UORB_TEST_UTILITY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "uORB/uORB.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct orb_test_s
{
  uint64_t timestamp;
  int32_t val;
};

struct orb_test_medium_s
{
  uint64_t timestamp;
  int32_t val;
};

struct orb_test_large_s
{
  uint64_t timestamp;
  int32_t val;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* register this as object request broker structure */

ORB_DECLARE(orb_test);
ORB_DECLARE(orb_multitest);
ORB_DECLARE(orb_test_large);
ORB_DECLARE(orb_test_medium);
ORB_DECLARE(orb_test_medium_multi);
ORB_DECLARE(orb_test_medium_wrap_around);
ORB_DECLARE(orb_test_medium_queue);
ORB_DECLARE(orb_test_medium_queue_poll);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int test_note(FAR const char *fmt, ...);
int test_fail(FAR const char *fmt, ...);

#endif /* __APP_SYSTEM_UORB_TEST_UTILITY_H */
