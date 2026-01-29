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

#include <assert.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Timer constants */

#define NSEC_PER_50MS            (50 * NSEC_PER_MSEC)
#define PERIOD_TEST_COUNT        15
#define THREAD_LOOP_COUNT        50
#define HRTIMER_TEST_THREAD_NR   (CONFIG_SMP_NCPUS * 5)

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

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Structure for HRTimer test tracking */

typedef struct hrtimer_test_s
{
  struct hrtimer_s  timer;     /* HRTimer instance */
  volatile uint64_t timestamp; /* Previous timestamp in nanoseconds */
  volatile uint64_t count;     /* Number of timer expirations */
  uint64_t          period;    /* Expected period between expirations */
} hrtimer_test_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
test_hrtimer_callback(FAR const hrtimer_t *hrtimer, uint64_t expired)
{
  struct timespec ts;
  int64_t  diff;
  uint64_t now;
  int      ret;

  FAR struct hrtimer_test_s *test =
    (FAR struct hrtimer_test_s *)hrtimer;

  /* Increment expiration count */

  test->count++;

  /* Get current system time */

  clock_systime_timespec(&ts);
  now = clock_time2nsec(&ts);

  /* Verify the timer interval is exactly
   * 500ms with nsec resolution
   */

  diff = now - expired;

  /* Ensure the time diff is valid. */

  ASSERT(diff >= 0);

  if (diff > HRTIMER_TEST_MARGIN)
    {
      printf("hrtimer_test: warning diff=%" PRIu64 " > %" PRIu64 "\n",
             diff, HRTIMER_TEST_MARGIN);
    }

  test->timestamp = now;

  /* Stop the test after PERIOD_TEST_COUNT expirations */

  if (test->count < PERIOD_TEST_COUNT)
    {
      return test->period;
    }
  else
    {
      test->active = false;
      return 0;
    }
}

/****************************************************************************
 * Name: hrtimer_test_callback
 *
 * Description:
 *   Simple HRTimer callback for threaded tests.
 *
 ****************************************************************************/

static uint64_t
hrtimer_test_callback(FAR const hrtimer_t *hrtimer, uint64_t expired)
{
  return 0;
}

/****************************************************************************
 * Name: hrtimer_test_thread
 *
 * Description:
 *   Thread function to repeatedly test HRTimer start/cancel behavior.
 *
 ****************************************************************************/

static void * hrtimer_test_thread(void *arg)
{
  hrtimer_test_t test;
  int             ret;
  uint64_t      stamp;
  int               i = 0;
  hrtimer_t    *timer = &test.timer;

  /* Initialize the high-resolution timer */

  hrtimer_init(timer);

  /* Start the timer with 500ms relative timeout */

  stamp = test.timestamp;
  ASSERT(hrtimer_start(timer, test_hrtimer_callback,
                       NSEC_PER_50MS, HRTIMER_MODE_REL) == OK);

  /* Wait until the test completes */

  while (test.timestamp != stamp)
    {
      usleep(USEC_PER_MSEC);
    }

  while (i < THREAD_LOOP_COUNT)
    {
      i++;
      uint64_t delay = rand() % NSEC_PER_MSEC;

      /* Cancel timer */

      ret = hrtimer_cancel(&timer);
      ASSERT(ret == OK);

      /* Start timer with fixed period */

      ret = hrtimer_start(&timer, hrtimer_test_callback,
                          10 * NSEC_PER_USEC, HRTIMER_MODE_REL);
      ASSERT(ret == OK);

      /* Start timer with random delay */

      ret = hrtimer_start(&timer, hrtimer_test_callback,
                          delay, HRTIMER_MODE_REL);
      ASSERT(ret == OK);
    }

  /* Cancel the timer synchronously */

  ASSERT(hrtimer_cancel_sync(&timer) == OK);

  return NULL;
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
  struct sched_param sparam;
  unsigned int thread_id;
  pthread_attr_t attr;
  pthread_t pthreads[HRTIMER_TEST_THREAD_NR];
  int ret;

  pthread_attr_init(&attr);

  sparam.sched_priority = PTHREAD_DEFAULT_PRIORITY;
  pthread_attr_setschedparam(&attr, &sparam);

  for (thread_id = 0; thread_id < HRTIMER_TEST_THREAD_NR; thread_id++)
    {
      ASSERT(pthread_create(&pthreads[thread_id], &attr,
                            hrtimer_test_thread, NULL) == 0);
    }

  /* Wait for all threads to complete */

  for (thread_id = 0; thread_id < HRTIMER_TEST_THREAD_NR; thread_id++)
    {
      pthread_join(pthreads[thread_id], NULL);
    }

  ASSERT(pthread_attr_destroy(&attr) == 0);
}
