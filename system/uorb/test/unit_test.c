/****************************************************************************
 * apps/system/uorb/test/unit_test.c
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

#include <errno.h>
#include <math.h>
#include <poll.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "utility.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LATENCY_THRESHOLD_LOCAL_US  100
#define LATENCY_THRESHOLD_CROSS_US  2000

#define nitems(_a) (sizeof(_a) / sizeof(0[(_a)]))
#define ORB_CLEAN(meta, adv, sub, instance) \
  if (adv >= 0) \
    { \
      orb_unadvertise(adv); \
    } \
  \
  if (sub >= 0) \
    { \
      orb_unsubscribe(sub); \
    } \
  \
  if (orb_unlink_multi(meta, instance) < 0) \
    { \
      test_fail("orb_unlink_multi failed\n"); \
    } \

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_server_should_exit;
static volatile bool g_thread_should_exit;
static volatile int  g_num_messages_sent;
static bool          g_pubsubtest_passed;
static bool          g_pubsubtest_print;
static int           g_pubsubtest_res;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void exit_handler(int signo)
{
  (void)signo;
  g_server_should_exit = true;
}

static FAR void *pubsubtest_thread_entry(FAR void *arg)
{
  /* poll on test topic and output latency */

  struct pollfd fds[1];
  struct orb_test_medium_s t;
  struct sensor_state_s state;

  static const unsigned MAX_RUNS = 1000;
  float latency_integral = 0.0f;
  float std_dev = 0.f;
  float mean;
  int test_multi_sub;
  int current_value;
  int num_missed = 0;
  unsigned timingsgroup = 0;
  FAR unsigned *timings;
  unsigned timing_min = 9999999;
  unsigned timing_max = 0;
  unsigned multiple = 1;
  unsigned i;

  (void)arg;
  timings = malloc(MAX_RUNS * sizeof(unsigned));
  if (timings == NULL)
    {
      return NULL;
    }

  /* clear all ready flags */

  test_multi_sub = orb_subscribe_multi(ORB_ID(orb_test_medium), 0);

  /* Query topic state to detect cross-core (pingpong) scenario.
   * If no local advertiser exists (nadvertisers <= 0), the message
   * travels across cores via rpmsg round-trip, so divide the measured
   * latency by 2 to get one-way latency.
   */

  if (OK == orb_ioctl(test_multi_sub, SNIOC_GET_STATE,
                      (unsigned long)(uintptr_t)&state))
    {
      if (state.nadvertisers <= 0)
        {
          multiple = 2;
        }
    }

  test_note("latency test multiple:%u", multiple);

  orb_copy(ORB_ID(orb_test_medium), test_multi_sub, &t);

  fds[0].fd     = test_multi_sub;
  fds[0].events = POLLIN;
  current_value = t.val;

  for (i = 0; i < MAX_RUNS; i++)
    {
      int pret;

      /* wait for up to 500ms for data */

      pret = poll(&fds[0], nitems(fds), 500);
      if (fds[0].revents & POLLIN)
        {
          unsigned elt;

          orb_copy(ORB_ID(orb_test_medium), test_multi_sub, &t);

          /* Divide elapsed time by multiple to get one-way latency
           * in cross-core pingpong scenario (multiple=2).
           */

          elt = (unsigned)orb_elapsed_time(&t.timestamp) / multiple;
          latency_integral += elt;
          timings[i] = elt;

          if (elt > timing_max)
            {
              timing_max = elt;
            }

          if (elt < timing_min)
            {
              timing_min = elt;
            }

          timingsgroup  = 0;
          num_missed   += t.val - current_value - 1;
          current_value = t.val;
        }

      if (pret < 0)
        {
          printf("poll error %d, %d", pret, errno);
          continue;
        }
    }

  orb_unsubscribe(test_multi_sub);

  if (g_pubsubtest_print)
    {
      char fname[32];
      FAR FILE *f;

      snprintf(fname, sizeof(fname),
               UORB_STORAGE_DIR"/uorb_timings%u.txt", timingsgroup);

      f = fopen(fname, "w");
      if (f == NULL)
        {
          printf("Error opening file!");
          free(timings);
          return NULL;
        }

      for (i = 0; i < MAX_RUNS; i++)
        {
          fprintf(f, "%u\n", timings[i]);
        }

      fclose(f);
    }

  std_dev = 0.f;
  mean    = latency_integral / MAX_RUNS;

  for (i = 0; i < MAX_RUNS; i++)
    {
      float diff;

      diff     = (float)timings[i] - mean;
      std_dev += diff * diff;
    }

  printf("mean:    %u us\n", (unsigned)(mean));
  printf("std dev: %u us\n", (unsigned)(sqrtf(std_dev / (MAX_RUNS - 1))));
  printf("min:     %u us\n", timing_min);
  printf("max:     %u us\n", timing_max);
  printf("missed topic updates: %i\n", num_missed);

  g_pubsubtest_passed = true;

  /* Check if average latency exceeds threshold:
   *   - Local (multiple==1): fail if mean > 100 us
   *   - Cross-core pingpong (multiple==2): fail if mean > 2000 us
   */

  if (latency_integral / MAX_RUNS >
      (multiple == 1 ? LATENCY_THRESHOLD_LOCAL_US
                     : LATENCY_THRESHOLD_CROSS_US))
    {
      g_pubsubtest_res = ERROR;
    }
  else
    {
      g_pubsubtest_res = OK;
    }

  free(timings);
  return NULL;
}

static int server_test(void)
{
  struct orb_test_medium_s ping;
  struct pollfd fds;
  int instance = 0;
  int ret = ERROR;
  int ping_fd;
  int pang_fd;

  /* Subscribe to instance 1 to receive ping messages from the client */

  ping_fd = orb_subscribe_multi(ORB_ID(orb_test_medium), 1);
  if (ping_fd < 0)
    {
      return test_fail("server subscribe failed: %d", ping_fd);
    }

  /* Advertise a new instance to publish pong (echo) messages back */

  pang_fd = orb_advertise_multi(ORB_ID(orb_test_medium), NULL, &instance);
  if (pang_fd < 0)
    {
      orb_unsubscribe(ping_fd);
      return test_fail("server advertise failed: %d, errno:%d", pang_fd,
                       errno);
    }

  fds.fd     = ping_fd;
  fds.events = POLLIN;
  test_note("Pingpang server start!");

  /* Main server loop: wait for ping, then immediately echo back as pong */

  while (!g_server_should_exit)
    {
      if (poll(&fds, 1, -1) > 0)
        {
          if (fds.revents & POLLIN)
            {
              ret = orb_copy(ORB_ID(orb_test_medium), fds.fd, &ping);
              if (ret == OK)
                {
                  ret = orb_publish(ORB_ID(orb_test_medium), pang_fd,
                                    &ping);
                  if (ret != OK)
                    {
                      test_note("Pingpang server data forwarding error! "
                                "time:%" PRIu64 ", current time:%" PRIu64 ","
                                "val:%" PRId32 ", ret:%d errno:%d",
                                ping.timestamp, orb_absolute_time(),
                                ping.val, ret, errno);
                    }
                }
              else
                {
                  test_note("server orb copy failed! ret:%d, errno:%d", ret,
                            errno);
                }
            }
          else
            {
              test_note("server receive bad value! fds.revents:%#X",
                        fds.revents);
            }
        }
    }

  /* Cleanup server resources */

  orb_unsubscribe(ping_fd);
  orb_unadvertise(pang_fd);
  orb_unlink(ORB_ID(orb_test_medium));
  test_note("pingpang server exit");
  return OK;
}

static int latency_test(bool print, int instance)
{
  struct orb_test_medium_s sample;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t pubsub_task;
  int ret = ERROR;
  int fd;

  test_note("---------------- LATENCY TEST ------------------");

  sample.val       = 308;
  sample.timestamp = orb_absolute_time();

  fd = orb_advertise_multi_queue_persist(ORB_ID(orb_test_medium),
                                         &sample, &instance, 1);
  if (fd < 0)
    {
      return test_fail("orb_advertise failed (%i)", errno);
    }

  g_pubsubtest_print  = print;
  g_pubsubtest_passed = false;

  pthread_attr_init(&attr);
  param.sched_priority = CONFIG_UORB_PRIORITY + 1;
  pthread_attr_setschedparam(&attr, &param);

  if (pthread_create(&pubsub_task, &attr, pubsubtest_thread_entry, NULL) < 0)
    {
      test_fail("failed launching latency test task");
      goto out;
    }

  /* give the test task some data */

  while (!g_pubsubtest_passed)
    {
      ++sample.val;
      sample.timestamp = orb_absolute_time();
      if (OK != orb_publish(ORB_ID(orb_test_medium), fd, &sample))
        {
          test_fail("mult. pub0 timing fail");
          goto out;
        }

      usleep(1000); /* simulate >800 Hz system operation */
    }

  ret = OK;

out:
  orb_unadvertise(fd);
  orb_unlink(ORB_ID(orb_test_medium));
  return ret < 0 ? ret : g_pubsubtest_res;
}

static int test_single(void)
{
  struct orb_test_s sample;
  struct orb_test_s sub_sample;
  int instance = 0;
  bool updated;
  int afd;
  int sfd;
  int ret = ERROR;

  test_note("try single-topic support");

  /* advertise, then subscribe */

  sample.val = 0;

  afd = orb_advertise_multi_queue_persist(ORB_ID(orb_test),
                                          &sample, &instance, 1);
  if (afd < 0)
    {
      return test_fail("advertise failed: %d", errno);
    }

  sfd = orb_subscribe(ORB_ID(orb_test));
  if (sfd < 0)
    {
      orb_unadvertise(afd);
      return test_fail("subscribe failed: %d", errno);
    }

  /* check first publish */

  sub_sample.val = 1;

  if (OK != orb_copy(ORB_ID(orb_test), sfd, &sub_sample))
    {
      test_fail("copy(1) failed: %d", errno);
      goto out;
    }

  if (sample.val != sub_sample.val)
    {
      test_fail("copy(1) mismatch: %d expected %d",
                sub_sample.val, sample.val);
      goto out;
    }

  if (OK != orb_check(sfd, &updated))
    {
      test_fail("check(1) failed");
      goto out;
    }

  if (updated)
    {
      test_fail("spurious updated flag");
      goto out;
    }

  /* check second publish */

  sample.val = 2;

  if (OK != orb_publish(ORB_ID(orb_test), afd, &sample))
    {
      test_fail("publish failed");
      goto out;
    }

  if (OK != orb_check(sfd, &updated))
    {
      test_fail("check(2) failed");
      goto out;
    }

  if (!updated)
    {
      test_fail("missing updated flag");
      goto out;
    }

  if (OK != orb_copy(ORB_ID(orb_test), sfd, &sub_sample))
    {
      test_fail("copy(2) failed: %d", errno);
      goto out;
    }

  if (sample.val != sub_sample.val)
    {
      test_fail("copy(2) mismatch: %d expected %d",
                sub_sample.val, sample.val);
      goto out;
    }

  /* unadvertise and out */

out:
  ret = orb_unadvertise(afd);
  if (ret != OK)
    {
      test_fail("orb_unadvertise failed: %i", ret);
    }

  ret = orb_unsubscribe(sfd);
  if (ret != OK)
    {
      test_fail("orb_unsubscribe failed: %i", ret);
    }

  if (ret == OK)
    {
      test_note("PASS single-topic test");
    }

  orb_unlink_multi(ORB_ID(orb_test), instance);
  return ret;
}

static int test_multi_inst10(void)
{
  const int max_inst = 10;
  int sfd[max_inst];
  int afd[max_inst];
  int ret = ERROR;
  int i;
  int j;

  for (i = 0; i < max_inst; i++)
    {
      sfd[i] = orb_subscribe_multi(ORB_ID(orb_test), i);
    }

  /* verify not advertised yet */

  for (i = 0; i < max_inst; i++)
    {
      if (OK == orb_exists(ORB_ID(orb_test), i))
        {
          test_fail("sub %d is advertised", i);
          goto out_with_sfd;
        }
    }

  /* advertise one at a time and verify instance */

  for (i = 0; i < max_inst; i++)
    {
      afd[i] = orb_advertise_multi_queue_persist(ORB_ID(orb_test),
                                                 NULL, &i, 1);
    }

  /* verify advertised yet */

  for (i = 0; i < max_inst; i++)
    {
      if (OK != orb_exists(ORB_ID(orb_test), i))
        {
          test_fail("sub %d advertise failed", i);
          goto out_with_sfd;
        }
    }

  /* publish one at a time and verify */

  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 10; j++)
        {
          int instance;
          int sub_instance;

          for (instance = 0; instance < max_inst; instance++)
            {
              struct orb_test_s sample;

              sample.val       = j * instance + i;
              sample.timestamp = orb_absolute_time();

              orb_publish(ORB_ID(orb_test), afd[instance], &sample);
            }

          for (sub_instance = 0; sub_instance < max_inst; sub_instance++)
            {
              bool updated = false;

              orb_check(sfd[sub_instance], &updated);
              if (!updated)
                {
                  test_fail("sub %d not updated", sub_instance);
                  goto out;
                }
              else
                {
                  struct orb_test_s sample;

                  if (orb_copy(ORB_ID(orb_test), sfd[sub_instance], &sample))
                    {
                      test_fail("sub %d copy failed", sub_instance);
                      goto out;
                    }
                  else
                    {
                      if (sample.val != (j * sub_instance + i))
                        {
                          test_fail("sub %d invalid value %d",
                                    sub_instance, sample.val);
                          goto out;
                        }
                    }
                }
            }
        }
    }

  ret = OK;

out:
  for (i = 0; i < max_inst; i++)
    {
      orb_unadvertise(afd[i]);
    }

out_with_sfd:
  for (i = 0; i < max_inst; i++)
    {
      orb_unsubscribe(sfd[i]);
    }

  if (ret == OK)
    {
      test_note("PASS orb 10-instances");
    }

  for (i = 0; i < max_inst; i++)
    {
      orb_unlink_multi(ORB_ID(orb_test), i);
    }

  return ret;
}

static int test_multi(int *afds, int *sfds)
{
  struct orb_test_s sub_sample;
  int instance0 = 0;
  int instance1 = 1;
  struct orb_test_s sample =
    {
      0
    };

  /* this routine tests the multi-topic support */

  test_note("try multi-topic support");

  afds[0] = orb_advertise_multi_queue_persist(ORB_ID(orb_multitest),
                                              &sample, &instance0, 1);

  if (instance0 != 0)
    {
      return test_fail("mult. id0: %d", instance0);
    }

  afds[1] = orb_advertise_multi_queue_persist(ORB_ID(orb_multitest),
                                              &sample, &instance1, 1);

  if (instance1 != 1)
    {
      return test_fail("mult. id1: %d", instance1);
    }

  sample.val = 103;

  if (OK != orb_publish(ORB_ID(orb_multitest), afds[0], &sample))
    {
      return test_fail("mult. pub0 fail");
    }

  sample.val = 203;

  if (OK != orb_publish(ORB_ID(orb_multitest), afds[1], &sample))
    {
      return test_fail("mult. pub1 fail");
    }

  /* subscribe to both topics and ensure valid data is received */

  sfds[0] = orb_subscribe_multi(ORB_ID(orb_multitest), 0);

  if (OK != orb_copy(ORB_ID(orb_multitest), sfds[0], &sub_sample))
    {
      return test_fail("sub #0 copy failed: %d", errno);
    }

  if (sub_sample.val != 103)
    {
      return test_fail("sub #0 val. mismatch: %d", sub_sample.val);
    }

  sfds[1] = orb_subscribe_multi(ORB_ID(orb_multitest), 1);

  if (OK != orb_copy(ORB_ID(orb_multitest), sfds[1], &sub_sample))
    {
      return test_fail("sub #1 copy failed: %d", errno);
    }

  if (sub_sample.val != 203)
    {
      return test_fail("sub #1 val. mismatch: %d", sub_sample.val);
    }

  orb_unsubscribe(sfds[0]);
  orb_unsubscribe(sfds[1]);

  return test_note("PASS multi-topic test");
}

static int test_multi_reversed(int *afds, int *sfds)
{
  struct orb_test_s sub_sample;
  int instance2 = 2;
  int instance3 = 3;
  int ret = ERROR;
  struct orb_test_s sample =
    {
      0
    };

  test_note("try multi-topic support subscribing before publishing");

  /* For these tests 0 and 1 instances are taken from before,
   * therefore continue with 2 and 3,
   * subscribe first and advertise afterwards.
   */

  sfds[2] = orb_subscribe_multi(ORB_ID(orb_multitest), 2);

  if (sfds[2] < 0)
    {
      return test_fail("sub. id2: ret: %d", sfds[2]);
    }

  afds[2] = orb_advertise_multi_queue_persist(ORB_ID(orb_multitest),
                                              &sample, &instance2, 1);

  if (instance2 != 2)
    {
      test_fail("mult. id2: %d", instance2);
      goto out2;
    }

  afds[3] = orb_advertise_multi_queue_persist(ORB_ID(orb_multitest),
                                              &sample, &instance3, 1);

  if (instance3 != 3)
    {
      test_fail("mult. id3: %d", instance3);
      goto out2;
    }

  sample.val = 204;

  if (OK != orb_publish(ORB_ID(orb_multitest), afds[2], &sample))
    {
      test_fail("mult. pub2 fail");
      goto out2;
    }

  sample.val = 304;

  if (OK != orb_publish(ORB_ID(orb_multitest), afds[3], &sample))
    {
      test_fail("mult. pub3 fail");
      goto out2;
    }

  /* ensure valid data is received */

  if (OK != orb_copy(ORB_ID(orb_multitest), sfds[2], &sub_sample))
    {
      test_fail("sub #2 copy failed: %d", errno);
      goto out2;
    }

  if (sub_sample.val != 204)
    {
      test_fail("sub #2 val. mismatch: %d", sub_sample.val);
      goto out2;
    }

  sfds[3] = orb_subscribe_multi(ORB_ID(orb_multitest), 3);

  if (sfds[3] < 0)
    {
      test_fail("sub. id3: ret: %d", sfds[3]);
      goto out2;
    }

  if (OK != orb_copy(ORB_ID(orb_multitest), sfds[3], &sub_sample))
    {
      test_fail("sub #3 copy failed: %d", errno);
      goto out3;
    }

  if (sub_sample.val != 304)
    {
      test_fail("sub #3 val. mismatch: %d", sub_sample.val);
      goto out3;
    }

  ret = OK;
  test_note("PASS multi-topic reversed");

out3:
  orb_unsubscribe(sfds[3]);

out2:
  orb_unsubscribe(sfds[2]);
  return ret;
}

static int test_unadvertise(int *afds)
{
  int ret;
  int i;

  test_note("Testing unadvertise");

  /* we still have the advertisements from the previous test_multi calls. */

  for (i = 0; i < 4; ++i)
    {
      ret = orb_unadvertise(afds[i]);
      if (ret != OK)
        {
          return test_fail("orb_unadvertise failed (%i)", ret);
        }

      orb_unlink_multi(ORB_ID(orb_multitest), i);
    }

  return OK;
}

static FAR void *pub_test_multi2_entry(FAR void *arg)
{
  struct orb_test_medium_s data_topic;
  const int num_instances = 3;
  int data_next_idx = 0;
  int orb_pub[num_instances];
  int message_counter = 0;
  int num_messages = 50 * num_instances;
  int i;

  (void)arg;
  memset(&data_topic, '\0', sizeof(data_topic));
  for (i = 0; i < num_instances; ++i)
    {
      orb_pub[i] = orb_advertise_multi_queue_persist(
        ORB_ID(orb_test_multi), &data_topic, &i, 1);
    }

  usleep(100 * 1000);

  while (message_counter++ < num_messages)
    {
      usleep(2); /* make sure the timestamps are different */

      data_topic.timestamp = orb_absolute_time();
      data_topic.val       = data_next_idx;

      orb_publish(ORB_ID(orb_test_multi), orb_pub[data_next_idx],
                  &data_topic);

      data_next_idx = (data_next_idx + 1) % num_instances;
      if (data_next_idx == 0)
        {
          usleep(50 * 1000);
        }
    }

  usleep(100 * 1000);
  for (i = 0; i < num_instances; ++i)
    {
      orb_unadvertise(orb_pub[i]);
    }

  g_thread_should_exit = true;
  return NULL;
}

static int test_multi2(void)
{
  const int num_instances = 3;
  int orb_data_fd[num_instances];
  int orb_data_next     = 0;
  orb_abstime last_time = 0;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t pubsub_task;
  int ret = ERROR;
  int i;

  test_note("Testing multi-topic 2 test (queue simulation)");

  g_thread_should_exit = false;

  /* test: first subscribe, then advertise */

  for (i = 0; i < num_instances; ++i)
    {
      orb_data_fd[i] = orb_subscribe_multi(ORB_ID(orb_test_multi), i);
    }

  /* launch the publisher thread */

  pthread_attr_init(&attr);
  param.sched_priority = sched_get_priority_max(0) - 5;
  pthread_attr_setschedparam(&attr, &param);

  if (pthread_create(&pubsub_task, &attr, pub_test_multi2_entry, NULL) < 0)
    {
      pthread_attr_destroy(&attr);
      test_fail("failed launching test multi task");
      goto out;
    }

  pthread_attr_destroy(&attr);

  /* loop check update and copy new data */

  while (!g_thread_should_exit)
    {
      bool updated        = false;
      int orb_data_cur_fd = orb_data_fd[orb_data_next];

      usleep(1000);

      orb_check(orb_data_cur_fd, &updated);
      if (updated)
        {
          struct orb_test_medium_s msg =
            {
              0, 0
            };

          if (OK != orb_copy(ORB_ID(orb_test_multi),
                             orb_data_cur_fd, &msg))
            {
              test_fail("copy failed: %d", errno);
              goto out;
            }

          if (last_time >= msg.timestamp && last_time != 0)
            {
              test_fail("Timestamp not increasing! (%" PRIu64
                        " >= %" PRIu64 ")", last_time, msg.timestamp);
              goto out;
            }

          last_time     = msg.timestamp;
          orb_data_next = (orb_data_next + 1) % num_instances;
        }
    }

  ret = OK;

out:
  for (i = 0; i < num_instances; ++i)
    {
      orb_unsubscribe(orb_data_fd[i]);
      orb_unlink_multi(ORB_ID(orb_test_multi), i);
    }

  if (ret == OK)
    {
      test_note("PASS multi-topic 2 test (queue simulation)");
    }

  return ret;
}

int test_queue(void)
{
  const int queue_size  = 16;
  const int overflow_by = 3;
  struct orb_test_medium_s sub_sample;
  bool updated;
  int instance = 0;
  int ptopic;
  int sfd;
  int ret = ERROR;
  int i;
  struct orb_test_medium_s sample =
    {
      0
    };

  test_note("Testing orb queuing");

  sfd = orb_subscribe(ORB_ID(orb_test_queue));

  if (sfd < 0)
    {
      return test_fail("subscribe failed: %d", errno);
    }

  /* Get all published messages,
   * ensure that publish and subscribe message match
   */

  do
    {
      orb_check(sfd, &updated);
      if (updated)
        {
          orb_copy(ORB_ID(orb_test_queue), sfd, &sub_sample);
        }
    }
  while (updated);

  ptopic = orb_advertise_multi_queue_persist(
    ORB_ID(orb_test_queue), &sample, &instance, queue_size);
  if (ptopic < 0)
    {
      orb_unsubscribe(sfd);
      return test_fail("advertise failed: %d", errno);
    }

  orb_check(sfd, &updated);
  if (!updated)
    {
      test_fail("update flag not set");
      goto out;
    }

  if (OK != orb_copy(ORB_ID(orb_test_queue), sfd, &sub_sample))
    {
      test_fail("copy(1) failed: %d", errno);
      goto out;
    }

  if (sub_sample.val != sample.val)
    {
      test_fail("copy(1) mismatch: %d expected %d",
                sub_sample.val, sample.val);
      goto out;
    }

  orb_check(sfd, &updated);
  if (updated)
    {
      test_fail("spurious updated flag");
      goto out;
    }

#define CHECK_UPDATED(element) \
  orb_check(sfd, &updated); \
  if (!updated) \
    { \
      test_fail("update flag not set, element %i", element); \
      goto out; \
    }

#define CHECK_NOT_UPDATED(element) \
  orb_check(sfd, &updated); \
  if (updated) \
    { \
      test_fail("update flag set, element %i", element); \
      goto out; \
    }

#define CHECK_COPY(i_got, i_correct) \
  orb_copy(ORB_ID(orb_test_queue), sfd, &sub_sample); \
  if (i_got != i_correct) \
    { \
      test_fail("got wrong element from the queue (got %i," \
                "should be %i)", i_got, i_correct); \
      goto out; \
    }

  /* no messages in the queue anymore */

  test_note("  Testing to write some elements...");

  for (i = 0; i < queue_size - 2; ++i)
    {
      sample.val = i;
      orb_publish(ORB_ID(orb_test_queue), ptopic, &sample);
    }

  for (i = 0; i < queue_size - 2; ++i)
    {
      CHECK_UPDATED(i);
      CHECK_COPY(sub_sample.val, i);
    }

  CHECK_NOT_UPDATED(queue_size);

  test_note("  Testing overflow...");

  for (i = 0; i < queue_size + overflow_by; ++i)
    {
      sample.val = i;
      orb_publish(ORB_ID(orb_test_queue), ptopic, &sample);
    }

  for (i = 0; i < queue_size; ++i)
    {
      CHECK_UPDATED(i);
      CHECK_COPY(sub_sample.val, i + overflow_by);
    }

  CHECK_NOT_UPDATED(queue_size);

  test_note("  Testing underflow...");

  for (i = 0; i < queue_size; ++i)
    {
      CHECK_NOT_UPDATED(i);
      CHECK_COPY(sub_sample.val, queue_size + overflow_by - 1);
    }

  sample.val = 943;

  orb_publish(ORB_ID(orb_test_queue), ptopic, &sample);

  CHECK_UPDATED(-1);
  CHECK_COPY(sub_sample.val, sample.val);

#undef CHECK_COPY
#undef CHECK_UPDATED
#undef CHECK_NOT_UPDATED

  ret = OK;

out:
  orb_unadvertise(ptopic);
  orb_unsubscribe(sfd);
  orb_unlink(ORB_ID(orb_test_queue));

  if (ret == OK)
    {
      test_note("PASS orb queuing");
    }

  return ret;
}

static FAR void *pub_test_queue_entry(FAR void *arg)
{
  const int queue_size = 50;
  struct orb_test_medium_s t;
  int num_messages = 20 * queue_size;
  int message_counter = 0;
  int instance = 0;
  int ptopic;

  (void)arg;
  memset(&t, '\0', sizeof(t));
  ptopic = orb_advertise_multi_queue_persist(
    ORB_ID(orb_test_queue_poll), &t, &instance, queue_size);
  if (ptopic < 0)
    {
      g_thread_should_exit = true;
      test_fail("advertise failed: %d", errno);
      return NULL;
    }

  ++t.val;

  while (message_counter < num_messages)
    {
      /* simulate burst */

      int burst_counter = 0;

      while (burst_counter++ < queue_size / 2 + 7)
        {
          /* make interval non-boundary aligned */

          orb_publish(ORB_ID(orb_test_queue_poll), ptopic, &t);
          ++t.val;
        }

      message_counter += burst_counter;
      usleep(50 * 1000); /* give subscriber a chance to catch up */
    }

  g_num_messages_sent  = t.val;
  g_thread_should_exit = true;
  g_pubsubtest_passed  = true;
  usleep(100 * 1000);
  orb_unadvertise(ptopic);
  orb_unlink(ORB_ID(orb_test_queue_poll));
  return NULL;
}

static int test_queue_poll_notify(void)
{
  struct pollfd fds[1];
  struct orb_test_medium_s t;
  struct sched_param param;
  pthread_attr_t attr;
  bool updated;
  int next_expected_val = 0;
  pthread_t pubsub_task;
  int sfd;

  test_note("Testing orb queuing (poll & notify)");

  if ((sfd = orb_subscribe(ORB_ID(orb_test_queue_poll))) < 0)
    {
      return test_fail("subscribe failed: %d", errno);
    }

  /* Get all published messages,
   * ensure that publish and subscribe message match
   */

  do
    {
      orb_check(sfd, &updated);
      if (updated)
        {
          orb_copy(ORB_ID(orb_test_queue_poll), sfd, &t);
        }
    }
  while (updated);

  g_pubsubtest_passed  = false;
  g_thread_should_exit = false;

  pthread_attr_init(&attr);
  param.sched_priority = CONFIG_UORB_PRIORITY + 1;
  pthread_attr_setschedparam(&attr, &param);

  if (pthread_create(&pubsub_task, &attr, pub_test_queue_entry, NULL) < 0)
    {
      orb_unsubscribe(sfd);
      pthread_attr_destroy(&attr);
      return test_fail("failed test queue task");
    }

  pthread_attr_destroy(&attr);

  fds[0].fd     = sfd;
  fds[0].events = POLLIN;

  while (!g_thread_should_exit)
    {
      int poll_ret;

      poll_ret = poll(fds, 1, 500);
      if (poll_ret == 0)
        {
          if (g_thread_should_exit)
            {
              break;
            }

          orb_unsubscribe(sfd);
          return test_fail("poll timeout");
        }
      else if (poll_ret < 0 && errno != EINTR)
        {
          orb_unsubscribe(sfd);
          return test_fail("poll error (%d, %d)", poll_ret, errno);
        }

      if (fds[0].revents & POLLIN)
        {
          orb_copy(ORB_ID(orb_test_queue_poll), sfd, &t);
          if (next_expected_val != t.val)
            {
              orb_unsubscribe(sfd);
              return test_fail("copy mismatch: %d expected %d",
                               t.val, next_expected_val);
            }

          ++next_expected_val;
        }
    }

  if (g_num_messages_sent != next_expected_val)
    {
      orb_unsubscribe(sfd);
      return test_fail("number of sent and received messages mismatch"
                       " (sent: %i, received: %i)",
                       g_num_messages_sent, next_expected_val);
    }

  orb_unsubscribe(sfd);

  return g_pubsubtest_passed ? OK : ERROR;
}

/* test orb_subscribe_multi orb_subscribe_wakeup orb_subscribe
 * orb_advertise_multi orb_advertise_queue orb_advertise orb_publish_auto
 */

#define ASSERT_TEST_FAIL(str, instance, ret, state, t, r) \
  if (ret < 0 || r.timestamp != t.timestamp || r.val != t.val) \
    { \
      test_fail("test advertise test %s failed! ret:%d,r.t:%" PRIu64 "," \
                "t.t:%" PRIu64 ",r.v:%" PRIu32 ",t.v:%" PRIu32 "," \
                "instance:%d,errno:%d", str, ret, r.timestamp, t.timestamp, \
                r.val, t.val, instance, errno); \
      state = ERROR; \
    } \

static int test_advsub(void)
{
  struct orb_test_medium_s t;
  struct orb_test_medium_s r;
  const int queue_size = 10;
  int instance = 2;
  int state = OK;
  int adv;
  int sub;
  int ret;
  int i;

  t.timestamp = 10086;
  t.val = 110;

  test_note("Testing subscribe and advertise family API");

  /* orb_subscribe_multi and orb_advertise_multi */

  sub = orb_subscribe_multi(ORB_ID(orb_test_medium), instance);
  if (sub < 0)
    {
      return test_fail("test subscribe multi failed ret:%d, errno:%d",
                       sub, errno);
    }

  adv = orb_advertise_multi(ORB_ID(orb_test_medium), &t, &instance);
  if (adv < 0)
    {
      ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, instance);
      return test_fail("test advertise multi failed ret:%d, errno:%d",
                       adv, errno);
    }

  ret = orb_copy(ORB_ID(orb_test_medium), sub, &r);
  ASSERT_TEST_FAIL("advertise&subscribe multi", instance, ret, state, t, r);
  ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, instance);

  /* orb_subscribe and orb_advertise */

  sub = orb_subscribe(ORB_ID(orb_test_medium));
  if (sub < 0)
    {
      return test_fail("test subscribe failed ret:%d, errno:%d",
                       sub, errno);
    }

  adv = orb_advertise(ORB_ID(orb_test_medium), &t);
  if (adv < 0)
    {
      ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, 0);
      return test_fail("test advertise failed ret:%d, errno:%d",
                       adv, errno);
    }

  ret = orb_copy(ORB_ID(orb_test_medium), sub, &r);
  ASSERT_TEST_FAIL("advertise&subscribe", 0, ret, state, t, r);
  ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, 0);

  /* orb_advertise_queue and orb_subscribe_wakeup */

  sub = orb_subscribe_wakeup(ORB_ID(orb_test_medium));
  if (sub < 0)
    {
      return test_fail("test subscribe wakeup failed ret:%d, errno:%d",
                       sub, errno);
    }

  adv = orb_advertise_queue(ORB_ID(orb_test_medium), NULL, queue_size);
  if (adv < 0)
    {
      ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, 0);
      return test_fail("test advertise queue failed ret:%d, errno:%d",
                       adv, errno);
    }

  for (i = -queue_size; i < queue_size; ++i)
    {
      t.timestamp++;
      t.val++;
      orb_publish(ORB_ID(orb_test_medium), adv, &t);
    }

  t.timestamp -= queue_size;
  t.val -= queue_size;
  for (i = 0; i < queue_size; ++i)
    {
      t.timestamp++;
      t.val++;
      ret = orb_copy(ORB_ID(orb_test_medium), sub, &r);
      ASSERT_TEST_FAIL("advertise_queue&subscribe_wakeup copy", 0, ret,
                       state, t, r);
    }

  ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, 0);

  /* orb_subscribe and orb_publish_auto */

  instance = 3;
  sub = orb_subscribe_multi(ORB_ID(orb_test_medium), instance);
  if (sub < 0)
    {
      return test_fail("test subscribe failed ret:%d, errno:%d", sub, errno);
    }

  adv = orb_publish_auto(ORB_ID(orb_test_medium), NULL, &t, &instance);
  if (adv < 0 || instance != 3)
    {
      ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, instance);
      return test_fail("test publish auto failed ret:%d, errno:%d",
                       adv, errno);
    }

  ret = orb_copy(ORB_ID(orb_test_medium), sub, &r);
  ASSERT_TEST_FAIL("publish_auto&subscribe copy",
                   instance, ret, state, t, r);
  ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, instance);
  if (state == OK)
    {
      test_note("PASS advertise and subscribe");
    }

  return ret;
}

#if CONFIG_UORB_LOOP_MAX_EVENTS
static int data_pollin_cb(FAR struct orb_handle_s *handle, FAR void *arg)
{
  FAR struct orb_test_loop_s *test = (FAR struct orb_test_loop_s *)arg;
  struct orb_test_medium_s t;
  int ret;

  ret = orb_copy(ORB_ID(orb_test_medium), handle->fd, &t);
  if (ret < 0)
    {
      test_fail("test loop cb copy failed instance:%d, ret:%d, errno:%d",
                test->instance, ret, errno);
    }

  if (test->last_val == 0)
    {
      test->last_val = t.val;
      test->cout = 1;
    }
  else
    {
      ret = t.val - test->last_val;
      test->last_val = t.val;
      test->cout++;
      if (ret != 1)
        {
          test_fail("test loop cb data failed! miss:%d, instance:%d, ret:%d,"
                    "errno:%d", ret - 1, test->instance, ret, errno);
        }
    }

  return 0;
}

static int data_pollpri_cb(FAR struct orb_handle_s *handle, FAR void *arg)
{
  FAR struct orb_test_loop_s *test = (FAR struct orb_test_loop_s *)arg;
  unsigned int events = 0;
  int ret;

  ret = orb_get_events(handle->fd, &events);

  if ((ret >= 0) && (events & SENSOR_EVENT_FLUSH_COMPLETE))
    {
      test->timestamp = orb_elapsed_time(&test->timestamp);
      test->flush = true;
    }

  return 0;
}

static FAR void *loop_entry(FAR void *arg)
{
  struct orb_loop_s *loop = (struct orb_loop_s *)arg;
  int ret;

  test_note("orb loop running......");
  ret = orb_loop_run(loop);
  if (ret < 0)
    {
      test_fail("failed to run orb loop! ret:%d, errno:%d", ret, errno);
    }

  test_note("orb loop exit!");
  return NULL;
}

static int test_orb_loop(void)
{
  struct orb_test_loop_s before_start;
  struct orb_test_loop_s after_start;
  struct orb_test_medium_s temp;
  struct orb_loop_s orb_loop;
  const int queue_size = 10;
  struct sched_param param;
  pthread_t pubsub_task;
  pthread_attr_t attr;
  bool thread_created = false;
  int ret;
  int i;

  test_note("Testing orb loop (subscribe callback, loop start and exit)");

  memset(&before_start, 0, sizeof(before_start));
  memset(&after_start, 0, sizeof(after_start));
  before_start.instance = 5;
  before_start.sub = -1;
  before_start.adv = -1;
  after_start.instance = 6;
  after_start.sub = -1;
  after_start.adv = -1;

  before_start.sub = orb_subscribe_multi(ORB_ID(orb_test_medium),
                                         before_start.instance);
  if (before_start.sub < 0)
    {
      test_fail("subscribe before loop start failed! ret:%d, errno:%d",
                before_start.sub, errno);
      return ERROR;
    }

  ret = orb_loop_init(&orb_loop, ORB_EPOLL_TYPE);
  if (ret < 0)
    {
      test_fail("failed init orb loop! ret:%d, errno:%d", ret, errno);
      goto clean_before;
    }

  before_start.loop = &orb_loop;
  after_start.loop  = &orb_loop;
  ret = orb_handle_init(&before_start.handle, before_start.sub,
                        EPOLLIN | EPOLLPRI, &before_start, data_pollin_cb,
                        NULL, data_pollpri_cb, NULL);
  if (ret < 0)
    {
      test_fail("failed init handler[before_start]! ret:%d, errno:%d",
                ret, errno);
      goto clean_before;
    }

  ret = orb_handle_start(&orb_loop, &before_start.handle);
  if (ret < 0)
    {
      test_fail("failed start handler[before_start]! ret:%d, errno:%d",
                ret, errno);
      goto clean_before;
    }

  /* launch the orb loop subscribe thread */

  pthread_attr_init(&attr);
  param.sched_priority = sched_get_priority_max(0) - 5;
  pthread_attr_setschedparam(&attr, &param);

  if (pthread_create(&pubsub_task, &attr, loop_entry, &orb_loop) < 0)
    {
      pthread_attr_destroy(&attr);
      test_fail("failed launching test multi task");
      goto clean_loop;
    }

  pthread_attr_destroy(&attr);
  thread_created = true;

  before_start.adv = orb_advertise_multi_queue(ORB_ID(orb_test_medium),
                                               NULL,
                                               &before_start.instance,
                                               queue_size);
  if (before_start.adv < 0)
    {
      test_fail("test before_start advertise failed ret:%d, errno:%d",
                before_start.adv, errno);
      goto clean_loop;
    }

  after_start.sub = orb_subscribe_multi(ORB_ID(orb_test_medium),
                                        after_start.instance);
  if (after_start.sub < 0)
    {
      test_fail("subscribe after loop start failed! ret:%d, errno:%d",
                after_start.sub, errno);
      goto clean_loop;
    }

  ret = orb_handle_init(&after_start.handle, after_start.sub,
                        EPOLLIN | EPOLLPRI, &after_start, data_pollin_cb,
                        NULL, data_pollpri_cb, NULL);
  if (ret < 0)
    {
      test_fail("failed init handler[after_start]! ret:%d, errno:%d",
                ret, errno);
      goto clean_loop;
    }

  ret = orb_handle_start(&orb_loop, &after_start.handle);
  if (ret < 0)
    {
      test_fail("failed start handler[after_start]! ret:%d, errno:%d",
                ret, errno);
      goto clean_loop;
    }

  after_start.adv = orb_advertise_multi_queue(ORB_ID(orb_test_medium),
                                              NULL,
                                              &after_start.instance,
                                              queue_size);
  if (after_start.adv < 0)
    {
      test_fail("test after_start advertise failed ret:%d, errno:%d",
                after_start.adv, errno);
      goto clean_loop;
    }

  before_start.timestamp = orb_absolute_time();
  ret = orb_flush(before_start.sub);
  if (ret < 0)
    {
      test_fail("test call flush failed! instance:%d, ret:%d, errno:%d",
                before_start.instance, ret, errno);
      goto clean_loop;
    }

  after_start.timestamp = orb_absolute_time();
  ret = orb_flush(after_start.sub);
  if (ret < 0)
    {
      test_fail("test call flush failed! instance:%d, ret:%d, errno:%d",
                after_start.instance, ret, errno);
      goto clean_loop;
    }

  temp.timestamp = orb_absolute_time();
  temp.val = 110;
  for (i = 0; i < queue_size; i++)
    {
      temp.timestamp = orb_absolute_time();
      temp.val++;
      ret = orb_publish(ORB_ID(orb_test_medium), before_start.adv, &temp);
      if (ret < 0)
        {
          test_fail("test publish failed! instance:%d, ret:%d, errno:%d",
                    before_start.instance, ret, errno);
        }

      ret = orb_publish(ORB_ID(orb_test_medium), after_start.adv, &temp);
      if (ret < 0)
        {
          test_fail("test publish failed! instance:%d, ret:%d, errno:%d",
                    after_start.instance, ret, errno);
        }
    }

  usleep(500 * 1000);
  ret = orb_handle_stop(&orb_loop, &before_start.handle);
  if (ret < 0)
    {
      test_fail("test stop loop failed ret:%d, errno:%d", ret, errno);
      goto clean_loop;
    }

  ret = orb_handle_stop(&orb_loop, &after_start.handle);
  if (ret < 0)
    {
      test_fail("test stop loop failed ret:%d, errno:%d", ret, errno);
      goto clean_loop;
    }

  ret = orb_loop_exit_async(&orb_loop);
  if (ret < 0)
    {
      test_fail("test stop loop failed ret:%d, errno:%d", ret, errno);
      goto clean_after;
    }

  if (before_start.cout == queue_size && after_start.cout == queue_size &&
      before_start.flush && after_start.flush)
    {
      test_note("  loop before[%d]: recv:%d/10, flush:%d/1, flush "
                "time:%" PRIu64 "us", before_start.instance,
                before_start.cout, before_start.flush,
                before_start.timestamp);
      test_note("  loop after[%d]: recv:%d/10, flush:%d/1, flush "
                "time:%" PRIu64 "us", after_start.instance, after_start.cout,
                after_start.flush, after_start.timestamp);
      test_note("PASS test loop success!");
    }
  else
    {
      ret = ERROR;
      test_fail("test loop failed, before:recv:%d flush:%d,after:recv:%d "
                "flush:%d, errno:%d", before_start.cout, before_start.flush,
                after_start.cout, after_start.flush, errno);
    }

  goto clean_after;

clean_loop:
  orb_loop_exit_async(&orb_loop);

clean_after:
  if (thread_created)
    {
      pthread_join(pubsub_task, NULL);
    }

  orb_loop_deinit(&orb_loop);

  ORB_CLEAN(ORB_ID(orb_test_medium), after_start.adv, after_start.sub,
            after_start.instance);

clean_before:
  ORB_CLEAN(ORB_ID(orb_test_medium), before_start.adv, before_start.sub,
            before_start.instance);

  if (ret != OK)
    {
      ret = ERROR;
    }

  return ret;
}
#endif

static int test_ioctl_family(void)
{
  FAR const struct orb_metadata *meta = NULL;
  const unsigned frequency = 20;
  const unsigned interval  = 20000;
  const unsigned batch     = 120000;
  unsigned get_frequency;
  unsigned get_interval;
  unsigned get_batch;
  orb_info_t info_sub;
  int instance = 0;
  int ret = ERROR;
  int adv;
  int sub;

  orb_info_t info_adv =
    {
      10085,            /* version */
      1.23,             /* power */
      78.48f,           /* max_range */
      0.0024f,          /* resolution */
      10000,            /* min_delay */
      40000,            /* max_delay */
      20,               /* fifo_reserved_event_count */
      20,               /* fifo_max_event_count */
      "test orb info",  /* name */
      "unit test"       /* vendor */
    };

  test_note("Testing orb ioctl family and tools");

  adv = orb_advertise_multi_queue_info(ORB_ID(orb_test_medium), NULL,
                                       &instance, 1, &info_adv);
  if (adv < 0)
    {
      test_fail("test ioctl family advertise failed! instance:%d adv:%d,"
                " errno:%d", instance, adv, errno);
      return ERROR;
    }

  sub = orb_subscribe_multi(ORB_ID(orb_test_medium), instance);
  if (sub < 0)
    {
      test_fail("test ioctl family subscribe failed! instance:%d sub:%d,"
                " errno:%d", instance, sub, errno);
      goto out;
    }

  meta = orb_get_meta("orb_test_medium");
  if (meta == NULL)
    {
      test_fail("test ioctl family get meta failed!");
      goto out;
    }

  ret = orb_get_info(sub, &info_sub);
  if (ret < 0)
    {
      test_fail("test ioctl family get info failed! ret:%d errno:%d",
                ret, errno);
      goto out;
    }

  if (info_sub.version != info_adv.version ||
      strncmp(info_sub.vendor, info_adv.vendor, SENSOR_INFO_NAME_SIZE))
    {
      test_fail("test ioctl family info compare failed!");
      goto out;
    }

  ret = orb_set_interval(sub, interval);
  if (ret < 0)
    {
      test_fail("test ioctl family set interval failed! ret:%d errno:%d",
                ret, errno);
      goto out;
    }

  ret = orb_get_interval(sub, &get_interval);
  if (ret < 0)
    {
      test_fail("test ioctl family get interval failed! ret:%d errno:%d",
                ret, errno);
      goto out;
    }

  if (interval != get_interval)
    {
      test_fail("test ioctl family set/get interval failed! get:%u",
                get_interval);
      goto out;
    }

  ret = orb_set_frequency(sub, frequency);
  if (ret < 0)
    {
      test_fail("test ioctl family set frequency failed! ret:%d errno:%d",
                ret, errno);
      goto out;
    }

  ret = orb_get_frequency(sub, &get_frequency);
  if (ret < 0)
    {
      test_fail("test ioctl family get frequency failed! ret:%d errno:%d",
                ret, errno);
      goto out;
    }

  if (frequency != get_frequency)
    {
      test_fail("test ioctl family set/get frequency failed! get:%u",
                get_frequency);
      goto out;
    }

  ret = orb_set_batch_interval(sub, batch);
  if (ret < 0)
    {
      test_fail("test ioctl family set batch failed! ret:%d errno:%d",
                ret, errno);
      goto out;
    }

  ret = orb_get_batch_interval(sub, &get_batch);
  if (ret < 0)
    {
      test_fail("test ioctl family get batch failed! ret:%d errno:%d",
                ret, errno);
      goto out;
    }

  if (batch != get_batch)
    {
      test_fail("test ioctl family set/get batch failed! get:%u",
                get_batch);
      goto out;
    }

  ret = orb_group_count(ORB_ID(orb_test_medium));
  if (ret < 1)
    {
      test_fail("test ioctl family group count failed! count:%d", ret);
      goto out;
    }

  test_note("PASS test orb ioctl family success!");
  ret = OK;

out:
  ORB_CLEAN(ORB_ID(orb_test_medium), adv, sub, instance)
  return ret;
}

static int test(void)
{
  int afds[4];
  int sfds[4];
  int ret;

  ret = test_single();
  if (ret != OK)
    {
      return ret;
    }

  ret = test_multi_inst10();
  if (ret != OK)
    {
      return ret;
    }

  ret = test_multi(afds, sfds);
  if (ret != OK)
    {
      return ret;
    }

  ret = test_multi_reversed(afds, sfds);
  if (ret != OK)
    {
      return ret;
    }

  ret = test_unadvertise(afds);
  if (ret != OK)
    {
      return ret;
    }

  ret = test_multi2();
  if (ret != OK)
    {
      return ret;
    }

  ret = test_queue();
  if (ret != OK)
    {
      return ret;
    }

  ret = test_advsub();
  if (ret != OK)
    {
      return ret;
    }

  ret = test_ioctl_family();
  if (ret != OK)
    {
      return ret;
    }

#if CONFIG_UORB_LOOP_MAX_EVENTS
  ret = test_orb_loop();
  if (ret != OK)
    {
      return ret;
    }
#endif

  return test_queue_poll_notify();
}

int main(int argc, FAR char *argv[])
{
  /* Test the driver/device. */

  g_server_should_exit = false;
  if (signal(SIGINT, exit_handler) == SIG_ERR)
    {
      return 1;
    }

  if (argc == 1)
    {
      if (test() == OK)
        {
          printf("TEST PASS\n");
          return 0;
        }
      else
        {
          printf("TEST FAIL\n");
          return -1;
        }
    }

  /* Test the latency.
   *   latency_test - local same-core latency measurement (instance 0)
   *   server_test  - pingpong echo server on remote core (instance 1 sub)
   *   client_test  - cross-core client, paired with server_test (instance 1
   *
   * Cross-core usage:
   *   Core A: uorb_tests server_test
   *   Core B: uorb_tests client_test
   */

  if (argc > 1 && !strcmp(argv[1], "latency_test"))
    {
      return latency_test(true, 0);
    }
  else if (argc > 1 && !strcmp(argv[1], "server_test"))
    {
      return server_test();
    }
  else if (argc > 1 && !strcmp(argv[1], "client_test"))
    {
      return latency_test(true, 1);
    }

  printf("Usage: uorb_tests [latency_test|server_test|client_test]\n");
  return -EINVAL;
}
