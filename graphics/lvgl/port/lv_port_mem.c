/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_mem.c
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

#include <nuttx/mm/mm.h>
#include "lv_port_mem.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

typedef CODE void *(*malloc_func_t)(FAR struct mm_heap_s *heap, size_t size);
typedef CODE void *(*realloc_func_t)(FAR struct mm_heap_s *heap,
                                     FAR void *oldmem, size_t size);
typedef CODE void *(*memalign_func_t)(FAR struct mm_heap_s *heap,
                                      size_t alignment, size_t size);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *malloc_first(FAR struct mm_heap_s *heap, size_t size);
static FAR void *realloc_first(FAR struct mm_heap_s *heap,
                               FAR void *oldmem, size_t size);
static FAR void *memalign_first(FAR struct mm_heap_s *heap,
                                size_t alignment, size_t size);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct mm_heap_s *g_lv_heap = NULL;
static malloc_func_t g_malloc_func = malloc_first;
static realloc_func_t g_realloc_func = realloc_first;
static memalign_func_t g_memalign_func = memalign_first;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_mem_init
 *
 * Description:
 *   Memory interface initialization.
 *
 ****************************************************************************/

static void lv_port_mem_init(void)
{
  static uint32_t heap_buf[CONFIG_LV_PORT_MEM_CUSTOM_SIZE
                           * 1024 / sizeof(uint32_t)];
  g_lv_heap = mm_initialize(CONFIG_LV_PORT_MEM_CUSTOM_NAME,
                            heap_buf, sizeof(heap_buf));
  LV_ASSERT_NULL(g_lv_heap);
  if (g_lv_heap == NULL)
    {
      LV_LOG_ERROR("NO memory for "
                   CONFIG_LV_PORT_MEM_CUSTOM_NAME
                   " heap");
    }
}

/****************************************************************************
 * Name: malloc_first
 ****************************************************************************/

static FAR void *malloc_first(FAR struct mm_heap_s *heap, size_t size)
{
  if (g_lv_heap == NULL)
    {
      lv_port_mem_init();
    }

  g_malloc_func = mm_malloc;
  return g_malloc_func(g_lv_heap, size);
}

/****************************************************************************
 * Name: realloc_first
 ****************************************************************************/

static FAR void *realloc_first(FAR struct mm_heap_s *heap,
                               FAR void *oldmem, size_t size)
{
  if (g_lv_heap == NULL)
    {
      lv_port_mem_init();
    }

  g_realloc_func = mm_realloc;
  return g_realloc_func(g_lv_heap, oldmem, size);
}

/****************************************************************************
 * Name: memalign_first
 ****************************************************************************/

static FAR void *memalign_first(FAR struct mm_heap_s *heap,
                                size_t alignment, size_t size)
{
  if (g_lv_heap == NULL)
    {
      lv_port_mem_init();
    }

  g_memalign_func = mm_memalign;
  return g_memalign_func(g_lv_heap, alignment, size);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_mem_alloc
 ****************************************************************************/

FAR void *lv_port_mem_alloc(size_t size)
{
  return g_malloc_func(g_lv_heap, size);
}

/****************************************************************************
 * Name: lv_port_mem_free
 ****************************************************************************/

void lv_port_mem_free(FAR void *mem)
{
  mm_free(g_lv_heap, mem);
}

/****************************************************************************
 * Name: lv_port_mem_realloc
 ****************************************************************************/

FAR void *lv_port_mem_realloc(FAR void *oldmem, size_t size)
{
  return g_realloc_func(g_lv_heap, oldmem, size);
}

/****************************************************************************
 * Name: lv_mem_custom_memalign
 ****************************************************************************/

FAR void *lv_mem_custom_memalign(size_t alignment, size_t size)
{
  return g_memalign_func(g_lv_heap, alignment, size);
}
