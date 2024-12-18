############################################################################
# apps/benchmarks/test-tlb/CMakeLists.txt
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
if(CONFIG_BENCHMARK_TESTTLB)

  set(SRCS test-tlb/test-tlb.c)

  set(TESTTLB_UNPACK ${CMAKE_CURRENT_LIST_DIR}/test-tlb)
  set(TESTTLB_URL https://github.com/torvalds/test-tlb/archive)
  set(TESTTLB_ZIP master.zip)

  if(NOT EXISTS ${TESTTLB_UNPACK})

    FetchContent_Declare(
      testtlb_fetch
      URL ${TESTTLB_URL}/${TESTTLB_ZIP} SOURCE_DIR ${TESTTLB_UNPACK} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/benchmarks/test-tlb/test-tlb
      PATCH_COMMAND patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
                    ${CMAKE_CURRENT_LIST_DIR}/0001-test-tlb-port-for-NuttX.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(testtlb_fetch)
    if(NOT testtlb_fetch_POPULATED)
      FetchContent_Populate(testtlb_fetch)
    endif()

  endif()

  nuttx_add_application(
    NAME
    testtlb
    PRIORITY
    ${CONFIG_BENCHMARK_TESTTLB_PRIORITY}
    STACKSIZE
    ${CONFIG_BENCHMARK_TESTTLB_STACKSIZE}
    MODULE
    ${CONFIG_BENCHMARK_TESTTLB}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})
endif()
