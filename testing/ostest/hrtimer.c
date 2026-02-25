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

#define HRTIMER_TEST_RAND_ITER (1024 * 2)
#define HRTIMER_TEST_CSECTION  1024
#define HRTIMER_TEST_THREAD_NR (CONFIG_SMP_NCPUS * 8)

/* Set the tolerent latency to 10ms to allow hrtimer_test to pass
 * in QEMU.
 *
 * QEMU is a virtual platform, vCPUs can be preempted by any
 * high priority thread. This can cause the timer to be triggered
 * later than expected. This is especially true on QEMU because it
 * has a lot of overhead. The timer resolution is also less precise
 * than on real hardware. Using a larger latency ensures that tests
 * do not fail due to timing inaccuracies.
 *
 * On real hardware (verified on the a2g-tc397-5v-tft board), this
 * latency can be reduced to less than 5 ns because timers are precise
 * and deterministic.
 */

#define HRTIMER_TEST_TOLERENT_LATENCY (10 * NSEC_PER_MSEC)

#define hrtimer_test_ndelay(delay_ns) usleep(delay_ns / 1000 + 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Structure for HRTimer test tracking */

typedef struct hrtimer_test_s
{
  struct hrtimer_s  timer;     /* HRTimer instance */
  spinlock_t        lock;      /* Spinlock */
  volatile uint64_t timestamp; /* Previous timestamp in nanoseconds */
  volatile uint64_t count;     /* Number of timer expirations */
  uint64_t          period;    /* Expected period between expirations */
  volatile uint8_t  state;     /* Test state */
} hrtimer_test_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint64_t hrtimer_test_callback_oneshot(FAR const hrtimer_t *timer,
                                              uint64_t expired_ns)
{
  FAR hrtimer_test_t *param = (FAR hrtimer_test_t *)timer;

  /* Save the timestamp when the callback was triggered */

  param->timestamp = clock_systime_nsec();

  /* Increment the callback count */

  param->count++;

  return 0;
}

static void hrtimer_test_checkdelay(uint64_t timestamp, uint64_t expected)
{
  int64_t diff = timestamp - expected;

  /* Ensure the hrtimer trigger time is not earlier than expected. */

  ASSERT(diff >= 0);

  /* If the timer latency exceeds the tolerance, print a warning. */

  if (diff > HRTIMER_TEST_TOLERENT_LATENCY)
    {
      printf("hrtimer_test: [WARNING] hrtimer latency %" PRId64
             " is too late!!! (> %u)\n", diff,
             (unsigned)HRTIMER_TEST_TOLERENT_LATENCY);
    }
}

static void hrtimer_test_oneshot(FAR hrtimer_test_t *param, uint64_t delay)
{
  uint64_t       count;
  uint64_t       now;
  FAR hrtimer_t *timer = &param->timer;

  printf("hrtimer_test_oneshot %" PRIu64 " ns\n", delay);

  /* Save the current callback count. */

  count = param->count;

  /* Save the current system time. */

  now = clock_systime_nsec();

  ASSERT(hrtimer_start(timer, hrtimer_test_callback_oneshot,
                       delay + now, HRTIMER_MODE_ABS) == OK);

  /* Wait until the callback is triggered exactly once. */

  while (count + 1 != param->count)
    {
      hrtimer_test_ndelay(delay);
    }

  /* Check if the delay is within the acceptable tolerance. */

  hrtimer_test_checkdelay(param->timestamp, now + delay);

  /* Cancel the timer. */

  hrtimer_cancel_sync(timer);
}

static void hrtimer_test_maximum(FAR hrtimer_test_t *param)
{
  uint64_t       count;
  uint64_t       rest;
  FAR hrtimer_t *timer = &param->timer;

  count = param->count;

  /* Start the hrtimer with maximum */

  ASSERT(hrtimer_start(timer, hrtimer_test_callback_oneshot, UINT64_MAX,
                       HRTIMER_MODE_REL) == OK);

  /* Sleep for at least 1s */

  hrtimer_test_ndelay(USEC_PER_SEC / 100);

  /* Ensure hrtimer is not alarmed */

  ASSERT(count == param->count);

  rest = hrtimer_gettime(timer);

  ASSERT(rest < UINT64_MAX);

  ASSERT(hrtimer_cancel_sync(timer) == OK);

  printf("hrtimer_start with maximum delay, rest %" PRIu64 "\n", rest);
}

static void hrtimer_test_rand(FAR hrtimer_test_t *param, uint64_t rand_ns)
{
  uint64_t       count;
  uint64_t       now;
  unsigned int   idx;
  uint64_t       delay;
  irqstate_t     flags;
  FAR hrtimer_t *timer = &param->timer;

  printf("hrtimer_test_rand %" PRIu64 " ns\n", rand_ns);

  /* Perform multiple iterations with random delays. */

  for (idx = 0; idx < HRTIMER_TEST_RAND_ITER; idx++)
    {
      /* Generate a random delay within the specified range. */

      delay = rand() % rand_ns;

      ASSERT(timer->func == NULL);

      /* Enter critical section if the callback count is odd. */

      count = param->count;

      if (count % 2u)
        {
          flags = up_irq_save();
        }

      now = clock_systime_nsec();
      ASSERT(hrtimer_start(timer, hrtimer_test_callback_oneshot,
                           delay, HRTIMER_MODE_REL) == 0);
      if (count % 2u)
        {
          up_irq_restore(flags);
        }

      /* Decide to wait for the callback or cancel the hrtimer. */

      if (delay % 2u)
        {
          /* Wait for the callback. */

          while (count + 1u != param->count)
            {
              hrtimer_test_ndelay(delay);
            }

          /* Check the delay if the callback count is odd. */

          if (count % 2u)
            {
              hrtimer_test_checkdelay(param->timestamp, now + delay);
            }
        }

      hrtimer_cancel_sync(timer);
      ASSERT(timer->func == NULL);
    }

  hrtimer_cancel_sync(timer);
}

static uint64_t hrtimer_test_cancel_callback(FAR const hrtimer_t *timer,
                                             uint64_t expired_ns)
{
  FAR hrtimer_test_t *param = (FAR hrtimer_test_t *)timer;
  FAR spinlock_t      *lock = &param->lock;
  uint64_t            delay = 0;
  irqstate_t          flags = spin_lock_irqsave(lock);

  /* Random sleep */

  delay = expired_ns % param->period;

  /* Check if the version is same. */

  if (expired_ns == timer->expired)
    {
      param->timestamp = clock_systime_nsec();

      /* Increment the callback count */

      param->count++;
    }

  spin_unlock_irqrestore(lock, flags);

  up_ndelay(delay);

  return 0;
}

static void hrtimer_test_rand_cancel(FAR hrtimer_test_t *param,
                                     uint64_t rand_ns)
{
  uint64_t       now;
  unsigned int   idx;
  uint64_t     count;
  uint64_t     delay;
  irqstate_t   flags;
  spinlock_t   *lock = &param->lock;

  printf("hrtimer_test_rand cancel %" PRIu64 " ns\n", rand_ns);

  param->period = rand_ns;

  /* Perform multiple iterations with random delays. */

  for (idx = 0; idx < HRTIMER_TEST_RAND_ITER; idx++)
    {
      /* Generate a random delay within the specified range. */

      delay = rand() % rand_ns;

      flags = spin_lock_irqsave(lock);

      now   = clock_systime_nsec();
      count = param->count;
      ASSERT(hrtimer_start(&param->timer, hrtimer_test_cancel_callback,
                           delay, HRTIMER_MODE_REL) == 0);

      spin_unlock_irqrestore(lock, flags);

      /* Decide to wait for the callback or cancel the hrtimer. */

      if (delay % 2u)
        {
          /* Wait for the callback finished. */

          while (param->count != count + 1u)
            {
              hrtimer_test_ndelay(delay);
            }

          hrtimer_test_checkdelay(param->timestamp, now + delay);
        }

      hrtimer_cancel(&param->timer);
    }

  hrtimer_cancel_sync(&param->timer);
}

static uint64_t hrtimer_test_callback_period(FAR const hrtimer_t *timer,
                                             uint64_t expired_ns)
{
  FAR hrtimer_test_t *param = (FAR hrtimer_test_t *)timer;
  uint64_t         interval = param->period;

  param->count++;
  param->timestamp = clock_systime_nsec();

  return interval;
}

static void hrtimer_test_period(FAR hrtimer_test_t *param,
                                uint64_t delay_ns,
                                unsigned int iters)
{
  uint64_t   timestamp;
  uint64_t       count = param->count;
  FAR hrtimer_t *timer = &param->timer;

  printf("hrtimer_test_period %" PRIu64 " ns\n", delay_ns);

  param->period = delay_ns;
  ASSERT(param->period > 0);

  timestamp = clock_systime_nsec();

  ASSERT(hrtimer_start(timer, hrtimer_test_callback_period,
                       delay_ns, HRTIMER_MODE_REL) == OK);

  hrtimer_test_ndelay(iters * delay_ns);

  hrtimer_cancel_sync(timer);
  ASSERT(timer->func == NULL);

  printf("periodical hrtimer triggered %" PRIu64 " times, "
         "elapsed nsec %" PRIu64 "\n", param->count - count,
         param->timestamp - timestamp);

  if (param->count - count < iters)
    {
      printf("hrtimer_test: [WARNING] periodical hrtimer"
             "triggered times < %u\n", iters);
    }
}

#ifdef CONFIG_SMP
static uint64_t hrtimer_test_callback_crita(FAR const hrtimer_t *timer,
                                            uint64_t expired_ns)
{
  FAR hrtimer_test_t *param = (FAR hrtimer_test_t *)timer;

  /* change status */

  if (param->state == 0)
    {
      param->state = 1;
      param->count++;
    }

  /* check whether parameter be changed by another critical section */

  ASSERT(param->state == 1);
  param->state = 0;

  return 0;
}

static uint64_t hrtimer_test_callback_critb(FAR const hrtimer_t *timer,
                                            uint64_t expired_ns)
{
  FAR hrtimer_test_t *param = (FAR hrtimer_test_t *)timer;

  /* change status */

  if (param->state == 1)
    {
      param->state = 0;
      param->count++;
    }

  /* check whether parameter be changed by another critical section */

  ASSERT(param->state == 0);
  param->state = 1;

  return 0;
}

static uint64_t hrtimer_test_callback_critdelay(FAR const hrtimer_t *timer,
                                                uint64_t expired_ns)
{
  FAR hrtimer_test_t *param = (FAR hrtimer_test_t *)timer;
  FAR spinlock_t *lock      = &param->lock;
  irqstate_t flags;

  flags = spin_lock_irqsave(lock);
  param->count++;
  spin_unlock_irqrestore(lock, flags);

  up_ndelay(100 * NSEC_PER_USEC);

  return 300 * NSEC_PER_USEC;
}

static void hrtimer_test_cancel_sync(FAR hrtimer_test_t *param)
{
  unsigned int idx = 0;

  ASSERT(!param->timer.func);

  param->count = 0;

  /* This test is to validate if the hrtimer can ensure the
   * callback function be finished after the hrtimer_cancel_sync
   * is called.
   */

  for (idx = 0; idx < HRTIMER_TEST_CSECTION; )
    {
      param->state = 0;
      hrtimer_start(&param->timer, hrtimer_test_callback_crita,
                    0, HRTIMER_MODE_REL);

      hrtimer_cancel_sync(&param->timer);
      param->state = 1;
      hrtimer_start(&param->timer, hrtimer_test_callback_critb,
                    0, HRTIMER_MODE_REL);

      if (++idx % (HRTIMER_TEST_CSECTION / 4) == 0)
        {
          printf("hrtimer_test_cancel_sync passed %d times.\n", idx);
        }

      hrtimer_cancel_sync(&param->timer);
    }
}

static void hrtimer_test_cancel_periodic(FAR hrtimer_test_t *param)
{
  uint64_t       count;
  unsigned int     idx = 0;
  FAR spinlock_t *lock = &param->lock;

  ASSERT(!param->timer.func);

  param->count = 0;

  /* This test to check if the hrtimer can ensure the perodical callback
   * can not restart the timer again after the hrtimer_cancel_sync is
   * called.
   */

  for (idx = 0; idx < HRTIMER_TEST_CSECTION; idx++)
    {
      irqstate_t flags = spin_lock_irqsave(lock);

      hrtimer_start(&param->timer, hrtimer_test_callback_critdelay,
                    0, HRTIMER_MODE_REL);

      spin_unlock_irqrestore(lock, flags);

      up_ndelay(10000);

      flags = spin_lock_irqsave(lock);

      hrtimer_start(&param->timer, hrtimer_test_callback_critdelay,
                    0, HRTIMER_MODE_REL);

      spin_unlock_irqrestore(lock, flags);

      hrtimer_cancel(&param->timer);

      up_ndelay(10000);

      /* The hrtimer should not be restarted again after the cancellation. */

      ASSERT(!param->timer.func);

      hrtimer_cancel_sync(&param->timer);
      count = param->count;

      hrtimer_test_ndelay(10000);

      ASSERT(count == param->count);

      if (++idx % (HRTIMER_TEST_CSECTION / 4) == 0)
        {
          printf("hrtimer_test_cancel_periodic passed %d times. count %"
                 PRIu64 "\n", idx, param->count);
        }
    }

  hrtimer_cancel_sync(&param->timer);
}
#endif

/****************************************************************************
 * Name: hrtimer_test_thread
 *
 * Description:
 *   Thread function to repeatedly test HRTimer start/cancel behavior.
 *
 ****************************************************************************/

static void * hrtimer_test_thread(void *arg)
{
  hrtimer_test_t param =
    {
      0
    };

  hrtimer_init(&param.timer);

  /* Delay = 0 */

  hrtimer_test_oneshot(&param, 0u);

  /* 0 < Delay < 10000 */

  hrtimer_test_oneshot(&param, 1u);
  hrtimer_test_oneshot(&param, 10u);
  hrtimer_test_oneshot(&param, 100u);
  hrtimer_test_oneshot(&param, 1000u);
  hrtimer_test_oneshot(&param, 10000u);

  /* 10000 < Delay < 10000000 */

  hrtimer_test_oneshot(&param, 100000u);
  hrtimer_test_oneshot(&param, 1000000u);
  hrtimer_test_oneshot(&param, 10000000u);

#ifdef CONFIG_SMP
  /* Test hrtimer_cancel_sync */

  hrtimer_test_cancel_sync(&param);

  /* Test hrtimer_cancel */

  hrtimer_test_cancel_periodic(&param);
#endif

  /* Maximum hrtimer delay test. */

  hrtimer_test_maximum(&param);

  /* Period hrtimer delay 100000ns */

  hrtimer_test_period(&param, 1000000u, 128u);

  /* Random delay 12345ns and 67890ns */

  hrtimer_test_rand(&param, 12345u);
  hrtimer_test_rand_cancel(&param, 67890u);

  return 0;
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
  unsigned int    thread_id;
  pthread_attr_t       attr;
  pthread_t        pthreads[HRTIMER_TEST_THREAD_NR];

  printf("hrtimer_test start...\n");

  ASSERT(pthread_attr_init(&attr) == 0);

  sparam.sched_priority = PTHREAD_DEFAULT_PRIORITY;
  ASSERT(pthread_attr_setschedparam(&attr, &sparam) == 0);

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

  printf("hrtimer_test end...\n");
}
