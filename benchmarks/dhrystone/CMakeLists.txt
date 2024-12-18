# ##############################################################################
# apps/benchmarks/dhrystone/CMakeLists.txt
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

if(CONFIG_BENCHMARK_DHRYSTONE)

  set(DHRYSTONE_DIR ${CMAKE_CURRENT_LIST_DIR}/dhrystone)

  if(NOT EXISTS ${DHRYSTONE_DIR})
    FetchContent_Declare(
      dhrystone_fetch
      URL https://github.com/Keith-S-Thompson/dhrystone/archive/refs/heads/master.zip
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/dhrystone
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/benchmarks/coremark/dhrystone
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-dry2.2-Fix-malloc-type-mismatch.patch &&
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0002-dhrystone-fix-redefine-warning.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(dhrystone_fetch)
    if(NOT dhrystone_fetch_POPULATED)
      FetchContent_Populate(dhrystone_fetch)
    endif()
  endif()

  set(CFLAGS
      -DMSC_CLOCK -Wno-maybe-uninitialized -Wno-implicit-int
      -Wno-strict-prototypes -Wno-implicit-function-declaration
      -Wno-return-type)
  set(SRCS dhrystone/v2.2/dry.c dhrystone/v2.1/dhry_2.c)

  nuttx_add_application(
    NAME
    ${CONFIG_BENCHMARK_DHRYSTONE_PROGNAME}
    PRIORITY
    ${CONFIG_BENCHMARK_DHRYSTONE_PRIORITY}
    STACKSIZE
    ${CONFIG_BENCHMARK_DHRYSTONE_STACKSIZE}
    MODULE
    ${CONFIG_BENCHMARK_DHRYSTONE}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})
endif()
