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
#include <nuttx/wdog.h>

#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WDOGTEST_RAND_ITER 1024
#define WDOGTEST_THREAD_NR 8

#define wdtest_assert(x)   _ASSERT(x, __ASSERT_FILE__, __ASSERT_LINE__)

#define wdtest_printf(...) printf(__VA_ARGS__)

/****************************************************************************
 * Private Type
 ****************************************************************************/

typedef struct wdtest_param_s
{
  uint64_t callback_cnt;
  clock_t  triggered_tick;
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
                        sclock_t delay_ns)
{
  uint64_t        cnt;
  long            diff;
  clock_t         wdset_tick;
  struct timespec tp;

  clock_gettime(CLOCK_MONOTONIC, &tp);

  wdset_tick = clock_time2ticks(&tp);
  cnt        = param->callback_cnt;
  wdtest_assert(wd_start(wdog, NSEC2TICK((clock_t)delay_ns), wdtest_callback,
                         (wdparm_t)param) == OK);
  usleep(delay_ns / 1000 + 1);
  diff = (long)(param->triggered_tick - wdset_tick);
  wdtest_printf("wd_start with delay %ld ns, diff ticks %ld\n",
                (long)delay_ns, diff);
  wdtest_assert(cnt + 1 == param->callback_cnt);
}

static void wdtest_rand(FAR struct wdog_s *wdog, FAR wdtest_param_t *param,
                        sclock_t rand_ns)
{
  int      idx;
  sclock_t delay_ns;
  uint64_t cnt = param->callback_cnt;

  for (idx = 0; idx < WDOGTEST_RAND_ITER; idx++)
    {
      delay_ns = rand() % rand_ns;
      wdtest_assert(wd_start(wdog, NSEC2TICK(delay_ns), wdtest_callback,
                             (wdparm_t)param) == 0);

      /* Wait or Cancel 50/50 */

      if (delay_ns % 2)
        {
          usleep((delay_ns / 1000) + 1);
          wdtest_assert(cnt + 1 == param->callback_cnt);
        }
      else
        {
          wd_cancel(wdog);
        }

      cnt = param->callback_cnt;
    }
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

  /* Delay > 0, middle 100us */

  wdtest_once(&test_wdog, param, 100000);

  /* Delay > 0, large ~123456us */

  wdtest_once(&test_wdog, param, 123456789);

  /* Delay > 0, maximum */

  cnt = param->callback_cnt;

  /* Maximum */

  delay = ((clock_t)1 << (sizeof(sclock_t) * CHAR_BIT - 1)) - 1;
  wdtest_assert(wd_start(&test_wdog, delay,
                         wdtest_callback, (wdparm_t)param) == OK);

  /* Sleep for 1s */

  usleep(USEC_PER_SEC);

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

  /* Random delay ~1024us */

  wdtest_rand(&test_wdog, param, 1024);

  /* Random delay ~12345us */

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
