/****************************************************************************
 * apps/benchmarks/rt-tests/numa.h
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

#ifndef __NUMA_H__
#define __NUMA_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <strings.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CACHE_LINESIZE                (64)
#define numa_run_on_node(...)         (-1)
#define numa_node_of_cpu(...)         (-1)
#define numa_alloc_onnode(size, node) memalign(CACHE_LINESIZE, size)
#define numa_free(ptr, size)          free(ptr)
#define numa_available()              (-1)

struct bitmask
{
  unsigned long  size; /* number of bits in the map */
  unsigned long *maskp;
};

static inline
struct bitmask *numa_allocate_cpumask(void)
{
    return NULL;
}

static inline void numa_bitmask_free(struct bitmask *mask)
{
    return;
}

static inline int numa_sched_getaffinity(pid_t pid, struct bitmask *mask)
{
    return -1;
}

static inline
unsigned int numa_bitmask_isbitset(const struct bitmask *mask,
                                   unsigned int          idx)
{
  return mask->maskp[idx / sizeof(unsigned long)] &
         ((unsigned long)1 << (idx % sizeof(unsigned long)));
}

static inline
struct bitmask *numa_bitmask_clearbit(struct bitmask *mask,
                                      unsigned int idx)
{
    mask->maskp[idx / sizeof(unsigned long)] &=
        ~((unsigned long)1 << (idx % sizeof(unsigned long)));
    return mask;
}

static inline
struct bitmask *numa_parse_cpustring_all(const char *s)
{
  return NULL;
}

static inline
unsigned int numa_bitmask_weight(const struct bitmask *bitmap)
{
  unsigned int idx;
  unsigned int ret = 0;
  for (idx = 0; idx < bitmap->size / sizeof(unsigned long); idx++)
    {
      ret += popcountl(bitmap->maskp[idx]);
    }
  return ret;
}

#define numa_sched_setaffinity(...) (-1)

#endif
