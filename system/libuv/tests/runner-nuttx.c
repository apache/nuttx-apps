/****************************************************************************
 * system/libuv/tests/runner-nuttx.c
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

#include <debug.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>

#include <uv.h>
#include "runner-nuttx.h"
#include "runner.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static uint32_t get_time_ms(void);
static void *process_launcher(void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint32_t get_time_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint32_t)(((uint64_t) ts.tv_sec) * 1000 +
    (ts.tv_nsec / 1000 / 1000));
}

static void *process_launcher(void *arg)
{
  process_info_t *p = (process_info_t *)arg;
  p->task->main();
  sem_post(&p->sem);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void platform_init(int argc, char **argv)
{
}

int process_start(task_entry_t *task, process_info_t *p, int is_helper)
{
  if (is_helper)
    {
      printf("===> START HELPER %s/%s\n",
             task->task_name, task->process_name);
    }
  else
    {
      printf("===> START TEST   %s\n", task->task_name);
    }

  p->task = task;

  if (sem_init(&p->sem, 0, 0))
    {
      return -1;
    }

  if (pthread_create(&p->tid, NULL, process_launcher, p) != 0)
    {
      fprintf(stderr, "Failed to start process %s\n", task->process_name);
      sem_destroy(&p->sem);
      return -1;
    }

  return 0;
}

int process_wait(process_info_t *vec, int n, int timeout)
{
  uv_time_t time = get_time_ms();

  while (--n >= 0)
    {
      int found = 0;
      while ((uv_time_t)(get_time_ms()-time) < (uv_time_t)timeout)
        {
          if (sem_trywait(&vec[n].sem) == 0)
            {
              FAR pthread_addr_t value;
              pthread_join(vec[n].tid, &value);
              found = 1;
              break;
            }

          if (errno != EAGAIN)
            {
              break;
            }

          /* Let NuttX update time */

          usleep(100 * 1000);
        }

      if (!found)
        {
          /* Timeout or error */

          return -1;
        }
    }

  return 0;
}

int process_terminate(process_info_t *p)
{
  /* TODO */

  return 0;
}

int process_reap(process_info_t *p)
{
  /* TODO */

  return 0;
}

void process_cleanup(process_info_t *p)
{
}
