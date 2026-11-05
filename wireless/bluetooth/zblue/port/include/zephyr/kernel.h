/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/kernel.h
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

/* The subset of the Zephyr kernel API used by the Bluetooth host, built on
 * POSIX threads.  Declarations follow include/zephyr/kernel.h of the
 * downloaded Zephyr release; only what the host uses is provided.
 *
 * Scheduling model: Zephyr cooperative threads are never preempted by other
 * threads, and parts of the host depend on that.  All threads created here
 * with a cooperative priority, and the HCI RX thread, run while holding
 * one global lock (the "coop lock") and release it only while blocked.
 * k_sched_lock() takes the same lock, so a preemptive caller cannot run
 * concurrently with a cooperative thread inside a k_sched_lock() section.
 */

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_KERNEL_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_KERNEL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/sflist.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel/thread_stack.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Timeouts */

#define K_NO_WAIT      Z_TIMEOUT_NO_WAIT
#define K_NSEC(t)      Z_TIMEOUT_NS(t)
#define K_USEC(t)      Z_TIMEOUT_US(t)
#define K_TICKS(t)     Z_TIMEOUT_TICKS(t)
#define K_MSEC(ms)     Z_TIMEOUT_MS(ms)
#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)
#define K_MINUTES(m)   K_SECONDS((m) * 60)
#define K_HOURS(h)     K_MINUTES((h) * 60)
#define K_FOREVER      Z_FOREVER

/* Priorities */

#define K_PRIO_COOP(x)    (-(CONFIG_NUM_COOP_PRIORITIES - (x)))
#define K_PRIO_PREEMPT(x) (x)
#define K_HIGHEST_THREAD_PRIO (-CONFIG_NUM_COOP_PRIORITIES)
#define K_LOWEST_THREAD_PRIO  CONFIG_NUM_PREEMPT_PRIORITIES

/* Threads */

struct k_thread
{
  pthread_t pthread;
  int prio;
  const char *name;
  void (*entry)(void *, void *, void *);
  void *p1;
  void *p2;
  void *p3;
  size_t stack_size;
  int coop_depth;
  bool coop;
};

typedef struct k_thread *k_tid_t;

k_tid_t k_current_get(void);
void k_yield(void);
int32_t k_sleep(k_timeout_t timeout);
int32_t k_msleep(int32_t ms);
void k_busy_wait(uint32_t usec_to_wait);
int k_thread_name_set(k_tid_t thread, const char *str);
k_tid_t k_thread_create(struct k_thread *new_thread, k_thread_stack_t *stack,
                        size_t stack_size,
                        void (*entry)(void *, void *, void *),
                        void *p1, void *p2, void *p3,
                        int prio, uint32_t options, k_timeout_t delay);

static inline bool k_is_in_isr(void)
{
  return false;
}

static inline bool k_is_pre_kernel(void)
{
  return false;
}

void k_sched_lock(void);
void k_sched_unlock(void);

/* Fatal errors of the current thread: there is no thread to abort alone
 * in a process, so both end the program.
 */

#define k_oops()  abort()
#define k_panic() abort()

/* Time */

int64_t k_uptime_ticks(void);

static inline int64_t k_uptime_get(void)
{
  return k_ticks_to_ms_floor64(k_uptime_ticks());
}

static inline uint32_t k_uptime_get_32(void)
{
  return (uint32_t)k_uptime_get();
}

static inline int64_t k_uptime_delta(int64_t *reftime)
{
  int64_t now = k_uptime_get();
  int64_t delta = now - *reftime;

  *reftime = now;
  return delta;
}

uint32_t k_cycle_get_32(void);
uint64_t k_cycle_get_64(void);

/* Semaphores */

struct k_sem
{
  pthread_mutex_t lock;
  pthread_cond_t cond;
  unsigned int count;
  unsigned int limit;
};

#define K_SEM_MAX_LIMIT UINT_MAX

#define Z_SEM_INITIALIZER(obj, initial_count, count_limit) \
  { \
    .lock  = PTHREAD_MUTEX_INITIALIZER, \
    .cond  = PTHREAD_COND_INITIALIZER, \
    .count = (initial_count), \
    .limit = (count_limit), \
  }

#define K_SEM_DEFINE(name, initial_count, count_limit) \
  struct k_sem name = Z_SEM_INITIALIZER(name, initial_count, count_limit)

int k_sem_init(struct k_sem *sem, unsigned int initial_count,
               unsigned int limit);
int k_sem_take(struct k_sem *sem, k_timeout_t timeout);
void k_sem_give(struct k_sem *sem);
void k_sem_reset(struct k_sem *sem);
unsigned int k_sem_count_get(struct k_sem *sem);

/* Mutexes (recursive, as in Zephyr) */

struct k_mutex
{
  pthread_mutex_t lock;
  pthread_cond_t cond;
  struct k_thread *owner;
  uint32_t lock_count;
};

#define Z_MUTEX_INITIALIZER(obj) \
  { \
    .lock = PTHREAD_MUTEX_INITIALIZER, \
    .cond = PTHREAD_COND_INITIALIZER, \
  }

#define K_MUTEX_DEFINE(name) struct k_mutex name = Z_MUTEX_INITIALIZER(name)

int k_mutex_init(struct k_mutex *mutex);
int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout);
int k_mutex_unlock(struct k_mutex *mutex);

/* Condition variables */

struct k_condvar
{
  pthread_cond_t cond;
};

#define Z_CONDVAR_INITIALIZER(obj) { .cond = PTHREAD_COND_INITIALIZER }
#define K_CONDVAR_DEFINE(name) \
  struct k_condvar name = Z_CONDVAR_INITIALIZER(name)

int k_condvar_init(struct k_condvar *condvar);
int k_condvar_signal(struct k_condvar *condvar);
int k_condvar_broadcast(struct k_condvar *condvar);
int k_condvar_wait(struct k_condvar *condvar, struct k_mutex *mutex,
                   k_timeout_t timeout);

/* Queues, FIFOs and LIFOs.  Items reserve their first word for the link,
 * as in Zephyr.
 */

struct k_queue
{
  sys_sflist_t data_q;
  pthread_mutex_t lock;
  pthread_cond_t cond;
};

#define Z_QUEUE_INITIALIZER(obj) \
  { \
    .data_q = SYS_SFLIST_STATIC_INIT(&obj.data_q), \
    .lock   = PTHREAD_MUTEX_INITIALIZER, \
    .cond   = PTHREAD_COND_INITIALIZER, \
  }

void k_queue_init(struct k_queue *queue);
void k_queue_append(struct k_queue *queue, void *data);
void k_queue_prepend(struct k_queue *queue, void *data);
void *k_queue_get(struct k_queue *queue, k_timeout_t timeout);
bool k_queue_remove(struct k_queue *queue, void *data);
void *k_queue_peek_head(struct k_queue *queue);
void *k_queue_peek_tail(struct k_queue *queue);
int k_queue_is_empty(struct k_queue *queue);
void k_queue_cancel_wait(struct k_queue *queue);

struct k_fifo
{
  struct k_queue _queue;
};

#define Z_FIFO_INITIALIZER(obj) { ._queue = Z_QUEUE_INITIALIZER(obj._queue) }
#define K_FIFO_DEFINE(name) struct k_fifo name = Z_FIFO_INITIALIZER(name)

#define k_fifo_init(fifo)            k_queue_init(&(fifo)->_queue)
#define k_fifo_put(fifo, data)       k_queue_append(&(fifo)->_queue, data)
#define k_fifo_get(fifo, timeout)    k_queue_get(&(fifo)->_queue, timeout)
#define k_fifo_is_empty(fifo)        k_queue_is_empty(&(fifo)->_queue)
#define k_fifo_peek_head(fifo)       k_queue_peek_head(&(fifo)->_queue)
#define k_fifo_peek_tail(fifo)       k_queue_peek_tail(&(fifo)->_queue)
#define k_fifo_cancel_wait(fifo)     k_queue_cancel_wait(&(fifo)->_queue)

struct k_lifo
{
  struct k_queue _queue;
};

#define Z_LIFO_INITIALIZER(obj) { ._queue = Z_QUEUE_INITIALIZER(obj._queue) }
#define K_LIFO_DEFINE(name) struct k_lifo name = Z_LIFO_INITIALIZER(name)

#define k_lifo_init(lifo)            k_queue_init(&(lifo)->_queue)
#define k_lifo_put(lifo, data)       k_queue_prepend(&(lifo)->_queue, data)
#define k_lifo_get(lifo, timeout)    k_queue_get(&(lifo)->_queue, timeout)

/* Memory slabs */

struct k_mem_slab
{
  struct k_lifo free_q;
  char *buffer;
  size_t block_size;
  uint32_t num_blocks;
  uint32_t num_used;
  bool ready;
};

#define Z_MEM_SLAB_INITIALIZER(_slab, _buf, _size, _num) \
  { \
    .free_q     = Z_LIFO_INITIALIZER(_slab.free_q), \
    .buffer     = (char *)(_buf), \
    .block_size = (_size), \
    .num_blocks = (_num), \
  }

#define K_MEM_SLAB_DEFINE_STATIC(name, slab_block_size, slab_num_blocks, \
                                 slab_align) \
  static char __aligned(MAX(slab_align, sizeof(void *))) \
    _k_mem_slab_buf_##name[(slab_num_blocks) * \
                           ROUND_UP(slab_block_size, sizeof(void *))]; \
  static struct k_mem_slab name = Z_MEM_SLAB_INITIALIZER(name, \
    _k_mem_slab_buf_##name, ROUND_UP(slab_block_size, sizeof(void *)), \
    slab_num_blocks)

#define K_MEM_SLAB_DEFINE(name, slab_block_size, slab_num_blocks, slab_align) \
  static char __aligned(MAX(slab_align, sizeof(void *))) \
    _k_mem_slab_buf_##name[(slab_num_blocks) * \
                           ROUND_UP(slab_block_size, sizeof(void *))]; \
  struct k_mem_slab name = Z_MEM_SLAB_INITIALIZER(name, \
    _k_mem_slab_buf_##name, ROUND_UP(slab_block_size, sizeof(void *)), \
    slab_num_blocks)

#define K_MEM_SLAB_DEFINE_STATIC_TYPE(name, type, slab_num_blocks) \
  K_MEM_SLAB_DEFINE_STATIC(name, sizeof(type), slab_num_blocks, \
                           __alignof__(type))

#define K_MEM_SLAB_DEFINE_TYPE(name, type, slab_num_blocks) \
  K_MEM_SLAB_DEFINE(name, sizeof(type), slab_num_blocks, __alignof__(type))

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
                    size_t block_size, uint32_t num_blocks);
int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem,
                     k_timeout_t timeout);
void k_mem_slab_free(struct k_mem_slab *slab, void *mem);
uint32_t k_mem_slab_num_used_get(struct k_mem_slab *slab);
uint32_t k_mem_slab_num_free_get(struct k_mem_slab *slab);

/* Heaps: backed by the C library heap */

struct k_heap
{
  size_t size;
};

#define K_HEAP_DEFINE(name, bytes) struct k_heap name = { .size = (bytes) }
#define K_HEAP_DEFINE_NOCACHE(name, bytes) K_HEAP_DEFINE(name, bytes)

#ifndef K_HEAP_MEM_POOL_SIZE
#  define K_HEAP_MEM_POOL_SIZE 1
#endif

void *k_heap_alloc(struct k_heap *h, size_t bytes, k_timeout_t timeout);
void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
                           k_timeout_t timeout);
void k_heap_free(struct k_heap *h, void *mem);
void *k_malloc(size_t size);
void *k_calloc(size_t nmemb, size_t size);
void *k_aligned_alloc(size_t align, size_t size);
void k_free(void *ptr);

/* Work queues */

struct k_work;
struct k_work_q;

typedef void (*k_work_handler_t)(struct k_work *work);

enum
{
  K_WORK_RUNNING_BIT   = 0,
  K_WORK_CANCELING_BIT = 1,
  K_WORK_QUEUED_BIT    = 2,
  K_WORK_DELAYED_BIT   = 3,
  K_WORK_FLUSHING_BIT  = 4,

  K_WORK_RUNNING   = BIT(K_WORK_RUNNING_BIT),
  K_WORK_CANCELING = BIT(K_WORK_CANCELING_BIT),
  K_WORK_QUEUED    = BIT(K_WORK_QUEUED_BIT),
  K_WORK_DELAYED   = BIT(K_WORK_DELAYED_BIT),
  K_WORK_FLUSHING  = BIT(K_WORK_FLUSHING_BIT),
  K_WORK_MASK      = K_WORK_DELAYED | K_WORK_QUEUED | K_WORK_RUNNING |
                     K_WORK_CANCELING | K_WORK_FLUSHING,
};

struct k_work
{
  sys_snode_t node;
  k_work_handler_t handler;
  struct k_work_q *queue;
  uint32_t flags;
};

#define Z_WORK_INITIALIZER(work_handler) { .handler = (work_handler) }
#define K_WORK_DEFINE(work, work_handler) \
  struct k_work work = Z_WORK_INITIALIZER(work_handler)

struct k_work_delayable
{
  struct k_work work;
  sys_dnode_t node;
  int64_t expiry;
  struct k_work_q *queue;
};

#define Z_WORK_DELAYABLE_INITIALIZER(work_handler) \
  { .work = { .handler = (work_handler), .flags = 0 } }
#define K_WORK_DELAYABLE_DEFINE(work, work_handler) \
  struct k_work_delayable work = Z_WORK_DELAYABLE_INITIALIZER(work_handler)

struct k_work_sync
{
  int unused;
};

struct k_work_queue_config
{
  const char *name;
  bool no_yield;
  bool essential;
  uint32_t work_timeout_ms;
};

struct k_work_q
{
  struct k_thread thread;
  k_tid_t thread_id;
  sys_slist_t pending;
  sys_dlist_t delayed;
  pthread_cond_t cond;
  uint32_t flags;
};

extern struct k_work_q k_sys_work_q;

void k_work_init(struct k_work *work, k_work_handler_t handler);
int k_work_busy_get(const struct k_work *work);

static inline bool k_work_is_pending(const struct k_work *work)
{
  return k_work_busy_get(work) != 0;
}

int k_work_submit_to_queue(struct k_work_q *queue, struct k_work *work);
int k_work_submit(struct k_work *work);
bool k_work_flush(struct k_work *work, struct k_work_sync *sync);
int k_work_cancel(struct k_work *work);
bool k_work_cancel_sync(struct k_work *work, struct k_work_sync *sync);

void k_work_queue_init(struct k_work_q *queue);
void k_work_queue_start(struct k_work_q *queue, k_thread_stack_t *stack,
                        size_t stack_size, int prio,
                        const struct k_work_queue_config *cfg);

static inline k_tid_t k_work_queue_thread_get(struct k_work_q *queue)
{
  return queue->thread_id;
}

static inline struct k_work_delayable *
k_work_delayable_from_work(struct k_work *work)
{
  return CONTAINER_OF(work, struct k_work_delayable, work);
}

void k_work_init_delayable(struct k_work_delayable *dwork,
                           k_work_handler_t handler);
int k_work_delayable_busy_get(const struct k_work_delayable *dwork);

static inline bool
k_work_delayable_is_pending(const struct k_work_delayable *dwork)
{
  return k_work_delayable_busy_get(dwork) != 0;
}

k_ticks_t k_work_delayable_remaining_get(
  const struct k_work_delayable *dwork);

int k_work_schedule_for_queue(struct k_work_q *queue,
                              struct k_work_delayable *dwork,
                              k_timeout_t delay);
int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay);
int k_work_reschedule_for_queue(struct k_work_q *queue,
                                struct k_work_delayable *dwork,
                                k_timeout_t delay);
int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay);
bool k_work_flush_delayable(struct k_work_delayable *dwork,
                            struct k_work_sync *sync);
int k_work_cancel_delayable(struct k_work_delayable *dwork);
bool k_work_cancel_delayable_sync(struct k_work_delayable *dwork,
                                  struct k_work_sync *sync);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_KERNEL_H */
