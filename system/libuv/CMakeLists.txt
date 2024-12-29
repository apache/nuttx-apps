# ##############################################################################
# apps/system/libuv/CMakeLists.txt
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

if(CONFIG_LIBUV)

  # ############################################################################
  # Config and Fetch libuv
  # ############################################################################

  set(LIBUV_DIR ${CMAKE_CURRENT_LIST_DIR}/libuv)

  if(NOT EXISTS ${LIBUV_DIR})
    set(LIBUV_URL https://github.com/libuv/libuv/archive/refs/tags/v1.46.0.zip)
    FetchContent_Declare(
      libuv_fetch
      URL ${LIBUV_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libuv BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/system/libuv/libuv
      PATCH_COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/libuv <
                    ${CMAKE_CURRENT_LIST_DIR}/0001-libuv-port-for-nuttx.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(libuv_fetch)

    if(NOT libuv_fetch_POPULATED)
      FetchContent_Populate(libuv_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS
      -Wno-shadow
      -DDEF_THREADPOOL_SIZE=CONFIG_LIBUV_THREADPOOL_SIZE
      -DDEF_THREADPOOL_STACKSIZE=CONFIG_LIBUV_THREAD_STACKSIZE
      -DDEF_THREADPOOL_PRIORITY=CONFIG_LIBUV_THREADPOOL_PRIORITY
      -DMAX_EPOLL_EVENTS=CONFIG_LIBUV_MAX_EPOLL_EVENTS
      -DPREP_EVENT_SIZE=CONFIG_LIBUV_PREP_EVENT_SIZE
      -DDEF_STREAM_READ_BUF_SIZE=CONFIG_LIBUV_STREAM_READ_BUF_SIZE)

  if(GCCVER GREATER_EQUAL 12)
    list(APPEND CFLAGS -Wno-dangling-pointer)
  endif()
  # ############################################################################
  # Sources
  # ############################################################################
  set(LIBUV_SRC_DIR ${LIBUV_DIR}/src)
  set(LIBUV_UNIX_DIR ${LIBUV_DIR}/src/unix)
  set(LIBUV_TEST_DIR ${LIBUV_DIR}/test)

  set(SRCS
      ${LIBUV_UNIX_DIR}/core.c ${LIBUV_UNIX_DIR}/poll.c
      ${LIBUV_UNIX_DIR}/loop.c ${LIBUV_UNIX_DIR}/thread.c
      ${LIBUV_SRC_DIR}/thread-common.c ${LIBUV_UNIX_DIR}/posix-hrtime.c)

  if(CONFIG_LIBUV_BACKEND_EPOLL)
    list(APPEND SRCS ${LIBUV_UNIX_DIR}/linux.c)
  else()
    list(APPEND SRCS ${LIBUV_UNIX_DIR}/posix-poll.c
         ${LIBUV_UNIX_DIR}/no-fsevents.c)
  endif()

  list(APPEND SRCS ${LIBUV_SRC_DIR}/uv-data-getter-setters.c
       ${LIBUV_SRC_DIR}/version.c)

  if(NOT CONFIG_LIBUV_UTILS_TEST)
    list(APPEND SRCS ${LIBUV_SRC_DIR}/idna.c ${LIBUV_SRC_DIR}/strscpy.c
         ${LIBUV_SRC_DIR}/strtok.c)
  endif()

  list(
    APPEND
    SRCS
    ${LIBUV_SRC_DIR}/uv-common.c
    ${LIBUV_UNIX_DIR}/random-devurandom.c
    ${LIBUV_SRC_DIR}/random.c
    ${LIBUV_UNIX_DIR}/nuttx.c
    ${LIBUV_UNIX_DIR}/tty.c
    ${LIBUV_UNIX_DIR}/loop-watcher.c
    ${LIBUV_UNIX_DIR}/signal.c
    ${LIBUV_UNIX_DIR}/stream.c
    ${LIBUV_SRC_DIR}/threadpool.c
    ${LIBUV_UNIX_DIR}/async.c
    ${LIBUV_UNIX_DIR}/pipe.c
    ${LIBUV_UNIX_DIR}/fs.c
    ${LIBUV_SRC_DIR}/fs-poll.c
    ${LIBUV_SRC_DIR}/timer.c
    ${LIBUV_UNIX_DIR}/process-spawn.c
    ${LIBUV_UNIX_DIR}/sysinfo-loadavg.c
    ${LIBUV_UNIX_DIR}/sysinfo-memory.c)

  if(CONFIG_LIBC_DLFCN)
    list(APPEND SRCS ${LIBUV_UNIX_DIR}/dl.c)
  endif()

  if(CONFIG_LIBC_NETDB)
    list(APPEND SRCS ${LIBUV_UNIX_DIR}/getaddrinfo.c
         ${LIBUV_UNIX_DIR}/getnameinfo.c)
  endif()

  if(CONFIG_NET)
    list(APPEND SRCS ${LIBUV_SRC_DIR}/inet.c)
  endif()

  if(CONFIG_NET_TCP)
    list(APPEND SRCS ${LIBUV_UNIX_DIR}/tcp.c)
  endif()

  if(CONFIG_NET_UDP)
    list(APPEND SRCS ${LIBUV_UNIX_DIR}/udp.c)
  endif()
  # ############################################################################
  # Include Directory
  # ############################################################################

  set(EXPORT_INCDIR ${CMAKE_CURRENT_LIST_DIR}/libuv/include)
  set(INTER_INCDIR ${LIBUV_DIR}/src ${LIBUV_DIR}/src/unix ${LIBUV_DIR}/test)

  # ############################################################################
  # Library Configuration
  # ############################################################################

  set(INCDIR ${EXPORT_INCDIR} ${INTER_INCDIR})

  nuttx_add_library(libuv STATIC)
  target_sources(libuv PRIVATE ${SRCS})
  target_include_directories(libuv PRIVATE ${INCDIR})
  target_compile_options(libuv PRIVATE ${CFLAGS})

  # Global FLAG
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_COMPILE_OPTIONS
             -DUV_HANDLE_BACKTRACE=CONFIG_LIBUV_HANDLE_BACKTRACE)
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${EXPORT_INCDIR})

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_LIBUV_UTILS_TEST)
    set(LIBUV_UTILS_TEST_SRCS
        ${LIBUV_TEST_DIR}/run-tests.c ${LIBUV_TEST_DIR}/runner.c
        ${LIBUV_TEST_DIR}/runner-unix.c ${LIBUV_TEST_DIR}/echo-server.c)
    file(GLOB TEST_CSRCS ${LIBUV_TEST_DIR}/test-*.c)
    list(APPEND LIBUV_UTILS_TEST_SRCS ${TEST_CSRCS})
    nuttx_add_application(
      NAME
      uv_run_tests
      STACKSIZE
      ${CONFIG_LIBUV_UTILS_STACKSIZE}
      PRIORITY
      ${CONFIG_LIBUV_UTILS_PRIORITY}
      SRCS
      ${LIBUV_UTILS_TEST_SRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libuv)
  endif()

  if(CONFIG_LIBUV_UTILS_BENCHMARK)
    set(LIBUV_UTILS_BENCHMARK_SRCS
        ${LIBUV_TEST_DIR}/run-benchmarks.c
        ${LIBUV_TEST_DIR}/runner.c
        ${LIBUV_TEST_DIR}/runner-unix.c
        ${LIBUV_TEST_DIR}/echo-server.c
        ${LIBUV_TEST_DIR}/blackhole-server.c
        ${LIBUV_TEST_DIR}/benchmark-async-pummel.c
        ${LIBUV_TEST_DIR}/benchmark-async.c
        ${LIBUV_TEST_DIR}/benchmark-fs-stat.c
        ${LIBUV_TEST_DIR}/benchmark-getaddrinfo.c
        ${LIBUV_TEST_DIR}/benchmark-loop-count.c
        ${LIBUV_TEST_DIR}/benchmark-million-async.c
        ${LIBUV_TEST_DIR}/benchmark-million-timers.c
        ${LIBUV_TEST_DIR}/benchmark-multi-accept.c
        ${LIBUV_TEST_DIR}/benchmark-ping-pongs.c
        ${LIBUV_TEST_DIR}/benchmark-ping-udp.c
        ${LIBUV_TEST_DIR}/benchmark-pound.c
        ${LIBUV_TEST_DIR}/benchmark-pump.c
        ${LIBUV_TEST_DIR}/benchmark-sizes.c
        ${LIBUV_TEST_DIR}/benchmark-spawn.c
        ${LIBUV_TEST_DIR}/benchmark-tcp-write-batch.c
        ${LIBUV_TEST_DIR}/benchmark-thread.c
        ${LIBUV_TEST_DIR}/benchmark-udp-pummel.c)

    nuttx_add_application(
      NAME
      uv_run_benchmarks
      STACKSIZE
      ${CONFIG_LIBUV_UTILS_STACKSIZE}
      PRIORITY
      ${CONFIG_LIBUV_UTILS_PRIORITY}
      SRCS
      ${LIBUV_UTILS_BENCHMARK_SRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libuv)
  endif()

endif()
