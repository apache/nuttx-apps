/****************************************************************************
 * apps/testing/ostest/spinlock.c
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
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <nuttx/spinlock.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define THREAD_NUM    CONFIG_TESTING_OSTEST_SPINLOCK_THREADS
#define LOOP_TIMES    (CONFIG_TEST_LOOP_SCALE * 10000)

enum lock_type_e
{
  RSPINLOCK,
  SPINLOCK
};

struct spinlock_pub_args_s
{
  volatile uint32_t counter;
  pthread_barrier_t barrier;
  union
    {
      rspinlock_t rlock;
      spinlock_t lock;
    };
};

struct spinlock_thread_args_s
{
  uint64_t delta;
  FAR struct spinlock_pub_args_s *pub;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Helper functions for timespec calculating */

static inline uint64_t calc_diff(FAR struct timespec *start,
                                    FAR struct timespec *end)
{
  uint64_t diff_sec = end->tv_sec - start->tv_sec;
  long diff_nsec = end->tv_nsec - start->tv_nsec;
  if (diff_nsec < 0)
    {
      diff_sec -= 1;
      diff_nsec += 1000000000L;
    }

  return diff_sec * 1000000000ULL + diff_nsec;
}

#define LOCK_TEST_FUNC(lock_type, lock_func, unlock_func) \
FAR static void * lock_type##_test_thread(FAR void *arg) \
{ \
  struct spinlock_thread_args_s *param = \
                                (struct spinlock_thread_args_s *)arg; \
  irqstate_t flags; \
  struct timespec start; \
  struct timespec end; \
  int i; \
  VERIFY(0 == pthread_barrier_wait(&param->pub->barrier)); \
  clock_gettime(CLOCK_REALTIME, &start); \
  for (i = 0; i < LOOP_TIMES; i++) \
    { \
      flags = lock_func(&param->pub->lock_type); \
      param->pub->counter++; \
      unlock_func(&param->pub->lock_type, flags); \
    } \
  clock_gettime(CLOCK_REALTIME, &end); \
  param->delta = calc_diff(&start, &end); \
  return NULL; \
}

LOCK_TEST_FUNC(rlock, rspin_lock_irqsave, rspin_unlock_irqrestore)
LOCK_TEST_FUNC(lock, spin_lock_irqsave, spin_unlock_irqrestore)

static inline void run_test_thread(
  enum lock_type_e lock_type,
  FAR void *(*thread_func)(FAR void *arg)
  )
{
  const char *test_type = (lock_type == RSPINLOCK)
                          ? "RSpin lock" : "Spin lock";
  printf("Test type: %s\n", test_type);
  pthread_t tid[THREAD_NUM];
  struct spinlock_pub_args_s pub;
  struct spinlock_thread_args_s param[THREAD_NUM];
  struct timespec stime;
  struct timespec etime;
  int i;

  VERIFY(0 == pthread_barrier_init(&pub.barrier, NULL, THREAD_NUM + 1));
  pub.counter = 0;
  if (lock_type == RSPINLOCK)
    {
      rspin_lock_init(&pub.rlock);
    }
  else
    {
      spin_lock_init(&pub.lock);
    }

  clock_gettime(CLOCK_REALTIME, &stime);
  for (i = 0; i < THREAD_NUM; i++)
    {
      param[i].pub = &pub;
      param[i].delta = 0;
      pthread_create(&tid[i], NULL, thread_func, &param[i]);
    }

  VERIFY(0 == pthread_barrier_wait(&pub.barrier));

  for (i = 0; i < THREAD_NUM; i++)
    {
      pthread_join(tid[i], NULL);
    }

  clock_gettime(CLOCK_REALTIME, &etime);
  VERIFY(0 == pthread_barrier_destroy(&pub.barrier));

  uint64_t total_ns = 0;
  for (i = 0; i < THREAD_NUM; i++)
    {
      total_ns += param[i].delta;
    }

  printf("%s: Test Results:\n", test_type);
  printf("%s: Final counter: %" PRIu32 "\n", test_type, pub.counter);
  assert(pub.counter == THREAD_NUM * LOOP_TIMES);
  printf("%s: Average time per thread: %" PRIu64 " ns\n"
        , test_type, total_ns / THREAD_NUM);
  printf("%s: Total execution time: %" PRIu64 " ns\n \n"
        , test_type, calc_diff(&stime, &etime));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spinlock_test
 ****************************************************************************/

void spinlock_test(void)
{
  printf("Start Spin lock test:\n");
  printf("Thread num: %d, Loop times: %d\n\n", THREAD_NUM, LOOP_TIMES);

  run_test_thread(SPINLOCK, lock_test_thread);

  run_test_thread(RSPINLOCK, rlock_test_thread);
}
