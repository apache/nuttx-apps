/****************************************************************************
 * apps/testing/fs/memfd/memfd_test.c
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
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

 #define BUFFER_SIZE 8
 #define THREAD_NUM 50
 #define LOOP_NUM 1000

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *func(FAR void *arg)
{
  void *buf;
  int size = BUFFER_SIZE;
  int i;
  int memfd;

  for (i = 0; i < LOOP_NUM; i++)
    {
      memfd = memfd_create("optee", O_CREAT | O_CLOEXEC);
      ftruncate(memfd, size);
      buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
      close(memfd);

      *((int *)buf) = 0xdeadbeef;
      usleep(10);
      *((int *)buf) = 0xdeadbeef;

      munmap(buf, size);
    }

  printf("thread %d test pass!\n", pthread_self());
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  pid_t pid[THREAD_NUM];
  int i;

  for (i = 0; i < THREAD_NUM; i++)
    {
      pthread_create(&pid[i], NULL, func, NULL);
    }

  for (i = 0; i < THREAD_NUM; i++)
    {
      pthread_join(pid[i], NULL);
    }

  return 0;
}
