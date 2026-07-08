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

#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/mm/iob.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAGIC_NUMBER1  47
#define MAGIC_NUMBER2  49
#define MAGIC_NUMBER3  37

#define IOB_TOTAL_BUFFER_SIZE (CONFIG_IOB_BUFSIZE * CONFIG_IOB_NBUFFERS)
#define IOB_TEST_BUFFER_SIZE  (IOB_TOTAL_BUFFER_SIZE - MAGIC_NUMBER1)

#define TEST_COUNT 10000
#define TEST_DATA_SIZE 256
#define TEST_PATTERN_1 0xAA
#define TEST_PATTERN_2 0x55

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

/****************************************************************************
 * Name: print_iob_chain
 *
 * Description:
 *   Print the structure of an IOB chain with addresses and flags
 *
 ****************************************************************************/

static void print_iob_chain(FAR struct iob_s *iob, const char *label)
{
  FAR struct iob_s *current;
  int count = 0;

  printf("\n%s:\n", label);

  if (iob == NULL)
    {
      printf("  (empty)\n");
      return;
    }

  current = iob;
  while (current != NULL)
    {
      printf("  [%d] IOB=%p, io_flink=%p, flags=0x%x, CHAIN_END=%d, "
             "next=%p\n", count, current, current->io_flink,
             (int)IOB_FLAG_GET(current),
             IOB_FLAG_TEST(current, IOB_CHAIN_END) ? 1 : 0,
             IOB_LINK_GET(current));

      count++;
      current = IOB_LINK_GET(current);

      /* Safety check to prevent infinite loop */

      if (count > 100)
        {
          printf("  ... (stopped at 100 to prevent infinite loop)\n");
          break;
        }
    }

  printf("  Total IOBs in chain: %d\n", count);
}

/****************************************************************************
 * Name: print_iob_queue
 *
 * Description:
 *   Print the entire queue structure showing all chains with their flags
 *
 ****************************************************************************/

static void print_iob_queue(FAR struct iob_queue_s *queue, const char *label)
{
  FAR struct iob_s *chain_head;
  FAR struct iob_s *current;
  int chain_num = 0;
  int iob_num;

  printf("\n========================================\n");
  printf("%s:\n", label);
  printf("========================================\n");
  printf("Queue: qh_head=%p, qh_tail=%p\n", queue->qh_head, queue->qh_tail);

  if (IOB_QEMPTY(queue))
    {
      printf("  (empty queue)\n");
      printf("========================================\n");
      return;
    }

  chain_head = queue->qh_head;

  while (chain_head != NULL)
    {
      printf("\nChain #%d (head=%p):\n", chain_num, chain_head);

      current = chain_head;
      iob_num = 0;

      /* Print all IOBs in this chain */

      while (current != NULL)
        {
          printf("  [%d.%d] IOB=%p, io_flink=%p (raw), ",
                 chain_num, iob_num, current, current->io_flink);
          printf("flags=0x%x, CHAIN_END=%d, next=%p\n",
                 (int)IOB_FLAG_GET(current),
                 IOB_FLAG_TEST(current, IOB_CHAIN_END) ? 1 : 0,
                 IOB_LINK_GET(current));

          /* If this IOB has CHAIN_END flag, next pointer is the next chain */

          if (IOB_FLAG_TEST(current, IOB_CHAIN_END))
            {
              chain_head = IOB_LINK_GET(current);
              printf("  └─> End of chain (next chain at %p)\n",
                     chain_head);
              break;
            }

          current = IOB_LINK_GET(current);
          iob_num++;

          /* Safety check */

          if (iob_num > 50)
            {
              printf("  ... (stopped to prevent infinite loop)\n");
              break;
            }
        }

      chain_num++;

      /* Safety check */

      if (chain_num > 20)
        {
          printf("\n... (stopped at 20 chains)\n");
          break;
        }
    }

  printf("\nTotal chains in queue: %d\n", chain_num);
  printf("========================================\n");
}

/****************************************************************************
 * Name: create_iob_chain
 *
 * Description:
 *   Create a chain of IOBs with specified length
 *
 ****************************************************************************/

static FAR struct iob_s *create_iob_chain(int length, uint8_t pattern)
{
  FAR struct iob_s *head = NULL;
  FAR struct iob_s *iob;
  uint8_t data[16];
  int i;

  if (length <= 0)
    {
      return NULL;
    }

  memset(data, pattern, sizeof(data));

  /* Allocate first IOB */

  head = iob_tryalloc(false);
  if (head == NULL)
    {
      printf("ERROR: Failed to allocate first IOB in chain\n");
      return NULL;
    }

  /* Add minimal data to first IOB */

  head->io_len = sizeof(data);
  head->io_offset = 0;
  head->io_pktlen = sizeof(data);
  memcpy(&head->io_data[0], data, sizeof(data));

  /* Create and concatenate remaining IOBs */

  for (i = 1; i < length; i++)
    {
      iob = iob_tryalloc(false);
      if (iob == NULL)
        {
          printf("ERROR: Failed to allocate IOB %d in chain\n", i);
          iob_free_chain(head);
          return NULL;
        }

      /* Add minimal data to this IOB */

      iob->io_len = sizeof(data);
      iob->io_offset = 0;
      iob->io_pktlen = sizeof(data);
      memcpy(&iob->io_data[0], data, sizeof(data));

      /* Use iob_concat to properly link the IOBs */

      iob_concat(head, iob);
    }

  return head;
}

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

  /* Other tasks hold part of the pool; size the copy from what is free so
   * the try-copy can complete, keeping two buffers as headroom.
   */

  size = CONFIG_IOB_BUFSIZE * (iob_navail(false) - 2);

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

  /* iob_navail(true) already excludes the throttle reserve. */

  size = CONFIG_IOB_BUFSIZE * (iob_navail(true) - 2);

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

static int test_iob_alloc_free(void)
{
  FAR struct iob_s *iob1;
  FAR struct iob_s *iob2;
  FAR struct iob_s *iob3;
  int initial_count;
  int count;

  printf("\n=== Test 1: IOB Allocation and Free ===\n");

  initial_count = iob_navail(false);
  printf("Initial available IOBs: %d\n", initial_count);

  /* Test tryalloc */

  iob1 = iob_tryalloc(false);
  if (iob1 == NULL)
    {
      printf("ERROR: iob_tryalloc failed\n");
      return -1;
    }

  printf("iob_tryalloc success, iob=%p\n", iob1);
  count = iob_navail(false);
  printf("Available IOBs after tryalloc: %d\n", count);
  DEBUGASSERT(count == initial_count - 1);

  /* Test alloc */

  iob2 = iob_alloc(false);
  if (iob2 == NULL)
    {
      printf("ERROR: iob_alloc failed\n");
      iob_free(iob1);
      return -1;
    }

  printf("iob_alloc success, iob=%p\n", iob2);
  count = iob_navail(false);
  printf("Available IOBs after alloc: %d\n", count);
  DEBUGASSERT(count == initial_count - 2);

  /* Test free single buffer */

  iob3 = iob_free(iob1);
  DEBUGASSERT(iob3 == NULL);  /* Should be NULL since iob1 has no chain */
  count = iob_navail(false);
  printf("Available IOBs after free: %d\n", count);
  DEBUGASSERT(count == initial_count - 1);

  iob_free(iob2);
  count = iob_navail(false);
  printf("Available IOBs after second free: %d\n", count);
  DEBUGASSERT(count == initial_count);

  printf("Test 1 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_copyin_copyout
 *
 * Description:
 *   Test data copy in and copy out operations
 *
 ****************************************************************************/

static int test_iob_copyin_copyout(void)
{
  FAR struct iob_s *iob;
  uint8_t src_data[TEST_DATA_SIZE];
  uint8_t dest_data[TEST_DATA_SIZE];
  int i;
  int ret;

  printf("\n=== Test 2: IOB Copy In/Out ===\n");

  /* Prepare test data */

  for (i = 0; i < TEST_DATA_SIZE; i++)
    {
      src_data[i] = i & 0xff;
    }

  /* Allocate IOB */

  iob = iob_alloc(false);
  if (iob == NULL)
    {
      printf("ERROR: Failed to allocate IOB\n");
      return -1;
    }

  /* Test iob_copyin() */

  ret = iob_copyin(iob, src_data, TEST_DATA_SIZE, 0, false);
  if (ret < 0)
    {
      printf("ERROR: iob_copyin failed, ret=%d\n", ret);
      iob_free_chain(iob);
      return -1;
    }

  printf("iob_copyin success, copied %d bytes\n", ret);
  printf("IOB packet length: %u\n", iob->io_pktlen);
  DEBUGASSERT(iob->io_pktlen == TEST_DATA_SIZE);

  /* Test copyout */

  memset(dest_data, 0, TEST_DATA_SIZE);
  ret = iob_copyout(dest_data, iob, TEST_DATA_SIZE, 0);
  if (ret < 0)
    {
      printf("ERROR: iob_copyout failed, ret=%d\n", ret);
      iob_free_chain(iob);
      return -1;
    }

  printf("iob_copyout success, copied %d bytes\n", ret);

  /* Verify data integrity */

  if (memcmp(src_data, dest_data, TEST_DATA_SIZE) != 0)
    {
      printf("ERROR: Data mismatch after copy\n");
      iob_free_chain(iob);
      return -1;
    }

  printf("Data integrity verified\n");

  /* Clean up */

  iob_free_chain(iob);
  printf("Test 2 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_queue_operations
 *
 * Description:
 *   Test IOB queue operations (add, remove, peek)
 *
 ****************************************************************************/

static int test_iob_queue_operations(void)
{
  struct iob_queue_s queue;
  FAR struct iob_s *iob1;
  FAR struct iob_s *iob2;
  FAR struct iob_s *iob3;
  FAR struct iob_s *removed;
  FAR struct iob_s *peeked;
  uint8_t data[32];
  unsigned int size;
  unsigned int count;

  printf("\n=== Test 3: IOB Queue Operations ===\n");

  /* Initialize queue */

  IOB_QINIT(&queue);
  DEBUGASSERT(IOB_QEMPTY(&queue));
  printf("Queue initialized\n");

  /* Allocate and prepare IOBs */

  iob1 = iob_alloc(false);
  iob2 = iob_alloc(false);
  iob3 = iob_alloc(false);

  if (iob1 == NULL || iob2 == NULL || iob3 == NULL)
    {
      printf("ERROR: Failed to allocate IOBs\n");
      if (iob1)
        {
          iob_free(iob1);
        }

      if (iob2)
        {
          iob_free(iob2);
        }

      if (iob3)
        {
          iob_free(iob3);
        }

      return -1;
    }

  /* Add data to each IOB */

  memset(data, TEST_PATTERN_1, sizeof(data));
  iob_copyin(iob1, data, sizeof(data), 0, false);

  memset(data, TEST_PATTERN_2, sizeof(data));
  iob_copyin(iob2, data, sizeof(data), 0, false);

  memset(data, 0x77, sizeof(data));
  iob_copyin(iob3, data, sizeof(data), 0, false);

  /* Test add_queue */

  iob_add_queue(iob1, &queue);

  printf("Added iob1 to queue\n");

  iob_add_queue(iob2, &queue);

  printf("Added iob2 to queue\n");

  iob_add_queue(iob3, &queue);

  printf("Added iob3 to queue\n");

  /* Test queue size and entry count */

  size = iob_get_queue_size(&queue);
  count = iob_get_queue_entry_count(&queue);
  printf("Queue size: %u bytes, entry count: %u\n", size, count);
  DEBUGASSERT(count == 3);
  DEBUGASSERT(size == sizeof(data) * 3);

  /* Test peek_queue */

  peeked = iob_peek_queue(&queue);
  if (peeked == NULL)
    {
      printf("ERROR: iob_peek_queue returned NULL\n");
      iob_free_queue(&queue);
      return -1;
    }

  printf("Peeked queue, iob=%p (should be iob1=%p)\n", peeked, iob1);
  DEBUGASSERT(peeked == iob1);

  /* Queue should still have same size after peek */

  count = iob_get_queue_entry_count(&queue);
  DEBUGASSERT(count == 3);

  /* Test remove_queue */

  removed = iob_remove_queue(&queue);
  if (removed == NULL)
    {
      printf("ERROR: iob_remove_queue returned NULL\n");
      iob_free_queue(&queue);
      return -1;
    }

  printf("Removed from queue, iob=%p\n", removed);
  DEBUGASSERT(removed == iob1);
  iob_free_chain(removed);

  count = iob_get_queue_entry_count(&queue);
  printf("Queue entry count after removal: %u\n", count);
  DEBUGASSERT(count == 2);

  /* Remove second element */

  removed = iob_remove_queue(&queue);
  DEBUGASSERT(removed == iob2);
  iob_free_chain(removed);

  count = iob_get_queue_entry_count(&queue);
  DEBUGASSERT(count == 1);

  /* Remove last element */

  removed = iob_remove_queue(&queue);
  DEBUGASSERT(removed == iob3);
  iob_free_chain(removed);

  /* Queue should be empty now */

  DEBUGASSERT(IOB_QEMPTY(&queue));
  count = iob_get_queue_entry_count(&queue);
  printf("Queue is empty, entry count: %u\n", count);
  DEBUGASSERT(count == 0);

  printf("Test 3 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_chain_operations
 *
 * Description:
 *   Test IOB chain operations (concat, trimhead, trimtail)
 *
 ****************************************************************************/

static int test_iob_chain_operations(void)
{
  FAR struct iob_s *iob1;
  FAR struct iob_s *iob2;
  FAR struct iob_s *result;
  uint8_t data[64];
  uint8_t verify[128];
  int i;
  int chain_count;
  unsigned int pktlen;

  printf("\n=== Test 4: IOB Chain Operations ===\n");

  /* Allocate two IOBs */

  iob1 = iob_alloc(false);
  iob2 = iob_alloc(false);

  if (iob1 == NULL || iob2 == NULL)
    {
      printf("ERROR: Failed to allocate IOBs\n");
      if (iob1)
        {
          iob_free(iob1);
        }

      if (iob2)
        {
          iob_free(iob2);
        }

      return -1;
    }

  /* Fill with test data */

  for (i = 0; i < sizeof(data); i++)
    {
      data[i] = i;
    }

  iob_copyin(iob1, data, sizeof(data), 0, false);

  for (i = 0; i < sizeof(data); i++)
    {
      data[i] = i + 64;
    }

  iob_copyin(iob2, data, sizeof(data), 0, false);

  /* Test concat */

  printf("Concatenating chains...\n");
  pktlen = iob1->io_pktlen + iob2->io_pktlen;
  iob_concat(iob1, iob2);

  printf("After concat: io_pktlen=%u (expected %u)\n",
         iob1->io_pktlen, pktlen);
  DEBUGASSERT(iob1->io_pktlen == pktlen);

  chain_count = iob_count(iob1);
  printf("Chain count: %d\n", chain_count);

  /* Verify data after concat */

  iob_copyout(verify, iob1, sizeof(verify), 0);
  for (i = 0; i < 64; i++)
    {
      if (verify[i] != i)
        {
          printf("ERROR: Data mismatch at position %d\n", i);
          iob_free_chain(iob1);
          return -1;
        }
    }

  for (i = 64; i < 128; i++)
    {
      if (verify[i] != i)
        {
          printf("ERROR: Data mismatch at position %d\n", i);
          iob_free_chain(iob1);
          return -1;
        }
    }

  printf("Concat data integrity verified\n");

  /* Test trimhead */

  printf("Testing trimhead (remove 32 bytes)...\n");
  result = iob_trimhead(iob1, 32);
  DEBUGASSERT(result != NULL);
  printf("After trimhead: io_pktlen=%u\n", result->io_pktlen);
  DEBUGASSERT(result->io_pktlen == pktlen - 32);

  /* Test trimtail */

  printf("Testing trimtail (remove 32 bytes)...\n");
  result = iob_trimtail(result, 32);
  printf("After trimtail: io_pktlen=%u\n", result->io_pktlen);
  DEBUGASSERT(result->io_pktlen == pktlen - 64);

  /* Clean up */

  iob_free_chain(result);
  printf("Test 4 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_clone
 *
 * Description:
 *   Test IOB clone operations
 *
 ****************************************************************************/

static int test_iob_clone(void)
{
  FAR struct iob_s *iob1;
  FAR struct iob_s *iob2;
  uint8_t src_data[TEST_DATA_SIZE];
  uint8_t dest_data[TEST_DATA_SIZE];
  int i;
  int ret;

  printf("\n=== Test 5: IOB Clone ===\n");

  /* Prepare test data */

  for (i = 0; i < TEST_DATA_SIZE; i++)
    {
      src_data[i] = (i * 3) & 0xff;
    }

  /* Allocate source IOB */

  iob1 = iob_alloc(false);
  if (iob1 == NULL)
    {
      printf("ERROR: Failed to allocate source IOB\n");
      return -1;
    }

  ret = iob_copyin(iob1, src_data, TEST_DATA_SIZE, 0, false);
  if (ret < 0)
    {
      printf("ERROR: Failed to copy data to source IOB\n");
      iob_free_chain(iob1);
      return -1;
    }

  /* Allocate destination IOB */

  iob2 = iob_alloc(false);
  if (iob2 == NULL)
    {
      printf("ERROR: Failed to allocate destination IOB\n");
      iob_free_chain(iob1);
      return -1;
    }

  /* Test clone */

  printf("Cloning IOB...\n");
  ret = iob_clone(iob1, iob2, false, true);
  if (ret < 0)
    {
      printf("ERROR: iob_clone failed, ret=%d\n", ret);
      iob_free_chain(iob1);
      iob_free_chain(iob2);
      return -1;
    }

  printf("Clone success\n");
  printf("Original io_pktlen=%u, Clone io_pktlen=%u\n",
         iob1->io_pktlen, iob2->io_pktlen);
  DEBUGASSERT(iob1->io_pktlen == iob2->io_pktlen);

  /* Verify cloned data */

  memset(dest_data, 0, TEST_DATA_SIZE);
  ret = iob_copyout(dest_data, iob2, TEST_DATA_SIZE, 0);
  if (ret < 0)
    {
      printf("ERROR: Failed to copy out cloned data\n");
      iob_free_chain(iob1);
      iob_free_chain(iob2);
      return -1;
    }

  if (memcmp(src_data, dest_data, TEST_DATA_SIZE) != 0)
    {
      printf("ERROR: Cloned data mismatch\n");
      iob_free_chain(iob1);
      iob_free_chain(iob2);
      return -1;
    }

  printf("Cloned data integrity verified\n");

  /* Clean up */

  iob_free_chain(iob1);
  iob_free_chain(iob2);
  printf("Test 5 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_pack_contig
 *
 * Description:
 *   Test IOB pack and contig operations
 *
 ****************************************************************************/

static int test_iob_pack_contig(void)
{
  FAR struct iob_s *iob;
  uint8_t data[100];
  int i;
  int ret;

  printf("\n=== Test 6: IOB Pack and Contig ===\n");

  /* Allocate IOB */

  iob = iob_alloc(false);
  if (iob == NULL)
    {
      printf("ERROR: Failed to allocate IOB\n");
      return -1;
    }

  /* Add some data with offset */

  for (i = 0; i < sizeof(data); i++)
    {
      data[i] = i & 0xff;
    }

  iob_copyin(iob, data, sizeof(data), 0, false);

  printf("Original io_pktlen=%u, io_offset=%u, io_len=%u\n",
         iob->io_pktlen, iob->io_offset, iob->io_len);

  /* Test pack - should consolidate data and remove offset */

  printf("Packing IOB...\n");
  iob = iob_pack(iob);
  if (iob == NULL)
    {
      printf("ERROR: iob_pack failed\n");
      return -1;
    }

  printf("After pack: io_pktlen=%u, io_offset=%u, io_len=%u\n",
         iob->io_pktlen, iob->io_offset, iob->io_len);

  /* Test contig - ensure contiguous space */

  printf("Testing contig (50 bytes)...\n");
  ret = iob_contig(iob, 50);
  if (ret < 0)
    {
      printf("ERROR: iob_contig failed, ret=%d\n", ret);
      iob_free_chain(iob);
      return -1;
    }

  printf("Contig success\n");

  /* Test reserve */

  printf("Testing reserve (10 bytes headroom)...\n");
  iob_reserve(iob, 10);
  printf("After reserve: io_offset=%u\n", iob->io_offset);

  /* Clean up */

  iob_free_chain(iob);
  printf("Test 6 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_update_pktlen
 *
 * Description:
 *   Test IOB packet length update
 *
 ****************************************************************************/

static int test_iob_update_pktlen(void)
{
  FAR struct iob_s *iob;
  uint8_t data[100];
  int i;
  int ret;

  printf("\n=== Test 7: IOB Update Packet Length ===\n");

  /* Allocate IOB */

  iob = iob_alloc(false);
  if (iob == NULL)
    {
      printf("ERROR: Failed to allocate IOB\n");
      return -1;
    }

  /* Add initial data */

  for (i = 0; i < sizeof(data); i++)
    {
      data[i] = i & 0xff;
    }

  iob_copyin(iob, data, sizeof(data), 0, false);
  printf("Initial io_pktlen=%u\n", iob->io_pktlen);

  /* Test expanding packet length */

  printf("Expanding packet length to 150 bytes...\n");
  ret = iob_update_pktlen(iob, 150, false);
  if (ret < 0)
    {
      printf("ERROR: iob_update_pktlen expand failed, ret=%d\n", ret);
      iob_free_chain(iob);
      return -1;
    }

  printf("After expand: io_pktlen=%u\n", iob->io_pktlen);
  DEBUGASSERT(iob->io_pktlen == 150);

  /* Test shrinking packet length */

  printf("Shrinking packet length to 50 bytes...\n");
  ret = iob_update_pktlen(iob, 50, false);
  if (ret < 0)
    {
      printf("ERROR: iob_update_pktlen shrink failed, ret=%d\n", ret);
      iob_free_chain(iob);
      return -1;
    }

  printf("After shrink: io_pktlen=%u\n", iob->io_pktlen);
  DEBUGASSERT(iob->io_pktlen == 50);

  /* Clean up */

  iob_free_chain(iob);
  printf("Test 7 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_queue_concat
 *
 * Description:
 *   Test IOB queue concatenation
 *
 ****************************************************************************/

static int test_iob_queue_concat(void)
{
  struct iob_queue_s queue1;
  struct iob_queue_s queue2;
  FAR struct iob_s *iob1;
  FAR struct iob_s *iob2;
  FAR struct iob_s *iob3;
  unsigned int count1;
  unsigned int count2;
  unsigned int total_count;

  printf("\n=== Test 8: IOB Queue Concatenation ===\n");

  /* Initialize queues */

  IOB_QINIT(&queue1);
  IOB_QINIT(&queue2);

  /* Allocate IOBs */

  iob1 = iob_alloc(false);
  iob2 = iob_alloc(false);
  iob3 = iob_alloc(false);

  if (iob1 == NULL || iob2 == NULL || iob3 == NULL)
    {
      printf("ERROR: Failed to allocate IOBs\n");
      if (iob1)
        {
          iob_free(iob1);
        }

      if (iob2)
        {
          iob_free(iob2);
        }

      if (iob3)
        {
          iob_free(iob3);
        }

      return -1;
    }

  /* Add to queues */

  iob_add_queue(iob1, &queue1);
  iob_add_queue(iob2, &queue1);
  count1 = iob_get_queue_entry_count(&queue1);
  printf("Queue1 entry count: %u\n", count1);
  DEBUGASSERT(count1 == 2);

  iob_add_queue(iob3, &queue2);
  count2 = iob_get_queue_entry_count(&queue2);
  printf("Queue2 entry count: %u\n", count2);
  DEBUGASSERT(count2 == 1);

  /* Test concat_queue */

  printf("Concatenating queue2 to queue1...\n");
  iob_concat_queue(&queue1, &queue2);

  total_count = iob_get_queue_entry_count(&queue1);
  printf("After concat, queue1 entry count: %u\n", total_count);
  DEBUGASSERT(total_count == 3);

  /* Queue2 should be empty */

  count2 = iob_get_queue_entry_count(&queue2);
  printf("After concat, queue2 entry count: %u\n", count2);
  DEBUGASSERT(count2 == 0);
  DEBUGASSERT(IOB_QEMPTY(&queue2));

  /* Clean up */

  iob_free_queue(&queue1);
  printf("Test 8 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: count_chain_iobs
 *
 * Description:
 *   Count the number of IOBs belonging to a single chain, walking up to and
 *   including the IOB that carries the IOB_CHAIN_END flag.  A correct chain
 *   is always terminated by IOB_CHAIN_END; if the flag has been lost the
 *   walk falls back to the raw end of the singly linked list.
 *
 ****************************************************************************/

static int count_chain_iobs(FAR struct iob_s *chain)
{
  FAR struct iob_s *iob = chain;
  int count = 0;

  while (iob != NULL)
    {
      count++;

      if (IOB_FLAG_TEST(iob, IOB_CHAIN_END))
        {
          break;
        }

      iob = IOB_LINK_GET(iob);
    }

  return count;
}

/****************************************************************************
 * Name: test_iob_queue_extend_tail
 *
 * Description:
 *   Regression test for 69c67a58493 ("mm/iob: fix stale qh_tail corrupting
 *   enqueued multi-IOB chains").
 *
 *   iob_add_queue()/iob_concat_queue() used to link a newly enqueued chain
 *   directly onto iobq->qh_tail.  But a chain may be extended (e.g. via
 *   iob_copyin(), as the UDP/TCP read-ahead buffering path does) AFTER it
 *   has been enqueued: iob_copyin() carries the IOB_CHAIN_END flag onto the
 *   freshly allocated last IOB while qh_tail still points at the original,
 *   now mid-chain, IOB.  Linking the next chain onto that stale tail
 *   overwrote the extended chain's internal io_flink, truncating it and
 *   planting a false chain boundary.
 *
 *   Reproduction sequence:
 *     1. Enqueue a small single-IOB chain (chain1).  qh_tail -> chain1.
 *     2. iob_copyin() enough data into chain1 to force allocation of a
 *        second IOB.  IOB_CHAIN_END moves to the new tail, but qh_tail is
 *        unchanged (still the original IOB).
 *     3. Enqueue a second chain (chain2).
 *
 *   With the bug, step 3 overwrites chain1's internal link: chain1 is
 *   truncated to a single IOB and the queue byte/entry accounting is
 *   corrupted.  After the fix the queue holds exactly two chains with all
 *   bytes and both of chain1's IOBs intact.
 *
 ****************************************************************************/

static int test_iob_queue_extend_tail(void)
{
  struct iob_queue_s queue;
  FAR struct iob_s *chain1;
  FAR struct iob_s *chain2;
  FAR struct iob_s *removed;
  unsigned int bufsize;
  unsigned int biglen;
  unsigned int chain1_pktlen;
  unsigned int chain2_pktlen;
  int ret;

  printf("\n=== Test 12: IOB Queue Extend Enqueued Tail Chain ===\n");

  IOB_QINIT(&queue);

  /* chain1: a single IOB carrying a few bytes. */

  chain1 = iob_alloc(false);
  if (chain1 == NULL)
    {
      printf("ERROR: Failed to allocate chain1\n");
      return -1;
    }

  bufsize = IOB_BUFSIZE(chain1);

  iob_copyin(chain1, (FAR const uint8_t *)"hello", 5, 0, false);

  /* Enqueue chain1 while it is still a single IOB.  qh_tail now points at
   * this IOB and it carries the IOB_CHAIN_END flag.
   */

  iob_add_queue(chain1, &queue);
  DEBUGASSERT(iob_get_queue_entry_count(&queue) == 1);

  /* Extend chain1 past a single IOB's capacity so iob_copyin() must
   * allocate a second IOB, moving IOB_CHAIN_END to it and leaving qh_tail
   * stale.  Append after the existing "hello" (offset 5).
   */

  biglen = bufsize + (bufsize / 2) + 1;
  memset(g_buffer1, 0xa5, biglen);

  ret = iob_copyin(chain1, g_buffer1, biglen, 5, false);
  if (ret != (int)biglen)
    {
      printf("ERROR: iob_copyin extend failed, ret=%d\n", ret);
      iob_free_queue(&queue);
      return -1;
    }

  chain1_pktlen = chain1->io_pktlen;
  DEBUGASSERT(chain1_pktlen == 5 + biglen);

  /* chain1 must now span more than one IOB. */

  if (count_chain_iobs(chain1) < 2)
    {
      printf("ERROR: chain1 did not extend to a second IOB\n");
      iob_free_queue(&queue);
      return -1;
    }

  /* chain2: another independent single-IOB chain. */

  chain2 = iob_alloc(false);
  if (chain2 == NULL)
    {
      printf("ERROR: Failed to allocate chain2\n");
      iob_free_queue(&queue);
      return -1;
    }

  iob_copyin(chain2, (FAR const uint8_t *)"world", 5, 0, false);
  chain2_pktlen = chain2->io_pktlen;

  /* The operation that used to corrupt the extended tail chain. */

  iob_add_queue(chain2, &queue);

  /* The queue must now report exactly two chains, and the total buffered
   * bytes must equal the sum of both chains' packet lengths: nothing was
   * lost to truncation.  Both helpers traverse the queue via IOB_CHAIN_END,
   * so a corrupted boundary shows up here.
   */

  if (iob_get_queue_entry_count(&queue) != 2)
    {
      printf("ERROR: expected 2 queue entries, got %u\n",
             iob_get_queue_entry_count(&queue));
      iob_free_queue(&queue);
      return -1;
    }

  if (iob_get_queue_size(&queue) != chain1_pktlen + chain2_pktlen)
    {
      printf("ERROR: queue size %u != expected %u (chain truncated)\n",
             iob_get_queue_size(&queue), chain1_pktlen + chain2_pktlen);
      iob_free_queue(&queue);
      return -1;
    }

  /* Dequeue the first chain and confirm it still contains all of its IOBs
   * and bytes (i.e. it was not truncated at the stale tail).
   */

  removed = iob_remove_queue(&queue);
  if (removed != chain1 || count_chain_iobs(removed) < 2 ||
      removed->io_pktlen != chain1_pktlen)
    {
      printf("ERROR: chain1 corrupted after dequeue\n");
      iob_free_chain(removed);
      iob_free_queue(&queue);
      return -1;
    }

  iob_free_chain(removed);

  removed = iob_remove_queue(&queue);
  if (removed != chain2 || removed->io_pktlen != chain2_pktlen)
    {
      printf("ERROR: chain2 corrupted after dequeue\n");
      iob_free_chain(removed);
      iob_free_queue(&queue);
      return -1;
    }

  iob_free_chain(removed);

  DEBUGASSERT(IOB_QEMPTY(&queue));
  printf("Test 12 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_tailroom
 *
 * Description:
 *   Test IOB tailroom calculation
 *
 ****************************************************************************/

static int test_iob_tailroom(void)
{
  FAR struct iob_s *iob;
  uint8_t data[50];
  unsigned int tailroom;
  int i;

  printf("\n=== Test 9: IOB Tailroom ===\n");

  /* Allocate IOB */

  iob = iob_alloc(false);
  if (iob == NULL)
    {
      printf("ERROR: Failed to allocate IOB\n");
      return -1;
    }

  /* Check initial tailroom */

  tailroom = iob_tailroom(iob);
  printf("Initial tailroom: %u bytes\n", tailroom);

  /* Add some data */

  for (i = 0; i < sizeof(data); i++)
    {
      data[i] = i & 0xff;
    }

  iob_copyin(iob, data, sizeof(data), 0, false);

  /* Check tailroom after adding data */

  tailroom = iob_tailroom(iob);
  printf("Tailroom after adding %zu bytes: %u bytes\n",
         sizeof(data), tailroom);

  /* Clean up */

  iob_free_chain(iob);
  printf("Test 9 PASSED\n");
  return 0;
}

/****************************************************************************
 * Name: test_iob_pointer_flags
 *
 * Description:
 *   Test pointer low-bit flags implementation (the key feature of commit
 *   9a4364d3ce5b2fdd8ead81ffd8a913ae4f6b88a6)
 *
 ****************************************************************************/

static int test_iob_pointer_flags(void)
{
  struct iob_queue_s queue;
  FAR struct iob_s *chain1;
  FAR struct iob_s *chain2;
  FAR struct iob_s *chain3;
  FAR struct iob_s *chain4;
  FAR struct iob_s *removed;

  printf("\n=== Test 10: IOB Pointer Low-Bit Flags (Enhanced) ===\n");
  printf("This test verifies the key feature of commit 9a4364d3ce5b2fd:\n");
  printf("Using pointer low-bit flags for queue management\n");
  printf("Testing with multi-IOB chains to fully validate the design\n\n");

  /* Initialize queue */

  IOB_QINIT(&queue);

  /* Create chains with different lengths:
   * Chain 1: 3 IOBs
   * Chain 2: 2 IOBs
   * Chain 3: 3 IOBs
   * Chain 4: 1 IOB
   * Total: 9 IOBs
   */

  printf("Creating IOB chains...\n");
  chain1 = create_iob_chain(3, 0xaa);
  if (chain1 == NULL)
    {
      printf("ERROR: Failed to create chain1\n");
      return -1;
    }

  chain2 = create_iob_chain(2, 0xbb);
  if (chain2 == NULL)
    {
      printf("ERROR: Failed to create chain2\n");
      iob_free_chain(chain1);
      return -1;
    }

  chain3 = create_iob_chain(3, 0xcc);
  if (chain3 == NULL)
    {
      printf("ERROR: Failed to create chain3\n");
      iob_free_chain(chain1);
      iob_free_chain(chain2);
      return -1;
    }

  chain4 = create_iob_chain(1, 0xdd);
  if (chain4 == NULL)
    {
      printf("ERROR: Failed to create chain4\n");
      iob_free_chain(chain1);
      iob_free_chain(chain2);
      iob_free_chain(chain3);
      return -1;
    }

  printf("✓ Created 4 chains: 3, 2, 3, 1 IOBs each (total 9 IOBs)\n\n");

  /* Print initial chains before adding to queue */

  print_iob_chain(chain1, "Chain 1 (3 IOBs) - before queue");
  print_iob_chain(chain2, "Chain 2 (2 IOBs) - before queue");
  print_iob_chain(chain3, "Chain 3 (3 IOBs) - before queue");
  print_iob_chain(chain4, "Chain 4 (1 IOB) - before queue");

  /* Add chains to queue */

  printf("\n--- Adding chains to queue ---\n");

  iob_add_queue(chain1, &queue);

  printf("Added chain1 (3 IOBs) to queue\n");
  print_iob_queue(&queue, "Queue after adding chain1");

  iob_add_queue(chain2, &queue);

  printf("Added chain2 (2 IOBs) to queue\n");
  print_iob_queue(&queue, "Queue after adding chain2");

  iob_add_queue(chain3, &queue);

  printf("Added chain3 (3 IOBs) to queue\n");
  print_iob_queue(&queue, "Queue after adding chain3");

  iob_add_queue(chain4, &queue);

  printf("Added chain4 (1 IOB) to queue\n");
  print_iob_queue(&queue, "Queue after adding all 4 chains");

  /* Verify queue statistics */

  printf("\n--- Verifying queue statistics ---\n");
  printf("Queue entry count: %u (expected 4)\n",
         iob_get_queue_entry_count(&queue));
  printf("Queue size: %u bytes\n", iob_get_queue_size(&queue));

  /* Remove chains one by one and verify */

  printf("\n--- Removing chains from queue ---\n");

  removed = iob_remove_queue(&queue);
  if (removed == NULL || removed != chain1)
    {
      printf("ERROR: Expected to remove chain1, got %p\n", removed);
      goto cleanup;
    }

  printf("✓ Removed chain1 (3 IOBs)\n");
  print_iob_queue(&queue, "Queue after removing chain1");
  iob_free_chain(removed);

  removed = iob_remove_queue(&queue);
  if (removed == NULL || removed != chain2)
    {
      printf("ERROR: Expected to remove chain2, got %p\n", removed);
      goto cleanup;
    }

  printf("✓ Removed chain2 (2 IOBs)\n");
  print_iob_queue(&queue, "Queue after removing chain2");
  iob_free_chain(removed);

  removed = iob_remove_queue(&queue);
  if (removed == NULL || removed != chain3)
    {
      printf("ERROR: Expected to remove chain3, got %p\n", removed);
      goto cleanup;
    }

  printf("✓ Removed chain3 (3 IOBs)\n");
  print_iob_queue(&queue, "Queue after removing chain3");
  iob_free_chain(removed);

  removed = iob_remove_queue(&queue);
  if (removed == NULL || removed != chain4)
    {
      printf("ERROR: Expected to remove chain4, got %p\n", removed);
      goto cleanup;
    }

  printf("✓ Removed chain4 (1 IOB)\n");
  print_iob_queue(&queue, "Queue after removing all chains");
  iob_free_chain(removed);

  /* Verify queue is empty */

  if (!IOB_QEMPTY(&queue))
    {
      printf("ERROR: Queue should be empty\n");
      return -1;
    }

  printf("\n✓ Queue is empty\n");

  printf("\nTest 10 PASSED\n");
  return 0;

cleanup:
  iob_free_queue(&queue);
  return -1;
}

/****************************************************************************
 * Name: test_iob_thread
 *
 * Description:
 *   Original IOB test with copy/trim/pack/reserve and threading tests
 *
 ****************************************************************************/

static int test_iob_thread(void)
{
  int            ret;
  cpu_set_t      cpuset;
  pthread_attr_t attr;
  pthread_t      thread[2];

  struct iob_s *iob;
  int nbytes;
  int i;
  int navail;
  int chain_size;
  int chain_total;

  printf("\n=== Original IOB Test (copy/trim/pack/reserve) ===\n");

  iob = iob_alloc(false);

  /* Other tasks may hold part of the pool, so size the chain from what is
   * actually free: a blocking copy that asks for more can never complete.
   * Keep four buffers as headroom for transient allocations.
   */

  navail = iob_navail(false);
  if (navail <= 4)
    {
      printf("Only %d IOBs available\n", navail);
      iob_free(iob);
      return -1;
    }

  chain_size  = CONFIG_IOB_BUFSIZE * (navail - 4);
  chain_total = MAGIC_NUMBER1 + chain_size;

  for (i = 0; i < chain_size; i++)
    {
      g_buffer1[i] = (uint8_t)(i & 0xff);
    }

  memset(g_buffer2, 0xff, chain_size);

  iob_copyin(iob, g_buffer2, MAGIC_NUMBER1, 0, false);
  printf("Copy IN: MAGIC_NUMBER1, offset 0\n");
  ASSERT(iob->io_pktlen == MAGIC_NUMBER1);
  ASSERT(iob->io_offset == 0);
  ASSERT(iob->io_len == MAGIC_NUMBER1);

  iob_copyin(iob, g_buffer1, chain_size, MAGIC_NUMBER1, false);
  printf("Copy IN: %d bytes, offset 0\n", chain_size);
  ASSERT(iob->io_pktlen == chain_total);
  ASSERT(iob->io_offset == 0);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  nbytes = iob_copyout(g_buffer2, iob, chain_size, MAGIC_NUMBER1);
  printf("Copy OUT: %d, offset 0\n", nbytes);
  ASSERT(nbytes == chain_size);
  ASSERT(iob->io_pktlen == chain_total);
  ASSERT(iob->io_offset == 0);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  if (memcmp(g_buffer1, g_buffer2, nbytes) != 0)
    {
      printf("Buffer1 does not match g_buffer2\n");
      ASSERT(false);
    }

  iob = iob_trimhead(iob, MAGIC_NUMBER1);
  printf("Trim: MAGIC_NUMBER1 from the beginning of the list\n");
  ASSERT(iob->io_pktlen == (chain_total - MAGIC_NUMBER1));
  ASSERT(iob->io_offset == MAGIC_NUMBER1);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  iob = iob_trimtail(iob, MAGIC_NUMBER2);
  printf("Trim: MAGIC_NUMBER2 from the end of the list\n");
  ASSERT(iob->io_pktlen == (chain_total - MAGIC_NUMBER1 -
         MAGIC_NUMBER2));
  ASSERT(iob->io_offset == MAGIC_NUMBER1);
  ASSERT(iob->io_len == (CONFIG_IOB_BUFSIZE -iob->io_offset));

  nbytes = iob_copyout(g_buffer2, iob, chain_size, 0);
  printf("Copy OUT: %d, offset MAGIC_NUMBER1\n", nbytes);
  ASSERT(nbytes == iob->io_pktlen);
  ASSERT(iob->io_pktlen == (chain_total - MAGIC_NUMBER1 -
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
  ASSERT(iob->io_pktlen == (chain_total - MAGIC_NUMBER1 -
         MAGIC_NUMBER2 - MAGIC_NUMBER2));

  nbytes = iob_copyout(g_buffer2, iob, chain_size, 0);
  printf("Copy OUT: %d, offset xxx\n", nbytes);
  ASSERT(nbytes == iob->io_pktlen);
  ASSERT(iob->io_pktlen == (chain_total - MAGIC_NUMBER1 -
         MAGIC_NUMBER2 - MAGIC_NUMBER2));

  if (memcmp(&g_buffer1[MAGIC_NUMBER2], g_buffer2, nbytes) != 0)
    {
      printf("Buffer1 does not match g_buffer2\n");
      ASSERT(false);
    }

  iob = iob_pack(iob);
  printf("Packed\n");
  ASSERT(iob->io_pktlen == (chain_total - MAGIC_NUMBER1 -
         MAGIC_NUMBER2 - MAGIC_NUMBER2));
  ASSERT(iob->io_offset == 0);

  nbytes = iob_copyout(g_buffer2, iob, chain_size, 0);
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

  /* test race condition between nothrottled and nothrottled */

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
  printf("nothrottled thread 0 statistic failed_count %d success_count %d\n",
          failed_count[0], success_count[0]);
  printf("nothrottled thread 1 statistic failed_count %d success_count %d\n",
          failed_count[1], success_count[1]);

  /* test race condition between throttled and nothrottled */

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
  printf("nothrottled thread 0 statistic failed_count %d success_count %d\n",
          failed_count[0], success_count[0]);
  printf("throttled thread 1 statistic failed_count %d success_count %d\n",
          failed_count[1], success_count[1]);

  /* test race condition between throttled and throttled */

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
  printf("throttled thread 0 statistic failed_count %d success_count %d\n",
          failed_count[0], success_count[0]);
  printf("throttled thread 1 statistic failed_count %d success_count %d\n",
          failed_count[1], success_count[1]);
  DEBUGASSERT(failed_count[0] == 0 && failed_count[1] == 0);

  printf("Original IOB Test PASSED\n");
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * iob_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret = 0;

  printf("==================================================\n");
  printf("IOB Comprehensive Test Suite\n");
  printf("==================================================\n");

  /* Run all API tests - stop on first failure */

  ret = test_iob_alloc_free();
  if (ret != 0)
    {
      printf("\n*** Test 1 (test_iob_alloc_free) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_copyin_copyout();
  if (ret != 0)
    {
      printf("\n*** Test 2 (test_iob_copyin_copyout) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_queue_operations();
  if (ret != 0)
    {
      printf("\n*** Test 3 (test_iob_queue_operations) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_chain_operations();
  if (ret != 0)
    {
      printf("\n*** Test 4 (test_iob_chain_operations) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_clone();
  if (ret != 0)
    {
      printf("\n*** Test 5 (test_iob_clone) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_pack_contig();
  if (ret != 0)
    {
      printf("\n*** Test 6 (test_iob_pack_contig) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_update_pktlen();
  if (ret != 0)
    {
      printf("\n*** Test 7 (test_iob_update_pktlen) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_queue_concat();
  if (ret != 0)
    {
      printf("\n*** Test 8 (test_iob_queue_concat) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_tailroom();
  if (ret != 0)
    {
      printf("\n*** Test 9 (test_iob_tailroom) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_pointer_flags();
  if (ret != 0)
    {
      printf("\n*** Test 10 (test_iob_pointer_flags) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_thread();
  if (ret != 0)
    {
      printf("\n*** Test 11 (test_iob_thread) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  ret = test_iob_queue_extend_tail();
  if (ret != 0)
    {
      printf("\n*** Test 12 (test_iob_queue_extend_tail) FAILED!"
             "Stopping execution. ***\n");
      goto test_failed;
    }

  /* All tests passed */

  printf("\n");
  printf("==================================================\n");
  printf("ALL IOB TESTS PASSED!\n");
  printf("==================================================\n");
  return 0;

test_failed:
  printf("==================================================\n");
  return ret;
}
