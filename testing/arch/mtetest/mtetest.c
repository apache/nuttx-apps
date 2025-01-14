/****************************************************************************
 * apps/testing/arch/mtetest/mtetest.c
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
#include <nuttx/compiler.h>

#include <sys/wait.h>

#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Must be a multiple of sixteen */
#define MTETEST_BUFFER_LEN 512

#define SCTLR_TCF1_BIT       (1ul << 40)

/****************************************************************************
 * Private Type
 ****************************************************************************/

struct mte_test_s
{
  FAR const char *name;
  FAR void (*func)(void);
};

struct args_s
{
  char   *buffer;
  size_t  safe_len;
  size_t  len;
  sem_t   sem;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void mtetest1(void);
static void mtetest2(void);
static void mtetest3(void);
static void mtetest4(void);
static void mtetest5(void);
static void switch_mtetest(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The instruction requires a 16-byte aligned memory block. */

static aligned_data(16) char g_buffer[MTETEST_BUFFER_LEN];

static const struct mte_test_s g_mtetest[] =
{
  { "mtetest1", mtetest1 },
  { "mtetest2", mtetest2 },
  { "mtetest3", mtetest3 },
  { "mtetest4", mtetest4 },
  { "mtetest5", mtetest5 },
  { "Thread switch MTE test", switch_mtetest },
  { NULL, NULL }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tagset
 *
 * Description:
 *   Applies tags to a memory block starting from the pointer `p` for a
 *   given size (`size`). The function iterates through the memory in
 *   16-byte chunks and uses the `stg` (store tag) instruction to assign
 *   a tag to each address.
 *
 *   - `p`: The starting pointer to the memory block.
 *   - `size`: The size of the memory block (in bytes) to tag.
 *
 *   The function uses inline assembly to store the tag at each memory
 *   location in the specified region, ensuring that the entire block is
 *   tagged.
 ****************************************************************************/

static void tagset(const void *p, size_t size)
{
  size_t i;
  for (i = 0; i < size; i += 16)
    {
      asm("stg %0, [%0]" : : "r"(p + i));
    }
}

/****************************************************************************
 * Name: tagcheck
 *
 * Description:
 *   Verifies the consistency of tags in memory block starting from ptr `p`
 *   for the given `size`. The function checks each 16-byte chunk using the
 *   `ldg` (load tag) instruction to load the tag and compares it with the
 *   original pointer `p` to ensure consistency.
 *
 *   - `p`: The starting pointer to the memory block.
 *   - `size`: The size of the memory block (in bytes) to check for
 *     tag consistency.
 *
 *   The function loads the tag for each chunk and ensures that the tag
 *   matches the expected value. If the tag does not match, the function
 *   triggers an assertion failure, providing a mechanism to validate
 *   correct memory tagging and access.
 ****************************************************************************/

static void tagcheck(const void *p, size_t size)
{
  size_t i;
  void *c;

  for (i = 0; i < size; i += 16)
    {
      asm("ldg %0, [%1]" : "=r"(c) : "r"(p + i), "0"(p));
      assert(c == p);
    }
}

/* Disable the mte function */

static void disable_mte(void)
{
  uint64_t val = read_sysreg(sctlr_el1);
  val &= ~SCTLR_TCF1_BIT;
  write_sysreg(val, sctlr_el1);
}

/****************************************************************************
 * Name: mtetest1
 *
 * Description:
 * 1. Initializes the pointer `p0` to point to `g_buffer`, which is assumed
 *    to contain enough data.
 * 2. Uses the assembly instruction `irg` to create a tagged pointer `p1`
 *    from `p0`. Asserts that `p1` is not equal to `p0`, confirming the
 *    tagging operation worked.
 * 3. Uses the assembly instruction `subp` to calculate the difference
 *    between `p0` and `p1`, storing it in `c`. Asserts that `c` is zero,
 *    confirming that `p1` and `p0` are the same address.
 * 4. Uses `stg` to store the tag from `p1` at the address of `p1`.
 * 5. Uses `ldg` to load the tag from `p0` into `p2`. Asserts that `p1`
 *    and `p2` are equal, confirming the tag stored at `p0` is correctly
 *    retrieved into `p2`.
 ****************************************************************************/

static void mtetest1(void)
{
  long c;
  int *p0;
  int *p1;
  int *p2;

  p0 = (int *)g_buffer;

  asm("irg %0,%1,%2" : "=r"(p1) : "r"(p0), "r"(1l));
  assert(p1 != p0);

  asm("subp %0,%1,%2" : "=r"(c) : "r"(p0), "r"(p1));
  assert(c == 0);

  asm("stg %0, [%0]" : : "r"(p1));
  asm("ldg %0, [%1]" : "=r"(p2) : "r"(p0), "0"(p0));
  assert(p1 == p2);
}

/****************************************************************************
 * Name: mtetest2
 *
 * Description:
 * 1. Initializes the pointer `p0` to point to `g_buffer`, which is assumed
 *    to contain sufficient data.
 * 2. Uses the assembly instruction `irg` to create a tagged pointer `p1`
 *    from `p0` using `excl`. The `gmi` instruction is used to modify `excl`,
 *    and asserts that `excl` is different from 1, confirming the change.
 * 3. Creates a second tagged pointer `p2` using the modified `excl` and
 *    asserts that `p1` and `p2` are different, validating that two distinct
 *    tagged pointers are created.
 ****************************************************************************/

static void mtetest2(void)
{
  long excl = 1;
  int *p0;
  int *p1;
  int *p2;

  p0 = (int *)g_buffer;

  /* Create two differently tagged pointers. */

  asm("irg %0,%1,%2" : "=r"(p1) : "r"(p0), "r"(excl));
  asm("gmi %0,%1,%0" : "+r"(excl) : "r" (p1));
  assert(excl != 1);

  asm("irg %0,%1,%2" : "=r"(p2) : "r"(p0), "r"(excl));
  assert(p1 != p2);
}

/****************************************************************************
 * Name: mtetest3
 *
 * Description:
 * 1. Initializes `p0` to point to `g_buffer`, which is assumed to contain
 *    enough data.
 * 2. Uses the assembly instruction `irg` to create a tagged pointer `p1`
 *    from `p0`. It then uses `gmi` to modify the `excl` value, ensuring it
 *    is different from 1 (validated by an `assert`).
 * 3. Uses `irg` again to create a tagged pointer `p2` from `p0`. Asserts
 *    that `p1` and `p2` are different, validating the creation of two
 *    distinct tagged pointers.
 * 4. Stores the tag from the first pointer (`p1`) using the assembly
 *    instruction `stg`.
 * 5. Stores the value at `p1` using the assembly instruction `str`, followed
 *    by a `yield` to allow other tasks to execute.
 ****************************************************************************/

static void mtetest3(void)
{
  long excl = 1;
  int *p0;
  int *p1;
  int *p2;

  p0 = (int *)g_buffer;

  /* Create two differently tagged pointers.  */

  asm("irg %0,%1,%2" : "=r"(p1) : "r"(p0), "r"(excl));
  asm("gmi %0,%1,%0" : "+r"(excl) : "r" (p1));
  assert(excl != 1);

  asm("irg %0,%1,%2" : "=r"(p2) : "r"(p0), "r"(excl));
  assert(p1 != p2);

  /* Store the tag from the first pointer.  */

  asm("stg %0, [%0]" : : "r"(p1));
  asm("str %0, [%0]; yield" : : "r"(p1));
}

/****************************************************************************
 * Name: mtetest4
 *
 * Description:
 * 1. Initializes the pointer `p0` to point to `g_buffer`, which is assumed
 *    to contain sufficient data.
 * 2. Uses the assembly instruction `irg` (likely a custom instruction) to
 *    process `p0` and `excl`, storing the result in `p1`.
 * 3. Calls the `tagset` function with `p1` and `MTETEST_BUFFER_LEN` to set
 *    tags for the buffer.
 * 4. Calls the `tagcheck` function with `p1` and `MTETEST_BUFFER_LEN` to
 *    verify the tags for the buffer.
 ****************************************************************************/

static void mtetest4(void)
{
  long excl = 1;
  int *p0;
  int *p1;

  p0 = (int *)g_buffer;

  /* Tag the pointer. */

  asm("irg %0,%1,%2" : "=r"(p1) : "r"(p0), "r"(excl));

  tagset(p1, MTETEST_BUFFER_LEN);
  tagcheck(p1, MTETEST_BUFFER_LEN);
}

/****************************************************************************
 * Name: mtetest5
 *
 * Description:
 * 1. Initializes the pointer `p0` to point to `g_buffer`, which is assumed
 *    to contain enough data.
 * 2. Uses the assembly instruction `irg` (possibly a custom instruction)
 *    to process `p0` and `excl`, storing the result in `p1`.
 * 3. Uses the assembly instruction `stg` to store data at the address
 *    `p0 + 16`.
 * 4. Uses standard C syntax to assign the value 1 to the address `p0 + 16`.
 * 5. Uses the assembly instruction `stg` to store data at the address
 *    `p1 + 16`.
 * 6. Uses `assert` to verify that the value at `p1 + 16` is 1, ensuring
 *    that the assignment was successful.
 ****************************************************************************/

static void mtetest5(void)
{
  long excl = 1;
  int *p0;
  int *p1;

  p0 = (int *)g_buffer;

  /* Tag the pointer. */

  asm("irg %0,%1,%2" : "=r"(p1) : "r"(p0), "r"(excl));

  /* Assign value 1 to the address p0 + 16 */

  asm("stg %0, [%0]" : : "r"(p0 + 16));
  *(p0 + 16) = 1;

  asm("stg %0, [%0]" : : "r"(p1 + 16));
  assert(1 == *(p1 + 16));
}

/* The first entry gets the semaphore for safe access,
 * and the next switch back to unsafe access
 */

static void *process1(void *arg)
{
  struct args_s *args = (struct args_s *)arg;
  int i;

  while (1)
    {
      sem_wait(&args->sem);
      printf("Process 1 holding lock\n");

      for (i = 0; i < args->safe_len; i++)
        {
          args->buffer[i]++;
        }

      sem_post(&args->sem);
      sleep(1);
      printf("Process 1 holding lock again\n");
      for (i = 0; i < args->len; i++)
        {
          args->buffer[i]++;
        }

      sem_post(&args->sem);
    }

  return NULL;
}

/* Disable unsafe access to MTE functions */

static void *process2(void *arg)
{
  struct args_s *args = (struct args_s *)arg;
  int i;

  while (1)
    {
      sem_wait(&args->sem);

      printf("Process 2 holding lock\n");
      disable_mte();

      for (i = 0; i < args->len; i++)
        {
          args->buffer[i]++;
        }

      sem_post(&args->sem);
      sleep(1);
    }

  return NULL;
}

static void switch_mtetest(void)
{
  struct args_s args;
  pthread_t t1;
  pthread_t t2;

  sem_init(&args.sem, 1, 1);

  asm("irg %0,%1,%2" : "=r"(args.buffer) : "r"(g_buffer), "r"(1l));
  assert(args.buffer != g_buffer);

  args.safe_len = sizeof(g_buffer) / 2;
  args.len = sizeof(g_buffer);

  tagset(args.buffer, args.safe_len);

  pthread_create(&t1, NULL, process1, &args);
  pthread_create(&t2, NULL, process2, &args);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  sem_destroy(&args.sem);
}

static void spawn_test_process(const struct mte_test_s *test)
{
  char *args[3];
  int status;
  pid_t pid;

  args[0] = "mtetest";
  args[1] = (char *)test->name;
  args[2] = NULL;

  if (posix_spawn(&pid, "mtetest", NULL, NULL, args, environ) != 0)
    {
      perror("posix_spawn");
      return;
    }

  waitpid(pid, &status, 0);
  printf("Test '%s' completed\n", test->name);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int i;

  if (argc < 2)
    {
      /* Loop through the tests and spawn a process for each one */

      for (i = 0; g_mtetest[i].name != NULL; i++)
        {
          printf("Spawning process for test: %s\n", g_mtetest[i].name);
          spawn_test_process(&g_mtetest[i]);
        }

      printf("All tests completed.\n");
    }
  else
    {
      for (i = 0; g_mtetest[i].name != NULL; i++)
        {
          if (strcmp(argv[1], g_mtetest[i].name) == 0)
            {
              printf("Running test: %s\n", g_mtetest[i].name);
              g_mtetest[i].func();
              break;
            }
        }
    }

  return 0;
}
