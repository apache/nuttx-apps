# ##############################################################################
# apps/benchmarks/fio/CMakeLists.txt
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

if(CONFIG_BENCHMARK_FIO)

  set(FIOAPP_DIR ${CMAKE_CURRENT_LIST_DIR}/fio)

  if(NOT EXISTS ${FIOAPP_DIR})
    FetchContent_Declare(
      fio_fetch
      URL https://github.com/ldorau/fio/archive/refs/heads/master.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/fio BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/benchmarks/fio/fio
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30
      PATCH_COMMAND
        patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0001-external-fio-fix-compile-warning.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0002-external-fio-add-os-nuttx-support.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0003-external-fio-modify-smaller-size-for-nuttx.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0004-external-fio-fix-runtime-error-by-asan.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0005-external-fio-fix-compile-warning.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0006-external-fio-reinit-global-var-issue-when-run-multip.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0007-external-fio-add-engine-init.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0008-fio-fix-memory-leak-run-cpuio.fio.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0009-fio-fix-memory-leak-ioengine-filecreate.patch
        COMMAND patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/fio <
        ${CMAKE_CURRENT_LIST_DIR}/0010-fio-fix-memory-leak-ioengine-exec.patch)

    FetchContent_GetProperties(fio_fetch)
    if(NOT fio_fetch_POPULATED)
      FetchContent_Populate(fio_fetch)
    endif()
  endif()

  file(GLOB LIB_SRCS ${FIOAPP_DIR}/lib/*.c)
  file(GLOB CRC_SRCS ${FIOAPP_DIR}/crc/*.c)

  set(CSRCS
      ${LIB_SRCS}
      ${CRC_SRCS}
      ${FIOAPP_DIR}/gettime.c
      ${FIOAPP_DIR}/ioengines.c
      ${FIOAPP_DIR}/init.c
      ${FIOAPP_DIR}/stat.c
      ${FIOAPP_DIR}/log.c
      ${FIOAPP_DIR}/time.c
      ${FIOAPP_DIR}/filesetup.c
      ${FIOAPP_DIR}/eta.c
      ${FIOAPP_DIR}/verify.c
      ${FIOAPP_DIR}/memory.c
      ${FIOAPP_DIR}/io_u.c
      ${FIOAPP_DIR}/parse.c
      ${FIOAPP_DIR}/fio_sem.c
      ${FIOAPP_DIR}/rwlock.c
      ${FIOAPP_DIR}/pshared.c
      ${FIOAPP_DIR}/options.c
      ${FIOAPP_DIR}/smalloc.c
      ${FIOAPP_DIR}/filehash.c
      ${FIOAPP_DIR}/profile.c
      ${FIOAPP_DIR}/debug.c
      ${FIOAPP_DIR}/server.c
      ${FIOAPP_DIR}/client.c
      ${FIOAPP_DIR}/iolog.c
      ${FIOAPP_DIR}/backend.c
      ${FIOAPP_DIR}/libfio.c
      ${FIOAPP_DIR}/flow.c
      ${FIOAPP_DIR}/cconv.c
      ${FIOAPP_DIR}/gettime-thread.c
      ${FIOAPP_DIR}/helpers.c
      ${FIOAPP_DIR}/json.c
      ${FIOAPP_DIR}/idletime.c
      ${FIOAPP_DIR}/td_error.c
      ${FIOAPP_DIR}/zbd.c
      ${FIOAPP_DIR}/profiles/tiobench.c
      ${FIOAPP_DIR}/profiles/act.c
      ${FIOAPP_DIR}/io_u_queue.c
      ${FIOAPP_DIR}/filelock.c
      ${FIOAPP_DIR}/steadystate.c
      ${FIOAPP_DIR}/workqueue.c
      ${FIOAPP_DIR}/rate-submit.c
      ${FIOAPP_DIR}/optgroup.c
      ${FIOAPP_DIR}/helper_thread.c
      ${FIOAPP_DIR}/zone-dist.c
      ${FIOAPP_DIR}/dedupe.c
      ${FIOAPP_DIR}/engines/exec.c
      ${FIOAPP_DIR}/engines/cpu.c
      ${FIOAPP_DIR}/engines/sync.c
      ${FIOAPP_DIR}/engines/ftruncate.c
      ${FIOAPP_DIR}/engines/falloc.c
      ${FIOAPP_DIR}/engines/fileoperations.c
      ${FIOAPP_DIR}/engines/mmap.c
      ${FIOAPP_DIR}/engines/null.c
      ${FIOAPP_DIR}/engines/net.c)

  set(CFLAGS
      -DCONFIG_GETTIMEOFDAY
      -DCONFIG_CLOCK_GETTIME
      -DCONFIG_HAVE_GETTID
      -DCONFIG_STATIC_ASSERT
      -DCONFIG_LITTLE_ENDIAN
      -DCONFIG_POSIX_FALLOCATE
      -DCONFIG_HAVE_BOOL
      -DFIO_INTERNAL
      -DCONFIG_NO_SHM
      -DFIO_VERSION=\"1.2.0\"
      -DFIO_USE_GENERIC_SWAP
      -DFIO_USE_GENERIC_INIT_RANDOM_STATE
      -DFIO_USE_GENERIC_BDEV_SIZE
      -DBLOOM_SIZE=1024*1024
      -DBUF_SIZE=512*1024
      -DMAX_POOLS=4
      -DINITIAL_POOLS=2
      -DINITIAL_SIZE=512*1024
      -DCONFIG_SEED_BUCKETS=8
      -DBITS_PER_LONG=32
      -DXXH32_digest=fio_XXH32_digest
      -DXXH32_update=fio_XXH32_update
      -DXXH32=fio_XXH32
      -Dparse_options=fio_parse_options
      -Dparse_option=fio_parse_option)

  set(INCDIR ${FIOAPP_DIR} ${FIOAPP_DIR}/arch ${FIOAPP_DIR}/crc
             ${FIOAPP_DIR}/os)
  include_directories(${INCDIR})

  if(CONFIG_ARCH_ARM)
    list(APPEND CFLAGS -D__ARM_ARCH_6__)
  endif()

  set(SRCS ${FIOAPP_DIR}/fio.c ${CSRCS})
  nuttx_add_application(
    NAME
    fio
    PRIORITY
    ${CONFIG_BENCHMARK_FIO_PRIORITY}
    STACKSIZE
    ${CONFIG_BENCHMARK_FIO_STACKSIZE}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})

endif()
