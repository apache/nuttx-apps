/****************************************************************************
 * apps/testing/ostest/task.c
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

#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <spawn.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/compiler.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_EXEC_ARGS 256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct main_args_s
{
  main_t     entry;
  int        prio;
  int        argc;
  FAR char  *argv[];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main_trampoline
 *
 * Description:
 *   Trampoline function for pthread to pass argv among other things
 *
 ****************************************************************************/

static void *main_trampoline(void *ptr)
{
  struct main_args_s *args = (struct main_args_s *) ptr;
  pthread_setschedprio(pthread_self(), args->prio);
  pthread_setname_np(pthread_self(), args->argv[0]);
  args->entry(args->argc, args->argv);
  free(ptr);
  return NULL;
}

/****************************************************************************
 * Name: task_create
 *
 * Description:
 *   Adaptation for CONFIG_BUILD_KERNEL to compile ostest using pthreads
 *   instead of NuttX tasks.
 *
 ****************************************************************************/

int task_create(FAR const char *name, int priority,
                int stack_size, main_t entry, FAR char * const argv[])
{
  FAR struct main_args_s *args;
  FAR char *ptr;
  pthread_t pid;
  pthread_attr_t attr;
  size_t argvsize = 0;
  size_t argssize;
  int argc = 0;
  int ret;
  int i;

  /* Get the number of arguments and the size of the argument list */

  if (argv)
    {
      while (argv[argc])
        {
          argvsize += strlen(argv[argc]) + 1;
          if (argc >= MAX_EXEC_ARGS)
            {
              ret = E2BIG;
              goto update_errno;
            }

          argc++;
        }
    }

  /* Name is a part of argv */

  argvsize += strlen(name) + 1;

  /* Allocate the struct + memory for argv + name */

  argssize = sizeof(struct main_args_s) + (argc + 2) * sizeof(FAR char *);

  args = (struct main_args_s *)malloc(argssize + argvsize);
  if (!args)
    {
      ret = ENOMEM;
      goto update_errno;
    }

  /* Initialize the struct */

  args->entry = entry;
  args->prio = priority;
  args->argc = argc + 1; /* +1 for name */

  /* Copy the name */

  ptr = (char *)args + argssize;
  args->argv[0] = ptr;
  strcpy(ptr, name);
  ptr += strlen(name) + 1;

  /* Copy the argv list */

  for (i = 0; i < argc; i++)
    {
      args->argv[i + 1] = ptr;
      strcpy(ptr, argv[i]);
      ptr += strlen(argv[i]) + 1;
    }

  /* Terminate the argv[] list */

  args->argv[args->argc] = NULL;

  /* Set the worker parameters */

  pthread_attr_init(&attr);
  pthread_attr_setschedpolicy(&attr, SCHED_RR);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setstacksize(&attr, stack_size);

  /* Start the worker */

  ret = pthread_create(&pid, &attr, &main_trampoline, args);
  if (ret != 0)
    {
      printf("ERROR: pthread_create:%d, errno:%s\n", ret, strerror(ret));
      free(args);
      pid = ERROR;
    }

  pthread_attr_destroy(&attr);

update_errno:
  if (ret != 0)
    {
      set_errno(-ret);
      pid = ERROR;
    }

  return (int)pid;
}

/****************************************************************************
 * Name: task_delete
 *
 * Description:
 *   Adaptation for CONFIG_BUILD_KERNEL to compile ostest using pthreads
 *   instead of NuttX tasks.
 *
 ****************************************************************************/

int task_delete(int pid)
{
  int ret;

  if (pid == pthread_self())
    {
      ret = pthread_join(pid, NULL);
      pthread_exit(NULL);
    }
  else
    {
      ret = pthread_cancel(pid);
    }

  if (ret < 0)
    {
      set_errno(-ret);
      ret = ERROR;
    }

  return ret;
}

