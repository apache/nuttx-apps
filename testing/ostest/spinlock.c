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
#include <nuttx/atomic.h>
#include <nuttx/spinlock.h>
#include <nuttx/seqlock.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Maximum number of test threads - typically one per CPU core */

#define MAX_THREAD_NUM (CONFIG_SMP_NCPUS)

/* Number of lock/unlock iterations per thread */


#define LOOP_TIMES     (CONFIG_TEST_LOOP_SCALE * 100000)

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

/* Shared arguments for all test threads.
 * This structure contains data that all threads need to access.
 */

aligned_data(64) struct spinlock_pub_args_s
{
  FAR void         *lock;       /* Pointer to the lock being tested */
  volatile uint32_t counter;    /* Shared counter protected by the lock */
  atomic_t          barrier;    /* Synchronization barrier for thread startup */
  uint32_t          thread_num; /* Total number of threads participating */
};

/* Per-thread arguments.
 * Each thread has its own instance of this structure.
 */

struct spinlock_thread_args_s
{
  uint64_t                      delta; /* Execution time for this thread (nanoseconds) */
  FAR struct spinlock_pub_args_s *pub; /* Pointer to shared arguments */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: calc_diff
 *
 * Description:
 *   Calculate time difference between two timespec structures.
 *   This function computes the elapsed time between start and end times,
 *   handling the case where the nanoseconds component of the end time
 *   is less than the start time (requiring a borrow from seconds).
 *
 * Input Parameters:
 *   start - Pointer to the starting timespec
 *   end   - Pointer to the ending timespec
 *
 * Returned Value:
 *   Time difference in nanoseconds
 *
 ****************************************************************************/

static inline uint64_t calc_diff(FAR struct timespec *start,
                                    FAR struct timespec *end)
{
  int64_t sec_diff  = end->tv_sec - start->tv_sec;
  int64_t nsec_diff = end->tv_nsec - start->tv_nsec;

  /* Handle negative nanoseconds by borrowing from seconds */

  if (nsec_diff < 0)
    {
      sec_diff--;
      nsec_diff += 1000000000L;
    }

  return sec_diff * 1000000000ULL + nsec_diff;
}

/****************************************************************************
 * Name: LOCK_TEST_FUNC
 *
 * Description:
 *   Test thread function for different lock types.
 *   This macro generates a test thread function for a specific lock type.
 *   Each thread performs LOOP_TIMES lock/unlock operations and measures
 *   the execution time.
 *
 * Input Parameters:
 *   lock_type   - Type of lock to test (e.g., spinlock_t, rspinlock_t)
 *   lock_func   - Lock acquisition function
 *   unlock_func - Lock release function
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#define LOCK_TEST_FUNC(lock_type, lock_func, unlock_func) \
FAR static void * lock_type##_test_thread(FAR void *arg) \
{ \
  struct spinlock_thread_args_s *param = \
                                (struct spinlock_thread_args_s *)arg; \
  irqstate_t      flags; \
  struct timespec start; \
  struct timespec end; \
  uint32_t        idx; \
  FAR struct spinlock_pub_args_s *pub = param->pub; \
  FAR lock_type *lock = (FAR lock_type *)pub->lock; \
  \
  /* Synchronize thread startup using atomic barrier */ \
  \
  atomic_add(&pub->barrier, 1); \
  while (atomic_read(&pub->barrier) != pub->thread_num) \
    { \
      sched_yield(); \
    } \
  \
  /* Start timing */ \
  \
  clock_gettime(CLOCK_REALTIME, &start); \
  \
  /* Perform lock/unlock operations */ \
  \
  for (idx = 0; idx < LOOP_TIMES; idx++) \
    { \
      flags = lock_func(lock); \
      pub->counter++; \
      unlock_func(lock, flags); \
    } \
  \
  /* Stop timing and calculate duration */ \
  \
  clock_gettime(CLOCK_REALTIME, &end); \
  param->delta = calc_diff(&start, &end); \
  \
  return NULL; \
}

LOCK_TEST_FUNC(spinlock_t, spin_lock_irqsave, spin_unlock_irqrestore)
LOCK_TEST_FUNC(rspinlock_t, rspin_lock_irqsave, rspin_unlock_irqrestore)
LOCK_TEST_FUNC(seqcount_t, write_seqlock_irqsave, write_sequnlock_irqrestore)

/****************************************************************************
 * Name: run_test_thread
 *
 * Description:
 *   Run a multi-threaded lock performance test.
 *   This function creates multiple threads that concurrently perform
 *   lock/unlock operations on the same lock. It measures the total
 *   execution time and calculates throughput.
 *
 * Input Parameters:
 *   lock        - Pointer to the lock being tested
 *   thread_func - Thread function that performs lock operations
 *   thread_num  - Number of threads to create
 *   lock_type   - String describing the lock type (for output)
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline
void run_test_thread(void *lock, FAR void *(*thread_func)(FAR void *arg),
                     uint32_t thread_num, const char *lock_type)
{
  struct spinlock_pub_args_s    pub;
  pthread_attr_t                attr;
  struct sched_param            sparam;
  pthread_t                     tid[MAX_THREAD_NUM];
  struct spinlock_thread_args_s param[MAX_THREAD_NUM];
  struct timespec               stime;
  struct timespec               etime;
  int                          status;
  int                               i;
  cpu_set_t                   cpu_set = 1u;
  uint64_t                   total_ns = 0u;

  /* Initialize the public parameters. */

  printf("Test type: %s\n", lock_type);

  pub.lock       = lock;
  pub.counter    = 0u;
  pub.thread_num = thread_num;
  atomic_set_release(&pub.barrier, 0u);

  /* Set affinity to CPU0 */

#ifdef CONFIG_SMP
  if (OK != sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set))
    {
      printf("spinlock_test: ERROR: nxsched_set_affinity failed");
      ASSERT(false);
    }
#else
  UNUSED(cpu_set);
#endif

  /* Boost to maximum priority for test threads. */

  status = pthread_attr_init(&attr);
  if (status != 0)
    {
      printf("spinlock_test: ERROR: "
             "pthread_attr_init failed, status=%d\n",  status);
      ASSERT(false);
    }

  status = sched_getparam(getpid(), &sparam);
  if (status != 0)
    {
      printf("spinlock_test: ERROR: "
             "sched_getparam failed, status=%d\n",  status);
      ASSERT(false);
    }

  sparam.sched_priority = SCHED_PRIORITY_MAX;
  status = pthread_attr_setschedparam(&attr, &sparam);
  if (status != OK)
    {
      printf("spinlock_test: ERROR: "
             "pthread_attr_setschedparam failed, status=%d\n",  status);
      ASSERT(false);
    }

  clock_gettime(CLOCK_REALTIME, &stime);

  /* Create new test threads. */

  for (i = 0; i < thread_num; i++)
    {
      param[i].pub = &pub;
      param[i].delta = 0;

      /* Set affinity */

#ifdef CONFIG_SMP
      cpu_set = 1u << ((i + 1) % CONFIG_SMP_NCPUS);

      status = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t),
                                           &cpu_set);
      if (status != OK)
        {
          printf("spinlock_test: ERROR: "
                 "pthread_attr_setaffinity_np failed, status=%d\n",  status);
          ASSERT(false);
        }
#endif

      status = pthread_create(&tid[i], &attr, thread_func, &param[i]);
      if (status != 0)
        {
          printf("spinlock_test: ERROR: "
                 "pthread_create failed, status=%d\n",  status);
          ASSERT(false);
        }
    }

  /* Wait for all threads to complete */

  for (i = 0; i < thread_num; i++)
    {
      status = pthread_join(tid[i], NULL);
      if (status != 0)
        {
          printf("spinlock_test: ERROR: "
                 "pthread_join failed, status=%d\n",  status);
          ASSERT(false);
        }
    }

  /* Calculate the average throughput. */

  clock_gettime(CLOCK_REALTIME, &etime);
  for (i = 0; i < thread_num; i++)
    {
      total_ns += param[i].delta;
    }

  if (total_ns != 0)
    {
      printf("%s: Test Results:\n", lock_type);
      printf("%s: Final counter: %" PRIu32 "\n", lock_type, pub.counter);
      ASSERT(pub.counter == thread_num * LOOP_TIMES);
      printf("%s: Average throughput : %" PRIu64 " op/s\n", lock_type,
            (uint64_t)NSEC_PER_SEC * LOOP_TIMES * thread_num / total_ns);
      printf("%s: Total execution time: %" PRIu64 " ns\n \n",
            lock_type, calc_diff(&stime, &etime));
    }
  else
    {
      printf("spinlock_test: ERROR: total_ns is 0\n");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: spinlock_test_thread_num
 *
 * Description:
 *   Run lock performance tests for a specific number of threads.
 *   This function tests three different lock types:
 *   - spinlock_t:  Basic spinlock
 *   - rspinlock_t: Recursive spinlock
 *   - seqcount_t:  Sequence counter lock
 *
 * Input Parameters:
 *   thread_num - Number of threads to use in the test
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void spinlock_test_thread_num(unsigned thread_num)
{
  aligned_data(64) union
    {
      spinlock_t  spinlock;
      rspinlock_t rspinlock;
      seqcount_t  seqcount;
    } lock;

  printf("Thread num: %u, Loop times: %d\n\n", thread_num, LOOP_TIMES);

  /* Test basic spinlock */

  spin_lock_init(&lock.spinlock);
  run_test_thread(&lock, spinlock_t_test_thread, thread_num, "spinlock");

  /* Test read-write spinlock */

  rspin_lock_init(&lock.rspinlock);
  run_test_thread(&lock, rspinlock_t_test_thread, thread_num, "rspinlock");

  /* Test sequence counter lock */

  seqlock_init(&lock.seqcount);
  run_test_thread(&lock, seqcount_t_test_thread, thread_num, "seqcount");
}

/****************************************************************************
 * Name: spinlock_test_thread
 *
 * Description:
 *   Main test thread function that runs tests with varying thread counts.
 *   This function iterates through different thread counts (1 to
 *   MAX_THREAD_NUM) to test lock performance under different concurrency
 *   levels.
 *
 * Input Parameters:
 *   arg - Unused argument
 *
 * Returned Value:
 *   Always returns NULL
 *
 ****************************************************************************/

static void *spinlock_test_thread(void *arg)
{
  unsigned tnr;

  for (tnr = 1; tnr <= MAX_THREAD_NUM; tnr++)
    {
      spinlock_test_thread_num(tnr);
    }

  return NULL;
}

/****************************************************************************
 * Name: spinlock_test
 *
 * Description:
 *   Entry point for the spinlock performance test suite.
 *   This function creates a test thread with reduced priority to run
 *   the lock performance tests. The tests measure throughput and
 *   execution time for different lock types under varying thread counts.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void spinlock_test(void)
{
  pthread_t          tid;
  pthread_attr_t     attr;
  struct sched_param sparam;

  /* Initialize thread attributes */

  ASSERT(0 == pthread_attr_init(&attr));

  /* Set thread priority to default - 1 */

  ASSERT(0 == sched_getparam(getpid(), &sparam));
  sparam.sched_priority = SCHED_PRIORITY_MAX;
  ASSERT(0 == pthread_attr_setschedparam(&attr, &sparam));

  printf("spinlock_test: start......\n");

  /* Create and run the test thread */

  ASSERT(0 == pthread_create(&tid, &attr, spinlock_test_thread, NULL));
  ASSERT(0 == pthread_join(tid, NULL));

  printf("spinlock_test: end......\n");
}
