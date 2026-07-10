/****************************************************************************
 * apps/system/zbus/zbus.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/clock.h>
#include <nuttx/compiler.h>
#include <nuttx/mqueue.h>

#include <fcntl.h>
#include <inttypes.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <system/zbus.h>

#include "zbus_priv.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_once_t g_zbus_once = PTHREAD_ONCE_INIT;

/* Protects observer enabled flags and observation masks */

static pthread_mutex_t g_zbus_obs_lock = PTHREAD_MUTEX_INITIALIZER;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zbus_deadline_to_realtime
 *
 * Description:
 *   Convert the remaining time of a monotonic deadline into an absolute
 *   CLOCK_REALTIME timespec as required by mq_timedsend/mq_timedreceive.
 *
 ****************************************************************************/

static void zbus_deadline_to_realtime(FAR const struct zbus_deadline *d,
                                      FAR struct timespec *rt)
{
  struct timespec remaining;
  struct timespec now;

  clock_gettime(CLOCK_REALTIME, rt);

  if (d->mode == ZBUS_DEADLINE_ABS)
    {
      clock_gettime(CLOCK_MONOTONIC, &now);

      /* clock_timespec_subtract() returns zero when the deadline has
       * already expired, which leaves rt at the current time.
       */

      clock_timespec_subtract(&d->abs, &now, &remaining);
      clock_timespec_add(rt, &remaining, rt);
    }
}

/****************************************************************************
 * Name: zbus_mq_send / zbus_mq_recv
 *
 * Description:
 *   Message queue send/receive honoring a zbus_deadline.  Following the
 *   Zephyr k_msgq semantics, a no-wait failure returns -ENOMSG and a
 *   timeout returns -EAGAIN.
 *
 ****************************************************************************/

static int zbus_mq_send(struct file *mq, const char *buf, size_t len,
                        const struct zbus_deadline *d)
{
  struct timespec rt;
  int ret;

  if (mq->f_inode == NULL)
    {
      return -ENODEV;
    }

  if (d->mode == ZBUS_DEADLINE_FOREVER)
    {
      do
        {
          ret = file_mq_send(mq, buf, len, 0);
        }
      while (ret == -EINTR);
    }
  else
    {
      zbus_deadline_to_realtime(d, &rt);
      do
        {
          ret = file_mq_timedsend(mq, buf, len, 0, &rt);
        }
      while (ret == -EINTR);
    }

  if (ret == -ETIMEDOUT)
    {
      return (d->mode == ZBUS_DEADLINE_NOWAIT) ? -ENOMSG : -EAGAIN;
    }

  return ret;
}

static ssize_t zbus_mq_recv(struct file *mq, char *buf, size_t len,
                            const struct zbus_deadline *d)
{
  struct timespec rt;
  ssize_t ret;

  if (mq->f_inode == NULL)
    {
      return -ENODEV;
    }

  if (d->mode == ZBUS_DEADLINE_FOREVER)
    {
      do
        {
          ret = file_mq_receive(mq, buf, len, NULL);
        }
      while (ret == -EINTR);
    }
  else
    {
      zbus_deadline_to_realtime(d, &rt);
      do
        {
          ret = file_mq_timedreceive(mq, buf, len, NULL, &rt);
        }
      while (ret == -EINTR);
    }

  if (ret == -ETIMEDOUT)
    {
      return (d->mode == ZBUS_DEADLINE_NOWAIT) ? -ENOMSG : -EAGAIN;
    }

  return ret;
}

#ifdef CONFIG_ZBUS_ASYNC_LISTENER

/****************************************************************************
 * Name: zbus_async_listener_task
 *
 * Description:
 *   Dedicated task of an async listener: block on the listener's queue
 *   and invoke its callback for every message copy, from an aligned
 *   buffer.  argv[1] carries the observer address.
 *
 ****************************************************************************/

static int zbus_async_listener_task(int argc, FAR char *argv[])
{
  const struct zbus_observer *obs;
  char buf[sizeof(struct zbus_channel *) +
           CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE];
  uint8_t msg[CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE] aligned_data(8);
  const struct zbus_channel *chan;
  struct zbus_deadline d;
  ssize_t nbytes;

  if (argc < 2)
    {
      return EXIT_FAILURE;
    }

  obs = (const struct zbus_observer *)(uintptr_t)strtoul(argv[1], NULL, 16);
  d.mode = ZBUS_DEADLINE_FOREVER;

  for (; ; )
    {
      nbytes = zbus_mq_recv(&obs->data->mq, buf, sizeof(buf), &d);
      if (nbytes < (ssize_t)sizeof(struct zbus_channel *))
        {
          continue;
        }

      memcpy(&chan, buf, sizeof(chan));
      memcpy(msg, buf + sizeof(chan), nbytes - sizeof(chan));
      obs->async_callback(chan, msg);
    }

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: zbus_async_listener_start
 *
 * Description:
 *   Spawn the task serving an async listener.  A task rather than a
 *   pthread: the lazy init runs in the context of the first API caller,
 *   and a pthread would die with that caller's task group.
 *
 ****************************************************************************/

static int zbus_async_listener_start(const struct zbus_observer *obs)
{
  char arg[2 + sizeof(uintptr_t) * 2 + 1];
  FAR char *argv[2];
  int pid;

  snprintf(arg, sizeof(arg), "%" PRIxPTR, (uintptr_t)obs);
  argv[0] = arg;
  argv[1] = NULL;

  pid = task_create("zbus_async", CONFIG_ZBUS_ASYNC_LISTENER_PRIORITY,
                    CONFIG_ZBUS_ASYNC_LISTENER_STACKSIZE,
                    zbus_async_listener_task, argv);
  if (pid < 0)
    {
      return -errno;
    }

  obs->data->pid = pid;
  return 0;
}

#endif /* CONFIG_ZBUS_ASYNC_LISTENER */

/****************************************************************************
 * Name: zbus_init_fn
 *
 * Description:
 *   One-time initialization: compute the observation index boundaries of
 *   every channel (relies on the linker sorting the observation section by
 *   name, which groups entries per channel in priority order) and open the
 *   notification queues of subscriber-type observers.
 *
 ****************************************************************************/

static void zbus_init_fn(void)
{
  FAR struct zbus_channel_observation *observation;
  FAR struct zbus_observer *obs;
  const struct zbus_channel *curr = NULL;
  const struct zbus_channel *prev = NULL;

  STRUCT_SECTION_FOREACH(zbus_channel_observation, observation)
    {
      /* Apply the ROM-preserved initial mask value */

      *observation->mask = observation->mask_init;

      curr = observation->chan;

      if (prev != curr)
        {
          if (prev == NULL)
            {
              curr->data->observers_start_idx = 0;
              curr->data->observers_end_idx = 0;
            }
          else
            {
              curr->data->observers_start_idx =
                prev->data->observers_end_idx;
              curr->data->observers_end_idx =
                prev->data->observers_end_idx;
            }

          prev = curr;
        }

      ++(curr->data->observers_end_idx);
    }

  /* Open the notification queues */

  STRUCT_SECTION_FOREACH(zbus_observer, obs)
    {
      struct mq_attr attr;
      char name[24];
      int ret;

      if (obs->type != ZBUS_OBSERVER_SUBSCRIBER_TYPE
#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
          && obs->type != ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE
#endif
#ifdef CONFIG_ZBUS_ASYNC_LISTENER
          && obs->type != ZBUS_OBSERVER_ASYNC_LISTENER_TYPE
#endif
         )
        {
          continue;
        }

      memset(&attr, 0, sizeof(attr));
      attr.mq_maxmsg = (obs->queue_size > 0) ? obs->queue_size : 1;

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
      if (obs->type != ZBUS_OBSERVER_SUBSCRIBER_TYPE)
        {
          /* Message subscribers and async listeners carry a copy of the
           * message after the channel pointer.
           */

          attr.mq_msgsize = sizeof(struct zbus_channel *) +
                            CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE;
        }
      else
#endif
        {
          attr.mq_msgsize = sizeof(struct zbus_channel *);
        }

      snprintf(name, sizeof(name), "zb%08" PRIxPTR, (uintptr_t)obs);

      /* file_mq_open() creates a queue usable from any task, unlike
       * mq_open() whose descriptor belongs to the calling task only.
       */

      ret = file_mq_open(&obs->data->mq, name, O_RDWR | O_CREAT, 0644,
                         &attr);
      if (ret < 0)
        {
          syslog(LOG_ERR, "zbus: cannot open queue %s: %d\n", name, ret);
          continue;
        }

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
      if (obs->type == ZBUS_OBSERVER_ASYNC_LISTENER_TYPE)
        {
          ret = zbus_async_listener_start(obs);
          if (ret < 0)
            {
              syslog(LOG_ERR, "zbus: cannot start async listener %p: %d\n",
                     obs, ret);
            }
        }
#endif
    }
}

/****************************************************************************
 * Name: zbus_notify_observer
 *
 * Description:
 *   Deliver one notification.  msgbuf carries the pre-built message
 *   subscriber datagram ({channel pointer, message copy}) or NULL when
 *   CONFIG_ZBUS_MSG_SUBSCRIBER is disabled.
 *
 ****************************************************************************/

static int zbus_notify_observer(const struct zbus_channel *chan,
                                const struct zbus_observer *obs,
                                const struct zbus_deadline *d,
                                const char *msgbuf)
{
  switch (obs->type)
    {
      case ZBUS_OBSERVER_LISTENER_TYPE:
        obs->callback(chan);
        return 0;

      case ZBUS_OBSERVER_SUBSCRIBER_TYPE:
        return zbus_mq_send(&obs->data->mq, (const char *)&chan,
                            sizeof(chan), d);

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
      case ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE:
        if (chan->message_size > CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE)
          {
            return -EMSGSIZE;
          }

        return zbus_mq_send(&obs->data->mq, msgbuf,
                            sizeof(struct zbus_channel *) +
                            chan->message_size, d);
#endif

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
      case ZBUS_OBSERVER_ASYNC_LISTENER_TYPE:
        {
          if (chan->message_size > CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE)
            {
              return -EMSGSIZE;
            }

          /* The listener's task drains the queue and runs the callback */

          return zbus_mq_send(&obs->data->mq, msgbuf,
                              sizeof(struct zbus_channel *) +
                              chan->message_size, d);
        }
#endif

      default:
        return -EINVAL;
    }
}

/****************************************************************************
 * Name: zbus_vded_exec
 *
 * Description:
 *   The event dispatcher: notify every enabled/unmasked observer of the
 *   channel.  The channel must be locked by the caller.
 *
 ****************************************************************************/

static int zbus_vded_exec(const struct zbus_channel *chan,
                          const struct zbus_deadline *d)
{
  const char *msgbuf = NULL;
  int last_error = 0;
  int16_t i;
  int err;

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
  char buf[sizeof(struct zbus_channel *) +
           CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE];

  memcpy(buf, &chan, sizeof(chan));
  if (chan->message_size <= CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE)
    {
      memcpy(buf + sizeof(chan), chan->message, chan->message_size);
    }

  msgbuf = buf;
#endif

  /* The observations of a channel are contiguous and sorted by the linker,
   * so notifying them in order follows the priority of the definition.
   */

  for (i = chan->data->observers_start_idx;
       i < chan->data->observers_end_idx; i++)
    {
      struct zbus_channel_observation *observation;
      const struct zbus_observer *obs;

      STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);

      obs = observation->obs;
      if (!obs->data->enabled || *observation->mask)
        {
          continue;
        }

      err = zbus_notify_observer(chan, obs, d, msgbuf);
      if (err)
        {
          last_error = err;
          syslog(LOG_ERR, "zbus: could not notify observer %p: %d\n",
                 obs, err);
        }
    }

#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
  struct zbus_observer_node *obs_nd;

  list_for_every_entry(&chan->data->observers, obs_nd,
                       struct zbus_observer_node, node)
    {
      if (!obs_nd->obs->data->enabled)
        {
          continue;
        }

      err = zbus_notify_observer(chan, obs_nd->obs, d, msgbuf);
      if (err)
        {
          last_error = err;
        }
    }
#endif

  return last_error;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zbus_port_init_once
 ****************************************************************************/

void zbus_port_init_once(void)
{
  pthread_once(&g_zbus_once, zbus_init_fn);
}

/****************************************************************************
 * Name: zbus_deadline_init
 ****************************************************************************/

void zbus_deadline_init(int32_t timeout_ms, struct zbus_deadline *d)
{
  if (timeout_ms < 0)
    {
      d->mode = ZBUS_DEADLINE_FOREVER;
    }
  else if (timeout_ms == 0)
    {
      d->mode = ZBUS_DEADLINE_NOWAIT;
    }
  else
    {
      struct timespec delay;

      d->mode = ZBUS_DEADLINE_ABS;
      clock_gettime(CLOCK_MONOTONIC, &d->abs);
      clock_nsec2time(&delay, (int64_t)timeout_ms * NSEC_PER_MSEC);
      clock_timespec_add(&d->abs, &delay, &d->abs);
    }
}

/****************************************************************************
 * Name: zbus_sem_take
 ****************************************************************************/

int zbus_sem_take(sem_t *sem, const struct zbus_deadline *d)
{
  int ret;

  switch (d->mode)
    {
      case ZBUS_DEADLINE_FOREVER:
        do
          {
            ret = sem_wait(sem);
          }
        while (ret < 0 && errno == EINTR);

        return (ret < 0) ? -errno : 0;

      case ZBUS_DEADLINE_NOWAIT:
        ret = sem_trywait(sem);
        if (ret < 0)
          {
            return (errno == EAGAIN) ? -EBUSY : -errno;
          }

        return 0;

      case ZBUS_DEADLINE_ABS:
      default:
        do
          {
            ret = sem_clockwait(sem, CLOCK_MONOTONIC, &d->abs);
          }
        while (ret < 0 && errno == EINTR);

        if (ret < 0)
          {
            return (errno == ETIMEDOUT) ? -EAGAIN : -errno;
          }

        return 0;
    }
}

/****************************************************************************
 * Name: zbus_chan_pub
 ****************************************************************************/

int zbus_chan_pub(const struct zbus_channel *chan, const void *msg,
                  int32_t timeout_ms)
{
  struct zbus_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  zbus_port_init_once();

  if (chan->validator != NULL &&
      !chan->validator(msg, chan->message_size))
    {
      return -ENOMSG;
    }

  zbus_deadline_init(timeout_ms, &d);

  err = zbus_sem_take(&chan->data->sem, &d);
  if (err)
    {
      return err;
    }

#ifdef CONFIG_ZBUS_CHANNEL_PUBLISH_STATS
  zbus_chan_pub_stats_update(chan);
#endif

  memcpy(chan->message, msg, chan->message_size);

  err = zbus_vded_exec(chan, &d);

  sem_post(&chan->data->sem);

  return err;
}

/****************************************************************************
 * Name: zbus_chan_read
 ****************************************************************************/

int zbus_chan_read(const struct zbus_channel *chan, void *msg,
                   int32_t timeout_ms)
{
  struct zbus_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  zbus_port_init_once();

  zbus_deadline_init(timeout_ms, &d);

  err = zbus_sem_take(&chan->data->sem, &d);
  if (err)
    {
      return err;
    }

  memcpy(msg, chan->message, chan->message_size);

  sem_post(&chan->data->sem);

  return 0;
}

/****************************************************************************
 * Name: zbus_chan_notify
 ****************************************************************************/

int zbus_chan_notify(const struct zbus_channel *chan, int32_t timeout_ms)
{
  struct zbus_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  zbus_deadline_init(timeout_ms, &d);

  err = zbus_sem_take(&chan->data->sem, &d);
  if (err)
    {
      return err;
    }

  err = zbus_vded_exec(chan, &d);

  sem_post(&chan->data->sem);

  return err;
}

/****************************************************************************
 * Name: zbus_chan_claim
 ****************************************************************************/

int zbus_chan_claim(const struct zbus_channel *chan, int32_t timeout_ms)
{
  struct zbus_deadline d;

  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  zbus_deadline_init(timeout_ms, &d);

  return zbus_sem_take(&chan->data->sem, &d);
}

/****************************************************************************
 * Name: zbus_chan_finish
 ****************************************************************************/

int zbus_chan_finish(const struct zbus_channel *chan)
{
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  sem_post(&chan->data->sem);

  return 0;
}

/****************************************************************************
 * Name: zbus_sub_wait
 ****************************************************************************/

int zbus_sub_wait(const struct zbus_observer *sub,
                  const struct zbus_channel **chan, int32_t timeout_ms)
{
  const struct zbus_channel *received;
  struct zbus_deadline d;
  ssize_t nbytes;

  _ZBUS_ASSERT(sub != NULL, "sub is required");
  _ZBUS_ASSERT(sub->type == ZBUS_OBSERVER_SUBSCRIBER_TYPE,
               "sub must be a SUBSCRIBER");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  zbus_deadline_init(timeout_ms, &d);

  nbytes = zbus_mq_recv(&sub->data->mq, (char *)&received,
                        sizeof(received), &d);
  if (nbytes < 0)
    {
      return nbytes;
    }

  *chan = received;
  return 0;
}

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER

/****************************************************************************
 * Name: zbus_sub_wait_msg
 ****************************************************************************/

int zbus_sub_wait_msg(const struct zbus_observer *sub,
                      const struct zbus_channel **chan, void *msg,
                      int32_t timeout_ms)
{
  char buf[sizeof(struct zbus_channel *) +
           CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE];
  struct zbus_deadline d;
  ssize_t nbytes;

  _ZBUS_ASSERT(sub != NULL, "sub is required");
  _ZBUS_ASSERT(sub->type == ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,
               "sub must be a MSG_SUBSCRIBER");
  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  zbus_port_init_once();

  zbus_deadline_init(timeout_ms, &d);

  nbytes = zbus_mq_recv(&sub->data->mq, buf, sizeof(buf), &d);
  if (nbytes < 0)
    {
      return nbytes;
    }

  if (nbytes < (ssize_t)sizeof(struct zbus_channel *))
    {
      return -EILSEQ;
    }

  memcpy(chan, buf, sizeof(struct zbus_channel *));
  memcpy(msg, buf + sizeof(struct zbus_channel *),
         nbytes - sizeof(struct zbus_channel *));

  return 0;
}

#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

/****************************************************************************
 * Name: zbus_obs_set_enable
 ****************************************************************************/

int zbus_obs_set_enable(const struct zbus_observer *obs, bool enabled)
{
  _ZBUS_ASSERT(obs != NULL, "obs is required");

  pthread_mutex_lock(&g_zbus_obs_lock);
  obs->data->enabled = enabled;
  pthread_mutex_unlock(&g_zbus_obs_lock);

  return 0;
}

/****************************************************************************
 * Name: zbus_obs_set_chan_notification_mask
 ****************************************************************************/

int zbus_obs_set_chan_notification_mask(const struct zbus_observer *obs,
                                        const struct zbus_channel *chan,
                                        bool masked)
{
  int16_t i;
  int err = -ESRCH;

  _ZBUS_ASSERT(obs != NULL, "obs is required");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  pthread_mutex_lock(&g_zbus_obs_lock);

  for (i = chan->data->observers_start_idx;
       i < chan->data->observers_end_idx; i++)
    {
      struct zbus_channel_observation *observation;

      STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);

      if (observation->obs == obs)
        {
          *observation->mask = masked;
          err = 0;
          break;
        }
    }

  pthread_mutex_unlock(&g_zbus_obs_lock);

  return err;
}

/****************************************************************************
 * Name: zbus_obs_is_chan_notification_masked
 ****************************************************************************/

int zbus_obs_is_chan_notification_masked(const struct zbus_observer *obs,
                                         const struct zbus_channel *chan,
                                         bool *masked)
{
  int16_t i;
  int err = -ESRCH;

  _ZBUS_ASSERT(obs != NULL, "obs is required");
  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(masked != NULL, "masked is required");

  zbus_port_init_once();

  pthread_mutex_lock(&g_zbus_obs_lock);

  for (i = chan->data->observers_start_idx;
       i < chan->data->observers_end_idx; i++)
    {
      struct zbus_channel_observation *observation;

      STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);

      if (observation->obs == obs)
        {
          *masked = *observation->mask;
          err = 0;
          break;
        }
    }

  pthread_mutex_unlock(&g_zbus_obs_lock);

  return err;
}

#ifdef CONFIG_ZBUS_CHANNEL_ID

/****************************************************************************
 * Name: zbus_chan_from_id
 ****************************************************************************/

const struct zbus_channel *zbus_chan_from_id(uint32_t channel_id)
{
  FAR struct zbus_channel *chan;

  if (channel_id == ZBUS_CHAN_ID_INVALID)
    {
      return NULL;
    }

  STRUCT_SECTION_FOREACH(zbus_channel, chan)
    {
      if (chan->id == channel_id)
        {
          return chan;
        }
    }

  return NULL;
}

#endif /* CONFIG_ZBUS_CHANNEL_ID */

#ifdef CONFIG_ZBUS_CHANNEL_NAME

/****************************************************************************
 * Name: zbus_chan_from_name
 ****************************************************************************/

const struct zbus_channel *zbus_chan_from_name(const char *name)
{
  FAR struct zbus_channel *chan;

  if (name == NULL)
    {
      return NULL;
    }

  STRUCT_SECTION_FOREACH(zbus_channel, chan)
    {
      if (strcmp(chan->name, name) == 0)
        {
          return chan;
        }
    }

  return NULL;
}

#endif /* CONFIG_ZBUS_CHANNEL_NAME */
