############################################################################
# apps/benchmarks/rt-tests/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

if(CONFIG_BENCHMARK_RTTESTS)

  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rt-tests/.git)
    set(RTTESTS_URL
        "https://git.kernel.org/pub/scm/utils/rt-tests/rt-tests.git")
    set(RTTESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rt-tests)

    execute_process(
      COMMAND git clone ${RTTESTS_URL} ${RTTESTS_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_CLONE_RESULT)

    if(NOT ${GIT_CLONE_RESULT} EQUAL 0)
      message(FATAL_ERROR "Failed to clone rt-tests repository")
    endif()

    execute_process(
      COMMAND cd ${RTTESTS_DIR} && git checkout -q ${RTTESTS_VERSION} && patch
              -p1 < ../0001-rt-tests-cyclictest-Port-for-NuttX.patch
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_custom_target(distclean COMMAND ${CMAKE_COMMAND} -E remove_directory
                                        ${RTTESTS_DIR})
  endif()

  list(
    APPEND
    SRCS
    rt-tests/src/cyclictest/cyclictest.c
    rt-tests/src/lib/rt-error.c
    rt-tests/src/lib/histogram.c
    rt-tests/src/lib/rt-utils.c
    rt-tests/src/lib/rt-numa.c)

  list(
    APPEND
    CFLAGS
    -Irt-tests/src/include
    -I.
    -DVERSION=0.1
    -DSYS_gettid=0
    -Dgettid=rttest_gettid
    -D'syscall
    (x)
    =
    ((pid_t) (x))
    '
    -w
    -DRTTESTS_PRIORITY=${CONFIG_BENCHMARK_RTTESTS_PRIORITY}+1)

  nuttx_add_application(
    NAME
    cyclictest
    PRIORITY
    ${CONFIG_BENCHMARK_RTTESTS_PRIORITY}
    STACKSIZE
    ${CONFIG_BENCHMARK_RTTESTS_STACKSIZE}
    MODULE
    ${CONFIG_BENCHMARK_RTTESTS}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})
endif()
