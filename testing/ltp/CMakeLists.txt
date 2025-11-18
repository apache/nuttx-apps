# ##############################################################################
# apps/testing/ltp/CMakeLists.txt
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

if(CONFIG_TESTING_LTP)
  set(LTPS_VERSION 20230516)
  set(LTP_DOWNLOAD_URL
      https://github.com/linux-test-project/ltp/archive/refs/tags/)
  set(LTP_UNPACK ${CMAKE_CURRENT_SOURCE_DIR}/ltp)
  set(LTP_URL https://github.com/linux-test-project/ltp.git)

  set(TESTDIR ${LTP_UNPACK}/testcases/open_posix_testsuite)

  if(NOT EXISTS ${TESTDIR})
    FetchContent_Declare(
      ltp_fetch
      DOWNLOAD_NAME "ltp-${LTPS_VERSION}.zip"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "${LTP_DOWNLOAD_URL}${LTPS_VERSION}.zip" SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/ltp BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/testing/ltp/ltp
      PATCH_COMMAND
        patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-pthread_rwlock_unlock-follow-linux.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0002-Use-ifdef-instead-of-if-for-__linux__.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0003-LTP-Re-init-static-variable-to-avoid-errors-during-m.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0004-test-update-ltp-test-case.patch && patch
        -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0005-ltp-update-pthread_cond_timedwait-testcase.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0006-test-modified-ltp-rwlock-need-init.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0007-pthread_kill-add-usleep-aviod-semcount-overturn.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0008-test-ltp-fix-ltp_interfaces_sigaction_23_10-deadloop.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0009-ltp-modify-user-code-for-fdcheck-compatibility.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0010-ltp-fix-build-warning.patch && patch -p1
        -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0011-ltp-fix-the-proc.h-header-file-duplicate-inclusion.patch
        && patch -p1 -d ${LTP_UNPACK} <
        ${CMAKE_CURRENT_LIST_DIR}/0012-ltp-fix-build-error.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(ltp_fetch)
    if(NOT ltp_fetch_POPULATED)
      FetchContent_Populate(ltp_fetch)
    endif()
  endif()

  if(EXISTS ${TESTDIR})
    set(BLACKWORDS)
    set(BLACKSRCS)

    if(NOT CONFIG_FS_AIO)
      list(APPEND BLACKWORDS "aio.h" "SIGPOLL")
    endif()
    list(APPEND BLACKWORDS "pthread_mutexattr_setprioceiling"
         "pthread_mutexattr_getprioceiling" "pthread_getattr_np"
         "pthread_mutex_getprioceiling")

    if(NOT CONFIG_PTHREAD_SPINLOCKS)
      list(APPEND BLACKWORDS "pthread_spin_init" "pthread_spin_destroy"
           "pthread_spin_trylock")
    endif()
    list(
      APPEND
      BLACKWORDS
      "CHILD_MAX"
      "setpgid("
      "PTHREAD_SCOPE_PROCESS"
      "setpgrp"
      "threads_scenarii.c"
      "pthread_mutex_lock"
      "ucontext.h"
      "msync"
      "lfind")

    list(
      APPEND
      BLACKSRCS
      ${TESTDIR}/conformance/definitions/time_h/19-1-buildonly.c
      ${TESTDIR}/conformance/definitions/signal_h/21-1-buildonly.c
      ${TESTDIR}/conformance/definitions/signal_h/27-1-buildonly.c
      ${TESTDIR}/conformance/definitions/time_h/19-1-buildonly.c
      ${TESTDIR}/conformance/interfaces/sem_post/8-1.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/9-1.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/2-2.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/10-1.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/1-5.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/1-3.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/2-3.c
      ${TESTDIR}/conformance/interfaces/clock_nanosleep/1-4.c
      ${TESTDIR}/conformance/interfaces/pthread_sigmask/18-1.c
      ${TESTDIR}/conformance/interfaces/pthread_sigmask/4-1.c
      ${TESTDIR}/conformance/interfaces/sigprocmask/4-1.c)

    if(NOT CONFIG_LIBC_LOCALTIME)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/definitions/time_h/34-1-buildonly.c
           ${TESTDIR}/conformance/definitions/time_h/35-3-buildonly.c)
    endif()

    list(
      APPEND
      BLACKSRCS
      ${TESTDIR}/conformance/definitions/time_h/35-1-buildonly.c
      ${TESTDIR}/conformance/definitions/time_h/35-2-buildonly.c
      ${TESTDIR}/stress/threads/pthread_kill/stress.c)

    if(NOT CONFIG_PTHREAD_SPINLOCKS)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/definitions/pthread_h/3-12-buildonly.c)
    endif()

    if(NOT CONFIG_SCHED_CHILD_STATUS)
      list(
        APPEND BLACKSRCS ${TESTDIR}/conformance/interfaces/pthread_exit/6-1.c
        ${TESTDIR}/conformance/behavior/WIFEXITED/1-1.c
        ${TESTDIR}/conformance/behavior/WIFEXITED/1-2.c)
    endif()

    if(NOT CONFIG_FS_TMPFS)
      list(APPEND BLACKWORDS "lio_listio")
    endif()

    if(NOT CONFIG_SIG_EVTHREAD)
      list(APPEND BLACKWORDS "fork")
    endif()

    if(NOT CONFIG_MQ_MAXMSGSIZE GREATER_EQUAL 64)
      list(
        APPEND
        BLACKWORDS
        "mq_close"
        "mq_getattr"
        "mq_notify"
        "mq_open"
        "mq_receive"
        "mq_send"
        "mq_setattr"
        "mq_timedreceive"
        "mq_timedsend"
        "mq_unlink")
    endif()

    if(NOT CONFIG_PTHREAD_MUTEX_TYPES)
      list(APPEND BLACKWORDS "pthread_mutexattr_settype")
    endif()

    if(NOT CONFIG_SCHED_SPORADIC)
      list(
        APPEND
        BLACKSRCS
        ${TESTDIR}/conformance/interfaces/sched_setscheduler/19-2.c
        ${TESTDIR}/conformance/interfaces/sched_setscheduler/19-3.c
        ${TESTDIR}/conformance/interfaces/sched_setscheduler/19-4.c)
    endif()

    if(NOT CONFIG_SIG_DEFAULT)
      list(APPEND BLACKWORDS "SIGKILL" "SIGSTOP")
    endif()

    if(NOT CONFIG_RR_INTERVAL GREATER 0)
      list(APPEND BLACKWORDS "SCHED_RR")
    endif()

    if(NOT CONFIG_FS_NAMED_SEMAPHORES)
      list(APPEND BLACKWORDS "sem_open" "sem_close" "sem_unlink")
    endif()

    if(NOT CONFIG_PIPES)
      list(APPEND BLACKWORDS "pipe")
    endif()

    if(NOT CONFIG_LIBC_MAX_EXITFUNS GREATER_EQUAL 32)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/interfaces/pthread_exit/4-1.c
           ${TESTDIR}/conformance/interfaces/pthread_exit/5-1.c)
    endif()

    if(NOT CONFIG_CANCELLATION_POINTS)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/interfaces/pthread_exit/3-1.c)
    endif()

    if(NOT CONFIG_TLS_NELEM GREATER_EQUAL 16)
      list(APPEND BLACKSRCS
           ${TESTDIR}/conformance/interfaces/pthread_setspecific/1-1.c)
    endif()

    if(NOT CONFIG_PTHREAD_SPINLOCKS)
      list(APPEND BLACKWORDS "pthread_spin_lock")
    endif()

    if(NOT CONFIG_SCHED_WAITPID)
      list(APPEND BLACKWORDS "waitpid")
    endif()

    if(NOT CONFIG_SCHED_USER_IDENTITY)
      list(
        APPEND
        BLACKWORDS
        "setuid"
        "setgid"
        "getuid"
        "getgid"
        "seteuid"
        "setegid"
        "geteuid"
        "getegid")
    endif()

    if(CONFIG_TLS_NELEM EQUAL 0)
      list(APPEND BLACKWORDS "pthread_key_create" "pthread_key_delete"
           "pthread_setspecific" "pthread_getspecific")
    endif()

    if(CONFIG_TLS_NCLEANUP EQUAL 0)
      list(APPEND BLACKWORDS "pthread_cleanup_push" "pthread_cleanup_pop")
    endif()

    if(NOT CONFIG_LIBC_EXECFUNCS)
      list(APPEND BLACKWORDS "execl" "execvp" "execlp" "execvpe")
    endif()

    if(NOT CONFIG_FS_SHMFS)
      list(APPEND BLACKWORDS "shm_open" "shm_unlink")
    endif()

    if(NOT LIBC_LOCALE_CATALOG)
      list(APPEND BLACKWORDS "catopen" "catgets" "catclose")
    endif()

    if(NOT CONFIG_MM_SHM)
      list(APPEND BLACKWORDS "shmget" "shmat")
    endif()

    file(GLOB_RECURSE LTP_ORIGS ${TESTDIR}/*.c)

    if(LTP_ORIGS)
      foreach(word IN LISTS BLACKWORDS)
        foreach(file ${LTP_ORIGS})
          file(STRINGS ${file} content)
          string(FIND "${content}" "${word}" pos)
          if(pos GREATER -1)
            list(APPEND BLACKLIST ${file})
          endif()
        endforeach()
      endforeach()
    endif()

    foreach(src ${BLACKSRCS})
      foreach(orig ${LTP_ORIGS})
        string(REGEX MATCH "${src}$" matched ${orig})
        if(matched)
          list(APPEND BLACKLIST ${orig})
        endif()
      endforeach()
    endforeach()

    foreach(ITEM ${BLACKLIST})
      list(REMOVE_ITEM LTP_ORIGS ${ITEM})
    endforeach()

    set(LTP_MAINCSRCS "")
    if(LTP_ORIGS)
      foreach(file ${LTP_ORIGS})
        file(STRINGS ${file} content)
        string(FIND "${content}" "main(" pos)
        if(pos GREATER -1)
          list(APPEND LTP_MAINCSRCS ${file})
        endif()
      endforeach()
    endif()

    set(LTP_CSRCS ${LTP_ORIGS})
    foreach(ITEM ${LTP_MAINCSRCS})
      list(REMOVE_ITEM LTP_CSRCS ${ITEM})
    endforeach()

    set(PROGNAME "")
    if(LTP_MAINCSRCS)
      foreach(file ${LTP_MAINCSRCS})
        string(REPLACE "/" ";" file_parts ${file})
        list(LENGTH file_parts num_parts)
        if(num_parts GREATER 2)
          list(GET file_parts -3 part1)
          list(GET file_parts -2 part2)
          list(GET file_parts -1 part3)
          string(REPLACE "-" "_" part1 ${part1})
          string(REPLACE "-" "_" part2 ${part2})
          string(REPLACE "-" "_" part3 ${part3})
          set(prog "ltp_${part1}_${part2}_${part3}")
          get_filename_component(progg ${prog} NAME_WE)
          list(APPEND PROGNAME ${progg})
        endif()
      endforeach()
    endif()

    if(CONFIG_NET_CAN)
      list(APPEND PROGNAME "can_rcv_own_msgs" "can_filter")
      list(APPEND LTP_MAINCSRCS
           "${TESTDIR}/../network/can/filter-tests/can_filter.c"
           "${TESTDIR}/../network/can/filter-tests/can_rcv_own_msgs.c")
    endif()

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${TESTDIR}/include)
    include_directories(${TESTDIR}/../../include)

    add_compile_options(
      -Wall
      -Wno-strict-prototypes
      -Wno-return-type
      -Wno-format
      -Wno-uninitialized
      -Wno-unused-variable
      -Wno-unused-function
      -Wno-unused-but-set-variable
      -Wno-unused-value
      -Wno-int-conversion
      -Wno-shadow
      -Wno-incompatible-pointer-types
      -Wno-overflow
      -Wno-int-to-pointer-cast)

    if(APPLE)
      add_compile_options(-Wno-integer-overflow -Wno-absolute-value)
    endif()

    list(LENGTH LTP_MAINCSRCS SOURCE_FILES_COUNT)
    math(EXPR LAST_INDEX "${SOURCE_FILES_COUNT} - 1")

    foreach(INDEX RANGE 0 ${LAST_INDEX})
      list(GET LTP_MAINCSRCS ${INDEX} SOURCE_FILES)
      list(GET PROGNAME ${INDEX} EXECUTABLE_NAMES)
      nuttx_add_application(
        NAME
        ${EXECUTABLE_NAMES}
        PRIORITY
        SCHED_PRIORITY_DEFAULT
        STACKSIZE
        ${CONFIG_TESTING_LTP_STACKSIZE}
        SRCS
        ${SOURCE_FILES})
    endforeach()
  endif()

  nuttx_add_library(ltp)
  target_sources(ltp PRIVATE ${LTP_CSRCS})

endif()
