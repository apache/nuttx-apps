/****************************************************************************
 * apps/testing/libc/sysconf/sysconf_main.c
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

#include <sys/param.h>
#include <sys/resource.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SC(n)     { #n, n, 0, false }
#define SCV(n, v) { #n, n, (v), false }
#define SCO(n, v) { #n, n, (v), true }

/* A value of errno that sysconf() does not set */

#define SYSCONF_ERRNO EDOM

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sysconf_name_s
{
  FAR const char *str;
  int name;
  long value;
  bool option;  /* An option: a constant of 0 means "ask sysconf()" */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Every name of <unistd.h> */

static const struct sysconf_name_s g_names[] =
{
  SC(_SC_2_CHAR_TERM),
  SC(_SC_2_C_BIND),
  SC(_SC_2_C_DEV),
  SC(_SC_2_FORT_DEV),
  SC(_SC_2_FORT_RUN),
  SC(_SC_2_LOCALEDEF),
  SC(_SC_2_PBS),
  SC(_SC_2_PBS_ACCOUNTING),
  SC(_SC_2_PBS_CHECKPOINT),
  SC(_SC_2_PBS_LOCATE),
  SC(_SC_2_PBS_MESSAGE),
  SC(_SC_2_PBS_TRACK),
  SC(_SC_2_SW_DEV),
  SC(_SC_2_UPE),
  SC(_SC_2_VERSION),
  SC(_SC_ADVISORY_INFO),
  SC(_SC_AIO_LISTIO_MAX),
  SC(_SC_AIO_MAX),
  SC(_SC_AIO_PRIO_DELTA_MAX),
  SC(_SC_ARG_MAX),
  SC(_SC_ASYNCHRONOUS_IO),
  SC(_SC_ATEXIT_MAX),
  SC(_SC_AVPHYS_PAGES),
  SC(_SC_BARRIERS),
  SC(_SC_BC_BASE_MAX),
  SC(_SC_BC_DIM_MAX),
  SC(_SC_BC_SCALE_MAX),
  SC(_SC_BC_STRING_MAX),
  SC(_SC_CHILD_MAX),
  SC(_SC_CLK_TCK),
  SC(_SC_CLOCK_SELECTION),
  SC(_SC_COLL_WEIGHTS_MAX),
  SC(_SC_CPUTIME),
  SC(_SC_DELAYTIMER_MAX),
#ifdef _SC_DEVICE_CONTROL
  SC(_SC_DEVICE_CONTROL),
#endif
  SC(_SC_EXPR_NEST_MAX),
  SC(_SC_FSYNC),
  SC(_SC_GETGR_R_SIZE_MAX),
  SC(_SC_GETPW_R_SIZE_MAX),
  SC(_SC_HOST_NAME_MAX),
  SC(_SC_IOV_MAX),
  SC(_SC_IPV6),
  SC(_SC_JOB_CONTROL),
  SC(_SC_LINE_MAX),
  SC(_SC_LOGIN_NAME_MAX),
  SC(_SC_MAPPED_FILES),
  SC(_SC_MEMLOCK),
  SC(_SC_MEMLOCK_RANGE),
  SC(_SC_MEMORY_PROTECTION),
  SC(_SC_MESSAGE_PASSING),
  SC(_SC_MONOTONIC_CLOCK),
  SC(_SC_MQ_OPEN_MAX),
  SC(_SC_MQ_PRIO_MAX),
  SC(_SC_NGROUPS_MAX),
  SC(_SC_NPROCESSORS_CONF),
  SC(_SC_NPROCESSORS_ONLN),
#ifdef _SC_NSIG
  SC(_SC_NSIG),
#endif
  SC(_SC_OPEN_MAX),
  SC(_SC_PAGE_SIZE),
  SC(_SC_PHYS_PAGES),
  SC(_SC_PRIORITIZED_IO),
  SC(_SC_PRIORITY_SCHEDULING),
  SC(_SC_RAW_SOCKETS),
  SC(_SC_READER_WRITER_LOCKS),
  SC(_SC_REALTIME_SIGNALS),
  SC(_SC_REGEXP),
  SC(_SC_RE_DUP_MAX),
  SC(_SC_RTSIG_MAX),
  SC(_SC_SAVED_IDS),
  SC(_SC_SEMAPHORES),
  SC(_SC_SEM_NSEMS_MAX),
  SC(_SC_SEM_VALUE_MAX),
  SC(_SC_SHARED_MEMORY_OBJECTS),
  SC(_SC_SHELL),
  SC(_SC_SIGQUEUE_MAX),
  SC(_SC_SPAWN),
  SC(_SC_SPIN_LOCKS),
  SC(_SC_SPORADIC_SERVER),
  SC(_SC_SS_REPL_MAX),
  SC(_SC_STREAM_MAX),
  SC(_SC_SYMLOOP_MAX),
  SC(_SC_SYNCHRONIZED_IO),
  SC(_SC_THREADS),
  SC(_SC_THREAD_ATTR_STACKADDR),
  SC(_SC_THREAD_ATTR_STACKSIZE),
  SC(_SC_THREAD_CPUTIME),
  SC(_SC_THREAD_DESTRUCTOR_ITERATIONS),
  SC(_SC_THREAD_KEYS_MAX),
  SC(_SC_THREAD_PRIORITY_SCHEDULING),
  SC(_SC_THREAD_PRIO_INHERIT),
  SC(_SC_THREAD_PRIO_PROTECT),
  SC(_SC_THREAD_PROCESS_SHARED),
#ifdef _SC_THREAD_ROBUST_PRIO_INHERIT
  SC(_SC_THREAD_ROBUST_PRIO_INHERIT),
#endif
#ifdef _SC_THREAD_ROBUST_PRIO_PROTECT
  SC(_SC_THREAD_ROBUST_PRIO_PROTECT),
#endif
  SC(_SC_THREAD_SAFE_FUNCTIONS),
  SC(_SC_THREAD_SPORADIC_SERVER),
  SC(_SC_THREAD_STACK_MIN),
  SC(_SC_THREAD_THREADS_MAX),
  SC(_SC_TIMEOUTS),
  SC(_SC_TIMERS),
  SC(_SC_TIMER_MAX),
  SC(_SC_TRACE),
  SC(_SC_TRACE_EVENT_FILTER),
  SC(_SC_TRACE_EVENT_NAME_MAX),
  SC(_SC_TRACE_INHERIT),
  SC(_SC_TRACE_LOG),
  SC(_SC_TRACE_NAME_MAX),
  SC(_SC_TRACE_SYS_MAX),
  SC(_SC_TRACE_USER_EVENT_MAX),
  SC(_SC_TTY_NAME_MAX),
  SC(_SC_TYPED_MEMORY_OBJECTS),
  SC(_SC_TZNAME_MAX),
  SC(_SC_V6_ILP32_OFF32),
  SC(_SC_V6_ILP32_OFFBIG),
  SC(_SC_V6_LP64_OFF64),
  SC(_SC_V6_LPBIG_OFFBIG),
#ifdef _SC_V7_ILP32_OFF32
  SC(_SC_V7_ILP32_OFF32),
#endif
#ifdef _SC_V7_ILP32_OFFBIG
  SC(_SC_V7_ILP32_OFFBIG),
#endif
#ifdef _SC_V7_LP64_OFF64
  SC(_SC_V7_LP64_OFF64),
#endif
#ifdef _SC_V7_LPBIG_OFFBIG
  SC(_SC_V7_LPBIG_OFFBIG),
#endif
#ifdef _SC_V8_ILP32_OFF32
  SC(_SC_V8_ILP32_OFF32),
#endif
#ifdef _SC_V8_ILP32_OFFBIG
  SC(_SC_V8_ILP32_OFFBIG),
#endif
#ifdef _SC_V8_LP64_OFF64
  SC(_SC_V8_LP64_OFF64),
#endif
#ifdef _SC_V8_LPBIG_OFFBIG
  SC(_SC_V8_LPBIG_OFFBIG),
#endif
  SC(_SC_VERSION),
  SC(_SC_XBS5_ILP32_OFF32),
  SC(_SC_XBS5_ILP32_OFFBIG),
  SC(_SC_XBS5_LP64_OFF64),
  SC(_SC_XBS5_LPBIG_OFFBIG),
  SC(_SC_XOPEN_CRYPT),
  SC(_SC_XOPEN_ENH_I18N),
  SC(_SC_XOPEN_LEGACY),
  SC(_SC_XOPEN_REALTIME),
  SC(_SC_XOPEN_REALTIME_THREADS),
  SC(_SC_XOPEN_SHM),
  SC(_SC_XOPEN_STREAMS),
  SC(_SC_XOPEN_UNIX),
#ifdef _SC_XOPEN_UUCP
  SC(_SC_XOPEN_UUCP),
#endif
  SC(_SC_XOPEN_VERSION),
};

/* The names that have a constant in <limits.h>, <signal.h>, <time.h> or
 * <unistd.h>
 */

static const struct sysconf_name_s g_values[] =
{
#ifdef AIO_LISTIO_MAX
  SCV(_SC_AIO_LISTIO_MAX, AIO_LISTIO_MAX),
#endif
#ifdef AIO_MAX
  SCV(_SC_AIO_MAX, AIO_MAX),
#endif
#ifdef ARG_MAX
  SCV(_SC_ARG_MAX, ARG_MAX),
#endif
#ifdef CHILD_MAX
  SCV(_SC_CHILD_MAX, CHILD_MAX),
#endif
#ifdef CLK_TCK
  SCV(_SC_CLK_TCK, CLK_TCK),
#endif
#ifdef DELAYTIMER_MAX
  SCV(_SC_DELAYTIMER_MAX, DELAYTIMER_MAX),
#endif
#ifdef HOST_NAME_MAX
  SCV(_SC_HOST_NAME_MAX, HOST_NAME_MAX),
#endif
#ifdef IOV_MAX
  SCV(_SC_IOV_MAX, IOV_MAX),
#endif
#ifdef LINE_MAX
  SCV(_SC_LINE_MAX, LINE_MAX),
#endif
#ifdef LOGIN_NAME_MAX
  SCV(_SC_LOGIN_NAME_MAX, LOGIN_NAME_MAX),
#endif
#ifdef MQ_OPEN_MAX
  SCV(_SC_MQ_OPEN_MAX, MQ_OPEN_MAX),
#endif
#ifdef MQ_PRIO_MAX
  SCV(_SC_MQ_PRIO_MAX, MQ_PRIO_MAX),
#endif
#ifdef NGROUPS_MAX
  SCV(_SC_NGROUPS_MAX, NGROUPS_MAX),
#endif
#ifdef RTSIG_MAX
  SCV(_SC_RTSIG_MAX, RTSIG_MAX),
#endif
#ifdef SEM_NSEMS_MAX
  SCV(_SC_SEM_NSEMS_MAX, SEM_NSEMS_MAX),
#endif
#ifdef SEM_VALUE_MAX
  SCV(_SC_SEM_VALUE_MAX, SEM_VALUE_MAX),
#endif
#ifdef SIGQUEUE_MAX
  SCV(_SC_SIGQUEUE_MAX, SIGQUEUE_MAX),
#endif
#ifdef STREAM_MAX
  SCV(_SC_STREAM_MAX, STREAM_MAX),
#endif
#ifdef SYMLOOP_MAX
  SCV(_SC_SYMLOOP_MAX, SYMLOOP_MAX),
#endif
#ifdef PTHREAD_DESTRUCTOR_ITERATIONS
  SCV(_SC_THREAD_DESTRUCTOR_ITERATIONS, PTHREAD_DESTRUCTOR_ITERATIONS),
#endif
#ifdef PTHREAD_STACK_MIN
  SCV(_SC_THREAD_STACK_MIN, PTHREAD_STACK_MIN),
#endif
#ifdef TIMER_MAX
  SCV(_SC_TIMER_MAX, TIMER_MAX),
#endif
#ifdef TTY_NAME_MAX
  SCV(_SC_TTY_NAME_MAX, TTY_NAME_MAX),
#endif
#ifdef TZNAME_MAX
  SCV(_SC_TZNAME_MAX, TZNAME_MAX),
#endif
#ifdef CONFIG_TLS_NELEM
  SCV(_SC_THREAD_KEYS_MAX, PTHREAD_KEYS_MAX),
#endif
#ifdef _SC_NSIG
  SCV(_SC_NSIG, NSIG),
#endif
#ifdef _POSIX_ASYNCHRONOUS_IO
  SCO(_SC_ASYNCHRONOUS_IO, _POSIX_ASYNCHRONOUS_IO),
#endif
#ifdef _POSIX_BARRIERS
  SCO(_SC_BARRIERS, _POSIX_BARRIERS),
#endif
#ifdef _POSIX_CLOCK_SELECTION
  SCO(_SC_CLOCK_SELECTION, _POSIX_CLOCK_SELECTION),
#endif
#ifdef _POSIX_CPUTIME
  SCO(_SC_CPUTIME, _POSIX_CPUTIME),
#endif
#ifdef _POSIX_FSYNC
  SCO(_SC_FSYNC, _POSIX_FSYNC),
#endif
#ifdef _POSIX_IPV6
  SCO(_SC_IPV6, _POSIX_IPV6),
#endif
#ifdef _POSIX_JOB_CONTROL
  SCO(_SC_JOB_CONTROL, _POSIX_JOB_CONTROL),
#endif
#ifdef _POSIX_MAPPED_FILES
  SCO(_SC_MAPPED_FILES, _POSIX_MAPPED_FILES),
#endif
#ifdef _POSIX_MEMLOCK
  SCO(_SC_MEMLOCK, _POSIX_MEMLOCK),
#endif
#ifdef _POSIX_MEMLOCK_RANGE
  SCO(_SC_MEMLOCK_RANGE, _POSIX_MEMLOCK_RANGE),
#endif
#ifdef _POSIX_MESSAGE_PASSING
  SCO(_SC_MESSAGE_PASSING, _POSIX_MESSAGE_PASSING),
#endif
#ifdef _POSIX_MONOTONIC_CLOCK
  SCO(_SC_MONOTONIC_CLOCK, _POSIX_MONOTONIC_CLOCK),
#endif
#ifdef _POSIX_PRIORITIZED_IO
  SCO(_SC_PRIORITIZED_IO, _POSIX_PRIORITIZED_IO),
#endif
#ifdef _POSIX_PRIORITY_SCHEDULING
  SCO(_SC_PRIORITY_SCHEDULING, _POSIX_PRIORITY_SCHEDULING),
#endif
#ifdef _POSIX_READER_WRITER_LOCKS
  SCO(_SC_READER_WRITER_LOCKS, _POSIX_READER_WRITER_LOCKS),
#endif
#ifdef _POSIX_REALTIME_SIGNALS
  SCO(_SC_REALTIME_SIGNALS, _POSIX_REALTIME_SIGNALS),
#endif
#ifdef _POSIX_SAVED_IDS
  SCO(_SC_SAVED_IDS, _POSIX_SAVED_IDS),
#endif
#ifdef _POSIX_SEMAPHORES
  SCO(_SC_SEMAPHORES, _POSIX_SEMAPHORES),
#endif
#ifdef _POSIX_SHARED_MEMORY_OBJECTS
  SCO(_SC_SHARED_MEMORY_OBJECTS, _POSIX_SHARED_MEMORY_OBJECTS),
#endif
#ifdef _POSIX_SPIN_LOCKS
  SCO(_SC_SPIN_LOCKS, _POSIX_SPIN_LOCKS),
#endif
#ifdef _POSIX_SPORADIC_SERVER
  SCO(_SC_SPORADIC_SERVER, _POSIX_SPORADIC_SERVER),
#endif
#ifdef _POSIX_SYNCHRONIZED_IO
  SCO(_SC_SYNCHRONIZED_IO, _POSIX_SYNCHRONIZED_IO),
#endif
#ifdef _POSIX_THREAD_ATTR_STACKADDR
  SCO(_SC_THREAD_ATTR_STACKADDR, _POSIX_THREAD_ATTR_STACKADDR),
#endif
#ifdef _POSIX_THREAD_ATTR_STACKSIZE
  SCO(_SC_THREAD_ATTR_STACKSIZE, _POSIX_THREAD_ATTR_STACKSIZE),
#endif
#ifdef _POSIX_THREAD_CPUTIME
  SCO(_SC_THREAD_CPUTIME, _POSIX_THREAD_CPUTIME),
#endif
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
  SCO(_SC_THREAD_PRIORITY_SCHEDULING, _POSIX_THREAD_PRIORITY_SCHEDULING),
#endif
#ifdef _POSIX_THREAD_PROCESS_SHARED
  SCO(_SC_THREAD_PROCESS_SHARED, _POSIX_THREAD_PROCESS_SHARED),
#endif
#ifdef _POSIX_THREAD_SPORADIC_SERVER
  SCO(_SC_THREAD_SPORADIC_SERVER, _POSIX_THREAD_SPORADIC_SERVER),
#endif
#ifdef _POSIX_THREADS
  SCO(_SC_THREADS, _POSIX_THREADS),
#endif
#ifdef _POSIX_TIMEOUTS
  SCO(_SC_TIMEOUTS, _POSIX_TIMEOUTS),
#endif
#ifdef _POSIX_TIMERS
  SCO(_SC_TIMERS, _POSIX_TIMERS),
#endif
#ifdef _POSIX_VERSION
  SCO(_SC_VERSION, _POSIX_VERSION),
#endif
#ifdef _XOPEN_UNIX
  SCO(_SC_XOPEN_UNIX, _XOPEN_UNIX),
#endif
#ifdef _XOPEN_VERSION
  SCO(_SC_XOPEN_VERSION, _XOPEN_VERSION),
#endif
};

/* Names that are not valid */

static const int g_invalid[] =
{
  -1, INT_MIN, INT_MAX
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Call sysconf() with errno set to a value that sysconf() does not set */

static long call_sysconf(int name, FAR int *errcode)
{
  long ret;

  errno = SYSCONF_ERRNO;
  ret = sysconf(name);
  *errcode = errno;
  return ret;
}

/* Each name gives a value, or -1 with errno unchanged */

static void test_names(FAR int *nerrors)
{
  long ret;
  int errcode;
  size_t i;

  for (i = 0; i < nitems(g_names); i++)
    {
      ret = call_sysconf(g_names[i].name, &errcode);
      if (ret < -1 || (ret == -1 && errcode != SYSCONF_ERRNO))
        {
          printf("ERROR: %s: %ld errno %d\n", g_names[i].str, ret,
                 errcode);
          (*nerrors)++;
        }
    }
}

/* A name that is not valid gives -1 with EINVAL */

static void test_invalid(FAR int *nerrors)
{
  long ret;
  int errcode;
  size_t i;

  for (i = 0; i < nitems(g_invalid); i++)
    {
      ret = call_sysconf(g_invalid[i], &errcode);
      if (ret != -1 || errcode != EINVAL)
        {
          printf("ERROR: name %d: %ld errno %d (expected -1, EINVAL)\n",
                 g_invalid[i], ret, errcode);
          (*nerrors)++;
        }
    }
}

/* Each name with a constant gives the value of the constant */

static void test_values(FAR int *nerrors)
{
  long ret;
  int errcode;
  size_t i;

  for (i = 0; i < nitems(g_values); i++)
    {
      if (g_values[i].option && g_values[i].value == 0)
        {
          continue;
        }

      ret = call_sysconf(g_values[i].name, &errcode);
      if (ret != g_values[i].value)
        {
          printf("ERROR: %s: %ld errno %d (expected %ld)\n",
                 g_values[i].str, ret, errcode, g_values[i].value);
          (*nerrors)++;
        }
    }
}

static void expect(FAR const char *what, bool ok, FAR int *nerrors)
{
  if (!ok)
    {
      printf("ERROR: %s\n", what);
      (*nerrors)++;
    }
}

/* The values that come from the system at run time */

static void test_system(FAR int *nerrors)
{
  struct rlimit rlim;
  long conf;
  long onln;
  long phys;
  long avphys;

  if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
    {
      expect("_SC_OPEN_MAX is the RLIMIT_NOFILE soft limit",
             sysconf(_SC_OPEN_MAX) == rlim.rlim_cur, nerrors);
    }

  expect("_SC_PAGESIZE > 0", sysconf(_SC_PAGESIZE) > 0, nerrors);
  expect("_SC_PAGE_SIZE is _SC_PAGESIZE",
         sysconf(_SC_PAGE_SIZE) == sysconf(_SC_PAGESIZE), nerrors);

  conf = sysconf(_SC_NPROCESSORS_CONF);
  onln = sysconf(_SC_NPROCESSORS_ONLN);
  expect("_SC_NPROCESSORS_ONLN >= 1, <= _SC_NPROCESSORS_CONF",
         onln >= 1 && onln <= conf, nerrors);

  phys = sysconf(_SC_PHYS_PAGES);
  avphys = sysconf(_SC_AVPHYS_PAGES);
  expect("_SC_PHYS_PAGES > 0, _SC_AVPHYS_PAGES <= _SC_PHYS_PAGES",
         phys > 0 && avphys >= 0 && avphys <= phys, nerrors);

  /* The compilation environments of POSIX.1-2008, 2017 and 2024 are the
   * same.  A build has at most one of them.
   */

  expect("at most one C compilation environment",
         (sysconf(_SC_V6_ILP32_OFF32) == 1) +
         (sysconf(_SC_V6_ILP32_OFFBIG) == 1) +
         (sysconf(_SC_V6_LP64_OFF64) == 1) +
         (sysconf(_SC_V6_LPBIG_OFFBIG) == 1) <= 1, nerrors);
#ifdef _SC_V7_ILP32_OFF32
  expect("_SC_V7_* are _SC_V6_*",
         sysconf(_SC_V7_ILP32_OFF32) == sysconf(_SC_V6_ILP32_OFF32) &&
         sysconf(_SC_V7_ILP32_OFFBIG) == sysconf(_SC_V6_ILP32_OFFBIG) &&
         sysconf(_SC_V7_LP64_OFF64) == sysconf(_SC_V6_LP64_OFF64) &&
         sysconf(_SC_V7_LPBIG_OFFBIG) == sysconf(_SC_V6_LPBIG_OFFBIG),
         nerrors);
#endif
#ifdef _SC_V8_ILP32_OFF32
  expect("_SC_V8_* are _SC_V6_*",
         sysconf(_SC_V8_ILP32_OFF32) == sysconf(_SC_V6_ILP32_OFF32) &&
         sysconf(_SC_V8_ILP32_OFFBIG) == sysconf(_SC_V6_ILP32_OFFBIG) &&
         sysconf(_SC_V8_LP64_OFF64) == sysconf(_SC_V6_LP64_OFF64) &&
         sysconf(_SC_V8_LPBIG_OFFBIG) == sysconf(_SC_V6_LPBIG_OFFBIG),
         nerrors);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int nerrors = 0;

  test_names(&nerrors);
  test_invalid(&nerrors);
  test_values(&nerrors);
  test_system(&nerrors);

  printf("sysconf test: %zu names, %d errors\n", nitems(g_names), nerrors);
  return nerrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
