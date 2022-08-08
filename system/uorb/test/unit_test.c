/****************************************************************************
 * apps/system/uorb/test/unit_test.c
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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "utility.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_thread_should_exit;
static volatile int  g_num_messages_sent;
static bool          g_pubsubtest_passed;
static bool          g_pubsubtest_print;
static int           g_pubsubtest_res;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pubsubtest_thread_entry(int argc, FAR char *argv[])
{
  /* poll on test topic and output latency */

  struct pollfd fds[1];
  struct orb_test_medium_s t;

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
  unsigned i;

  timings = malloc(MAX_RUNS * sizeof(unsigned));
  if (timings == NULL)
    {
      return -ENOMEM;
    }

  /* clear all ready flags */

  test_multi_sub = orb_subscribe_multi(ORB_ID(orb_test_medium), 0);

  orb_copy(ORB_ID(orb_test_medium), test_multi_sub, &t);

  fds[0].fd     = test_multi_sub;
  fds[0].events = POLLIN;
  current_value = t.val;

  for (i = 0; i < MAX_RUNS; i++)
    {
      int pret;

      /* wait for up to 500ms for data */

      pret = poll(&fds[0], (sizeof(fds) / sizeof(fds[0])), 500);
      if (fds[0].revents & POLLIN)
        {
          unsigned elt;

          orb_copy(ORB_ID(orb_test_medium), test_multi_sub, &t);
          elt = (unsigned)orb_elapsed_time(&t.timestamp);
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
          snerr("poll error %d, %d", pret, errno);
          continue;
        }
    }

  orb_unsubscribe(test_multi_sub);

  if (g_pubsubtest_print)
    {
      char fname[32];
      FAR FILE *f;

      sprintf(fname, CONFIG_UORB_SRORAGE_DIR"/uorb_timings%u.txt",
              timingsgroup);

      f = fopen(fname, "w");
      if (f == NULL)
        {
          snerr("Error opening file!");
          free(timings);
          return ERROR;
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

  if (latency_integral / MAX_RUNS > 100.0f)
    {
      g_pubsubtest_res = ERROR;
    }
  else
    {
      g_pubsubtest_res = OK;
    }

  free(timings);
  return g_pubsubtest_res;
}

static int latency_test(bool print)
{
  FAR char *const args[1];
  struct orb_test_medium_s sample;
  int pubsub_task;
  int instance = 0;
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

  /* test pub / sub latency
   * Can't pass a pointer in args, must be a null terminated
   * array of strings because the strings are copied to
   * prevent access if the caller data goes out of scope
   */

  args[0] = NULL;
  pubsub_task = task_create("uorb_latency",
                            SCHED_PRIORITY_DEFAULT,
                            CONFIG_UORB_STACKSIZE,
                            pubsubtest_thread_entry,
                            args);

  /* give the test task some data */

  while (!g_pubsubtest_passed)
    {
      ++sample.val;
      sample.timestamp = orb_absolute_time();
      if (OK != orb_publish(ORB_ID(orb_test_medium), fd, &sample))
        {
          return test_fail("mult. pub0 timing fail");
        }

      usleep(1000); /* simulate >800 Hz system operation */
    }

  if (pubsub_task < 0)
    {
      return test_fail("failed launching task");
    }

  orb_unadvertise(fd);
  return g_pubsubtest_res;
}

static int test_single(void)
{
  struct orb_test_s sample;
  struct orb_test_s sub_sample;
  int instance = 0;
  bool updated;
  int afd;
  int sfd;
  int ret;

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
      return test_fail("subscribe failed: %d", errno);
    }

  /* check first publish */

  sub_sample.val = 1;

  if (OK != orb_copy(ORB_ID(orb_test), sfd, &sub_sample))
    {
      return test_fail("copy(1) failed: %d", errno);
    }

  if (sample.val != sub_sample.val)
    {
      return test_fail("copy(1) mismatch: %d expected %d",
                       sub_sample.val, sample.val);
    }

  if (OK != orb_check(sfd, &updated))
    {
      return test_fail("check(1) failed");
    }

  if (updated)
    {
      return test_fail("spurious updated flag");
    }

  /* check second publish */

  sample.val = 2;

  if (OK != orb_publish(ORB_ID(orb_test), afd, &sample))
    {
      return test_fail("publish failed");
    }

  if (OK != orb_check(sfd, &updated))
    {
      return test_fail("check(2) failed");
    }

  if (!updated)
    {
      return test_fail("missing updated flag");
    }

  if (OK != orb_copy(ORB_ID(orb_test), sfd, &sub_sample))
    {
      return test_fail("copy(2) failed: %d", errno);
    }

  if (sample.val != sub_sample.val)
    {
      return test_fail("copy(2) mismatch: %d expected %d",
                       sub_sample.val, sample.val);
    }

  /* unadvertise and out */

  ret = orb_unadvertise(afd);
  if (ret != OK)
    {
      return test_fail("orb_unadvertise failed: %i", ret);
    }

  ret = orb_unsubscribe(sfd);
  if (ret != OK)
    {
      return test_fail("orb_unsubscribe failed: %i", ret);
    }

  return test_note("PASS single-topic test");
}

static int test_multi_inst10(void)
{
  const int max_inst = 10;
  int sfd[max_inst];
  int afd[max_inst];
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
          return test_fail("sub %d is advertised", i);
        }
    }

  /* advertise one at a time and verify instance */

  for (i = 0; i < max_inst; i++)
    {
      afd[i] = orb_advertise_multi_queue_persist(ORB_ID(orb_test),
                                                 NULL, &i, 1);
      if (OK != orb_exists(ORB_ID(orb_test), i))
        {
          return test_fail("sub %d advertise failed", i);
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
                  return test_fail("sub %d not updated", sub_instance);
                }
              else
                {
                  struct orb_test_s sample;

                  if (orb_copy(ORB_ID(orb_test), sfd[sub_instance], &sample))
                    {
                      return test_fail("sub %d copy failed", sub_instance);
                    }
                  else
                    {
                      if (sample.val != (j * sub_instance + i))
                        {
                          return test_fail("sub %d invalid value %d",
                                           sub_instance, sample.val);
                        }
                    }
                }
            }
        }
    }

  /* force unsubscribe all, then repeat */

  for (i = 0; i < max_inst; i++)
    {
      orb_unsubscribe(sfd[i]);
    }

  return test_note("PASS orb 10-instances");
}

static int test_multi(int *afds, int *sfds)
{
  struct orb_test_s sample;
  struct orb_test_s sub_sample;
  int instance0 = 0;
  int instance1 = 1;

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

  if (OK != latency_test(false))
    {
      return test_fail("latency test failed");
    }

  orb_unsubscribe(sfds[0]);
  orb_unsubscribe(sfds[1]);

  return test_note("PASS multi-topic test");
}

static int test_multi_reversed(int *afds, int *sfds)
{
  struct orb_test_s sample;
  struct orb_test_s sub_sample;
  int instance2 = 2;
  int instance3 = 3;

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
      return test_fail("mult. id2: %d", instance2);
    }

  afds[3] = orb_advertise_multi_queue_persist(ORB_ID(orb_multitest),
                                              &sample, &instance3, 1);

  if (instance3 != 3)
    {
      return test_fail("mult. id3: %d", instance3);
    }

  sample.val = 204;

  if (OK != orb_publish(ORB_ID(orb_multitest), afds[2], &sample))
    {
      return test_fail("mult. pub2 fail");
    }

  sample.val = 304;

  if (OK != orb_publish(ORB_ID(orb_multitest), afds[3], &sample))
    {
      return test_fail("mult. pub3 fail");
    }

  /* subscribe to both topics and ensure valid data is received */

  sfds[2] = orb_subscribe_multi(ORB_ID(orb_multitest), 2);

  if (OK != orb_copy(ORB_ID(orb_multitest), sfds[2], &sub_sample))
    {
      return test_fail("sub #2 copy failed: %d", errno);
    }

  if (sub_sample.val != 204)
    {
      return test_fail("sub #2 val. mismatch: %d", sub_sample.val);
    }

  sfds[3] = orb_subscribe_multi(ORB_ID(orb_multitest), 3);

  if (OK != orb_copy(ORB_ID(orb_multitest), sfds[3], &sub_sample))
    {
      return test_fail("sub #3 copy failed: %d", errno);
    }

  if (sub_sample.val != 304)
    {
      return test_fail("sub #3 val. mismatch: %d", sub_sample.val);
    }

  return test_note("PASS multi-topic reversed");
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
    }

  return OK;
}

static int pub_test_multi2_entry(int argc, char *argv[])
{
  struct orb_test_medium_s data_topic;
  const int num_instances = 3;
  int data_next_idx = 0;
  int orb_pub[num_instances];
  int message_counter = 0;
  int num_messages = 50 * num_instances;
  int i;

  memset(&data_topic, '\0', sizeof(data_topic));
  for (i = 0; i < num_instances; ++i)
    {
      orb_pub[i] = orb_advertise_multi_queue_persist(
        ORB_ID(orb_test_medium_multi), &data_topic, &i, 1);
    }

  usleep(100 * 1000);

  while (message_counter++ < num_messages)
    {
      usleep(2); /* make sure the timestamps are different */

      data_topic.timestamp = orb_absolute_time();
      data_topic.val       = data_next_idx;

      orb_publish(ORB_ID(orb_test_medium_multi), orb_pub[data_next_idx],
                  &data_topic);

      data_next_idx = (data_next_idx + 1) % num_instances;
      if (data_next_idx == 0)
        {
          usleep(50 * 1000);
        }
    }

  usleep(100 * 1000);

  g_thread_should_exit = true;

  for (i = 0; i < num_instances; ++i)
    {
      orb_unadvertise(orb_pub[i]);
    }

  return OK;
}

static int test_multi2(void)
{
  const int num_instances = 3;
  int orb_data_fd[num_instances];
  int orb_data_next     = 0;
  orb_abstime last_time = 0;
  FAR char *const args[1];
  int pubsub_task;
  int i;

  test_note("Testing multi-topic 2 test (queue simulation)");

  g_thread_should_exit = false;

  /* test: first subscribe, then advertise */

  for (i = 0; i < num_instances; ++i)
    {
      orb_data_fd[i] = orb_subscribe_multi(ORB_ID(orb_test_medium_multi), i);
    }

  /* launch the publisher thread */

  args[0] = NULL;
  pubsub_task = task_create("uorb_test_multi",
                            SCHED_PRIORITY_MAX - 5,
                            CONFIG_UORB_STACKSIZE,
                            (main_t)&pub_test_multi2_entry,
                            args);
  if (pubsub_task < 0)
    {
      return test_fail("failed launching task");
    }

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

          orb_copy(ORB_ID(orb_test_medium_multi), orb_data_cur_fd, &msg);
          if (last_time >= msg.timestamp && last_time != 0)
            {
              return test_fail("Timestamp not increasing! (%" PRIu64
                               " >= %" PRIu64 ")", last_time, msg.timestamp);
            }

          last_time     = msg.timestamp;
          orb_data_next = (orb_data_next + 1) % num_instances;
        }
    }

  for (i = 0; i < num_instances; ++i)
    {
      orb_unsubscribe(orb_data_fd[i]);
    }

  return test_note("PASS multi-topic 2 test (queue simulation)");
}

int test_queue(void)
{
  const int queue_size  = 16;
  const int overflow_by = 3;
  struct orb_test_medium_s sample;
  struct orb_test_medium_s sub_sample;
  bool updated;
  int instance = 0;
  int ptopic;
  int sfd;
  int i;

  test_note("Testing orb queuing");

  sfd = orb_subscribe(ORB_ID(orb_test_medium_queue));

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
          orb_copy(ORB_ID(orb_test_medium_queue), sfd, &sub_sample);
        }
    }
  while (updated);

  ptopic = orb_advertise_multi_queue_persist(
    ORB_ID(orb_test_medium_queue), &sample, &instance, queue_size);
  if (ptopic < 0)
    {
      return test_fail("advertise failed: %d", errno);
    }

  orb_check(sfd, &updated);
  if (!updated)
    {
      return test_fail("update flag not set");
    }

  if (OK != orb_copy(ORB_ID(orb_test_medium_queue), sfd, &sub_sample))
    {
      return test_fail("copy(1) failed: %d", errno);
    }

  if (sub_sample.val != sample.val)
    {
      return test_fail("copy(1) mismatch: %d expected %d",
                       sub_sample.val, sample.val);
    }

  orb_check(sfd, &updated);
  if (updated)
    {
      return test_fail("spurious updated flag");
    }

#define CHECK_UPDATED(element) \
  orb_check(sfd, &updated); \
  if (!updated) \
    { \
      return test_fail("update flag not set, element %i", element); \
    }

#define CHECK_NOT_UPDATED(element) \
  orb_check(sfd, &updated); \
  if (updated) \
    { \
      return test_fail("update flag set, element %i", element); \
    }

#define CHECK_COPY(i_got, i_correct) \
  orb_copy(ORB_ID(orb_test_medium_queue), sfd, &sub_sample); \
  if (i_got != i_correct) \
    { \
      return test_fail("got wrong element from the queue (got %i," \
                       "should be %i)", i_got, i_correct); \
    }

  /* no messages in the queue anymore */

  test_note("  Testing to write some elements...");

  for (i = 0; i < queue_size - 2; ++i)
    {
      sample.val = i;
      orb_publish(ORB_ID(orb_test_medium_queue), ptopic, &sample);
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
      orb_publish(ORB_ID(orb_test_medium_queue), ptopic, &sample);
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

  orb_publish(ORB_ID(orb_test_medium_queue), ptopic, &sample);

  CHECK_UPDATED(-1);
  CHECK_COPY(sub_sample.val, sample.val);

#undef CHECK_COPY
#undef CHECK_UPDATED
#undef CHECK_NOT_UPDATED

  orb_unadvertise(ptopic);
  orb_unsubscribe(sfd);

  return test_note("PASS orb queuing");
}

static int pub_test_queue_entry(int argc, char *argv[])
{
  const int queue_size = 50;
  struct orb_test_medium_s t;
  int num_messages = 20 * queue_size;
  int message_counter = 0;
  int instance = 0;
  int ptopic;

  memset(&t, '\0', sizeof(t));
  ptopic = orb_advertise_multi_queue_persist(
    ORB_ID(orb_test_medium_queue_poll), &t, &instance, queue_size);
  if (ptopic < 0)
    {
      g_thread_should_exit = true;
      return test_fail("advertise failed: %d", errno);
    }

  ++t.val;

  while (message_counter < num_messages)
    {
      /* simulate burst */

      int burst_counter = 0;

      while (burst_counter++ < queue_size / 2 + 7)
        {
          /* make interval non-boundary aligned */

          orb_publish(ORB_ID(orb_test_medium_queue_poll), ptopic, &t);
          ++t.val;
        }

      message_counter += burst_counter;
      usleep(20 * 1000); /* give subscriber a chance to catch up */
    }

  g_num_messages_sent = t.val;
  usleep(100 * 1000);
  g_thread_should_exit = true;
  orb_unadvertise(ptopic);

  return 0;
}

static int test_queue_poll_notify(void)
{
  FAR char *const args[1];
  struct pollfd fds[1];
  struct orb_test_medium_s t;
  bool updated;
  int next_expected_val = 0;
  int pubsub_task;
  int sfd;

  test_note("Testing orb queuing (poll & notify)");

  if ((sfd = orb_subscribe(ORB_ID(orb_test_medium_queue_poll))) < 0)
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
          orb_copy(ORB_ID(orb_test_medium_queue_poll), sfd, &t);
        }
    }
  while (updated);

  g_thread_should_exit = false;

  args[0] = NULL;
  pubsub_task = task_create("uorb_test_queue",
                            SCHED_PRIORITY_MIN + 5,
                            CONFIG_UORB_STACKSIZE,
                            (main_t)&pub_test_queue_entry,
                            args);

  if (pubsub_task < 0)
    {
      return test_fail("failed launching task");
    }

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

          return test_fail("poll timeout");
        }
      else if (poll_ret < 0 && errno != EINTR)
        {
          return test_fail("poll error (%d, %d)", poll_ret, errno);
        }

      if (fds[0].revents & POLLIN)
        {
          orb_copy(ORB_ID(orb_test_medium_queue_poll), sfd, &t);
          if (next_expected_val != t.val)
            {
              return test_fail("copy mismatch: %d expected %d",
                               t.val, next_expected_val);
            }

          ++next_expected_val;
        }
    }

  if (g_num_messages_sent != next_expected_val)
    {
      return test_fail("number of sent and received messages mismatch"
                       " (sent: %i, received: %i)",
                       g_num_messages_sent, next_expected_val);
    }

  orb_unsubscribe(sfd);

  return OK;
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

  return test_queue_poll_notify();
}

int main(int argc, FAR char *argv[])
{
  /* Test the driver/device. */

  if (argc == 1)
    {
      if (test() == OK)
        {
          printf("PASS\n");
          return 0;
        }
      else
        {
          printf("FAIL\n");
          return -1;
        }
    }

  /* Test the latency. */

  if (argc > 1 && !strcmp(argv[1], "latency_test"))
    {
      return latency_test(true);
    }

  printf("Usage: uorb_tests [latency_test]\n");
  return -EINVAL;
}
