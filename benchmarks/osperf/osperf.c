/****************************************************************************
 * apps/benchmarks/osperf/osperf.c
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

#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/poll.h>

#include <nuttx/sched.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct performance_time_s
{
  clock_t start;
  clock_t end;
};

struct performance_thread_s
{
  sem_t sem;
  struct performance_time_s time;
};

struct performance_entry_s
{
  const char name[NAME_MAX];
  CODE size_t (*entry)(void);
};

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static size_t pthread_create_performance(void);
static size_t pthread_switch_performance(void);
static size_t context_switch_performance(void);
static size_t hpwork_performance(void);
static size_t poll_performance(void);
static size_t pipe_performance(void);
static size_t semwait_performance(void);
static size_t sempost_performance(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct performance_entry_s g_entry_list[] =
{
  {"pthread-create", pthread_create_performance},
  {"pthread-switch", pthread_switch_performance},
  {"context-switch", context_switch_performance},
  {"hpwork", hpwork_performance},
  {"poll-write", poll_performance},
  {"pipe-rw", pipe_performance},
  {"semwait", semwait_performance},
  {"sempost", sempost_performance},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int performance_thread_create(FAR void *(*entry)(FAR void *),
                                     FAR void *arg, int priority)
{
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t tid;

  param.sched_priority = priority;
  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  pthread_attr_setschedparam(&attr, &param);
  pthread_create(&tid, &attr, entry, arg);
  DEBUGASSERT(tid > 0);
  return tid;
}

static void performance_start(FAR struct performance_time_s *result)
{
  result->start = perf_gettime();
}

static void performance_end(FAR struct performance_time_s *result)
{
  result->end = perf_gettime();
}

static size_t performance_gettime(FAR struct performance_time_s *result)
{
  struct timespec ts;
  perf_convert(result->end - result->start, &ts);
  return ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
}

/****************************************************************************
 * Pthread swtich performance
 ****************************************************************************/

static FAR void *pthread_switch_task(FAR void *arg)
{
  FAR struct performance_thread_s *perf = arg;
  irqstate_t flags = enter_critical_section();
  sem_wait(&perf->sem);
  performance_end(&perf->time);
  leave_critical_section(flags);
  return NULL;
}

static size_t pthread_switch_performance(void)
{
  struct performance_thread_s perf;
  pthread_t tid;

  sem_init(&perf.sem, 0, 0);
  tid = performance_thread_create(pthread_switch_task, &perf,
                                  CONFIG_BENCHMARK_OSPERF_PRIORITY + 1);

  performance_start(&perf.time);
  sem_post(&perf.sem);
  pthread_join(tid, NULL);

  return performance_gettime(&perf.time);
}

/****************************************************************************
 * Pthread create performance
 ****************************************************************************/

static FAR void *pthread_create_task(FAR void *arg)
{
  FAR struct performance_time_s *time = arg;
  performance_end(time);
  return NULL;
}

static size_t pthread_create_performance(void)
{
  struct performance_time_s result;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t tid;

  sched_getparam(gettid(), &param);
  param.sched_priority++;
  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  pthread_attr_setschedparam(&attr, &param);

  performance_start(&result);
  pthread_create(&tid, &attr, pthread_create_task, &result);
  pthread_join(tid, NULL);

  return performance_gettime(&result);
}

/****************************************************************************
 * Context create performance
 ****************************************************************************/

static FAR void *context_switch_task(FAR void *arg)
{
  FAR struct performance_time_s *time = arg;
  sched_yield();
  performance_end(time);
  return 0;
}

static size_t context_switch_performance(void)
{
  struct performance_time_s time;
  int tid;

  tid = performance_thread_create(context_switch_task, &time,
                                  CONFIG_INIT_PRIORITY);
  sched_yield();
  performance_start(&time);
  sched_yield();
  pthread_join(tid, NULL);
  return performance_gettime(&time);
}

/****************************************************************************
 * wdog performance
 ****************************************************************************/

static void work_handle(void *arg)
{
  FAR struct performance_time_s *time = ((FAR void **)arg)[2];
  FAR sem_t *sem = ((void **)arg)[1];
  performance_end(time);
  sem_post(sem);
}

static size_t hpwork_performance(void)
{
  struct performance_time_s result;
  struct work_s work;
  sem_t sem = SEM_INITIALIZER(0);
  int ret;

  FAR void *args = (void *[])
  {
    (FAR void *)&work,
    (FAR void *)&sem,
    (FAR void *)&result
  };

  memset(&result, 0, sizeof(result));
  memset(&work, 0, sizeof(work));
  performance_start(&result);
  ret = work_queue(HPWORK, &work, work_handle, args, 0);
  DEBUGASSERT(ret == 0);

  sem_wait(&sem);
  return performance_gettime(&result);
}

/****************************************************************************
 * poll-write performance
 ****************************************************************************/

static FAR void *poll_task(FAR void *arg)
{
  FAR void **argv = arg;
  FAR struct performance_time_s *time = argv[0];
  int pipefd = (int)(uintptr_t)argv[1];

  performance_start(time);
  write(pipefd, "a", 1);
  return 0;
}

static size_t poll_performance(void)
{
  struct performance_time_s result;
  struct pollfd fds;
  FAR void *argv[2];
  int pipefd[2];
  int ret;

  ret = pipe(pipefd);
  DEBUGASSERT(ret == 0);
  fds.fd = pipefd[0];
  fds.events = POLLIN;
  argv[0] = (FAR char *)&result;
  argv[1] = (FAR char *)(uintptr_t)pipefd[1];

  ret = performance_thread_create(poll_task, argv, CONFIG_INIT_PRIORITY);

  poll(&fds, 1, -1);
  performance_end(&result);

  pthread_join(ret, NULL);
  close(pipefd[0]);
  close(pipefd[1]);
  return performance_gettime(&result);
}

/****************************************************************************
 *  pipe performance
 ****************************************************************************/

static size_t pipe_performance(void)
{
  struct performance_time_s result;
  int pipefd[2];
  int ret;
  char r;

  ret = pipe(pipefd);
  DEBUGASSERT(ret == 0);

  performance_start(&result);
  write(pipefd[0], "a", 1);
  read(pipefd[1], &r, 1);
  performance_end(&result);

  close(pipefd[0]);
  close(pipefd[1]);
  return performance_gettime(&result);
}

/****************************************************************************
 * semwait_performance
 ****************************************************************************/

static size_t semwait_performance(void)
{
  struct performance_time_s result;
  sem_t sem;

  sem_init(&sem, 0, 2);

  performance_start(&result);
  sem_wait(&sem);
  performance_end(&result);

  sem_destroy(&sem);
  return performance_gettime(&result);
}

/****************************************************************************
 * sempost_performance
 ****************************************************************************/

static size_t sempost_performance(void)
{
  struct performance_time_s result;
  sem_t sem;

  sem_init(&sem, 0, 0);

  performance_start(&result);
  sem_post(&sem);
  performance_end(&result);

  sem_destroy(&sem);
  return performance_gettime(&result);
}

/****************************************************************************
 * performance_help
 ****************************************************************************/

static void performance_help(void)
{
  printf("Usage: performance [OPTIONS] [name]\n\n");
  printf("OPTIONS:\n");
  printf("\t-c, \tNumber of times to run each test\n");
  printf("\t-d, \tShow detail of each test\n");
  printf("\t-h, \tShow this help message\n");
  printf("\t-l, \tList all tests\n");
}

/****************************************************************************
 * performance_run
 ****************************************************************************/

static void performance_run(const FAR struct performance_entry_s *item,
                            size_t count, bool detail)
{
  size_t total = 0;
  size_t max = 0;
  size_t min = 0;
  size_t i;

  for (i = 0; i < count; i++)
    {
      irqstate_t flags = enter_critical_section();
      size_t time = item->entry();
      leave_critical_section(flags);

      total += time;
      if (time > max)
        {
          max = time;
        }

      if (time < min || min == 0)
        {
          min = time;
        }

      if (detail)
        {
          printf("\t%zu: %zu\n", i, time);
        }
    }

  printf("%-*s %10zu %10zu %10zu\n", NAME_MAX, item->name, max, min,
         total / count);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static const FAR
struct performance_entry_s *find_entry(FAR const char *name)
{
  size_t i;

  for (i = 0; i < nitems(g_entry_list); i++)
    {
      if (strcmp(name, g_entry_list[i].name) == 0)
        {
          return &g_entry_list[i];
        }
    }

  return NULL;
}

void performance_list(void)
{
  size_t i;

  for (i = 0; i < nitems(g_entry_list); i++)
    {
      printf("%s\n", g_entry_list[i].name);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const FAR struct performance_entry_s *item = NULL;
  bool detail = false;
  size_t count = 100;
  size_t i;
  int opt;

  while ((opt = getopt(argc, argv, "dc:hl")) != -1)
    {
      switch (opt)
        {
          case 'd':
            detail = true;
            break;
          case 'c':
            count = strtoul(optarg, NULL, 0);
            break;
          case 'h':
            performance_help();
            return EXIT_SUCCESS;
          case 'l':
            performance_list();
            return EXIT_SUCCESS;
          default:
            performance_help();
            return EXIT_FAILURE;
        }
    }

  if (optind < argc)
    {
      item = find_entry(argv[optind]);
      if (item == NULL)
        {
          printf("Can't find %s\n", argv[optind]);
          return EXIT_FAILURE;
        }
    }

  printf("OS performance args: count:%zu, detail:%s\n", count,
         detail ? "true" : "false");

  printf("==============================================================\n");
  printf("%-*s %10s %10s %10s\n", NAME_MAX, "Describe", "Max", "Min", "Avg");

  if (item != NULL)
    {
      performance_run(item, count, detail);
      return EXIT_SUCCESS;
    }

  for (i = 0; i < nitems(g_entry_list); i++)
    {
      item = &g_entry_list[i];
      performance_run(item, count, detail);
    }

  return EXIT_SUCCESS;
}
