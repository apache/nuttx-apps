/****************************************************************************
 * apps/include/system/zbus.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * Copyright (c) 2026 NuttX port
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.  You may obtain
 * a copy of the License at
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

/* NuttX port of the Zephyr zbus message bus.
 *
 * Differences from the Zephyr original:
 *  - Timeouts are given in milliseconds (int32_t): ZBUS_NO_WAIT (0) and
 *    ZBUS_FOREVER (-1) replace K_NO_WAIT/K_FOREVER.
 *  - Subscribers and message subscribers use POSIX message queues opened
 *    lazily on first zbus API call (no k_msgq/k_fifo/net_buf).
 *  - Priority boost (HLP) is not implemented; enable NuttX native
 *    CONFIG_PRIORITY_INHERITANCE for equivalent protection.
 *  - Publishing from interrupt context is not supported.
 *  - Requires the board linker script to include the iterable section
 *    fragments <nuttx/linker/common-rom.ld> and common-ram.ld.
 */

#ifndef __APPS_INCLUDE_SYSTEM_ZBUS_H
#define __APPS_INCLUDE_SYSTEM_ZBUS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/fs/fs.h>
#include <nuttx/iterable_sections.h>

#include <assert.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
#  include <nuttx/list.h>
#endif

#if defined(CONFIG_ZBUS_ASYNC_LISTENER) || defined(CONFIG_ZBUS_ISR_PUBLISHER)
#  include <nuttx/wqueue.h>
#endif

#include <system/zbus_macros.h>

#ifdef __cplusplus
#define _ZBUS_CPP_EXTERN extern
extern "C"
{
#else
#define _ZBUS_CPP_EXTERN
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Timeout special values (milliseconds) */

#define ZBUS_NO_WAIT  0
#define ZBUS_FOREVER  (-1)

/* Channel without a unique numeric identifier */

#define ZBUS_CHAN_ID_INVALID UINT32_MAX

#ifdef CONFIG_ZBUS_ASSERT_MOCK
#  define _ZBUS_ASSERT(cond, msg) \
  do                              \
    {                             \
      if (!(cond))                \
        {                         \
          return -EFAULT;         \
        }                         \
    }                             \
  while (0)
#else
#  define _ZBUS_ASSERT(cond, msg) DEBUGASSERT(cond)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct zbus_channel;

/* Mutable data associated with every channel */

struct zbus_channel_data
{
  /* Boundaries of this channel's static observations inside the sorted
   * zbus_channel_observation iterable section (computed on first use).
   */

  int16_t observers_start_idx;
  int16_t observers_end_idx;

  /* Channel access semaphore */

  sem_t sem;

#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
  /* Runtime (dynamically added) observers */

  struct list_node observers;
#endif

#ifdef CONFIG_ZBUS_CHANNEL_PUBLISH_STATS
  struct timespec publish_timestamp;
  uint32_t publish_count;
#endif
};

/* A channel: constant descriptor placed in ROM (iterable section) */

struct zbus_channel
{
#ifdef CONFIG_ZBUS_CHANNEL_NAME
  const char *name;
#endif
#ifdef CONFIG_ZBUS_CHANNEL_ID
  uint32_t id;
#endif

  /* Shared message memory, its size, and optional user data/validator */

  void *message;
  size_t message_size;
  void *user_data;
  bool (*validator)(const void *msg, size_t msg_size);

  struct zbus_channel_data *data;
};

/* Observer types */

enum zbus_observer_type
{
  ZBUS_OBSERVER_LISTENER_TYPE = 0,
  ZBUS_OBSERVER_SUBSCRIBER_TYPE,
  ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,
  ZBUS_OBSERVER_ASYNC_LISTENER_TYPE,
};

/* Mutable data associated with every observer */

struct zbus_observer_data
{
  bool enabled;

  /* Notification queue (subscriber/msg subscriber/async listener), opened
   * lazily with file_mq_open() so it is usable from any task, unlike
   * per-task mqd_t descriptors.  mq.f_inode == NULL means "not opened".
   */

  struct file mq;

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
  /* Work item used to run the async listener callback on the low
   * priority work queue.
   */

  struct work_s work;
#endif
};

/* An observer: constant descriptor placed in ROM (iterable section) */

struct zbus_observer
{
#ifdef CONFIG_ZBUS_OBSERVER_NAME
  const char *name;
#endif

  enum zbus_observer_type type;

  /* Notification queue depth (subscriber types only) */

  uint16_t queue_size;

  struct zbus_observer_data *data;

  /* Listener callback (listener type only) */

  void (*callback)(const struct zbus_channel *chan);

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
  /* Async listener callback (async listener type only).  Executed on the
   * low priority work queue with a copy of the published message.
   */

  void (*async_callback)(const struct zbus_channel *chan, const void *msg);
#endif
};

/* Link between one channel and one observer (ROM iterable section, sorted
 * by name so that entries are grouped by channel and ordered by observer
 * priority).  The mutable notification mask lives in .bss and is pointed
 * to from here; its initial value is preserved in ROM (mask_init) and
 * applied by the one-time lazy initialization.
 */

struct zbus_channel_observation
{
  const struct zbus_channel *chan;
  const struct zbus_observer *obs;
  bool *mask;
  bool mask_init;
};

#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
/* Node linking a runtime observer to a channel */

struct zbus_observer_node
{
  struct list_node node;
  const struct zbus_observer *obs;
};
#endif

/****************************************************************************
 * Definition macros
 ****************************************************************************/

#ifdef CONFIG_ZBUS_CHANNEL_NAME
#  define ZBUS_CHANNEL_NAME_INIT(_name) .name = #_name,
#else
#  define ZBUS_CHANNEL_NAME_INIT(_name)
#endif

#ifdef CONFIG_ZBUS_CHANNEL_ID
#  define _ZBUS_CHANNEL_ID_INIT(_id) .id = _id,
#else
#  define _ZBUS_CHANNEL_ID_INIT(_id)
#endif

#ifdef CONFIG_ZBUS_OBSERVER_NAME
#  define ZBUS_OBSERVER_NAME_INIT(_name) .name = #_name,
#else
#  define ZBUS_OBSERVER_NAME_INIT(_name)
#endif

#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
#  define _ZBUS_RUNTIME_OBS_INIT(_name) \
  .observers = LIST_INITIAL_VALUE(_zbus_chan_data_##_name.observers),
#else
#  define _ZBUS_RUNTIME_OBS_INIT(_name)
#endif

#define _ZBUS_MESSAGE_NAME(_name) _zbus_message_##_name

/* Declare channels/observers defined in other files */

#define _ZBUS_OBS_EXTERN(_name)  extern const struct zbus_observer _name;
#define _ZBUS_CHAN_EXTERN(_name) extern const struct zbus_channel _name;

#define ZBUS_OBS_DECLARE(...)  ZBUS_FOR_EACH(_ZBUS_OBS_EXTERN, __VA_ARGS__)
#define ZBUS_CHAN_DECLARE(...) ZBUS_FOR_EACH(_ZBUS_CHAN_EXTERN, __VA_ARGS__)

/* Observer list helpers for ZBUS_CHAN_DEFINE */

#define ZBUS_OBSERVERS_EMPTY
#define ZBUS_OBSERVERS(...) __VA_ARGS__

/* Message initializer: ZBUS_MSG_INIT(.a = 1, .b = 2) -> {.a = 1, .b = 2} */

#define ZBUS_MSG_INIT(_val, ...) {_val, ##__VA_ARGS__}

/* One channel<->observer observation + its mask.  The variable name embeds
 * the channel name and the two-digit list position so that the linker's
 * SORT_BY_NAME() groups observations per channel, ordered by priority.
 */

#define _ZBUS_CHAN_OBSERVATION(_idx2, _obs, _chan)                          \
  static bool _chan##_##_idx2##_mask;                                       \
  const STRUCT_SECTION_ITERABLE(zbus_channel_observation,                   \
                                _chan##_##_idx2) =                          \
  {                                                                         \
    .chan = &_chan,                                                         \
    .obs = &_obs,                                                           \
    .mask = &_chan##_##_idx2##_mask,                                        \
    .mask_init = false,                                                     \
  };

#define _ZBUS_CHAN_DEFINE(_name, _id, _type, _validator, _user_data)        \
  static struct zbus_channel_data _zbus_chan_data_##_name =                 \
  {                                                                         \
    .observers_start_idx = -1,                                              \
    .observers_end_idx = -1,                                                \
    .sem = SEM_INITIALIZER(1),                                              \
    _ZBUS_RUNTIME_OBS_INIT(_name)                                           \
  };                                                                        \
  _ZBUS_CPP_EXTERN const STRUCT_SECTION_ITERABLE(zbus_channel, _name) =     \
  {                                                                         \
    ZBUS_CHANNEL_NAME_INIT(_name)                                           \
    _ZBUS_CHANNEL_ID_INIT(_id)                                              \
    .message = &_ZBUS_MESSAGE_NAME(_name),                                  \
    .message_size = sizeof(_type),                                          \
    .user_data = _user_data,                                                \
    .validator = _validator,                                                \
    .data = &_zbus_chan_data_##_name,                                       \
  }

/* Define a channel.
 *
 * _name      channel name (C identifier)
 * _type      message type (struct or union)
 * _validator optional validator function or NULL
 * _user_data optional user data pointer or NULL
 * _observers ZBUS_OBSERVERS(obs1, obs2, ...) or ZBUS_OBSERVERS_EMPTY;
 *            list order defines notification priority
 * _init_val  message initial value, e.g. ZBUS_MSG_INIT(0)
 */

#define ZBUS_CHAN_DEFINE(_name, _type, _validator, _user_data, _observers, \
                         _init_val)                                        \
  static _type _ZBUS_MESSAGE_NAME(_name) = _init_val;                      \
  _ZBUS_CHAN_DEFINE(_name, ZBUS_CHAN_ID_INVALID, _type, _validator,        \
                    _user_data);                                           \
  ZBUS_OBS_DECLARE(_observers)                                             \
  ZBUS_OBS_FOR_EACH(_ZBUS_CHAN_OBSERVATION, _name, _observers)

/* Same as ZBUS_CHAN_DEFINE with a unique numeric channel identifier */

#define ZBUS_CHAN_DEFINE_WITH_ID(_name, _id, _type, _validator, _user_data, \
                                 _observers, _init_val)                     \
  static _type _ZBUS_MESSAGE_NAME(_name) = _init_val;                       \
  _ZBUS_CHAN_DEFINE(_name, _id, _type, _validator, _user_data);             \
  ZBUS_OBS_DECLARE(_observers)                                              \
  ZBUS_OBS_FOR_EACH(_ZBUS_CHAN_OBSERVATION, _name, _observers)

/* Add a static observation to a channel defined elsewhere.  _prio defines
 * the notification order relative to other ADD_OBS observations of the
 * same channel (use two-digit literals, e.g. 01, 02, ... so the linker
 * name sort orders them correctly).  ADD_OBS observations are notified
 * after the ones listed in ZBUS_CHAN_DEFINE.
 */

#define ZBUS_CHAN_ADD_OBS_WITH_MASK(_chan, _obs, _masked, _prio)            \
  ZBUS_CHAN_DECLARE(_chan)                                                  \
  ZBUS_OBS_DECLARE(_obs)                                                    \
  static bool _chan##_zz##_prio##_obs##_mask;                               \
  const STRUCT_SECTION_ITERABLE(zbus_channel_observation,                   \
                                _chan##_zz##_prio##_obs) =                  \
  {                                                                         \
    .chan = &_chan,                                                         \
    .obs = &_obs,                                                           \
    .mask = &_chan##_zz##_prio##_obs##_mask,                                \
    .mask_init = _masked,                                                   \
  }

#define ZBUS_CHAN_ADD_OBS(_chan, _obs, _prio) \
  ZBUS_CHAN_ADD_OBS_WITH_MASK(_chan, _obs, false, _prio)

/* Define a listener observer (synchronous callback) */

#define ZBUS_LISTENER_DEFINE_WITH_ENABLE(_name, _cb, _enable)               \
  static struct zbus_observer_data _zbus_obs_data_##_name =                 \
  {                                                                         \
    .enabled = _enable,                                                     \
  };                                                                        \
  _ZBUS_CPP_EXTERN const STRUCT_SECTION_ITERABLE(zbus_observer, _name) =    \
  {                                                                         \
    ZBUS_OBSERVER_NAME_INIT(_name)                                          \
    .type = ZBUS_OBSERVER_LISTENER_TYPE,                                    \
    .queue_size = 0,                                                        \
    .data = &_zbus_obs_data_##_name,                                        \
    .callback = (_cb),                                                      \
  }

#define ZBUS_LISTENER_DEFINE(_name, _cb) \
  ZBUS_LISTENER_DEFINE_WITH_ENABLE(_name, _cb, true)

/* Define a subscriber observer (receives channel references through a
 * message queue of depth _queue_size; use zbus_sub_wait() to wait).
 */

#define ZBUS_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, _queue_size, _enable)     \
  static struct zbus_observer_data _zbus_obs_data_##_name =                 \
  {                                                                         \
    .enabled = _enable,                                                     \
  };                                                                        \
  _ZBUS_CPP_EXTERN const STRUCT_SECTION_ITERABLE(zbus_observer, _name) =    \
  {                                                                         \
    ZBUS_OBSERVER_NAME_INIT(_name)                                          \
    .type = ZBUS_OBSERVER_SUBSCRIBER_TYPE,                                  \
    .queue_size = _queue_size,                                              \
    .data = &_zbus_obs_data_##_name,                                        \
    .callback = NULL,                                                       \
  }

#define ZBUS_SUBSCRIBER_DEFINE(_name, _queue_size) \
  ZBUS_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, _queue_size, true)

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER

/* Define a message subscriber observer (receives copies of the published
 * messages through a message queue; use zbus_sub_wait_msg() to wait).
 * Messages larger than CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE cannot be
 * delivered to message subscribers.
 */

#define ZBUS_MSG_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, _enable)              \
  static struct zbus_observer_data _zbus_obs_data_##_name =                 \
  {                                                                         \
    .enabled = _enable,                                                     \
  };                                                                        \
  _ZBUS_CPP_EXTERN const STRUCT_SECTION_ITERABLE(zbus_observer, _name) =    \
  {                                                                         \
    ZBUS_OBSERVER_NAME_INIT(_name)                                          \
    .type = ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,                              \
    .queue_size = CONFIG_ZBUS_MSG_SUBSCRIBER_QUEUE_SIZE,                    \
    .data = &_zbus_obs_data_##_name,                                        \
    .callback = NULL,                                                       \
  }

#define ZBUS_MSG_SUBSCRIBER_DEFINE(_name) \
  ZBUS_MSG_SUBSCRIBER_DEFINE_WITH_ENABLE(_name, true)

#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

#ifdef CONFIG_ZBUS_ASYNC_LISTENER

/* Define an async listener observer.  The callback executes on the low
 * priority work queue (not in the publisher context) and receives a copy
 * of the published message.  Messages larger than
 * CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE cannot be delivered.
 */

#define ZBUS_ASYNC_LISTENER_DEFINE_WITH_ENABLE(_name, _cb, _enable)         \
  static struct zbus_observer_data _zbus_obs_data_##_name =                 \
  {                                                                         \
    .enabled = _enable,                                                     \
  };                                                                        \
  _ZBUS_CPP_EXTERN const STRUCT_SECTION_ITERABLE(zbus_observer, _name) =    \
  {                                                                         \
    ZBUS_OBSERVER_NAME_INIT(_name)                                          \
    .type = ZBUS_OBSERVER_ASYNC_LISTENER_TYPE,                              \
    .queue_size = CONFIG_ZBUS_MSG_SUBSCRIBER_QUEUE_SIZE,                    \
    .data = &_zbus_obs_data_##_name,                                        \
    .callback = NULL,                                                       \
    .async_callback = (_cb),                                                \
  }

#define ZBUS_ASYNC_LISTENER_DEFINE(_name, _cb) \
  ZBUS_ASYNC_LISTENER_DEFINE_WITH_ENABLE(_name, _cb, true)

#endif /* CONFIG_ZBUS_ASYNC_LISTENER */

#ifdef CONFIG_ZBUS_ISR_PUBLISHER

/* Deferred publisher usable from interrupt handlers.  zbus_isr_pub()
 * snapshots the message into a staging buffer and schedules the actual
 * zbus_chan_pub() on a work queue, since the zbus API itself is not
 * ISR-safe.  Semantics are last-value-wins: if the interrupt fires again
 * before the worker runs, the staged message is overwritten and a single
 * publication may deliver only the newest sample.  Use one publisher per
 * interrupt source.
 */

struct zbus_isr_publisher
{
  struct work_s work;
  const struct zbus_channel *chan;

  /* Written by zbus_isr_pub(); snapshotted by the worker */

  void *staging;

  /* Worker-side copy, so a concurrent interrupt cannot tear the
   * message while it is being published.
   */

  void *shadow;
};

/* Define a deferred ISR publisher bound to a channel.
 *
 * _name  publisher variable name
 * _chan  channel to publish to
 * _type  the channel message type (sizes the internal buffers)
 */

#define ZBUS_ISR_PUBLISHER_DEFINE(_name, _chan, _type)                      \
  ZBUS_CHAN_DECLARE(_chan)                                                  \
  static _type _zbus_isr_staging_##_name;                                   \
  static _type _zbus_isr_shadow_##_name;                                    \
  static struct zbus_isr_publisher _name =                                  \
  {                                                                         \
    .chan = &_chan,                                                         \
    .staging = &_zbus_isr_staging_##_name,                                  \
    .shadow = &_zbus_isr_shadow_##_name,                                    \
  }

/* Publish from interrupt context (also callable from threads): copy *msg
 * to the staging buffer and schedule the deferred publication.  Returns 0
 * on success (including when a publication was already pending: the
 * pending worker will deliver the new data) or a negative errno.
 */

int zbus_isr_pub(struct zbus_isr_publisher *pub, const void *msg);

#endif /* CONFIG_ZBUS_ISR_PUBLISHER */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Publish a message to a channel.  Copies *msg into the channel and runs
 * the dispatcher, notifying every observer.  Returns 0 or -errno
 * (-ENOMSG: validator rejected; -EBUSY/-EAGAIN: could not lock in time).
 */

int zbus_chan_pub(const struct zbus_channel *chan, const void *msg,
                  int32_t timeout_ms);

/* Read a channel message (copies the channel message into *msg) */

int zbus_chan_read(const struct zbus_channel *chan, void *msg,
                   int32_t timeout_ms);

/* Force the notification of a channel's observers without publishing */

int zbus_chan_notify(const struct zbus_channel *chan, int32_t timeout_ms);

/* Claim/finish a channel for direct access to zbus_chan_msg() */

int zbus_chan_claim(const struct zbus_channel *chan, int32_t timeout_ms);
int zbus_chan_finish(const struct zbus_channel *chan);

/* Wait for a notification (subscriber observers) */

int zbus_sub_wait(const struct zbus_observer *sub,
                  const struct zbus_channel **chan, int32_t timeout_ms);

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
/* Wait for a message copy (message subscriber observers) */

int zbus_sub_wait_msg(const struct zbus_observer *sub,
                      const struct zbus_channel **chan, void *msg,
                      int32_t timeout_ms);
#endif

/* Enable/disable an observer */

int zbus_obs_set_enable(const struct zbus_observer *obs, bool enabled);

/* Mask/unmask the notifications from one channel to one observer */

int zbus_obs_set_chan_notification_mask(const struct zbus_observer *obs,
                                        const struct zbus_channel *chan,
                                        bool masked);
int zbus_obs_is_chan_notification_masked(const struct zbus_observer *obs,
                                         const struct zbus_channel *chan,
                                         bool *masked);

#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
/* Add/remove observers at runtime */

int zbus_chan_add_obs(const struct zbus_channel *chan,
                      const struct zbus_observer *obs, int32_t timeout_ms);
int zbus_chan_rm_obs(const struct zbus_channel *chan,
                     const struct zbus_observer *obs, int32_t timeout_ms);
#endif

#ifdef CONFIG_ZBUS_CHANNEL_ID
const struct zbus_channel *zbus_chan_from_id(uint32_t channel_id);
#endif

#ifdef CONFIG_ZBUS_CHANNEL_NAME
const struct zbus_channel *zbus_chan_from_name(const char *name);
#endif

/* Iteration over all channels/observers.  The iterator function returns
 * false to stop the iteration.
 */

bool zbus_iterate_over_channels(
    bool (*iterator_func)(const struct zbus_channel *chan));
bool zbus_iterate_over_channels_with_user_data(
    bool (*iterator_func)(const struct zbus_channel *chan, void *user_data),
    void *user_data);
bool zbus_iterate_over_observers(
    bool (*iterator_func)(const struct zbus_observer *obs));
bool zbus_iterate_over_observers_with_user_data(
    bool (*iterator_func)(const struct zbus_observer *obs, void *user_data),
    void *user_data);

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

#ifdef CONFIG_ZBUS_CHANNEL_NAME
static inline const char *zbus_chan_name(const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);
  return chan->name;
}
#endif

/* Direct access to the channel message.  Only valid while the channel is
 * locked (inside a listener callback or between claim/finish).
 */

static inline void *zbus_chan_msg(const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);
  return chan->message;
}

static inline const void *zbus_chan_const_msg(
    const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);
  return chan->message;
}

static inline size_t zbus_chan_msg_size(const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);
  return chan->message_size;
}

static inline void *zbus_chan_user_data(const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);
  return chan->user_data;
}

static inline int zbus_obs_is_enabled(const struct zbus_observer *obs,
                                      bool *enable)
{
  _ZBUS_ASSERT(obs != NULL, "obs is required");
  _ZBUS_ASSERT(enable != NULL, "enable is required");

  *enable = obs->data->enabled;
  return 0;
}

#ifdef CONFIG_ZBUS_OBSERVER_NAME
static inline const char *zbus_obs_name(const struct zbus_observer *obs)
{
  DEBUGASSERT(obs != NULL);
  return obs->name;
}
#endif

#ifdef CONFIG_ZBUS_CHANNEL_PUBLISH_STATS

/* Update the publish statistics (claim/finish workflow only; the channel
 * must be locked).
 */

static inline void zbus_chan_pub_stats_update(
    const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);

  clock_gettime(CLOCK_MONOTONIC, &chan->data->publish_timestamp);
  chan->data->publish_count += 1;
}

static inline struct timespec zbus_chan_pub_stats_last_time(
    const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);
  return chan->data->publish_timestamp;
}

static inline uint32_t zbus_chan_pub_stats_count(
    const struct zbus_channel *chan)
{
  DEBUGASSERT(chan != NULL);
  return chan->data->publish_count;
}

#else

static inline void zbus_chan_pub_stats_update(
    const struct zbus_channel *chan)
{
  (void)chan;
}

#endif /* CONFIG_ZBUS_CHANNEL_PUBLISH_STATS */

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_ZBUS_H */
