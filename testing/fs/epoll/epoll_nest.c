/****************************************************************************
 * apps/testing/fs/epoll/epoll_nest.c
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

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* A wait that must end at once: a timeout is the failure */

#define WAIT_MS   2000

/* The time after which the writer thread writes a late byte */

#define LATE_MS   200

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* A pipe in an inner epoll set, the inner set in an outer set */

struct epoll_nest_s
{
  int       fds[2];   /* The pipe, fds[0] is in the inner set */
  int       inner;    /* The set that holds the pipe */
  int       outer;    /* The set that holds the inner set */
  int       outer2;   /* A set that holds the outer set */
  int       tfd;      /* A timerfd in the inner set */
  int       got;      /* The bytes read from the pipe */
  int       count;    /* The bytes that the writer thread writes */
  int       first_ms; /* The delay before the first byte */
  int       gap_ms;   /* The delay between two bytes */
  bool      started;  /* The writer thread runs */
  pthread_t writer;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static long now_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int epoll_add(int epfd, int fd)
{
  struct epoll_event ev;

  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;
  ev.data.fd = fd;
  return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

static bool poll_in(int fd, int timeout)
{
  struct pollfd pfd;

  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  return poll(&pfd, 1, timeout) > 0 && (pfd.revents & POLLIN) != 0;
}

/* Read what is there, nonblocking, until EAGAIN */

static int drain(int fd)
{
  char buf[64];
  int n = 0;
  int ret;

  while ((ret = read(fd, buf, sizeof(buf))) > 0)
    {
      n += ret;
    }

  return n;
}

/* One round of a library that gives its set to the event loop of its
 * user: one epoll_wait() with timeout 0, then read each reported file.
 */

static int inner_round(FAR struct epoll_nest_s *nest)
{
  struct epoll_event evs[4];
  int n;
  int i;

  n = epoll_wait(nest->inner, evs, 4, 0);
  for (i = 0; i < n; i++)
    {
      nest->got += drain(evs[i].data.fd);
    }

  return n;
}

static FAR void *writer_main(FAR void *arg)
{
  FAR struct epoll_nest_s *nest = arg;
  int i;

  usleep(nest->first_ms * 1000);
  for (i = 0; i < nest->count; i++)
    {
      if (i > 0)
        {
          usleep(nest->gap_ms * 1000);
        }

      if (write(nest->fds[1], "x", 1) != 1)
        {
          break;
        }
    }

  return NULL;
}

static void writer_start(FAR struct epoll_nest_s *nest, int count,
                         int first_ms, int gap_ms)
{
  nest->count = count;
  nest->first_ms = first_ms;
  nest->gap_ms = gap_ms;
  assert_int_equal(pthread_create(&nest->writer, NULL, writer_main, nest),
                   0);
  nest->started = true;
}

static void writer_join(FAR struct epoll_nest_s *nest)
{
  if (nest->started)
    {
      pthread_join(nest->writer, NULL);
      nest->started = false;
    }
}

static int nest_setup(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;

  memset(nest, 0, sizeof(*nest));
  nest->fds[0] = -1;
  nest->fds[1] = -1;
  nest->inner = -1;
  nest->outer = -1;
  nest->outer2 = -1;
  nest->tfd = -1;

  assert_int_equal(pipe2(nest->fds, O_CLOEXEC), 0);
  assert_int_equal(fcntl(nest->fds[0], F_SETFL, O_NONBLOCK), 0);

  nest->inner = epoll_create1(EPOLL_CLOEXEC);
  assert_true(nest->inner >= 0);
  assert_int_equal(epoll_add(nest->inner, nest->fds[0]), 0);

  nest->outer = epoll_create1(EPOLL_CLOEXEC);
  assert_true(nest->outer >= 0);
  assert_int_equal(epoll_add(nest->outer, nest->inner), 0);
  return 0;
}

static void close_fd(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

static int nest_teardown(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;

  writer_join(nest);
  close_fd(nest->tfd);
  close_fd(nest->outer2);
  close_fd(nest->outer);
  close_fd(nest->inner);
  close_fd(nest->fds[0]);
  close_fd(nest->fds[1]);
  return 0;
}

/****************************************************************************
 * Name: epoll_nest_pending
 *
 * Description:
 *   After one round, a byte that is already there must wake the outer set.
 *
 ****************************************************************************/

static void epoll_nest_pending(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;
  struct epoll_event ev;

  assert_int_equal(write(nest->fds[1], "a", 1), 1);
  assert_int_equal(epoll_wait(nest->outer, &ev, 1, WAIT_MS), 1);
  assert_int_equal(inner_round(nest), 1);
  assert_int_equal(nest->got, 1);

  assert_int_equal(write(nest->fds[1], "b", 1), 1);
  assert_int_equal(epoll_wait(nest->outer, &ev, 1, WAIT_MS), 1);
  assert_int_equal(inner_round(nest), 1);
  assert_int_equal(nest->got, 2);
}

/****************************************************************************
 * Name: epoll_nest_later
 *
 * Description:
 *   After one round, a byte that comes while the outer set waits must wake
 *   the outer set.
 *
 ****************************************************************************/

static void epoll_nest_later(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;
  struct epoll_event ev;

  assert_int_equal(write(nest->fds[1], "a", 1), 1);
  assert_int_equal(epoll_wait(nest->outer, &ev, 1, WAIT_MS), 1);
  assert_int_equal(inner_round(nest), 1);

  writer_start(nest, 1, LATE_MS, 0);
  assert_int_equal(epoll_wait(nest->outer, &ev, 1, WAIT_MS), 1);
  assert_int_equal(ev.data.fd, nest->inner);
  writer_join(nest);
  assert_int_equal(inner_round(nest), 1);
  assert_int_equal(nest->got, 2);
}

/****************************************************************************
 * Name: epoll_poll_pending
 *
 * Description:
 *   After one round, a byte that is already there must make poll() of the
 *   inner set return POLLIN.
 *
 ****************************************************************************/

static void epoll_poll_pending(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;

  assert_int_equal(write(nest->fds[1], "a", 1), 1);
  assert_int_equal(inner_round(nest), 1);
  assert_int_equal(nest->got, 1);

  assert_int_equal(write(nest->fds[1], "b", 1), 1);
  assert_true(poll_in(nest->inner, WAIT_MS));
}

/****************************************************************************
 * Name: epoll_nest_stream
 *
 * Description:
 *   20 bytes 20 ms apart through the nested set: the loop must read all of
 *   them, and no wait of 1 s on the outer set may time out.
 *
 ****************************************************************************/

static void epoll_nest_stream(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;
  struct epoll_event ev;
  int timeouts = 0;
  long start;

  writer_start(nest, 20, 50, 20);
  start = now_ms();
  while (nest->got < 20 && now_ms() - start < 10000)
    {
      if (epoll_wait(nest->outer, &ev, 1, 1000) == 0)
        {
          timeouts++;
        }

      inner_round(nest);
    }

  writer_join(nest);
  assert_int_equal(nest->got, 20);
  assert_int_equal(timeouts, 0);
}

/****************************************************************************
 * Name: epoll_poll_set
 *
 * Description:
 *   poll() of the inner set: no event, an event, still ready after
 *   epoll_wait() reported it, and drained.
 *
 ****************************************************************************/

static void epoll_poll_set(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;
  struct epoll_event ev;

  assert_false(poll_in(nest->inner, 50));

  assert_int_equal(write(nest->fds[1], "a", 1), 1);
  assert_true(poll_in(nest->inner, WAIT_MS));
  assert_true(poll_in(nest->inner, 0));
  assert_int_equal(epoll_wait(nest->inner, &ev, 1, 0), 1);
  assert_int_equal(ev.data.fd, nest->fds[0]);

  /* The byte is not read: the set is still ready */

  assert_true(poll_in(nest->inner, 0));

  assert_int_equal(drain(nest->fds[0]), 1);
  epoll_wait(nest->inner, &ev, 1, 0);
  assert_false(poll_in(nest->inner, 50));
}

/****************************************************************************
 * Name: epoll_poll_wakes
 *
 * Description:
 *   A byte that comes while poll() of the inner set waits must wake it.
 *
 ****************************************************************************/

static void epoll_poll_wakes(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;

  writer_start(nest, 1, LATE_MS, 0);
  assert_true(poll_in(nest->inner, WAIT_MS));
  writer_join(nest);
  assert_int_equal(drain(nest->fds[0]), 1);
}

/****************************************************************************
 * Name: epoll_nest_level
 *
 * Description:
 *   The outer set: no event, a late event, level-triggered while the byte
 *   is not read, the inner set reports the pipe, and drained.
 *
 ****************************************************************************/

static void epoll_nest_level(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;
  struct epoll_event ev;

  assert_int_equal(epoll_wait(nest->outer, &ev, 1, 50), 0);

  writer_start(nest, 1, LATE_MS, 0);
  assert_int_equal(epoll_wait(nest->outer, &ev, 1, WAIT_MS), 1);
  assert_int_equal(ev.data.fd, nest->inner);
  writer_join(nest);

  assert_int_equal(epoll_wait(nest->outer, &ev, 1, 0), 1);
  assert_int_equal(epoll_wait(nest->inner, &ev, 1, 0), 1);
  assert_int_equal(ev.data.fd, nest->fds[0]);

  assert_int_equal(drain(nest->fds[0]), 1);
  epoll_wait(nest->inner, &ev, 1, 0);
  assert_int_equal(epoll_wait(nest->outer, &ev, 1, 50), 0);
}

/****************************************************************************
 * Name: epoll_nest_three
 *
 * Description:
 *   A set in a set in a set: a late byte must wake the outermost set.
 *
 ****************************************************************************/

static void epoll_nest_three(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;
  struct epoll_event ev;

  nest->outer2 = epoll_create1(EPOLL_CLOEXEC);
  assert_true(nest->outer2 >= 0);
  assert_int_equal(epoll_add(nest->outer2, nest->outer), 0);

  writer_start(nest, 1, LATE_MS, 0);
  assert_int_equal(epoll_wait(nest->outer2, &ev, 1, WAIT_MS), 1);
  assert_int_equal(ev.data.fd, nest->outer);
}

/****************************************************************************
 * Name: epoll_nest_timer
 *
 * Description:
 *   A timerfd in the inner set: its expiry must wake the outer set.
 *
 ****************************************************************************/

static void epoll_nest_timer(FAR void **state)
{
  FAR struct epoll_nest_s *nest = *state;
  struct epoll_event ev;
  struct itimerspec its;

  nest->tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
  assert_true(nest->tfd >= 0);
  assert_int_equal(epoll_add(nest->inner, nest->tfd), 0);
  epoll_wait(nest->inner, &ev, 1, 0);

  memset(&its, 0, sizeof(its));
  its.it_value.tv_nsec = 100 * 1000 * 1000;
  assert_int_equal(timerfd_settime(nest->tfd, 0, &its, NULL), 0);
  assert_int_equal(epoll_wait(nest->outer, &ev, 1, WAIT_MS), 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct epoll_nest_s nest;
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(epoll_nest_pending,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_nest_later,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_poll_pending,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_nest_stream,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_poll_set,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_poll_wakes,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_nest_level,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_nest_three,
                                               nest_setup, nest_teardown,
                                               &nest),
      cmocka_unit_test_prestate_setup_teardown(epoll_nest_timer,
                                               nest_setup, nest_teardown,
                                               &nest),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
