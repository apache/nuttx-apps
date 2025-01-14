/****************************************************************************
 * apps/testing/mm/iob/iob_main.c
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
#include <nuttx/sched.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <iob.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAGIC_NUMBER1  47
#define MAGIC_NUMBER2  49
#define MAGIC_NUMBER3  37

#define IOB_TOTAL_BUFFER_SIZE (CONFIG_IOB_BUFSIZE * CONFIG_IOB_NBUFFERS)
#define IOB_TEST_BUFFER_SIZE  (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1)

#define TEST_COUNT 10000

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_buffer1[IOB_TOTAL_BUFFER_SIZE];
static uint8_t g_buffer2[IOB_TOTAL_BUFFER_SIZE];
static uint8_t g_iob_buffer1[IOB_TOTAL_BUFFER_SIZE];
static uint8_t g_iob_buffer2[IOB_TOTAL_BUFFER_SIZE];
static int failed_count[2];
static int success_count[2];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

FAR static void *thread_nothrottled(FAR void *arg)
{
  int idx = (int)(uintptr_t)arg;
  int nbytes;
  int size;
  uint8_t *inbuffer;
  uint8_t *outbuffer;

  printf("thread_nothrottled %d\n", idx);
  inbuffer = ((idx == 0) ? g_buffer1 : g_buffer2);
  outbuffer = ((idx == 0) ? g_iob_buffer1 : g_iob_buffer2);
  size = CONFIG_IOB_BUFSIZE * CONFIG_IOB_NBUFFERS;

  for (int i = 0; i < TEST_COUNT; i++)
    {
      struct iob_s *iob;

      iob = iob_alloc(false);
      for (int j = 0; j < IOB_TOTAL_BUFFER_SIZE; j++)
        {
          inbuffer[j] = (uint8_t)(i & j & 0xff);
        }

      if (iob_trycopyin(iob, inbuffer, size, 0, false) != size)
        {
          failed_count[idx]++;
          iob_free_chain(iob);
          continue;
        }

      success_count[idx]++;
      nbytes = iob_copyout(outbuffer, iob, IOB_TOTAL_BUFFER_SIZE, 0);
      ASSERT(size == nbytes);

      if (memcmp(inbuffer, outbuffer, nbytes) != 0)
        {
          printf("thread_nothrottled inbuffer does not match outbuffer\n");
          ASSERT(false);
        }

      iob_free_chain(iob);
    }

  return NULL;
}

FAR static void *thread_throttled(FAR void *arg)
{
  int idx = (int)(uintptr_t)arg;
  int nbytes;
  int size;
  uint8_t *inbuffer;
  uint8_t *outbuffer;

  printf("thread_throttled %d\n", idx);
  inbuffer = ((idx == 0) ? g_buffer1 : g_buffer2);
  outbuffer = ((idx == 0) ? g_iob_buffer1 : g_iob_buffer2);
  size = CONFIG_IOB_BUFSIZE * (CONFIG_IOB_NBUFFERS - CONFIG_IOB_THROTTLE);

  for (int i = 0; i < TEST_COUNT; i++)
    {
      struct iob_s *iob;

      iob = iob_alloc(true);
      for (int j = 0; j < IOB_TOTAL_BUFFER_SIZE; j++)
        {
          inbuffer[j] = (uint8_t)(i & j & 0xff);
        }

      if (iob_trycopyin(iob, inbuffer, size, 0, true) != size)
        {
          failed_count[idx]++;
          iob_free_chain(iob);
          continue;
        }

      success_count[idx]++;
      nbytes = iob_copyout(outbuffer, iob, IOB_TOTAL_BUFFER_SIZE, 0);
      ASSERT(size == nbytes);

      if (memcmp(inbuffer, outbuffer, nbytes) != 0)
        {
          printf("thread_throttled inbuffer does not match outbuffer\n");
          ASSERT(false);
        }

      iob_free_chain(iob);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * iob_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("iob_test!!\n");

  int            ret;
  cpu_set_t      cpuset;
  pthread_attr_t attr;
  pthread_t      thread[2];

  struct iob_s *iob;
  int nbytes;
  int i;

  iob = iob_alloc(false);

  for (i = 0; i < IOB_TEST_BUFFER_SIZE; i++)
    {
      g_buffer1[i] = (uint8_t)(i & 0xff);
    }

  memset(g_buffer2, 0xff, IOB_TEST_BUFFER_SIZE);

  iob_copyin(iob, g_buffer2, MAGIC_NUMBER1, 0, false);
  printf("Copy IN: MAGIC_NUMBER1, offset 0\n");
  ASSERT(iob->io_pktlen == MAGIC_NUMBER1);
  ASSERT(iob->io_offset == 0);
  ASSERT(iob->io_len == MAGIC_NUMBER1);

  iob_copyin(iob, g_buffer1, IOB_TEST_BUFFER_SIZE, MAGIC_NUMBER1, false);
  printf("Copy IN: IOB_TEST_BUFFER_SIZE, offset 0\n");
  ASSERT(iob->io_pktlen == IOB_TOTAL_BUFFER_SIZE);
  ASSERT(iob->io_offset == 0);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  nbytes = iob_copyout(g_buffer2, iob, IOB_TEST_BUFFER_SIZE, MAGIC_NUMBER1);
  printf("Copy OUT: %d, offset 0\n", nbytes);
  ASSERT(nbytes == IOB_TEST_BUFFER_SIZE);
  ASSERT(iob->io_pktlen == IOB_TOTAL_BUFFER_SIZE);
  ASSERT(iob->io_offset == 0);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  if (memcmp(g_buffer1, g_buffer2, nbytes) != 0)
    {
      printf("Buffer1 does not match g_buffer2\n");
      ASSERT(false);
    }

  iob = iob_trimhead(iob, MAGIC_NUMBER1);
  printf("Trim: MAGIC_NUMBER1 from the beginning of the list\n");
  ASSERT(iob->io_pktlen == (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1));
  ASSERT(iob->io_offset == MAGIC_NUMBER1);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  iob = iob_trimtail(iob, MAGIC_NUMBER2);
  printf("Trim: MAGIC_NUMBER2 from the end of the list\n");
  ASSERT(iob->io_pktlen == (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1 -
         MAGIC_NUMBER2));
  ASSERT(iob->io_offset == MAGIC_NUMBER1);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  nbytes = iob_copyout(g_buffer2, iob, IOB_TEST_BUFFER_SIZE, 0);
  printf("Copy OUT: %d, offset MAGIC_NUMBER1\n", nbytes);
  ASSERT(nbytes == iob->io_pktlen);
  ASSERT(iob->io_pktlen == (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1 -
         MAGIC_NUMBER2));
  ASSERT(iob->io_offset == MAGIC_NUMBER1);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  if (memcmp(g_buffer1, g_buffer2, nbytes) != 0)
    {
      printf("Buffer1 does not match g_buffer2\n");
      ASSERT(false);
    }

  iob = iob_trimhead(iob, MAGIC_NUMBER2);
  printf("Trim: MAGIC_NUMBER2 from the beginning of the list\n");
  ASSERT(iob->io_pktlen == (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1 -
         MAGIC_NUMBER2 - MAGIC_NUMBER2));

  nbytes = iob_copyout(g_buffer2, iob, IOB_TEST_BUFFER_SIZE, 0);
  printf("Copy OUT: %d, offset xxx\n", nbytes);
  ASSERT(nbytes == iob->io_pktlen);
  ASSERT(iob->io_pktlen == (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1 -
         MAGIC_NUMBER2 - MAGIC_NUMBER2));

  if (memcmp(&g_buffer1[MAGIC_NUMBER2], g_buffer2, nbytes) != 0)
    {
      printf("Buffer1 does not match g_buffer2\n");
      ASSERT(false);
    }

  iob = iob_pack(iob);
  printf("Packed\n");
  ASSERT(iob->io_pktlen == (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1 -
         MAGIC_NUMBER2 - MAGIC_NUMBER2));
  ASSERT(iob->io_offset == 0);

  nbytes = iob_copyout(g_buffer2, iob, IOB_TEST_BUFFER_SIZE, 0);
  printf("Copy OUT: %d, offset 0\n", nbytes);
  ASSERT(nbytes == iob->io_pktlen);
  ASSERT(iob->io_offset == 0);

  if (memcmp(&g_buffer1[MAGIC_NUMBER2], g_buffer2, nbytes) != 0)
    {
      printf("Buffer1 does not match g_buffer2\n");
      ASSERT(false);
    }

  nbytes = iob->io_pktlen;

  iob_reserve(iob, 55);
  printf("Reserve: adjust offset to 55\n");

  if (iob->io_offset != 55 || iob->io_pktlen + 55 != nbytes)
    {
      printf("Offset or packet length wrong\n");
      ASSERT(false);
    }

  iob_reserve(iob, 28);
  printf("Reserve: adjust offset to 28\n");

  if (iob->io_offset != 28 || iob->io_pktlen + 28 != nbytes)
    {
      printf("Offset or packet length wrong\n");
      ASSERT(false);
    }

  iob_free_chain(iob);

  /* test race conditon between nothrottled and nothrottled */

  failed_count[0] = 0;
  failed_count[1] = 0;
  success_count[0] = 0;
  success_count[1] = 0;

  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  pthread_attr_init(&attr);
#ifdef CONFIG_SMP
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
#endif
  ret = pthread_create(&thread[0], &attr, thread_nothrottled, (void *)0);
  ASSERT(ret == 0);

  CPU_ZERO(&cpuset);
  CPU_SET(1, &cpuset);
  pthread_attr_init(&attr);
#ifdef CONFIG_SMP
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
#endif
  ret = pthread_create(&thread[1], &attr, thread_nothrottled, (void *)1);
  ASSERT(ret == 0);

  pthread_join(thread[0], NULL);
  pthread_join(thread[1], NULL);
  printf("nothrottled thread 0 statistc failed_count %d success_count %d\n",
          failed_count[0], success_count[0]);
  printf("nothrottled thread 1 statistc failed_count %d success_count %d\n",
          failed_count[1], success_count[1]);

  /* test race conditon between throttled and nothrottled */

  failed_count[0] = 0;
  failed_count[1] = 0;
  success_count[0] = 0;
  success_count[1] = 0;

  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  pthread_attr_init(&attr);
#ifdef CONFIG_SMP
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
#endif
  ret = pthread_create(&thread[0], &attr, thread_nothrottled, (void *)0);
  ASSERT(ret == 0);

  CPU_ZERO(&cpuset);
  CPU_SET(1, &cpuset);
  pthread_attr_init(&attr);
#ifdef CONFIG_SMP
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
#endif
  ret = pthread_create(&thread[1], &attr, thread_throttled, (void *)1);
  ASSERT(ret == 0);

  pthread_join(thread[0], NULL);
  pthread_join(thread[1], NULL);
  printf("nothrottled thread 0 statistc failed_count %d success_count %d\n",
          failed_count[0], success_count[0]);
  printf("throttled thread 1 statistc failed_count %d success_count %d\n",
          failed_count[1], success_count[1]);

  /* test race conditon between throttled and throttled */

  failed_count[0] = 0;
  failed_count[1] = 0;
  success_count[0] = 0;
  success_count[1] = 0;

  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  pthread_attr_init(&attr);
#ifdef CONFIG_SMP
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
#endif
  ret = pthread_create(&thread[0], &attr, thread_throttled, (void *)0);
  ASSERT(ret == 0);

  CPU_ZERO(&cpuset);
  CPU_SET(1, &cpuset);
  pthread_attr_init(&attr);
#ifdef CONFIG_SMP
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
#endif
  ret = pthread_create(&thread[1], &attr, thread_throttled, (void *)1);
  ASSERT(ret == 0);

  pthread_join(thread[0], NULL);
  pthread_join(thread[1], NULL);
  printf("throttled thread 0 statistc failed_count %d success_count %d\n",
          failed_count[0], success_count[0]);
  printf("throttled thread 1 statistc failed_count %d success_count %d\n",
          failed_count[1], success_count[1]);

  return 0;
}
