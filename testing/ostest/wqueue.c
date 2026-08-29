/****************************************************************************
 * apps/testing/ostest/wqueue.c
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

#include <assert.h>
#include <errno.h>
#ifndef CONFIG_DISABLE_PTHREAD
#include <pthread.h>
#endif
#include <sched.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/clock.h>
#include <nuttx/wqueue.h>

#include "ostest.h"

#ifdef CONFIG_DISABLE_PTHREAD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WQUEUE_TEST_TIMEOUT_SEC 2
#define WQUEUE_TEST_DELAY_USEC  (50 * 1000)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct periodic_test_s
{
  struct work_s work;
  sem_t done;
  int calls;
  int result;
};

struct replace_test_s
{
  sem_t done;
  int total;
};

struct replace_arg_s
{
  FAR struct replace_test_s *test;
  int value;
};

struct sync_test_s
{
  sem_t started;
  sem_t finished;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_wqueue_errors;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void check_result(FAR const char *name, int actual, int expected)
{
  if (actual != expected)
    {
      printf("wqueue_test: ERROR %s: got %d, expected %d\n",
             name, actual, expected);
      g_wqueue_errors++;
    }
}

static void check_true(FAR const char *name, bool result)
{
  if (!result)
    {
      printf("wqueue_test: ERROR %s\n", name);
      g_wqueue_errors++;
    }
}

static int wait_sem(FAR sem_t *sem)
{
  struct timespec abstime;
  int ret;

  ret = clock_gettime(CLOCK_REALTIME, &abstime);
  if (ret < 0)
    {
      return -errno;
    }

  abstime.tv_sec += WQUEUE_TEST_TIMEOUT_SEC;

  do
    {
      ret = sem_timedwait(sem, &abstime);
    }
  while (ret < 0 && errno == EINTR);

  return ret < 0 ? -errno : OK;
}

static void empty_worker(FAR void *arg)
{
  UNUSED(arg);
}

static void count_worker(FAR void *arg)
{
  FAR sem_t *sem = arg;

  sem_post(sem);
}

static void replace_worker(FAR void *arg)
{
  FAR struct replace_arg_s *replace = arg;

  replace->test->total += replace->value;
  sem_post(&replace->test->done);
}

static void sync_worker(FAR void *arg)
{
  FAR struct sync_test_s *test = arg;

  sem_post(&test->started);
  usleep(WQUEUE_TEST_DELAY_USEC);
  sem_post(&test->finished);
}

static void periodic_worker(FAR void *arg)
{
  FAR struct periodic_test_s *test = arg;

  test->calls++;
  if (test->calls < 3)
    {
      test->result = work_queue_next(USRWORK, &test->work,
                                     periodic_worker, test, 1);
      if (test->result < 0)
        {
          sem_post(&test->done);
        }
    }
  else
    {
      sem_post(&test->done);
    }
}

static void api_validation_test(void)
{
  struct work_s work;
  clock_t excessive_delay = WDOG_MAX_DELAY + 1;

  printf("wqueue_test: API validation\n");
  memset(&work, 0, sizeof(work));

  check_result("null work",
               work_queue(USRWORK, NULL, empty_worker, NULL, 0), -EINVAL);
  check_result("null worker",
               work_queue(USRWORK, &work, NULL, NULL, 0), -EINVAL);
  check_result("negative delay",
               work_queue(USRWORK, &work, empty_worker, NULL, -1), -EINVAL);
  check_result("excessive delay",
               work_queue(USRWORK, &work, empty_worker, NULL,
                          excessive_delay), -EINVAL);
  check_result("negative periodic delay",
               work_queue_next(USRWORK, &work, empty_worker, NULL, -1),
               -EINVAL);
  check_result("excessive periodic delay",
               work_queue_next(USRWORK, &work, empty_worker, NULL,
                               excessive_delay), -EINVAL);
  check_result("invalid queue",
               work_queue(-1, &work, empty_worker, NULL, 0), -EINVAL);
  check_result("invalid periodic queue",
               work_queue_next(-1, &work, empty_worker, NULL, 0), -EINVAL);
  check_result("invalid cancel", work_cancel(-1, &work), -EINVAL);
  check_result("invalid sync cancel", work_cancel_sync(-1, &work),
               -EINVAL);
  check_result("null cancel", work_cancel(USRWORK, NULL), -EINVAL);
  check_result("null sync cancel", work_cancel_sync(USRWORK, NULL),
               -EINVAL);
  check_result("idle cancel", work_cancel(USRWORK, &work), OK);
  check_result("idle sync cancel", work_cancel_sync(USRWORK, &work), OK);
  check_true("idle work available", work_available(&work));
  printf("wqueue_test: API validation done\n");
}

static void priority_test(void)
{
  int priority;

  priority = work_queue_priority(USRWORK);
  printf("wqueue_test: priority = %d, expect = %d\n",
         priority, CONFIG_LIBC_USRWORKPRIORITY);
  check_result("USRWORK priority", priority,
               CONFIG_LIBC_USRWORKPRIORITY);
}

static void queue_test(clock_t delay)
{
  struct work_s work;
  sem_t called;
  int ret;

  memset(&work, 0, sizeof(work));
  ret = sem_init(&called, 0, 0);
  check_result(delay == 0 ? "immediate sem init" : "delayed sem init",
               ret, OK);
  if (ret < 0)
    {
      return;
    }

  ret = work_queue(USRWORK, &work, count_worker, &called, delay);
  check_result(delay == 0 ? "immediate queue" : "delayed queue", ret, OK);
  if (ret == OK)
    {
      ret = wait_sem(&called);
      check_result(delay == 0 ? "immediate callback" : "delayed callback",
                   ret, OK);
    }

  check_result("queue cleanup", work_cancel_sync(USRWORK, &work), OK);
  check_true("queued work available", work_available(&work));
  check_result("queue sem destroy", sem_destroy(&called), OK);
}

static void pending_replace_test(void)
{
  struct replace_arg_s first;
  struct replace_arg_s second;
  struct replace_test_s test;
  struct work_s work;
  int ret;

  printf("wqueue_test: pending replacement\n");
  memset(&test, 0, sizeof(test));
  memset(&work, 0, sizeof(work));
  first.test = &test;
  first.value = 1;
  second.test = &test;
  second.value = 2;

  ret = sem_init(&test.done, 0, 0);
  check_result("replacement sem init", ret, OK);
  if (ret < 0)
    {
      return;
    }

  ret = work_queue(USRWORK, &work, replace_worker, &first,
                   MSEC2TICK(100));
  check_result("replacement first queue", ret, OK);
  if (ret == OK)
    {
      ret = work_queue(USRWORK, &work, replace_worker, &second, 1);
      check_result("replacement second queue", ret, OK);
      if (ret == OK)
        {
          check_result("replacement callback", wait_sem(&test.done), OK);
        }
    }

  check_result("replacement cleanup", work_cancel_sync(USRWORK, &work), OK);
  check_result("replacement total", test.total, 2);
  check_true("replacement work available", work_available(&work));
  check_result("replacement sem destroy", sem_destroy(&test.done), OK);
}

static void pending_cancel_test(void)
{
  struct work_s work;
  sem_t called;
  int count = -1;
  int ret;

  printf("wqueue_test: pending cancel\n");
  memset(&work, 0, sizeof(work));
  ret = sem_init(&called, 0, 0);
  check_result("pending sem init", ret, OK);
  if (ret < 0)
    {
      return;
    }

  ret = work_queue(USRWORK, &work, count_worker, &called,
                   MSEC2TICK(100));
  check_result("pending queue", ret, OK);
  if (ret == OK)
    {
      check_result("pending cancel", work_cancel(USRWORK, &work), OK);
      usleep(150 * 1000);
      check_result("pending sem value", sem_getvalue(&called, &count), OK);
      check_result("pending callback count", count, 0);
    }

  check_result("pending cleanup", work_cancel_sync(USRWORK, &work), OK);
  check_true("pending work available", work_available(&work));
  check_result("pending sem destroy", sem_destroy(&called), OK);
}

static void sync_cancel_test(void)
{
  struct sync_test_s test;
  struct work_s work;
  int count = -1;
  int ret;

  printf("wqueue_test: synchronous cancel\n");
  memset(&work, 0, sizeof(work));
  ret = sem_init(&test.started, 0, 0);
  check_result("sync started sem init", ret, OK);
  if (ret < 0)
    {
      return;
    }

  ret = sem_init(&test.finished, 0, 0);
  check_result("sync finished sem init", ret, OK);
  if (ret < 0)
    {
      sem_destroy(&test.started);
      return;
    }

  ret = work_queue(USRWORK, &work, sync_worker, &test, 0);
  check_result("sync queue", ret, OK);
  if (ret == OK)
    {
      ret = wait_sem(&test.started);
      check_result("sync callback start", ret, OK);
      if (ret == OK)
        {
          check_result("sync cancel", work_cancel_sync(USRWORK, &work), OK);
          check_result("sync finished value",
                       sem_getvalue(&test.finished, &count), OK);
          check_result("sync finished count", count, 1);
        }
    }

  check_result("sync cleanup", work_cancel_sync(USRWORK, &work), OK);
  check_true("sync work available", work_available(&work));
  check_result("sync finished sem destroy", sem_destroy(&test.finished),
               OK);
  check_result("sync started sem destroy", sem_destroy(&test.started), OK);
}

static void periodic_test(void)
{
  struct periodic_test_s test;
  int ret;

  printf("wqueue_test: periodic requeue\n");
  memset(&test, 0, sizeof(test));
  ret = sem_init(&test.done, 0, 0);
  check_result("periodic sem init", ret, OK);
  if (ret < 0)
    {
      return;
    }

  test.result = work_queue(USRWORK, &test.work, periodic_worker, &test, 1);
  check_result("periodic queue", test.result, OK);
  if (test.result == OK)
    {
      check_result("periodic callback", wait_sem(&test.done), OK);
    }

  check_result("periodic cleanup", work_cancel_sync(USRWORK, &test.work),
               OK);
  check_result("periodic result", test.result, OK);
  check_result("periodic calls", test.calls, 3);
  check_true("periodic work available", work_available(&test.work));
  check_result("periodic sem destroy", sem_destroy(&test.done), OK);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void wqueue_test(void)
{
  g_wqueue_errors = 0;

  printf("wqueue_test: backend = predefined USRWORK (pthread disabled)\n");
  api_validation_test();
  priority_test();
  queue_test(0);
  queue_test(MSEC2TICK(20));
  pending_replace_test();
  pending_cancel_test();
  sync_cancel_test();
  periodic_test();

  if (g_wqueue_errors == 0)
    {
      printf("wqueue_test: PASS\n");
    }
  else
    {
      printf("wqueue_test: FAIL (%d errors)\n", g_wqueue_errors);
    }

  ASSERT(g_wqueue_errors == 0);
}

#else /* CONFIG_DISABLE_PTHREAD */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SLEEP_TIME   (100 * 1000)
#define TEST_COUNT   (100)
#define VERIFY_COUNT (100)
#define WQUEUE_TEST_TIMEOUT_SEC 2

#define MULTI_QUEUE_COUNT    4
#define MULTI_QUEUE_LOOPS    4
#define MULTI_WORK_PER_QUEUE 8
#define CUSTOM_PRIORITY      100
#define CUSTOM_STACKSIZE     2048

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef FAR void *(*test_thread_entry_t)(FAR void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int wait_sem(FAR sem_t *sem)
{
  struct timespec abstime;
  int ret;

  ret = clock_gettime(CLOCK_REALTIME, &abstime);
  if (ret < 0)
    {
      return -errno;
    }

  abstime.tv_sec += WQUEUE_TEST_TIMEOUT_SEC;

  do
    {
      ret = sem_timedwait(sem, &abstime);
    }
  while (ret < 0 && errno == EINTR);

  return ret < 0 ? -errno : OK;
}

static void run_test_thread(test_thread_entry_t entry, FAR void *arg,
                            int priority, int stacksize)
{
  pthread_t thread;
  pthread_attr_t attr;
  struct sched_param sparam;
  int status;

  status = pthread_attr_init(&attr);
  ASSERT(status == OK);

  status = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  ASSERT(status == OK);

  memset(&sparam, 0, sizeof(sparam));
  sparam.sched_priority = priority;
  status = pthread_attr_setschedparam(&attr, &sparam);
  ASSERT(status == OK);

  status = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  ASSERT(status == OK);

  if (stacksize > 0)
    {
      status = pthread_attr_setstacksize(&attr, stacksize);
      ASSERT(status == OK);
    }

  status = pthread_create(&thread, &attr, entry, arg);
  ASSERT(status == OK);
  status = pthread_join(thread, NULL);
  ASSERT(status == OK);
  status = pthread_attr_destroy(&attr);
  ASSERT(status == OK);
}

static void empty_worker(FAR void *arg)
{
}

static void sleep_worker(FAR void *arg)
{
  FAR sem_t *sem = arg;

  usleep(SLEEP_TIME);
  sem_post(sem);
}

static void count_worker(FAR void *arg)
{
  FAR sem_t *sem = arg;

  sem_post(sem);
}

struct sync_cancel_s
{
  sem_t started;
  sem_t finished;
};

struct requeue_s
{
  FAR struct kwork_wqueue_s *wqueue;
  FAR struct work_s *work;
  sem_t started;
  int next_result;
  int queue_result;
};

struct self_free_s
{
  FAR struct kwork_wqueue_s *wqueue;
  sem_t done;
  int result;
};

struct periodic_s
{
  FAR struct kwork_wqueue_s *wqueue;
  struct work_s work;
  sem_t done;
  int calls;
  int result;
};

struct replace_s
{
  sem_t done;
  int total;
};

struct replace_arg_s
{
  FAR struct replace_s *test;
  int value;
};

struct parallel_cancel_s;

struct parallel_worker_s
{
  FAR struct parallel_cancel_s *test;
  sem_t started;
  sem_t release;
  bool short_delay;
};

struct parallel_cancel_s
{
  struct parallel_worker_s worker[2];
  sem_t finished;
};

static void sync_worker(FAR void *arg)
{
  FAR struct sync_cancel_s *sync = arg;

  sem_post(&sync->started);
  usleep(SLEEP_TIME);
  sem_post(&sync->finished);
}

static void requeue_worker(FAR void *arg)
{
  FAR struct requeue_s *requeue = arg;

  sem_post(&requeue->started);
  usleep(SLEEP_TIME);
  requeue->next_result = work_queue_next_wq(requeue->wqueue,
                                             requeue->work,
                                             empty_worker, NULL, 1);
  requeue->queue_result = work_queue_wq(requeue->wqueue, requeue->work,
                                         empty_worker, NULL, 0);
}

static void self_free_worker(FAR void *arg)
{
  FAR struct self_free_s *self_free = arg;

  self_free->result = work_queue_free(self_free->wqueue);
  sem_post(&self_free->done);
}

static void periodic_worker(FAR void *arg)
{
  FAR struct periodic_s *periodic = arg;

  periodic->calls++;

  if (periodic->calls < 3)
    {
      periodic->result = work_queue_next_wq(periodic->wqueue,
                                             &periodic->work,
                                             periodic_worker,
                                             periodic, 1);

      if (periodic->result < 0)
        {
          sem_post(&periodic->done);
        }
    }
  else
    {
      sem_post(&periodic->done);
    }
}

static void replace_worker(FAR void *arg)
{
  FAR struct replace_arg_s *replace = arg;

  replace->test->total += replace->value;
  sem_post(&replace->test->done);
}

static void parallel_worker(FAR void *arg)
{
  FAR struct parallel_worker_s *worker = arg;

  sem_post(&worker->started);
  ASSERT(wait_sem(&worker->release) == OK);

  if (worker->short_delay)
    {
      usleep(SLEEP_TIME / 10);
    }
  else
    {
      usleep(SLEEP_TIME);
    }

  sem_post(&worker->test->finished);
}

static FAR void *release_thread(FAR void *arg)
{
  FAR struct parallel_cancel_s *test = arg;

  usleep(SLEEP_TIME / 10);
  sem_post(&test->worker[0].release);
  sem_post(&test->worker[1].release);
  return NULL;
}

static void sync_cancel_test(FAR void *wq)
{
  struct sync_cancel_s sync;
  struct work_s work;
  int count;
  int ret;

  ASSERT(sem_init(&sync.started, 0, 0) == OK);
  ASSERT(sem_init(&sync.finished, 0, 0) == OK);
  memset(&work, 0, sizeof(work));

  ret = work_queue_wq(wq, &work, sync_worker, &sync, 0);
  ASSERT(ret == OK);

  ASSERT(wait_sem(&sync.started) == OK);
  ret = work_cancel_sync_wq(wq, &work);
  ASSERT(ret == OK);

  sem_getvalue(&sync.finished, &count);
  printf("wqueue_test: sync cancel finished = %d, expect = 1\n", count);
  ASSERT(count == 1);

  ASSERT(sem_destroy(&sync.finished) == OK);
  ASSERT(sem_destroy(&sync.started) == OK);
}

static void parallel_cancel_test(FAR void *wq)
{
  struct parallel_cancel_s test;
  struct work_s work;
  pthread_t releaser;
  int count;
  int ret;

  printf("wqueue_test: parallel sync cancel\n");
  memset(&test, 0, sizeof(test));
  memset(&work, 0, sizeof(work));
  ASSERT(sem_init(&test.finished, 0, 0) == OK);

  test.worker[0].test = &test;
  test.worker[1].test = &test;
  test.worker[0].short_delay = true;
  ASSERT(sem_init(&test.worker[0].started, 0, 0) == OK);
  ASSERT(sem_init(&test.worker[0].release, 0, 0) == OK);
  ASSERT(sem_init(&test.worker[1].started, 0, 0) == OK);
  ASSERT(sem_init(&test.worker[1].release, 0, 0) == OK);

  ret = work_queue_wq(wq, &work, parallel_worker, &test.worker[0], 0);
  ASSERT(ret == OK);
  ASSERT(wait_sem(&test.worker[0].started) == OK);

  ret = work_queue_wq(wq, &work, parallel_worker, &test.worker[1], 0);
  ASSERT(ret == OK);
  ASSERT(wait_sem(&test.worker[1].started) == OK);

  ret = pthread_create(&releaser, NULL, release_thread, &test);
  ASSERT(ret == OK);
  ret = work_cancel_sync_wq(wq, &work);
  ASSERT(ret == OK);
  ASSERT(pthread_join(releaser, NULL) == OK);

  ASSERT(sem_getvalue(&test.finished, &count) == OK);
  printf("wqueue_test: parallel callbacks = %d, expect = 2\n", count);
  ASSERT(count == 2);
  ASSERT(work_available(&work));

  ASSERT(sem_destroy(&test.worker[1].release) == OK);
  ASSERT(sem_destroy(&test.worker[1].started) == OK);
  ASSERT(sem_destroy(&test.worker[0].release) == OK);
  ASSERT(sem_destroy(&test.worker[0].started) == OK);
  ASSERT(sem_destroy(&test.finished) == OK);
}

static void self_free_test(void)
{
  struct self_free_s self_free;
  struct work_s work;
  int ret;

  printf("wqueue_test: self free\n");
  memset(&work, 0, sizeof(work));
  ASSERT(sem_init(&self_free.done, 0, 0) == OK);
  self_free.wqueue = work_queue_create("test", CUSTOM_PRIORITY, NULL,
                                       CUSTOM_STACKSIZE, 1);
  ASSERT(self_free.wqueue != NULL);
  self_free.result = OK;

  ret = work_queue_wq(self_free.wqueue, &work, self_free_worker,
                      &self_free, 0);
  ASSERT(ret == OK);
  ASSERT(wait_sem(&self_free.done) == OK);
  printf("wqueue_test: self free result = %d, expect = %d\n",
         self_free.result, -EDEADLK);
  ASSERT(self_free.result == -EDEADLK);
  ASSERT(work_queue_free(self_free.wqueue) == OK);
  ASSERT(work_available(&work));
  ASSERT(sem_destroy(&self_free.done) == OK);
}

static void api_validation_test(FAR struct kwork_wqueue_s *wqueue)
{
  struct work_s work;
  clock_t excessive_delay = WDOG_MAX_DELAY + 1;

  printf("wqueue_test: API validation\n");
  memset(&work, 0, sizeof(work));

  ASSERT(work_queue_wq(NULL, &work, empty_worker, NULL, 0) == -EINVAL);
  ASSERT(work_queue_wq(wqueue, NULL, empty_worker, NULL, 0) == -EINVAL);
  ASSERT(work_queue_wq(wqueue, &work, NULL, NULL, 0) == -EINVAL);
  ASSERT(work_queue_wq(wqueue, &work, empty_worker, NULL, -1) == -EINVAL);
  ASSERT(work_queue_wq(wqueue, &work, empty_worker, NULL,
                       excessive_delay) == -EINVAL);
  ASSERT(work_queue_next_wq(wqueue, &work, empty_worker, NULL, -1) ==
         -EINVAL);
  ASSERT(work_queue_next_wq(wqueue, &work, empty_worker, NULL,
                            excessive_delay) == -EINVAL);
  ASSERT(work_queue(-1, &work, empty_worker, NULL, 0) == -EINVAL);
  ASSERT(work_queue_next(-1, &work, empty_worker, NULL, 0) == -EINVAL);
  ASSERT(work_cancel(-1, &work) == -EINVAL);
  ASSERT(work_cancel_sync(-1, &work) == -EINVAL);
  ASSERT(work_cancel_wq(NULL, &work) == -EINVAL);
  ASSERT(work_cancel_wq(wqueue, NULL) == -EINVAL);
  ASSERT(work_cancel_sync_wq(NULL, &work) == -EINVAL);
  ASSERT(work_cancel_sync_wq(wqueue, NULL) == -EINVAL);
  ASSERT(work_queue_priority_wq(NULL) == -EINVAL);

  /* Cancelling idle work is intentionally idempotent. */

  ASSERT(work_cancel_wq(wqueue, &work) == OK);
  ASSERT(work_cancel_sync_wq(wqueue, &work) == OK);
  ASSERT(work_available(&work));
  printf("wqueue_test: API validation done\n");
}

static void periodic_test(FAR struct kwork_wqueue_s *wqueue)
{
  struct periodic_s periodic;

  printf("wqueue_test: periodic requeue\n");
  memset(&periodic, 0, sizeof(periodic));
  periodic.wqueue = wqueue;
  periodic.result = OK;
  ASSERT(sem_init(&periodic.done, 0, 0) == OK);
  ASSERT(work_queue_wq(wqueue, &periodic.work, periodic_worker,
                       &periodic, 1) == OK);
  ASSERT(wait_sem(&periodic.done) == OK);
  ASSERT(periodic.result == OK);
  ASSERT(periodic.calls == 3);
  ASSERT(work_available(&periodic.work));
  ASSERT(sem_destroy(&periodic.done) == OK);
  printf("wqueue_test: periodic calls = %d, expect = 3\n",
         periodic.calls);
}

static void pending_replace_test(FAR struct kwork_wqueue_s *wqueue)
{
  struct replace_arg_s first;
  struct replace_arg_s second;
  struct replace_s replace;
  struct work_s work;
  int count;

  printf("wqueue_test: pending replacement\n");
  memset(&replace, 0, sizeof(replace));
  memset(&work, 0, sizeof(work));
  first.test = &replace;
  first.value = 1;
  second.test = &replace;
  second.value = 2;
  ASSERT(sem_init(&replace.done, 0, 0) == OK);

  ASSERT(work_queue_wq(wqueue, &work, replace_worker, &first,
                       MSEC2TICK(200)) == OK);
  ASSERT(work_queue_next_wq(wqueue, &work, replace_worker,
                            &second, 1) == OK);
  ASSERT(wait_sem(&replace.done) == OK);
  usleep(300 * 1000);
  ASSERT(sem_getvalue(&replace.done, &count) == OK);
  ASSERT(count == 0);
  ASSERT(replace.total == 2);
  ASSERT(work_available(&work));
  ASSERT(sem_destroy(&replace.done) == OK);
  printf("wqueue_test: replacement total = %d, expect = 2\n",
         replace.total);
}

static FAR void *tester(FAR void *arg)
{
  FAR void **val = arg;
  struct work_s work;
  int i;
  int ret;

  memset(&work, 0, sizeof(work));
  for (i = 0; i < TEST_COUNT; i++)
    {
      if (val[1] != NULL)
        {
          ret = work_queue_wq(val[1], &work, empty_worker, NULL, 0);
          ASSERT(ret == OK);
          ret = work_cancel_wq(val[1], &work);
          ASSERT(ret == OK);
        }
      else
        {
          ret = work_queue((int)(uintptr_t)val[0], &work,
                           empty_worker, NULL, 0);
          ASSERT(ret == OK);
          ret = work_cancel((int)(uintptr_t)val[0], &work);
          ASSERT(ret == OK);
        }

      usleep((int)(uintptr_t)val[2]);
    }

  usleep(SLEEP_TIME); /* Wait for workers to run. */
  return NULL;
}

static FAR void *verifier(FAR void *arg)
{
  FAR void **val = arg;
  sem_t sem;
  sem_t call_sem;
  int call_count;
  int extra_count;
  int i;
  int ret;
  struct work_s work[VERIFY_COUNT + 1];

  ASSERT(sem_init(&sem, 0, 0) == OK);
  ASSERT(sem_init(&call_sem, 0, 0) == OK);
  memset(&work, 0, sizeof(work));

  /* Queue sleep worker. */

  if (val[1] != NULL)
    {
      ret = work_queue_wq(val[1], &work[0], sleep_worker, &sem, 0);
    }
  else
    {
      ret = work_queue((int)(uintptr_t)val[0], &work[0],
                       sleep_worker, &sem, 0);
    }

  ASSERT(ret == OK);

  /* Queue count workers when qid is busy. */

  for (i = 1; i <= VERIFY_COUNT; i++)
    {
      if (val[1] != NULL)
        {
          ret = work_queue_wq(val[1], &work[i], count_worker,
                              &call_sem, 0);
        }
      else
        {
          ret = work_queue((int)(uintptr_t)val[0], &work[i],
                           count_worker, &call_sem, 0);
        }

      ASSERT(ret == OK);
    }

  /* Wait for sleep worker to run. */

  ASSERT(wait_sem(&sem) == OK);

  /* Wait for count workers to run. */

  for (call_count = 0; call_count < VERIFY_COUNT; call_count++)
    {
      ASSERT(wait_sem(&call_sem) == OK);
    }

  usleep(SLEEP_TIME);
  ASSERT(sem_getvalue(&call_sem, &extra_count) == OK);
  ASSERT(extra_count == 0);

  printf("wqueue_test: call = %d, expect = %d\n", call_count, VERIFY_COUNT);

  for (i = 0; i <= VERIFY_COUNT; i++)
    {
      ASSERT(work[i].worker == NULL);
    }

  ASSERT(call_count == VERIFY_COUNT);
  ASSERT(sem_destroy(&call_sem) == OK);
  ASSERT(sem_destroy(&sem) == OK);
  return NULL;
}

static void run_once(int qid, FAR void *wq, int interval,
                     int priority_test, int priority_verify)
{
  FAR void *val[3];

  /* Tester: try race conditions. */

  val[0] = (FAR void *)(uintptr_t)qid;
  val[1] = wq;
  val[2] = (FAR void *)(uintptr_t)interval;
  run_test_thread(tester, val, priority_test, CONFIG_PTHREAD_STACK_DEFAULT);

  /* Verifier: make sure queue is still working properly. */

  run_test_thread(verifier, val, priority_verify,
                  VERIFY_COUNT * sizeof(struct work_s) +
                  CONFIG_PTHREAD_STACK_DEFAULT);
}

void wqueue_priority_test(int qid, FAR void *wq, int prio)
{
  int interval;
  int priority_test;
  int priority_verify;

  for (interval = 0; interval <= 1; interval++)
    {
      for (priority_test  = prio - 1;
           priority_test <= prio + 1;
           priority_test++)
        {
          for (priority_verify  = prio - 1;
               priority_verify <= prio + 1;
               priority_verify++)
            {
              run_once(qid, wq, interval, priority_test, priority_verify);
            }
        }
    }
}

static void multiple_queue_test(void)
{
  FAR void *wqueue[MULTI_QUEUE_COUNT];
  struct work_s work[MULTI_QUEUE_COUNT][MULTI_WORK_PER_QUEUE];
  char name[MULTI_QUEUE_COUNT][16];
  sem_t finished;
  int loop;
  int q;
  int w;
  int ret;

  printf("wqueue_test: multiple custom queues\n");
  ASSERT(work_queue_create(NULL, CUSTOM_PRIORITY, NULL,
                           CUSTOM_STACKSIZE, 1) == NULL);
  ASSERT(work_queue_create("test", CUSTOM_PRIORITY, NULL, 0, 1) == NULL);
  ASSERT(work_queue_create("test", CUSTOM_PRIORITY, NULL,
                           CUSTOM_STACKSIZE, 0) == NULL);
  ASSERT(work_queue_free(NULL) < 0);
  ASSERT(sem_init(&finished, 0, 0) == OK);

  for (loop = 0; loop < MULTI_QUEUE_LOOPS; loop++)
    {
      memset(work, 0, sizeof(work));

      for (q = 0; q < MULTI_QUEUE_COUNT; q++)
        {
          snprintf(name[q], sizeof(name[q]), "ostest-wq%d", q);
          wqueue[q] = work_queue_create(name[q], CUSTOM_PRIORITY + q,
                                        NULL, CUSTOM_STACKSIZE, q + 1);
          ASSERT(wqueue[q] != NULL);
          ASSERT(work_queue_priority_wq(wqueue[q]) ==
                 CUSTOM_PRIORITY + q);
        }

      for (q = 0; q < MULTI_QUEUE_COUNT; q++)
        {
          for (w = 0; w < MULTI_WORK_PER_QUEUE; w++)
            {
              ret = work_queue_wq(wqueue[q], &work[q][w], count_worker,
                                  &finished, (w & 1) != 0 ? 1 : 0);
              ASSERT(ret == OK);
            }
        }

      for (q = 0; q < MULTI_QUEUE_COUNT * MULTI_WORK_PER_QUEUE; q++)
        {
          ASSERT(wait_sem(&finished) == OK);
        }

      for (q = 0; q < MULTI_QUEUE_COUNT; q++)
        {
          for (w = 0; w < MULTI_WORK_PER_QUEUE; w++)
            {
              ASSERT(work_available(&work[q][w]));
            }
        }

      for (q = MULTI_QUEUE_COUNT - 1; q >= 0; q--)
        {
          ASSERT(work_queue_free(wqueue[q]) == OK);
        }

      printf("wqueue_test: multiple queues loop %d/%d done\n",
             loop + 1, MULTI_QUEUE_LOOPS);
    }

  ASSERT(sem_destroy(&finished) == OK);
  printf("wqueue_test: multiple custom queues done\n");
}

static void teardown_test(void)
{
  FAR void *wqueue;
  struct requeue_s requeue;
  struct sync_cancel_s sync;
  struct work_s work;
  sem_t called;
  int count;
  int ret;

  printf("wqueue_test: teardown\n");

  /* Free a queue while delayed work is still pending. */

  memset(&work, 0, sizeof(work));
  ASSERT(sem_init(&called, 0, 0) == OK);
  wqueue = work_queue_create("test", CUSTOM_PRIORITY, NULL,
                             CUSTOM_STACKSIZE, 1);
  ASSERT(wqueue != NULL);
  ret = work_queue_wq(wqueue, &work, count_worker, &called,
                      MSEC2TICK(500));
  ASSERT(ret == OK);
  ASSERT(work_queue_free(wqueue) == OK);
  ASSERT(work_available(&work));
  usleep(SLEEP_TIME);
  ASSERT(sem_getvalue(&called, &count) == OK);
  printf("wqueue_test: pending callback = %d, expect = 0\n", count);
  ASSERT(count == 0);
  ASSERT(sem_destroy(&called) == OK);

  /* Free a queue while a callback is running. */

  memset(&work, 0, sizeof(work));
  ASSERT(sem_init(&sync.started, 0, 0) == OK);
  ASSERT(sem_init(&sync.finished, 0, 0) == OK);
  wqueue = work_queue_create("test", CUSTOM_PRIORITY, NULL,
                             CUSTOM_STACKSIZE, 1);
  ASSERT(wqueue != NULL);
  ret = work_queue_wq(wqueue, &work, sync_worker, &sync, 0);
  ASSERT(ret == OK);
  ASSERT(wait_sem(&sync.started) == OK);
  ASSERT(work_queue_free(wqueue) == OK);
  ASSERT(work_available(&work));
  ASSERT(sem_getvalue(&sync.finished, &count) == OK);
  printf("wqueue_test: running callback = %d, expect = 1\n", count);
  ASSERT(count == 1);
  ASSERT(sem_destroy(&sync.finished) == OK);
  ASSERT(sem_destroy(&sync.started) == OK);

  /* Reject attempts by a running callback to requeue work after teardown
   * starts.
   */

  memset(&work, 0, sizeof(work));
  ASSERT(sem_init(&requeue.started, 0, 0) == OK);
  wqueue = work_queue_create("test", CUSTOM_PRIORITY, NULL,
                             CUSTOM_STACKSIZE, 1);
  ASSERT(wqueue != NULL);
  requeue.wqueue = wqueue;
  requeue.work = &work;
  requeue.next_result = OK;
  requeue.queue_result = OK;
  ret = work_queue_wq(wqueue, &work, requeue_worker, &requeue, 0);
  ASSERT(ret == OK);
  ASSERT(wait_sem(&requeue.started) == OK);
  ASSERT(work_queue_free(wqueue) == OK);
  ASSERT(requeue.next_result == -ESHUTDOWN);
  ASSERT(requeue.queue_result == -ESHUTDOWN);
  ASSERT(work_available(&work));
  printf("wqueue_test: teardown requeue rejected\n");
  ASSERT(sem_destroy(&requeue.started) == OK);

  printf("wqueue_test: teardown done\n");
}

static FAR void *wqueue_test_entry(FAR void *arg)
{
  FAR void *wq;
  int priority;
  int i;

  UNUSED(arg);

#ifdef CONFIG_BUILD_FLAT
  printf("wqueue_test: backend = flat\n");
#else
  printf("wqueue_test: backend = libc user\n");
#endif

#ifdef CONFIG_BUILD_FLAT
#ifdef CONFIG_SCHED_HPWORK
  printf("wqueue_test: HPWORK\n");
  wqueue_priority_test(HPWORK, NULL, CONFIG_SCHED_HPWORKPRIORITY);
  printf("wqueue_test: HPWORK done\n");
#endif

#ifdef CONFIG_SCHED_LPWORK
  printf("wqueue_test: LPWORK\n");
  wqueue_priority_test(LPWORK, NULL, CONFIG_SCHED_LPWORKPRIORITY);
  printf("wqueue_test: LPWORK done\n");
#endif
#endif

  for (i = 1; i < 3; i++)
    {
      printf("wqueue_test: custom queue, threads = %d\n", i);
      wq = work_queue_create("test", CUSTOM_PRIORITY, NULL,
                             CUSTOM_STACKSIZE, i);
      ASSERT(wq != NULL);

      priority = work_queue_priority_wq(wq);
      printf("wqueue_test: priority = %d, expect = %d\n",
             priority, CUSTOM_PRIORITY);
      ASSERT(priority == CUSTOM_PRIORITY);

      if (i == 1)
        {
          api_validation_test(wq);
          periodic_test(wq);
          pending_replace_test(wq);
        }

      sync_cancel_test(wq);

      if (i == 2)
        {
          parallel_cancel_test(wq);
        }

      wqueue_priority_test(0, wq, CUSTOM_PRIORITY);
      ASSERT(work_queue_free(wq) == OK);
      printf("wqueue_test: custom queue, threads = %d done\n", i);
    }

  multiple_queue_test();
  self_free_test();
  teardown_test();

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void wqueue_test(void)
{
  run_test_thread(wqueue_test_entry, NULL, CUSTOM_PRIORITY, STACKSIZE);
}

#endif /* CONFIG_DISABLE_PTHREAD */
