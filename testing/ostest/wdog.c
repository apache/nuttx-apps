/****************************************************************************
 * apps/testing/ostest/wdog.c
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
#include <nuttx/arch.h>
#include <nuttx/wdog.h>

#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WDOGTEST_RAND_ITER           1024
#define WDOGTEST_THREAD_NR           8
#define WDOGTEST_TOLERENT_LATENCY_US 5000

#define wdtest_assert(x)             _ASSERT(x, __FILE__, __LINE__)

#define wdtest_printf(...)           printf(__VA_ARGS__)

#define wdtest_delay(delay_us)       usleep(delay_us)

/****************************************************************************
 * Private Type
 ****************************************************************************/

typedef struct wdtest_param_s
{
  FAR struct wdog_s  *wdog;
  sclock_t            interval;
  uint64_t            callback_cnt;
  clock_t             triggered_tick;
} wdtest_param_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_FLAT
static void wdtest_callback(wdparm_t param)
{
  struct timespec tp;
  FAR wdtest_param_t *wdtest_param = (FAR wdtest_param_t *)param;

  clock_gettime(CLOCK_MONOTONIC, &tp);

  wdtest_param->callback_cnt   += 1;
  wdtest_param->triggered_tick  = clock_time2ticks(&tp);
}

static void wdtest_once(FAR struct wdog_s *wdog, FAR wdtest_param_t *param,
                        sclock_t delay_us)
{
  uint64_t        cnt;
  long long       diff;
  clock_t         wdset_tick;
  struct timespec tp;
  clock_t         delay_ticks = USEC2TICK((clock_t)delay_us);

  wdtest_printf("wdtest_once %lld us\n", (long long)delay_us);

  clock_gettime(CLOCK_MONOTONIC, &tp);

  wdset_tick = clock_time2ticks(&tp);
  cnt        = param->callback_cnt;
  wdtest_assert(wd_start(wdog, delay_ticks, wdtest_callback,
                         (wdparm_t)param) == OK);

  while (cnt + 1 != param->callback_cnt)
    {
      wdtest_delay(delay_us);
    }

  diff = (long long)(param->triggered_tick - wdset_tick);

  /* Ensure diff - delay_ticks is within the tolerance limit. */

  wdtest_assert(diff - delay_ticks >= 0);
  wdtest_assert(diff - delay_ticks <
                USEC2TICK(WDOGTEST_TOLERENT_LATENCY_US) + 1);
  wdtest_printf("wdtest_once latency ticks %lld\n", diff - delay_ticks);
}

static void wdtest_rand(FAR struct wdog_s *wdog, FAR wdtest_param_t *param,
                        sclock_t rand_us)
{
  int      idx;
  sclock_t delay_us;
  uint64_t cnt = param->callback_cnt;

  for (idx = 0; idx < WDOGTEST_RAND_ITER; idx++)
    {
      delay_us = rand() % rand_us;
      wdtest_assert(wd_start(wdog, USEC2TICK(delay_us), wdtest_callback,
                             (wdparm_t)param) == 0);

      /* Wait or Cancel 50/50 */

      if (delay_us % 2)
        {
          while (cnt + 1 != param->callback_cnt)
            {
              wdtest_delay(delay_us);
            }
        }
      else
        {
          wd_cancel(wdog);
        }

      cnt = param->callback_cnt;
    }
}

static void wdtest_callback_recursive(wdparm_t param)
{
  struct timespec     tp;
  FAR wdtest_param_t *wdtest_param = (FAR wdtest_param_t *)param;
  sclock_t            interval = wdtest_param->interval;

  wd_start(wdtest_param->wdog, interval,
           wdtest_callback_recursive, param);

  clock_gettime(CLOCK_MONOTONIC, &tp);

  wdtest_param->callback_cnt   += 1;
  wdtest_param->triggered_tick  = clock_time2ticks(&tp);
}

static void wdtest_recursive(FAR struct wdog_s *wdog,
                             FAR wdtest_param_t *param,
                             sclock_t delay_us,
                             unsigned int times)
{
  uint64_t        cnt;
  struct timespec tp;
  clock_t         wdset_tick;

  wdtest_printf("wdtest_recursive %lldus\n", (long long)delay_us);
  cnt = param->callback_cnt;
  param->wdog = wdog;
  param->interval = (sclock_t)USEC2TICK((clock_t)delay_us);

  wdtest_assert(param->interval >= 0);

  clock_gettime(CLOCK_MONOTONIC, &tp);
  wdset_tick = clock_time2ticks(&tp);

  wdtest_assert(wd_start(param->wdog, param->interval,
                         wdtest_callback_recursive,
                         (wdparm_t)param) == OK);

  while (param->callback_cnt < cnt + times)
    {
      wdtest_delay(delay_us);
    }

  wdtest_assert(wd_cancel(param->wdog) == 0);

  wdtest_printf("recursive wdog triggered %u times, elapsed tick %lld\n",
                times,
                (long long)(param->triggered_tick - wdset_tick));
}

static void wdog_test_run(FAR wdtest_param_t *param)
{
  uint64_t      cnt;
  sclock_t      rest;
  sclock_t      delay;
  struct wdog_s test_wdog =
  {
    0
  };

  /* Wrong arguments, all 7 combinations */

  wdtest_assert(wd_start(NULL, 0, NULL, (wdparm_t)NULL) != OK);
  wdtest_assert(wd_start(NULL, 0, wdtest_callback, (wdparm_t)NULL) != OK);
  wdtest_assert(wd_start(NULL, -1, NULL, (wdparm_t)NULL) != OK);
  wdtest_assert(wd_start(NULL, -1, wdtest_callback, (wdparm_t)NULL) != OK);
  wdtest_assert(wd_start(&test_wdog, 0, NULL, (wdparm_t)NULL) != OK);
  wdtest_assert(wd_start(&test_wdog, -1, NULL, (wdparm_t)NULL) != OK);
  wdtest_assert(wd_start(&test_wdog, -1, wdtest_callback, (wdparm_t)NULL)
                != OK);

  /* Delay = 0 */

  wdtest_once(&test_wdog, param, 0);

  /* Delay > 0, small */

  wdtest_once(&test_wdog, param, 1);
  wdtest_once(&test_wdog, param, 10);
  wdtest_once(&test_wdog, param, 100);
  wdtest_once(&test_wdog, param, 1000);
  wdtest_once(&test_wdog, param, 10000);
  wdtest_delay(10);

  /* Delay > 0, middle 100us */

  wdtest_once(&test_wdog, param, 100000);
  wdtest_once(&test_wdog, param, 1000000);

  /* Delay > 0, maximum */

  cnt = param->callback_cnt;

  /* Maximum */

  delay = ((clock_t)1 << (sizeof(sclock_t) * CHAR_BIT - 1)) - 1;
  wdtest_assert(wd_start(&test_wdog, delay,
                         wdtest_callback, (wdparm_t)param) == OK);

  /* Sleep for 1s */

  wdtest_delay(USEC_PER_SEC);

  /* Testing wd_gettime */

  wdtest_assert(wd_gettime(NULL) == 0);
  wdtest_assert(wd_cancel(NULL) != 0);

  /* Ensure watchdog not alarmed */

  wdtest_assert(cnt == param->callback_cnt);

  rest = wd_gettime(&test_wdog);

  wdtest_assert(rest < delay && rest > (delay >> 1));

  wdtest_printf("wd_start with maximum delay, cancel %ld\n", (long)rest);

  wdtest_assert(wd_cancel(&test_wdog) == 0);

  /* Delay wraparound (delay < 0) */

  delay = (sclock_t)((clock_t)delay + 1);
  wdtest_assert(wd_start(&test_wdog, delay,
                wdtest_callback, (wdparm_t)param) != OK);
  wdtest_assert(wd_gettime(&test_wdog) == 0);

  /* Recursive wdog delay from 10us to 1000us */

  wdtest_recursive(&test_wdog, param, 10, 1000);
  wdtest_delay(10);
  wdtest_recursive(&test_wdog, param, 100, 100);
  wdtest_delay(10);
  wdtest_recursive(&test_wdog, param, 1000, 10);
  wdtest_delay(10);

  /* Random delay ~1us */

  wdtest_rand(&test_wdog, param, 1024);
  wdtest_delay(10);

  /* Random delay ~12us */

  wdtest_rand(&test_wdog, param, 12345);
}

/* Multi threaded */

static FAR void *wdog_test_thread(FAR void *param)
{
  wdog_test_run(param);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void wdog_test(void)
{
  unsigned int   thread_id;
  pthread_attr_t attr;
  pthread_t      pthreads[WDOGTEST_THREAD_NR];
  wdtest_param_t params[WDOGTEST_THREAD_NR] =
    {
      0
    };

  printf("wdog_test start...\n");

  wdtest_assert(pthread_attr_init(&attr) == 0);

  /* Create wdog test thread */

  for (thread_id = 0; thread_id < WDOGTEST_THREAD_NR; thread_id++)
    {
      wdtest_assert(pthread_create(&pthreads[thread_id], &attr,
                                   wdog_test_thread, &params[thread_id])
                                   == 0);
    }

  for (thread_id = 0; thread_id < WDOGTEST_THREAD_NR; thread_id++)
    {
      pthread_join(pthreads[thread_id], NULL);
    }

  wdtest_assert(pthread_attr_destroy(&attr) == 0);

  printf("wdog_test end...\n");
}
#endif
