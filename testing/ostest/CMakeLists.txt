# ##############################################################################
# apps/testing/ostest/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_TESTING_OSTEST)

  set(SRCS
      getopt.c
      libc_memmem.c
      restart.c
      sigprocmask.c
      sighand.c
      signest.c
      sighelper.c)

  if(CONFIG_DEV_NULL)
    list(APPEND SRCS dev_null.c)
  endif()

  if(CONFIG_SIG_SIGSTOP_ACTION)
    if(CONFIG_SIG_SIGKILL_ACTION)
      list(APPEND SRCS suspend.c)
    endif()
  endif()

  if(CONFIG_ARCH_FPU)
    if(NOT CONFIG_TESTING_OSTEST_FPUTESTDISABLE)
      list(APPEND SRCS fpu.c)
    endif()
  endif()

  if(NOT CONFIG_STDIO_DISABLE_BUFFERING)
    list(APPEND SRCS setvbuf.c)
  endif()

  if(NOT CONFIG_TLS_NELEM EQUAL 0)
    list(APPEND SRCS tls.c)
  endif()

  if(CONFIG_TESTING_OSTEST_AIO)
    list(APPEND SRCS aio.c)
  endif()

  if(CONFIG_SCHED_WAITPID)
    list(APPEND SRCS waitpid.c)
  endif()

  if(NOT CONFIG_DISABLE_PTHREAD)
    list(
      APPEND
      SRCS
      cancel.c
      cond.c
      mutex.c
      timedmutex.c
      sem.c
      semtimed.c
      barrier.c)
    list(APPEND SRCS timedwait.c pthread_rwlock.c pthread_rwlock_cancel.c
         schedlock.c)

    if(CONFIG_SCHED_WAITPID)
      list(APPEND SRCS pthread_exit.c)
    endif()

    if(NOT CONFIG_TLS_NELEM EQUAL 0)
      list(APPEND SRCS specific.c)
    endif()

    if(CONFIG_SCHED_THREAD_LOCAL)
      list(APPEND SRCS sched_thread_local.c)
    endif()

    if(NOT CONFIG_TLS_NCLEANUP EQUAL 0)
      list(APPEND SRCS pthread_cleanup.c)
    endif()

    if(NOT CONFIG_PTHREAD_MUTEX_UNSAFE)
      list(APPEND SRCS robust.c)
    endif()

    if(CONFIG_PTHREAD_MUTEX_TYPES)
      list(APPEND SRCS rmutex.c)
    endif()

    if(CONFIG_FS_NAMED_SEMAPHORES)
      list(APPEND SRCS nsem.c)
    endif()

    if(NOT "${CONFIG_RR_INTERVAL}" STREQUAL "0")
      list(APPEND SRCS roundrobin.c)
    endif()

    if(CONFIG_SCHED_SPORADIC)
      list(APPEND SRCS sporadic.c sporadic2.c)
    endif()

    if(CONFIG_SCHED_WORKQUEUE)
      list(APPEND SRCS wqueue.c)
    endif()

    if(CONFIG_PRIORITY_INHERITANCE)
      list(APPEND SRCS prioinherit.c)
    endif() # CONFIG_PRIORITY_INHERITANCE
  endif() # CONFIG_DISABLE_PTHREAD

  if(NOT CONFIG_DISABLE_MQUEUE)
    if(NOT CONFIG_DISABLE_PTHREAD)
      list(APPEND SRCS mqueue.c timedmqueue.c)
    endif() # CONFIG_DISABLE_PTHREAD
  endif() # CONFIG_DISABLE_MQUEUE

  if(NOT CONFIG_DISABLE_POSIX_TIMERS)
    list(APPEND SRCS posixtimer.c)
    if(CONFIG_SIG_EVTHREAD)
      list(APPEND SRCS sigev_thread.c)
    endif()
  endif()

  if(CONFIG_ARCH_HAVE_FORK)
    if(CONFIG_SCHED_WAITPID)
      list(APPEND SRCS vfork.c)
    endif()
  endif()

  if(CONFIG_ARCH_SETJMP_H)
    list(APPEND SRCS setjmp.c)
  endif()

  if(CONFIG_SMP)
    list(APPEND SRCS smp_call.c)
  endif()

  if(CONFIG_SCHED_EVENTS AND CONFIG_BUILD_FLAT)
    list(APPEND SRCS nxevent.c)
  endif()

  if(CONFIG_BUILD_FLAT)
    list(APPEND SRCS wdog.c)
  endif()

  set(OSTEST_SRCS ostest_main.c ${SRCS})
  nuttx_add_application(NAME ostest SRCS ${OSTEST_SRCS})

endif()
