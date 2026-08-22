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
#include <nuttx/irq.h>
#include <nuttx/mqueue.h>

#include <fcntl.h>
#include <inttypes.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
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
 * Name: zb_ts_add_ms / zb_ts_cmp / zb_ts_sub
 *
 * Description:
 *   Small timespec helpers.
 *
 ****************************************************************************/

static void zb_ts_add_ms(struct timespec *ts, int32_t ms)
{
  ts->tv_sec += ms / 1000;
  ts->tv_nsec += (long)(ms % 1000) * 1000000L;
  if (ts->tv_nsec >= 1000000000L)
    {
      ts->tv_sec += 1;
      ts->tv_nsec -= 1000000000L;
    }
}

static int zb_ts_cmp(const struct timespec *a, const struct timespec *b)
{
  if (a->tv_sec != b->tv_sec)
    {
      return (a->tv_sec < b->tv_sec) ? -1 : 1;
    }

  if (a->tv_nsec != b->tv_nsec)
    {
      return (a->tv_nsec < b->tv_nsec) ? -1 : 1;
    }

  return 0;
}

/****************************************************************************
 * Name: zb_deadline_to_realtime
 *
 * Description:
 *   Convert the remaining time of a monotonic deadline into an absolute
 *   CLOCK_REALTIME timespec as required by mq_timedsend/mq_timedreceive.
 *
 ****************************************************************************/

static void zb_deadline_to_realtime(const struct zb_deadline *d,
                                    struct timespec *rt)
{
  struct timespec now;

  clock_gettime(CLOCK_REALTIME, rt);

  if (d->mode == ZB_DEADLINE_ABS)
    {
      clock_gettime(CLOCK_MONOTONIC, &now);
      if (zb_ts_cmp(&now, &d->abs) < 0)
        {
          rt->tv_sec += d->abs.tv_sec - now.tv_sec;
          rt->tv_nsec += d->abs.tv_nsec - now.tv_nsec;
          while (rt->tv_nsec >= 1000000000L)
            {
              rt->tv_sec += 1;
              rt->tv_nsec -= 1000000000L;
            }

          while (rt->tv_nsec < 0)
            {
              rt->tv_sec -= 1;
              rt->tv_nsec += 1000000000L;
            }
        }
    }
}

/****************************************************************************
 * Name: zb_mq_send / zb_mq_recv
 *
 * Description:
 *   Message queue send/receive honoring a zb_deadline.  Following the
 *   Zephyr k_msgq semantics, a no-wait failure returns -ENOMSG and a
 *   timeout returns -EAGAIN.
 *
 ****************************************************************************/

static int zb_mq_send(struct file *mq, const char *buf, size_t len,
                      const struct zb_deadline *d)
{
  struct timespec rt;
  int ret;

  if (mq->f_inode == NULL)
    {
      return -ENODEV;
    }

  if (d->mode == ZB_DEADLINE_FOREVER)
    {
      do
        {
          ret = file_mq_send(mq, buf, len, 0);
        }
      while (ret == -EINTR);
    }
  else
    {
      zb_deadline_to_realtime(d, &rt);
      do
        {
          ret = file_mq_timedsend(mq, buf, len, 0, &rt);
        }
      while (ret == -EINTR);
    }

  if (ret == -ETIMEDOUT)
    {
      return (d->mode == ZB_DEADLINE_NOWAIT) ? -ENOMSG : -EAGAIN;
    }

  return ret;
}

static ssize_t zb_mq_recv(struct file *mq, char *buf, size_t len,
                          const struct zb_deadline *d)
{
  struct timespec rt;
  ssize_t ret;

  if (mq->f_inode == NULL)
    {
      return -ENODEV;
    }

  if (d->mode == ZB_DEADLINE_FOREVER)
    {
      do
        {
          ret = file_mq_receive(mq, buf, len, NULL);
        }
      while (ret == -EINTR);
    }
  else
    {
      zb_deadline_to_realtime(d, &rt);
      do
        {
          ret = file_mq_timedreceive(mq, buf, len, NULL, &rt);
        }
      while (ret == -EINTR);
    }

  if (ret == -ETIMEDOUT)
    {
      return (d->mode == ZB_DEADLINE_NOWAIT) ? -ENOMSG : -EAGAIN;
    }

  return ret;
}

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
        }
    }
}

#ifdef CONFIG_ZBUS_ASYNC_LISTENER

/****************************************************************************
 * Name: zb_async_listener_worker
 *
 * Description:
 *   Low priority work queue handler: drain the async listener's queue
 *   invoking its callback for every pending message copy.
 *
 ****************************************************************************/

static void zb_async_listener_worker(FAR void *arg)
{
  const struct zbus_observer *obs = (const struct zbus_observer *)arg;
  char buf[sizeof(struct zbus_channel *) +
           CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE];
  const struct zbus_channel *chan;
  struct zb_deadline d;
  ssize_t nbytes;

  d.mode = ZB_DEADLINE_NOWAIT;

  for (; ; )
    {
      nbytes = zb_mq_recv(&obs->data->mq, buf, sizeof(buf), &d);
      if (nbytes < (ssize_t)sizeof(struct zbus_channel *))
        {
          break;
        }

      memcpy(&chan, buf, sizeof(chan));
      obs->async_callback(chan, buf + sizeof(chan));
    }
}

#endif /* CONFIG_ZBUS_ASYNC_LISTENER */

/****************************************************************************
 * Name: zb_notify_observer
 *
 * Description:
 *   Deliver one notification.  msgbuf carries the pre-built message
 *   subscriber datagram ({channel pointer, message copy}) or NULL when
 *   CONFIG_ZBUS_MSG_SUBSCRIBER is disabled.
 *
 ****************************************************************************/

static int zb_notify_observer(const struct zbus_channel *chan,
                              const struct zbus_observer *obs,
                              const struct zb_deadline *d,
                              const char *msgbuf)
{
  switch (obs->type)
    {
      case ZBUS_OBSERVER_LISTENER_TYPE:
        obs->callback(chan);
        return 0;

      case ZBUS_OBSERVER_SUBSCRIBER_TYPE:
        return zb_mq_send(&obs->data->mq, (const char *)&chan,
                          sizeof(chan), d);

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
      case ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE:
        if (chan->message_size > CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE)
          {
            return -EMSGSIZE;
          }

        return zb_mq_send(&obs->data->mq, msgbuf,
                          sizeof(struct zbus_channel *) +
                          chan->message_size, d);
#endif

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
      case ZBUS_OBSERVER_ASYNC_LISTENER_TYPE:
        {
          int ret;

          if (chan->message_size > CONFIG_ZBUS_MSG_SUBSCRIBER_MAX_MSG_SIZE)
            {
              return -EMSGSIZE;
            }

          ret = zb_mq_send(&obs->data->mq, msgbuf,
                           sizeof(struct zbus_channel *) +
                           chan->message_size, d);
          if (ret < 0)
            {
              return ret;
            }

          ret = work_queue(LPWORK, &obs->data->work,
                           zb_async_listener_worker, (FAR void *)obs, 0);
          if (ret < 0 && ret != -EALREADY)
            {
              return ret;
            }

          return 0;
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
                          const struct zb_deadline *d)
{
  const char *msgbuf = NULL;
  int last_error = 0;
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

  for (int16_t i = chan->data->observers_start_idx,
       limit = chan->data->observers_end_idx; i < limit; i++)
    {
      struct zbus_channel_observation *observation;

      STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);

      const struct zbus_observer *obs = observation->obs;

      if (!obs->data->enabled || *observation->mask)
        {
          continue;
        }

      err = zb_notify_observer(chan, obs, d, msgbuf);
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

      err = zb_notify_observer(chan, obs_nd->obs, d, msgbuf);
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
 * Name: zb_deadline_init
 ****************************************************************************/

void zb_deadline_init(int32_t timeout_ms, struct zb_deadline *d)
{
  if (timeout_ms < 0)
    {
      d->mode = ZB_DEADLINE_FOREVER;
    }
  else if (timeout_ms == 0)
    {
      d->mode = ZB_DEADLINE_NOWAIT;
    }
  else
    {
      d->mode = ZB_DEADLINE_ABS;
      clock_gettime(CLOCK_MONOTONIC, &d->abs);
      zb_ts_add_ms(&d->abs, timeout_ms);
    }
}

/****************************************************************************
 * Name: zb_sem_take
 ****************************************************************************/

int zb_sem_take(sem_t *sem, const struct zb_deadline *d)
{
  int ret;

  switch (d->mode)
    {
      case ZB_DEADLINE_FOREVER:
        do
          {
            ret = sem_wait(sem);
          }
        while (ret < 0 && errno == EINTR);

        return (ret < 0) ? -errno : 0;

      case ZB_DEADLINE_NOWAIT:
        ret = sem_trywait(sem);
        if (ret < 0)
          {
            return (errno == EAGAIN) ? -EBUSY : -errno;
          }

        return 0;

      case ZB_DEADLINE_ABS:
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

#ifdef CONFIG_ZBUS_ISR_PUBLISHER

#if defined(CONFIG_SCHED_HPWORK)
#  define ZBUS_ISR_PUB_WORK HPWORK
#else
#  define ZBUS_ISR_PUB_WORK LPWORK
#endif

/****************************************************************************
 * Name: zb_isr_pub_worker
 *
 * Description:
 *   Work queue side of zbus_isr_pub(): snapshot the staged message under
 *   a critical section (so a concurrent interrupt cannot tear it) and
 *   perform the actual publication in thread context.
 *
 ****************************************************************************/

static void zb_isr_pub_worker(FAR void *arg)
{
  FAR struct zbus_isr_publisher *pub = arg;
  irqstate_t flags;
  int ret;

  flags = enter_critical_section();
  memcpy(pub->shadow, pub->staging, pub->chan->message_size);
  leave_critical_section(flags);

  ret = zbus_chan_pub(pub->chan, pub->shadow, ZBUS_NO_WAIT);
  if (ret < 0)
    {
      syslog(LOG_ERR, "zbus: deferred publish to %p failed: %d\n",
             pub->chan, ret);
    }
}

/****************************************************************************
 * Name: zbus_isr_pub
 ****************************************************************************/

int zbus_isr_pub(FAR struct zbus_isr_publisher *pub, FAR const void *msg)
{
  int ret;

  _ZBUS_ASSERT(pub != NULL, "pub is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  memcpy(pub->staging, msg, pub->chan->message_size);

  ret = work_queue(ZBUS_ISR_PUB_WORK, &pub->work, zb_isr_pub_worker,
                   pub, 0);
  if (ret == -EALREADY)
    {
      /* A publication is already pending; the worker will pick up the
       * data just staged.
       */

      ret = 0;
    }

  return ret;
}

#endif /* CONFIG_ZBUS_ISR_PUBLISHER */

/****************************************************************************
 * Name: zbus_chan_pub
 ****************************************************************************/

int zbus_chan_pub(const struct zbus_channel *chan, const void *msg,
                  int32_t timeout_ms)
{
  struct zb_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  zbus_port_init_once();

  if (chan->validator != NULL &&
      !chan->validator(msg, chan->message_size))
    {
      return -ENOMSG;
    }

  zb_deadline_init(timeout_ms, &d);

  err = zb_sem_take(&chan->data->sem, &d);
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
  struct zb_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  zbus_port_init_once();

  zb_deadline_init(timeout_ms, &d);

  err = zb_sem_take(&chan->data->sem, &d);
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
  struct zb_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  zb_deadline_init(timeout_ms, &d);

  err = zb_sem_take(&chan->data->sem, &d);
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
  struct zb_deadline d;

  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  zb_deadline_init(timeout_ms, &d);

  return zb_sem_take(&chan->data->sem, &d);
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
  struct zb_deadline d;
  ssize_t nbytes;

  _ZBUS_ASSERT(sub != NULL, "sub is required");
  _ZBUS_ASSERT(sub->type == ZBUS_OBSERVER_SUBSCRIBER_TYPE,
               "sub must be a SUBSCRIBER");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  zb_deadline_init(timeout_ms, &d);

  nbytes = zb_mq_recv(&sub->data->mq, (char *)&received,
                      sizeof(received), &d);
  if (nbytes < 0)
    {
      return (int)nbytes;
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
  struct zb_deadline d;
  ssize_t nbytes;

  _ZBUS_ASSERT(sub != NULL, "sub is required");
  _ZBUS_ASSERT(sub->type == ZBUS_OBSERVER_MSG_SUBSCRIBER_TYPE,
               "sub must be a MSG_SUBSCRIBER");
  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(msg != NULL, "msg is required");

  zbus_port_init_once();

  zb_deadline_init(timeout_ms, &d);

  nbytes = zb_mq_recv(&sub->data->mq, buf, sizeof(buf), &d);
  if (nbytes < 0)
    {
      return (int)nbytes;
    }

  if ((size_t)nbytes < sizeof(struct zbus_channel *))
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
  int err = -ESRCH;

  _ZBUS_ASSERT(obs != NULL, "obs is required");
  _ZBUS_ASSERT(chan != NULL, "chan is required");

  zbus_port_init_once();

  pthread_mutex_lock(&g_zbus_obs_lock);

  for (int16_t i = chan->data->observers_start_idx,
       limit = chan->data->observers_end_idx; i < limit; i++)
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
  int err = -ESRCH;

  _ZBUS_ASSERT(obs != NULL, "obs is required");
  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(masked != NULL, "masked is required");

  zbus_port_init_once();

  pthread_mutex_lock(&g_zbus_obs_lock);

  for (int16_t i = chan->data->observers_start_idx,
       limit = chan->data->observers_end_idx; i < limit; i++)
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
