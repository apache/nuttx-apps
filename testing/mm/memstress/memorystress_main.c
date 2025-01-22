/****************************************************************************
 * apps/testing/mm/memstress/memorystress_main.c
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
#include <stdlib.h>
#include <debug.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MEMSTRESS_PREFIX "MemoryStress:"
#define DEBUG_MAGIC 0xaa

#define OPTARG_TO_VALUE(value, type) \
  do \
  { \
    FAR char *ptr; \
    value = (type)strtoul(optarg, &ptr, 10); \
    if (*ptr != '\0') \
      { \
        printf(MEMSTRESS_PREFIX "Parameter error -%c %s\n", ch, optarg); \
        show_usage(argv[0]); \
      } \
  } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum memorystress_rwerror_e
{
  MEMORY_STRESS_READ_ERROR,
  MEMORY_STRESS_WRITE_ERROR
};

struct memorystress_func_s
{
  void *(*malloc)(size_t size);
  void *(*aligned_alloc)(size_t align, size_t nbytes);
  void *(*realloc)(FAR void *ptr, size_t new_size);
  void (*freefunc)(FAR void *ptr);
};

struct memorystress_error_s
{
  FAR uint8_t *buf;
  uintptr_t node;
  size_t size;
  size_t offset;
  size_t cnt;
  size_t index;
  uint8_t readvalue;
  uint8_t writevalue;
  enum memorystress_rwerror_e rwerror;
};

struct memorystress_node_s
{
  FAR uint8_t *buf;
  size_t size;
};

struct memorystress_thread_context_s
{
  FAR struct memorystress_node_s *node_array;
  FAR struct memorystress_global_s *global;
  struct memorystress_error_s error;
};

struct memorystress_global_s
{
  FAR struct memorystress_func_s func;
  FAR pthread_t *threads;
  size_t max_allocsize;
  size_t nthreads;
  size_t nodelen;
  uint32_t sleep_us;
  bool debug;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("\nUsage: %s -m [max allocsize Default:8192] "
         "-n [node length Default:1024] -t [sleep us Default:100]"
         " -x [nthreads Default:1] -d [debuger mode]\n",
        progname);
  printf("\nWhere:\n");
  printf("  -m [max-allocsize] max alloc size.\n");
  printf("  -n [node length] Number of allocated memory blocks .\n");
  printf("  -t [sleep us] Length of time between each test.\n");
  printf("  -x [nthreads] Enable multi-thread stress testing. \n");
  printf("  -d [debug mode] Helps to localize the problem situation,"
         "there is a lot of information output in this mode.\n");
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Name: randnum
 ****************************************************************************/

static uint32_t randnum(uint32_t max, FAR uint32_t *seed)
{
  uint32_t x = *seed;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *seed = x;
  return x % max;
}

/****************************************************************************
 * Name: genvalue
 ****************************************************************************/

static uint32_t genvalue(FAR uint32_t *seed, bool debug)
{
  if (debug)
    {
      return DEBUG_MAGIC;
    }

  return randnum(UINT32_MAX, seed);
}

/****************************************************************************
 * Name: error_result
 ****************************************************************************/

static void error_result(struct memorystress_error_s error)
{
  syslog(LOG_ERR, MEMSTRESS_PREFIX "%s ERROR!, "
         "buf = %p, "
         "node = %p, "
         "size = %zu, "
         "offset = %zu(addr = %p), "
         "cnt = %zu, "
         "index = %zu, "
         "readValue = 0x%x, "
         "writeValue = 0x%x\n",
         error.rwerror == MEMORY_STRESS_READ_ERROR ? "READ" : "WRITE",
         error.buf,
         (uintptr_t *)error.node,
         error.size,
         error.offset, error.buf + error.offset,
         error.cnt,
         error.index,
         error.readvalue,
         error.writevalue);
}

/****************************************************************************
 * Name: checknode
 ****************************************************************************/

static void checknode(FAR struct memorystress_thread_context_s *context,
                      FAR struct memorystress_node_s *node,
                      enum memorystress_rwerror_e rwerror)
{
  size_t size = node->size;
  uint32_t seed = size;
  size_t i;

  /* check data */

  for (i = 0; i < size; i++)
    {
      uint8_t write_value = genvalue(&seed, context->global->debug);
      uint8_t read_value = node->buf[i];

      if (read_value != write_value)
        {
          context->error.buf = node->buf;
          context->error.size = node->size;
          context->error.offset = i;
          context->error.readvalue = read_value;
          context->error.writevalue = write_value;
          context->error.rwerror = rwerror;

          error_result(context->error);
          lib_dumpbuffer("debuger", node->buf, size);

          /* Trigger the ASSET once it occurs, retaining the error site */

          DEBUGASSERT(false);
        }
    }
}

/****************************************************************************
 * Name: memorystress_iter
 ****************************************************************************/

static bool memorystress_iter(FAR struct memorystress_thread_context_s
                              *context)
{
  FAR struct memorystress_node_s *node;
  FAR struct memorystress_func_s *func;
  uint32_t seed = rand() % UINT32_MAX;
  bool debug = context->global->debug;
  size_t index;

  index = randnum(context->global->nodelen, &seed);
  node = &(context->node_array[index]);
  func = &context->global->func;

  /* Record the current index and node address */

  context->error.index = index;
  context->error.node = (uintptr_t)node;

  /* check state */

  if (!node->buf)
    {
      /* Selection of test type and test size by random number */

      FAR uint8_t *ptr = NULL;
      size_t size = randnum(context->global->max_allocsize, &seed);
      int switch_func = randnum(3, &seed);
      int align = 1 << (randnum(4, &seed) + 2);

      /* There are currently three types of tests:
       *  0.standard malloc
       *  1.align_alloc
       *  2.realloc
       */

      switch (switch_func)
        {
          case 0:
            ptr = func->malloc(size);
            break;
          case 1:
            ptr = func->aligned_alloc(align, size);
            break;
          case 2:
            /* We have to allocate memory randomly once first,
             * otherwise realloc's behavior is equivalent to malloc
             */

            ptr = func->malloc(1024);
            if (ptr == NULL)
              {
                return true;
              }

            ptr = func->realloc(ptr, size);
            break;
          default:
            syslog(LOG_ERR, "Invalid switch_func number.\n");
            break;
        }

      /* Check the pointer to the test, if it is null there
       * may not be enough memory allocated.
       */

      if (ptr == NULL)
        {
          return true;
        }

      node->buf = ptr;
      node->size = size;

      /* fill random data */

      seed = size;
      while (size--)
        {
          *ptr++ = genvalue(&seed, debug);
        }

      /* Check write success */

      checknode(context, node, MEMORY_STRESS_WRITE_ERROR);
    }
  else
    {
      /* check read */

      checknode(context, node, MEMORY_STRESS_READ_ERROR);

      /* free node */

      func->freefunc(node->buf);
      node->buf = NULL;
    }

  context->error.cnt++;
  return true;
}

/****************************************************************************
 * Name: debug_malloc
 ****************************************************************************/

static FAR void *debug_malloc(size_t size)
{
  void *ptr = malloc(size);
  syslog(LOG_INFO, MEMSTRESS_PREFIX "malloc: %zu bytes, ptr = %p\n",
         size, ptr);
  return ptr;
}

/****************************************************************************
 * Name: debug_free
 ****************************************************************************/

static void debug_free(FAR void *ptr)
{
  syslog(LOG_INFO, MEMSTRESS_PREFIX "free: %p\n", ptr);
  free(ptr);
}

/****************************************************************************
 * Name: debug_aligned_alloc
 ****************************************************************************/

static FAR void *debug_aligned_alloc(size_t align, size_t nbytes)
{
  void *ptr = memalign(align, nbytes);
  syslog(LOG_INFO, MEMSTRESS_PREFIX "aligned_alloc: %zu bytes, align: %zu,"
         " ptr: %p\n", nbytes, align, ptr);
  return ptr;
}

/****************************************************************************
 * Name: debug_realloc
 ****************************************************************************/

static FAR void *debug_realloc(FAR void *ptr, size_t new_size)
{
  ptr = realloc(ptr, new_size);
  syslog(LOG_INFO, MEMSTRESS_PREFIX "realloc: %zu bytes, ptr: %p\n",
         new_size, ptr);
  return ptr;
}

/****************************************************************************
 * Name: global_init
 ****************************************************************************/

static void global_init(FAR struct memorystress_global_s *global, int argc,
                        FAR char *argv[])
{
  int ch;

  memset(global, 0, sizeof(struct memorystress_global_s));

  /* The default maximum memory held by a single thread is 8M */

  global->nthreads = 1;
  global->sleep_us = 100;
  global->max_allocsize = 8192;
  global->nodelen = 1024;

  while ((ch = getopt(argc, argv, "dm:n:t:x::")) != ERROR)
    {
      switch (ch)
        {
          case 'd':
            global->debug = true;
            break;
          case 'm':
            OPTARG_TO_VALUE(global->max_allocsize, size_t);
            break;
          case 'n':
            OPTARG_TO_VALUE(global->nodelen, int);
            break;
          case 't':
            OPTARG_TO_VALUE(global->sleep_us, uint32_t);
            break;
          case 'x':
            OPTARG_TO_VALUE(global->nthreads, int);
            break;
          default:
            show_usage(argv[0]);
            break;
        }
    }

    if (global->debug)
      {
        global->func.malloc = debug_malloc;
        global->func.aligned_alloc = debug_aligned_alloc;
        global->func.realloc = debug_realloc;
        global->func.freefunc = debug_free;
      }
    else
      {
        global->func.malloc = malloc;
        global->func.aligned_alloc = aligned_alloc;
        global->func.realloc = realloc;
        global->func.freefunc = free;
      }

    global->threads = (FAR pthread_t *)malloc(sizeof(pthread_t) *
                                              global->nthreads);
    if (global->threads == NULL)
      {
        syslog(LOG_ERR, MEMSTRESS_PREFIX "Malloc threads Failed\n");
        exit(EXIT_FAILURE);
      }

    syslog(LOG_INFO, MEMSTRESS_PREFIX "\n max_allocsize: %zu\n"
           " nodelen: %zu\n sleep_us: %" PRIu32 "\n nthreads: %zu\n "
           "debug: %s\n",
           global->max_allocsize, global->nodelen, global->sleep_us,
           global->nthreads, global->debug ? "true" : "false");

    srand(time(NULL));
}

/****************************************************************************
 * Name: thread_init
 ****************************************************************************/

static void thread_init(FAR struct memorystress_thread_context_s *context,
                        FAR struct memorystress_global_s *global)
{
  context->global = global;
  context->node_array = zalloc(sizeof(struct memorystress_node_s) *
                               global->nodelen);
  if (context->node_array == NULL)
    {
      syslog(LOG_ERR, MEMSTRESS_PREFIX "Malloc Node Array Failed\n");
      exit(EXIT_FAILURE);
    }
}

/****************************************************************************
 * Name: memorystress_thread
 ****************************************************************************/

FAR void *memorystress_thread(FAR void *arg)
{
  FAR struct memorystress_global_s *global;
  struct memorystress_thread_context_s context;

  global = (FAR struct memorystress_global_s *)arg;
  thread_init(&context, global);
  while (memorystress_iter(&context))
    {
      usleep(global->sleep_us);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct memorystress_global_s global;
  int i;

  global_init(&global, argc, argv);
  syslog(LOG_INFO, MEMSTRESS_PREFIX "testing...\n");
  for (i = 0; i < global.nthreads; i++)
    {
      if (pthread_create(&global.threads[i], NULL, memorystress_thread,
                         (FAR void *)&global) != 0)
        {
          syslog(LOG_ERR, "Failed to create thread\n");
          return 1;
        }
    }

  for (i = 0; i < global.nthreads; i++)
    {
      pthread_join(global.threads[i], NULL);
    }

  return 0;
}
