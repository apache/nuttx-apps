/****************************************************************************
 * apps/testing/zbus/zbustest.c
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

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <cmocka.h>

#include <system/zbus.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct zbt_msg_s
{
  uint32_t seq;
  uint32_t value;
};

/* Sensor-like payload: exercises float/double members through the whole
 * pipeline (channel storage, listener const view, read-back and the
 * message subscriber mq copy), which is the typical zbus use case.
 */

struct zbt_imu_msg_s
{
  float accel[3];
  double magnitude;
  uint32_t seq;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_listener_a_count;
static uint32_t g_listener_a_last;
static int g_listener_b_count;
static uint32_t g_listener_b_last;
static int g_listener_rt_count;

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
static volatile int g_async_count;
static volatile uint32_t g_async_last;
#endif

static uint32_t g_user_word = 0xcafe;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void listener_a_cb(const struct zbus_channel *chan)
{
  const struct zbt_msg_s *msg = zbus_chan_const_msg(chan);

  g_listener_a_count++;
  g_listener_a_last = msg->value;
}

static void listener_b_cb(const struct zbus_channel *chan)
{
  const struct zbt_msg_s *msg = zbus_chan_const_msg(chan);

  g_listener_b_count++;
  g_listener_b_last = msg->value;
}

static void listener_rt_cb(const struct zbus_channel *chan)
{
  (void)chan;
  g_listener_rt_count++;
}

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
static void async_listener_cb(const struct zbus_channel *chan,
                              const void *msg)
{
  const struct zbt_msg_s *m = msg;

  (void)chan;
  g_async_last = m->value;
  g_async_count++;
}
#endif

/* Notification order: each callback records when it ran, so the test can
 * check that the observers are notified in the order they were listed.
 */

static int g_order_seq;
static int g_order_first;
static int g_order_second;
static int g_order_added;

static void order_first_cb(const struct zbus_channel *chan)
{
  (void)chan;
  g_order_first = ++g_order_seq;
}

static void order_second_cb(const struct zbus_channel *chan)
{
  (void)chan;
  g_order_second = ++g_order_seq;
}

static void order_added_cb(const struct zbus_channel *chan)
{
  (void)chan;
  g_order_added = ++g_order_seq;
}

/* Validator: rejects messages with value == 0xdead */

static bool chan_b_validator(const void *msg, size_t msg_size)
{
  const struct zbt_msg_s *m = msg;

  (void)msg_size;
  return m->value != 0xdead;
}

static int g_imu_listener_count;
static struct zbt_imu_msg_s g_imu_listener_last;

/* Timer-driven sampler (see test_timer_driven_publisher) */

#define ZBT_SAMPLER_SIGNAL   SIGUSR1
#define ZBT_SAMPLER_SAMPLES  5
#define ZBT_SAMPLER_PERIOD   10000000L    /* 10 ms */

static volatile int g_sampler_published;
static volatile int g_sampler_errors;

static void imu_listener_cb(const struct zbus_channel *chan)
{
  const struct zbt_imu_msg_s *msg = zbus_chan_const_msg(chan);

  g_imu_listener_last = *msg;
  g_imu_listener_count++;
}

/* Validator doing float math: rejects non-finite samples */

static bool imu_validator(const void *msg, size_t msg_size)
{
  const struct zbt_imu_msg_s *m = msg;

  (void)msg_size;
  return isfinite(m->accel[0]) && isfinite(m->accel[1]) &&
         isfinite(m->accel[2]) && isfinite(m->magnitude);
}

/* Interrupt-driven sampling, the NuttX way for a userspace library: the
 * kernel timer interrupt delivers a signal, a thread waits for it and
 * publishes from thread context.  Sample values are exactly
 * representable so the consumers can compare them bit-exact.
 */

ZBUS_CHAN_DECLARE(zbt_chan_imu);

static FAR void *sampler_thread(FAR void *arg)
{
  struct zbt_imu_msg_s msg;
  struct itimerspec its;
  struct sigevent sev;
  sigset_t set;
  timer_t timer;
  int n = 0;

  (void)arg;

  sigemptyset(&set);
  sigaddset(&set, ZBT_SAMPLER_SIGNAL);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  memset(&sev, 0, sizeof(sev));
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = ZBT_SAMPLER_SIGNAL;
  if (timer_create(CLOCK_MONOTONIC, &sev, &timer) != 0)
    {
      g_sampler_errors++;
      return NULL;
    }

  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = ZBT_SAMPLER_PERIOD;
  its.it_interval = its.it_value;
  timer_settime(timer, 0, &its, NULL);

  while (n < ZBT_SAMPLER_SAMPLES)
    {
      if (sigwaitinfo(&set, NULL) < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          g_sampler_errors++;
          break;
        }

      /* Timer tick: "read the sensor" and publish */

      msg.accel[0] = 0.5f * n;
      msg.accel[1] = -9.5f;
      msg.accel[2] = 0.25f;
      msg.magnitude = 9.5 + n;
      msg.seq = ++n;

      if (zbus_chan_pub(&zbt_chan_imu, &msg, 100) == 0)
        {
          g_sampler_published++;
        }
      else
        {
          g_sampler_errors++;
        }
    }

  timer_delete(timer);
  return NULL;
}

/****************************************************************************
 * Channel and observer definitions
 ****************************************************************************/

ZBUS_LISTENER_DEFINE(zbt_listener_a, listener_a_cb);
ZBUS_SUBSCRIBER_DEFINE(zbt_sub_a, 4);
ZBUS_LISTENER_DEFINE(zbt_listener_b, listener_b_cb);
ZBUS_LISTENER_DEFINE(zbt_listener_rt, listener_rt_cb);

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
ZBUS_MSG_SUBSCRIBER_DEFINE(zbt_msgsub_b);
#endif

ZBUS_CHAN_DEFINE(zbt_chan_a,
                 struct zbt_msg_s,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(zbt_listener_a, zbt_sub_a),
                 ZBUS_MSG_INIT(.seq = 0, .value = 0));

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
ZBUS_CHAN_DEFINE(zbt_chan_b,
                 struct zbt_msg_s,
                 chan_b_validator,
                 &g_user_word,
                 ZBUS_OBSERVERS(zbt_listener_b, zbt_msgsub_b),
                 ZBUS_MSG_INIT(.seq = 0, .value = 0));
#else
ZBUS_CHAN_DEFINE(zbt_chan_b,
                 struct zbt_msg_s,
                 chan_b_validator,
                 &g_user_word,
                 ZBUS_OBSERVERS(zbt_listener_b),
                 ZBUS_MSG_INIT(.seq = 0, .value = 0));
#endif

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
/* Attached to zbt_chan_b through ZBUS_CHAN_ADD_OBS (also exercises the
 * out-of-line observation macro).
 */

ZBUS_ASYNC_LISTENER_DEFINE(zbt_async_l, async_listener_cb);
ZBUS_CHAN_ADD_OBS(zbt_chan_b, zbt_async_l, 01);
#endif

ZBUS_LISTENER_DEFINE(zbt_imu_listener, imu_listener_cb);

ZBUS_LISTENER_DEFINE(zbt_ord_first, order_first_cb);
ZBUS_LISTENER_DEFINE(zbt_ord_second, order_second_cb);
ZBUS_LISTENER_DEFINE(zbt_ord_added, order_added_cb);

ZBUS_CHAN_DEFINE(zbt_chan_ord,
                 struct zbt_msg_s,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(zbt_ord_first, zbt_ord_second),
                 ZBUS_MSG_INIT(.seq = 0, .value = 0));

/* Bound out of line:  must be notified after the two listed above */

ZBUS_CHAN_ADD_OBS(zbt_chan_ord, zbt_ord_added, 01);

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
ZBUS_MSG_SUBSCRIBER_DEFINE(zbt_imu_msgsub);

ZBUS_CHAN_DEFINE(zbt_chan_imu,
                 struct zbt_imu_msg_s,
                 imu_validator,
                 NULL,
                 ZBUS_OBSERVERS(zbt_imu_listener, zbt_imu_msgsub),
                 ZBUS_MSG_INIT(.seq = 0));
#else
ZBUS_CHAN_DEFINE(zbt_chan_imu,
                 struct zbt_imu_msg_s,
                 imu_validator,
                 NULL,
                 ZBUS_OBSERVERS(zbt_imu_listener),
                 ZBUS_MSG_INIT(.seq = 0));
#endif

#ifdef CONFIG_ZBUS_CHANNEL_ID
ZBUS_CHAN_DEFINE_WITH_ID(zbt_chan_c,
                         42,
                         struct zbt_msg_s,
                         NULL,
                         NULL,
                         ZBUS_OBSERVERS_EMPTY,
                         ZBUS_MSG_INIT(.seq = 0, .value = 0));
#else
ZBUS_CHAN_DEFINE(zbt_chan_c,
                 struct zbt_msg_s,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .value = 0));
#endif

/****************************************************************************
 * Test helpers
 ****************************************************************************/

static void drain_subscriber(const struct zbus_observer *sub)
{
  const struct zbus_channel *chan;

  while (zbus_sub_wait(sub, &chan, ZBUS_NO_WAIT) == 0)
    {
    }
}

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
static void drain_msg_subscriber(const struct zbus_observer *sub)
{
  const struct zbus_channel *chan;
  struct zbt_msg_s msg;

  while (zbus_sub_wait_msg(sub, &chan, &msg, ZBUS_NO_WAIT) == 0)
    {
    }
}
#endif

static void reset_all(void)
{
  g_listener_a_count = 0;
  g_listener_b_count = 0;
  g_listener_rt_count = 0;

  zbus_obs_set_enable(&zbt_listener_a, true);
  zbus_obs_set_enable(&zbt_listener_b, true);
  zbus_obs_set_chan_notification_mask(&zbt_listener_a, &zbt_chan_a, false);

  drain_subscriber(&zbt_sub_a);
#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
  drain_msg_subscriber(&zbt_msgsub_b);
#endif

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
  /* Let the async listener task drain deliveries from previous tests
   * before resetting its counters.
   */

  usleep(20 * 1000);
  g_async_count = 0;
#endif
}

/****************************************************************************
 * Test cases
 ****************************************************************************/

/* Basic publish: listener receives synchronously, subscriber gets the
 * notification through its queue, read returns the published message.
 */

static void test_pub_read_listener_subscriber(FAR void **state)
{
  const struct zbus_channel *chan;
  struct zbt_msg_s msg;
  int ret;

  (void)state;
  reset_all();

  msg.seq = 1;
  msg.value = 100;
  ret = zbus_chan_pub(&zbt_chan_a, &msg, 1000);
  assert_int_equal(ret, 0);

  assert_int_equal(g_listener_a_count, 1);
  assert_int_equal(g_listener_a_last, 100);

  ret = zbus_sub_wait(&zbt_sub_a, &chan, 1000);
  assert_int_equal(ret, 0);
  assert_ptr_equal(chan, &zbt_chan_a);

  memset(&msg, 0, sizeof(msg));
  ret = zbus_chan_read(&zbt_chan_a, &msg, 500);
  assert_int_equal(ret, 0);
  assert_int_equal(msg.value, 100);
}

/* Publishing to one channel must not notify observers of another channel
 * (validates the observation index grouping computed at init).
 */

static void test_multi_channel_isolation(FAR void **state)
{
  struct zbt_msg_s msg;

  (void)state;
  reset_all();

  msg.seq = 1;
  msg.value = 111;
  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, 1000), 0);
  assert_int_equal(g_listener_a_count, 1);
  assert_int_equal(g_listener_b_count, 0);

  msg.value = 222;
  assert_int_equal(zbus_chan_pub(&zbt_chan_b, &msg, 1000), 0);
  assert_int_equal(g_listener_b_count, 1);
  assert_int_equal(g_listener_b_last, 222);
  assert_int_equal(g_listener_a_count, 1);

  /* Channel with no observers: publish must succeed and reach nobody */

  msg.value = 333;
  assert_int_equal(zbus_chan_pub(&zbt_chan_c, &msg, 1000), 0);
  assert_int_equal(g_listener_a_count, 1);
  assert_int_equal(g_listener_b_count, 1);
}

/* Validator: invalid messages are rejected with -ENOMSG and nobody is
 * notified.
 */

static void test_validator(FAR void **state)
{
  struct zbt_msg_s msg;

  (void)state;
  reset_all();

  msg.seq = 1;
  msg.value = 0xdead;
  assert_int_equal(zbus_chan_pub(&zbt_chan_b, &msg, 1000), -ENOMSG);
  assert_int_equal(g_listener_b_count, 0);

  msg.value = 7;
  assert_int_equal(zbus_chan_pub(&zbt_chan_b, &msg, 1000), 0);
  assert_int_equal(g_listener_b_count, 1);
}

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
/* Message subscriber: receives a copy of every message, in order, even if
 * the channel is republished before the subscriber runs.
 */

static void test_msg_subscriber(FAR void **state)
{
  const struct zbus_channel *chan;
  struct zbt_msg_s msg;
  uint32_t expected[3] =
  {
    10, 20, 30
  };

  int i;

  (void)state;
  reset_all();

  for (i = 0; i < 3; i++)
    {
      msg.seq = i;
      msg.value = expected[i];
      assert_int_equal(zbus_chan_pub(&zbt_chan_b, &msg, 1000), 0);
    }

  for (i = 0; i < 3; i++)
    {
      memset(&msg, 0, sizeof(msg));
      assert_int_equal(zbus_sub_wait_msg(&zbt_msgsub_b, &chan, &msg, 1000),
                       0);
      assert_ptr_equal(chan, &zbt_chan_b);
      assert_int_equal(msg.value, expected[i]);
    }
}
#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER */

/* Float payload: float/double members must survive bit-exact through
 * publish, the listener const view, read-back and the message subscriber
 * copy; the validator exercises float math (non-finite rejection).
 */

static void test_float_payload(FAR void **state)
{
#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
  const struct zbus_channel *chan;
#endif
  struct zbt_imu_msg_s msg;
  struct zbt_imu_msg_s rd;
  int ret;

  (void)state;

  g_imu_listener_count = 0;

  msg.accel[0] = 0.5f;
  msg.accel[1] = -9.80665f;
  msg.accel[2] = 3.1415927f;
  msg.magnitude = 9.83180020299;
  msg.seq = 1;

  ret = zbus_chan_pub(&zbt_chan_imu, &msg, 1000);
  assert_int_equal(ret, 0);

  /* Listener saw a bit-exact copy */

  assert_int_equal(g_imu_listener_count, 1);
  assert_true(g_imu_listener_last.accel[0] == msg.accel[0]);
  assert_true(g_imu_listener_last.accel[1] == msg.accel[1]);
  assert_true(g_imu_listener_last.accel[2] == msg.accel[2]);
  assert_true(g_imu_listener_last.magnitude == msg.magnitude);

  /* Read-back from channel storage */

  memset(&rd, 0, sizeof(rd));
  ret = zbus_chan_read(&zbt_chan_imu, &rd, 500);
  assert_int_equal(ret, 0);
  assert_true(rd.accel[0] == msg.accel[0]);
  assert_true(rd.accel[1] == msg.accel[1]);
  assert_true(rd.accel[2] == msg.accel[2]);
  assert_true(rd.magnitude == msg.magnitude);
  assert_int_equal(rd.seq, 1);

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
  /* Message subscriber received a bit-exact copy through the mq */

  memset(&rd, 0, sizeof(rd));
  ret = zbus_sub_wait_msg(&zbt_imu_msgsub, &chan, &rd, 1000);
  assert_int_equal(ret, 0);
  assert_ptr_equal(chan, &zbt_chan_imu);
  assert_true(rd.accel[0] == msg.accel[0]);
  assert_true(rd.accel[1] == msg.accel[1]);
  assert_true(rd.accel[2] == msg.accel[2]);
  assert_true(rd.magnitude == msg.magnitude);
#endif

  /* Validator rejects non-finite samples with -ENOMSG */

  msg.accel[1] = NAN;
  assert_int_equal(zbus_chan_pub(&zbt_chan_imu, &msg, 1000), -ENOMSG);
  assert_int_equal(g_imu_listener_count, 1);

  msg.accel[1] = INFINITY;
  assert_int_equal(zbus_chan_pub(&zbt_chan_imu, &msg, 1000), -ENOMSG);
  assert_int_equal(g_imu_listener_count, 1);
}

/* Timer-driven publisher: a kernel timer interrupt wakes a sampling
 * thread through a signal, which publishes float samples; the message
 * subscriber receives every sample in order, bit-exact, and the listener
 * sees each publication.
 */

static void test_timer_driven_publisher(FAR void **state)
{
#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
  const struct zbus_channel *chan;
  struct zbt_imu_msg_s rd;
  int i;
#endif
  pthread_t thread;

  (void)state;

  g_imu_listener_count = 0;
  g_sampler_published = 0;
  g_sampler_errors = 0;

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
  while (zbus_sub_wait_msg(&zbt_imu_msgsub, &chan, &rd, ZBUS_NO_WAIT) == 0)
    {
    }
#endif

  assert_int_equal(pthread_create(&thread, NULL, sampler_thread, NULL), 0);

#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
  for (i = 1; i <= ZBT_SAMPLER_SAMPLES; i++)
    {
      memset(&rd, 0, sizeof(rd));
      assert_int_equal(zbus_sub_wait_msg(&zbt_imu_msgsub, &chan, &rd, 1000),
                       0);
      assert_ptr_equal(chan, &zbt_chan_imu);
      assert_int_equal(rd.seq, i);
      assert_true(rd.accel[0] == 0.5f * (i - 1));
      assert_true(rd.accel[1] == -9.5f);
      assert_true(rd.magnitude == 9.5 + (i - 1));
    }
#endif

  assert_int_equal(pthread_join(thread, NULL), 0);
  assert_int_equal(g_sampler_errors, 0);
  assert_int_equal(g_sampler_published, ZBT_SAMPLER_SAMPLES);
  assert_int_equal(g_imu_listener_count, ZBT_SAMPLER_SAMPLES);
  assert_int_equal(g_imu_listener_last.seq, ZBT_SAMPLER_SAMPLES);
}

/* Notification order: the observers listed in the channel definition are
 * notified in the order they appear, and an observation bound out of line
 * with ZBUS_CHAN_ADD_OBS() comes after all of them.
 */

static void test_observer_order(FAR void **state)
{
  struct zbt_msg_s msg;

  (void)state;

  g_order_seq = 0;
  g_order_first = 0;
  g_order_second = 0;
  g_order_added = 0;

  msg.seq = 1;
  msg.value = 7;
  assert_int_equal(zbus_chan_pub(&zbt_chan_ord, &msg, 1000), 0);

  assert_int_equal(g_order_first, 1);
  assert_int_equal(g_order_second, 2);
  assert_int_equal(g_order_added, 3);
}

/* Claim/finish: direct access to the message memory; notify dispatches
 * without publishing.
 */

static void test_claim_finish_notify(FAR void **state)
{
  struct zbt_msg_s *direct;
  struct zbt_msg_s msg;

  (void)state;
  reset_all();

  assert_int_equal(zbus_chan_claim(&zbt_chan_a, 500), 0);

  direct = zbus_chan_msg(&zbt_chan_a);
  assert_non_null(direct);
  direct->value = 55;

  assert_int_equal(zbus_chan_finish(&zbt_chan_a), 0);

  /* No notification happened yet */

  assert_int_equal(g_listener_a_count, 0);

  /* Force the notification: the listener must see value 55 */

  assert_int_equal(zbus_chan_notify(&zbt_chan_a, 1000), 0);
  assert_int_equal(g_listener_a_count, 1);
  assert_int_equal(g_listener_a_last, 55);

  memset(&msg, 0, sizeof(msg));
  assert_int_equal(zbus_chan_read(&zbt_chan_a, &msg, 500), 0);
  assert_int_equal(msg.value, 55);

  drain_subscriber(&zbt_sub_a);
}

/* Notification masks: a masked observer is skipped; unrelated pairs
 * return -ESRCH.
 */

static void test_masks(FAR void **state)
{
  struct zbt_msg_s msg;
  bool masked;

  (void)state;
  reset_all();

  assert_int_equal(zbus_obs_set_chan_notification_mask(&zbt_listener_a,
                                                       &zbt_chan_a, true),
                   0);
  assert_int_equal(zbus_obs_is_chan_notification_masked(&zbt_listener_a,
                                                        &zbt_chan_a,
                                                        &masked), 0);
  assert_true(masked);

  msg.seq = 1;
  msg.value = 77;
  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, 1000), 0);
  assert_int_equal(g_listener_a_count, 0);

  assert_int_equal(zbus_obs_set_chan_notification_mask(&zbt_listener_a,
                                                       &zbt_chan_a, false),
                   0);

  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, 1000), 0);
  assert_int_equal(g_listener_a_count, 1);

  /* listener_b does not observe chan_a */

  assert_int_equal(zbus_obs_set_chan_notification_mask(&zbt_listener_b,
                                                       &zbt_chan_a, true),
                   -ESRCH);

  drain_subscriber(&zbt_sub_a);
}

/* Observer enable/disable */

static void test_enable_disable(FAR void **state)
{
  struct zbt_msg_s msg;
  bool enabled;

  (void)state;
  reset_all();

  assert_int_equal(zbus_obs_set_enable(&zbt_listener_a, false), 0);
  assert_int_equal(zbus_obs_is_enabled(&zbt_listener_a, &enabled), 0);
  assert_false(enabled);

  msg.seq = 1;
  msg.value = 88;
  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, 1000), 0);
  assert_int_equal(g_listener_a_count, 0);

  assert_int_equal(zbus_obs_set_enable(&zbt_listener_a, true), 0);

  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, 1000), 0);
  assert_int_equal(g_listener_a_count, 1);

  drain_subscriber(&zbt_sub_a);
}

#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
/* Runtime observers: add/remove, duplicate detection */

static void test_runtime_observers(FAR void **state)
{
  struct zbt_msg_s msg;

  (void)state;
  reset_all();

  assert_int_equal(zbus_chan_add_obs(&zbt_chan_a, &zbt_listener_rt, 500),
                   0);

  /* Duplicates: already a runtime observer / already a static observer */

  assert_int_equal(zbus_chan_add_obs(&zbt_chan_a, &zbt_listener_rt, 500),
                   -EALREADY);
  assert_int_equal(zbus_chan_add_obs(&zbt_chan_a, &zbt_listener_a, 500),
                   -EEXIST);

  msg.seq = 1;
  msg.value = 99;
  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, 1000), 0);
  assert_int_equal(g_listener_rt_count, 1);
  assert_int_equal(g_listener_a_count, 1);

  assert_int_equal(zbus_chan_rm_obs(&zbt_chan_a, &zbt_listener_rt, 500),
                   0);

  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, 1000), 0);
  assert_int_equal(g_listener_rt_count, 1);

  assert_int_equal(zbus_chan_rm_obs(&zbt_chan_a, &zbt_listener_rt, 500),
                   -ENODATA);

  drain_subscriber(&zbt_sub_a);
}
#endif /* CONFIG_ZBUS_RUNTIME_OBSERVERS */

#ifdef CONFIG_ZBUS_ASYNC_LISTENER
/* Async listener: callback runs on the listener's task with a copy of
 * the message; a burst of publishes is delivered completely and in order.
 */

static void test_async_listener(FAR void **state)
{
  struct zbt_msg_s msg;
  int i;

  (void)state;
  reset_all();

  msg.seq = 1;
  msg.value = 4242;
  assert_int_equal(zbus_chan_pub(&zbt_chan_b, &msg, 1000), 0);

  for (i = 0; i < 100 && g_async_count < 1; i++)
    {
      usleep(10 * 1000);
    }

  assert_int_equal(g_async_count, 1);
  assert_int_equal(g_async_last, 4242);

  /* Burst: all copies must be delivered */

  for (i = 1; i <= 3; i++)
    {
      msg.value = 4242 + i;
      assert_int_equal(zbus_chan_pub(&zbt_chan_b, &msg, 1000), 0);
    }

  for (i = 0; i < 100 && g_async_count < 4; i++)
    {
      usleep(10 * 1000);
    }

  assert_int_equal(g_async_count, 4);
  assert_int_equal(g_async_last, 4245);
}
#endif /* CONFIG_ZBUS_ASYNC_LISTENER */

#ifdef CONFIG_ZBUS_CHANNEL_NAME
/* Channel lookup by name */

static void test_from_name(FAR void **state)
{
  (void)state;

  assert_ptr_equal(zbus_chan_from_name("zbt_chan_a"), &zbt_chan_a);
  assert_ptr_equal(zbus_chan_from_name("zbt_chan_b"), &zbt_chan_b);
  assert_null(zbus_chan_from_name("does_not_exist"));

  assert_string_equal(zbus_chan_name(&zbt_chan_a), "zbt_chan_a");
}
#endif

#ifdef CONFIG_ZBUS_CHANNEL_ID
/* Channel lookup by numeric id */

static void test_from_id(FAR void **state)
{
  (void)state;

  assert_ptr_equal(zbus_chan_from_id(42), &zbt_chan_c);
  assert_null(zbus_chan_from_id(0xfffffff0));
  assert_null(zbus_chan_from_id(ZBUS_CHAN_ID_INVALID));
}
#endif

/* Iteration over channels and observers */

static bool count_channel(const struct zbus_channel *chan, void *user_data)
{
  int *count = user_data;

  (void)chan;
  (*count)++;
  return true;
}

static bool count_observer(const struct zbus_observer *obs, void *user_data)
{
  int *count = user_data;

  (void)obs;
  (*count)++;
  return true;
}

static void test_iterate(FAR void **state)
{
  int channels = 0;
  int observers = 0;

  (void)state;

  assert_true(zbus_iterate_over_channels_with_user_data(count_channel,
                                                        &channels));
  assert_true(zbus_iterate_over_observers_with_user_data(count_observer,
                                                         &observers));

  /* At least the three test channels and four test observers exist
   * (other zbus users may add more to the image).
   */

  assert_true(channels >= 3);
  assert_true(observers >= 4);
}

/* Message metadata accessors */

static void test_accessors(FAR void **state)
{
  (void)state;

  assert_int_equal(zbus_chan_msg_size(&zbt_chan_a),
                   sizeof(struct zbt_msg_s));
  assert_ptr_equal(zbus_chan_user_data(&zbt_chan_b), &g_user_word);
  assert_null(zbus_chan_user_data(&zbt_chan_a));
}

/* Timeout semantics: subscriber queue overflow reports -ENOMSG on
 * no-wait publish; empty queue reports -ENOMSG (no-wait) or -EAGAIN
 * (timed out).
 */

static void test_timeouts(FAR void **state)
{
  const struct zbus_channel *chan;
  struct zbt_msg_s msg;
  int i;

  (void)state;
  reset_all();

  /* zbt_sub_a queue depth is 4: four publishes succeed... */

  msg.seq = 1;
  for (i = 0; i < 4; i++)
    {
      msg.value = i;
      assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, ZBUS_NO_WAIT), 0);
    }

  /* ...the fifth overflows the subscriber queue.  The publish itself
   * happens (the message is copied and the listener notified); the
   * -ENOMSG is the collected delivery error, matching the Zephyr VDED
   * semantics.
   */

  msg.value = 4;
  assert_int_equal(zbus_chan_pub(&zbt_chan_a, &msg, ZBUS_NO_WAIT), -ENOMSG);
  assert_int_equal(g_listener_a_count, 5);

  /* Drain the four queued notifications */

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(zbus_sub_wait(&zbt_sub_a, &chan, ZBUS_NO_WAIT), 0);
      assert_ptr_equal(chan, &zbt_chan_a);
    }

  /* Empty queue: no-wait -> -ENOMSG, timed -> -EAGAIN */

  assert_int_equal(zbus_sub_wait(&zbt_sub_a, &chan, ZBUS_NO_WAIT), -ENOMSG);
  assert_int_equal(zbus_sub_wait(&zbt_sub_a, &chan, 50), -EAGAIN);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test(test_pub_read_listener_subscriber),
    cmocka_unit_test(test_multi_channel_isolation),
    cmocka_unit_test(test_validator),
#ifdef CONFIG_ZBUS_MSG_SUBSCRIBER
    cmocka_unit_test(test_msg_subscriber),
#endif
    cmocka_unit_test(test_float_payload),
    cmocka_unit_test(test_timer_driven_publisher),
    cmocka_unit_test(test_observer_order),
    cmocka_unit_test(test_claim_finish_notify),
    cmocka_unit_test(test_masks),
    cmocka_unit_test(test_enable_disable),
#ifdef CONFIG_ZBUS_RUNTIME_OBSERVERS
    cmocka_unit_test(test_runtime_observers),
#endif
#ifdef CONFIG_ZBUS_ASYNC_LISTENER
    cmocka_unit_test(test_async_listener),
#endif
#ifdef CONFIG_ZBUS_CHANNEL_NAME
    cmocka_unit_test(test_from_name),
#endif
#ifdef CONFIG_ZBUS_CHANNEL_ID
    cmocka_unit_test(test_from_id),
#endif
    cmocka_unit_test(test_iterate),
    cmocka_unit_test(test_accessors),
    cmocka_unit_test(test_timeouts),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
