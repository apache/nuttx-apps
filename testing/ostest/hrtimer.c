/****************************************************************************
 * apps/testing/ostest/hrtimer.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/hrtimer.h>

#include <stdio.h>
#include <sched.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NSEC_PER_50MS (50 * NSEC_PER_MSEC)

/* Set a 1ms margin to allow hrtimertest to pass in QEMU.
 *
 * QEMU is a virtual platform, and its timer resolution and scheduling
 * latency may be less precise than on real hardware. Using a larger
 * margin ensures that tests do not fail due to timing inaccuracies.
 *
 * On real hardware (verified on the a2g-tc397-5v-tft board), this
 * margin can be reduced to less than 5 ns because timers are precise
 * and deterministic.
 */

#define HRTIMER_TEST_MARGIN    (NSEC_PER_MSEC)

/* Simple assertion macro for HRTimer test cases */
#define HRTIMER_TEST(expr, value)                                   \
  do                                                                \
    {                                                               \
      ret = (expr);                                                 \
      if (ret != (value))                                           \
        {                                                           \
          printf("ERROR: HRTimer test failed, line=%d ret=%d\n",   \
                 __LINE__, ret);                                    \
          ASSERT(false);                                            \
        }                                                           \
    }                                                               \
  while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Structure for HRTimer test tracking */

struct hrtimer_test_s
{
  hrtimer_t timer;    /* HRTimer instance */
  uint64_t  previous; /* Previous timestamp in nanoseconds */
  uint32_t  count;    /* Number of timer expirations */
  uint32_t  period;   /* Expected period between expirations */
  bool      active;   /* True while the test is still running */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hrtimer_test_init
 *
 * Description:
 *   Initialize a hrtimer_test_s structure for a new test.
 *
 * Input Parameters:
 *   hrtimer_test - Pointer to the test structure to initialize
 *   period       - Expected timer period in nanoseconds
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void hrtimer_test_init(FAR struct hrtimer_test_s *hrtimer_test,
                              uint32_t period)
{
  hrtimer_test->previous = 0;
  hrtimer_test->count = 0;
  hrtimer_test->active = true;
  hrtimer_test->period = period;
}

/****************************************************************************
 * Name: test_hrtimer_callback
 *
 * Description:
 *   HRTimer callback function for test.
 *
 *   - Verifies the timer interval is exactly 500ms (nanosecond precision)
 *   - Stops the test after 15 expirations
 *   - Re-arms the timer in absolute mode
 *
 * Input Parameters:
 *   hrtimer - Pointer to the expired HRTimer instance
 *   expired - The expired value of hrtimer
 *
 * Returned Value:
 *   Timer period in nanoseconds (NSEC_PER_50MS)
 *
 ****************************************************************************/

static uint64_t
test_hrtimer_callback(FAR hrtimer_t *hrtimer, uint64_t expired)
{
  struct timespec ts;
  uint32_t diff;
  uint64_t now;
  int ret;

  UNUSED(expired);

  FAR struct hrtimer_test_s *test =
    (FAR struct hrtimer_test_s *)hrtimer;

  /* Increment expiration count */

  test->count++;

  /* Get current system time */

  clock_systime_timespec(&ts);
  now = clock_time2nsec(&ts);

  /* Skip comparison for first two invocations */

  if (test->count > 2)
    {
      /* Verify the timer interval is exactly
       * 500ms with nsec resolution
       */

      diff = (uint32_t)(now - test->previous);

      HRTIMER_TEST(NSEC_PER_50MS < diff + HRTIMER_TEST_MARGIN, true);
      HRTIMER_TEST(NSEC_PER_50MS > diff - HRTIMER_TEST_MARGIN, true);
    }

  test->previous = now;

  /* Stop the test after 15 expirations */

  if (test->count >= 15)
    {
      ret = hrtimer_cancel(hrtimer);
      HRTIMER_TEST(ret, 0);

      test->active = false;
    }

  return test->period;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hrtimer_test
 *
 * Description:
 *   Entry point for high-resolution timer functional test.
 *
 *   - Initializes a HRTimer
 *   - Starts it with a 500ms relative timeout
 *   - Verifies subsequent expirations occur at 500ms intervals
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void hrtimer_test(void)
{
  int ret;
  struct hrtimer_test_s hrtimer_test;

  /* Initialize test structure */

  hrtimer_test_init(&hrtimer_test, NSEC_PER_50MS);

  /* Initialize the high-resolution timer */

  hrtimer_init(&hrtimer_test.timer);

  /* Start the timer with 500ms relative timeout */

  ret = hrtimer_start(&hrtimer_test.timer,
                      test_hrtimer_callback,
                      hrtimer_test.period,
                      HRTIMER_MODE_REL);

  HRTIMER_TEST(ret, OK);

  /* Wait until the test completes */

  while (hrtimer_test.active)
    {
      usleep(USEC_PER_MSEC);
    }
}
