/****************************************************************************
 * apps/testing/fs/epoll/epoll.c
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

#include <debug.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TIME_ERROR_MS               (MSEC_PER_TICK * 2)
#define TIME_ASSERT_RANGE(e,t)      \
  assert_in_range(e, (t) - TIME_ERROR_MS, (t) + TIME_ERROR_MS) 

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct epoll_args_s
{
  int                efd;
  struct epoll_event ev;
  int                fd[2];
  unsigned long      start;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct sockaddr_un g_epoll03_addr =
{
    .sun_family = AF_UNIX,
    .sun_path = "epoll_test03"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("Usage: %s ?\n", progname);
  printf("  This is an epoll testcase to test the epoll.\n");
  printf("  Test case 1: epoll_wait timeout test.\n");
  printf("  Test case 2: epoll_wait with driver call poll_notify()\n");
  printf("               many times simultaneously.\n");
  printf("  Test case 3: epoll_wait with some driver/socket need call\n");
  printf("               poll_setup() again when it's internal state\n");
  printf("               changed.\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: get_time_ms
 ****************************************************************************/

static unsigned long get_time_ms(void)
{
  struct timespec tm;

  clock_gettime(CLOCK_MONOTONIC, &tm);
  return tm.tv_sec * MSEC_PER_SEC + tm.tv_nsec / NSEC_PER_MSEC;
}

/****************************************************************************
 * Name: get_elapse
 ****************************************************************************/

static unsigned long get_elapse(unsigned long start)
{
  return get_time_ms() - start;
}

/****************************************************************************
 * Name: timerfd_stop
 ****************************************************************************/

static int timerfd_stop(int fd)
{
  struct itimerspec tms;

  tms.it_value.tv_sec = 0;
  tms.it_value.tv_nsec = 0;
  return timerfd_settime(fd, 0, &tms, NULL);
}

/****************************************************************************
 * Name: epoll01_setup
 ****************************************************************************/

static int epoll01_setup(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  struct itimerspec tms;
  int ret;
  int fd;

  /* Create timer fd */

  fd = timerfd_create(CLOCK_MONOTONIC, 0);
  assert_true(fd >= 0);
  args->fd[0] = fd;

  /* Create epoll fd */

  fd = epoll_create(1);
  assert_true(fd >= 0);
  args->efd = fd;

  args->ev.events = EPOLLIN;
  args->ev.data.fd = args->fd[0];
  ret = epoll_ctl(fd, EPOLL_CTL_ADD, args->fd[0], &args->ev);
  assert_true(ret >= 0);

  /* Start the timer */

  tms.it_value.tv_sec = 1;
  tms.it_value.tv_nsec = 0;
  tms.it_interval.tv_sec = 0;
  tms.it_interval.tv_nsec = 0;
  args->start = get_time_ms();
  ret = timerfd_settime(args->fd[0], 0, &tms, NULL);
  assert_true(ret >= 0);

  return 0;
}

/****************************************************************************
 * Name: epoll01
 ****************************************************************************/

static void epoll01(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  struct epoll_event evs;
  int ret;

  /* 1st wait should timeout, timer is 1s */

  ret = epoll_wait(args->efd, &evs, 1, 700);
  TIME_ASSERT_RANGE(get_elapse(args->start), 700);
  assert_true(ret == 0);

  /* 2nd wait should success and return 1 at 1s */

  ret = epoll_wait(args->efd, &evs, 1, 700);
  TIME_ASSERT_RANGE(get_elapse(args->start), 1000);
  assert_true(ret == 1);
  assert_true(evs.data.fd = args->fd[0]);

  ret = timerfd_stop(args->fd[0]);
  assert_true(ret == 0);

  /* 3rd wait should directly return 0, timer has stoped */

  ret = epoll_wait(args->efd, &evs, 1, 0);
  TIME_ASSERT_RANGE(get_elapse(args->start), 1000);
  assert_true(ret == 0);
}

/****************************************************************************
 * Name: epoll01_teardown
 ****************************************************************************/

static int epoll01_teardown(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  int ret;

  ret = epoll_ctl(args->efd, EPOLL_CTL_DEL, args->fd[0], &args->ev);
  assert_true(ret >= 0);

  close(args->efd);
  close(args->fd[0]);

  return 0;
}

/****************************************************************************
 * Name: epoll02_setup
 ****************************************************************************/

static int epoll02_setup(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  struct itimerspec tms;
  int ret;
  int fd;
  int i;

  /* Create epoll fd */

  fd = epoll_create(2);
  assert_true(fd >= 0);
  args->efd = fd;

  for (i = 0; i < 2; i++)
    {
      fd = timerfd_create(CLOCK_MONOTONIC, 0);
      assert_true(fd >= 0);
      args->fd[i] = fd;

      args->ev.events = EPOLLIN;
      args->ev.data.fd = fd;
      ret = epoll_ctl(args->efd, EPOLL_CTL_ADD, fd, &args->ev);
      assert_true(ret >= 0);
    }

  /* Start timer */

  tms.it_value.tv_sec = 1;
  tms.it_value.tv_nsec = 0;
  tms.it_interval.tv_sec = 0;
  tms.it_interval.tv_nsec = 0;
  args->start = get_time_ms();
  for (i = 0; i < 2; i++)
    {
      ret = timerfd_settime(args->fd[i], 0, &tms, NULL);
      assert_true(ret >= 0);
    }

  return 0;
}

/****************************************************************************
 * Name: epoll02
 ****************************************************************************/

static void epoll02(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  struct epoll_event evs[2];
  int ret;

  /* 1st wait, shoud return after 1000ms */

  ret = epoll_wait(args->efd, evs, 2, 1100);
  TIME_ASSERT_RANGE(get_elapse(args->start), 1000);
  assert_true(ret == 2);
  assert_true((evs[0].data.fd == args->fd[0] &&
               evs[1].data.fd == args->fd[1]) ||
              (evs[0].data.fd == args->fd[1] &&
               evs[1].data.fd == args->fd[0]));

  /* Stop the timers */

  ret = timerfd_stop(args->fd[0]);
  assert_true(ret >= 0);
  ret = timerfd_stop(args->fd[1]);
  assert_true(ret >= 0);

  /* 2nd wait, should timeout until 200ms */

  args->start = get_time_ms();
  ret = epoll_wait(args->efd, evs, 2, 200);
  TIME_ASSERT_RANGE(get_elapse(args->start), 200);
  assert_true(ret == 0);
}

/****************************************************************************
 * Name: epoll02_teardown
 ****************************************************************************/

static int epoll02_teardown(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  int ret;

  ret = epoll_ctl(args->efd, EPOLL_CTL_DEL, args->fd[0], &args->ev);
  assert_true(ret >= 0);
  ret = epoll_ctl(args->efd, EPOLL_CTL_DEL, args->fd[1], &args->ev);
  assert_true(ret >= 0);

  close(args->efd);
  close(args->fd[0]);
  close(args->fd[1]);

  return 0;
}

/****************************************************************************
 * Name: epoll03_setup
 ****************************************************************************/

static int epoll03_setup(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  int fd;

  fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
  assert_true(fd >= 0);
  args->fd[0] = fd;

  fd = epoll_create(1);
  assert_true(fd >= 0);
  args->efd = fd;

  return 0;
}

/****************************************************************************
 * Name: epoll03_server
 ****************************************************************************/

static void *epoll03_server(FAR void *arg)
{
  int ret;
  int new;
  int fd;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  assert_true(fd >= 0);

  ret = bind(fd, (const struct sockaddr *)&g_epoll03_addr,
             sizeof(g_epoll03_addr));
  assert_true(ret >= 0);

  ret = listen(fd, SOMAXCONN);
  assert_true(ret >= 0);

  new = accept(fd, NULL, NULL);
  assert_true(new >= 0);
  usleep(500 * USEC_PER_MSEC);

  ret = write(new, "1234567890", 10);
  assert_true(ret == 10);
  usleep(500 * USEC_PER_MSEC);

  close(new);
  close(fd);

  return NULL;
}

/****************************************************************************
 * Name: epoll03
 ****************************************************************************/

static void epoll03(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  struct epoll_event evs;
  pthread_t thread;
  char buf[10];
  int ret;

  ret = pthread_create(&thread, NULL, epoll03_server, args);
  assert_true(ret == 0);

  /* Sleep to make server run */

  usleep(100 * USEC_PER_MSEC);

  ret = connect(args->fd[0], (const struct sockaddr *)&g_epoll03_addr,
                sizeof(g_epoll03_addr));
  assert_true(ret == -1 && errno == EINPROGRESS);

  /* Add socket fd to epoll */

  args->ev.events = EPOLLIN;
  args->ev.data.fd = args->fd[0];
  ret = epoll_ctl(args->efd, EPOLL_CTL_ADD, args->fd[0], &args->ev);
  assert_true(ret >= 0);

  /* 1st wait should timeout, because server accept will trigger
   * EPOLLOUT event, but we do not set EPOLLOUT in ev.events.
   * Even the server has writen the data, but to check the EPOLLIN event,
   * client need call epoll_wait again, because local socket internal state
   * changed, we need setup it again to get the EPOLLIN event.
   */

  args->start = get_time_ms();
  ret = epoll_wait(args->efd, &evs, 1, 1000);
  TIME_ASSERT_RANGE(get_elapse(args->start), 1000);
  assert_true(ret == 0);

  /* 2nd wait shoud return immediately, because server has send
   * data to client.
   */

  ret = epoll_wait(args->efd, &evs, 1, 1000);
  TIME_ASSERT_RANGE(get_elapse(args->start), 1000);
  assert_true(ret == 1);
  assert_true(args->fd[0] == evs.data.fd);

  ret = read(args->fd[0], buf, 10);
  assert_true(ret == 10);
  assert_true(memcmp(buf, "1234567890", 10) == 0);

  ret = pthread_join(thread, NULL);
  assert_true(ret == 0);
}

/****************************************************************************
 * Name: epoll03_teardown
 ****************************************************************************/

static int epoll03_teardown(FAR void **state)
{
  FAR struct epoll_args_s *args = *state;
  int ret;

  ret = epoll_ctl(args->efd, EPOLL_CTL_DEL, args->fd[0], &args->ev);
  assert_true(ret >= 0);

  close(args->efd);
  close(args->fd[0]);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct epoll_args_s epoll_args;

  memset(&epoll_args, 0, sizeof(epoll_args));

  if (argc >= 2 && *argv[1] == '?')
    {
      show_usage(argv[0], 0);
    }

  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(epoll01,
                                               epoll01_setup,
                                               epoll01_teardown,
                                               &epoll_args),
      cmocka_unit_test_prestate_setup_teardown(epoll02,
                                               epoll02_setup,
                                               epoll02_teardown,
                                               &epoll_args),
      cmocka_unit_test_prestate_setup_teardown(epoll03,
                                               epoll03_setup,
                                               epoll03_teardown,
                                               &epoll_args),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
