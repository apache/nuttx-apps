# ##############################################################################
# apps/testing/mm/memtester/CMakeLists.txt
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

if(CONFIG_UTILS_MEMTESTER)

  set(MEMTESTER_DIR ${CMAKE_CURRENT_LIST_DIR}/memtester)

  if(NOT EXISTS ${MEMTESTER_DIR})
    set(CONFIG_MEMTESTER_URL
        https://github.com/jnavila/memtester/archive/refs/heads/master.zip)

    FetchContent_Declare(
      memtester_fetch
      URL ${CONFIG_MEMTESTER_URL}SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/memtester
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/testing/memtester/memtester
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(memtester_fetch)

    if(NOT memtester_fetch_POPULATED)
      FetchContent_Populate(memtester_fetch)
    endif()
  endif()

  set(SRCS ${MEMTESTER_DIR}/memtester.c ${MEMTESTER_DIR}/tests.c
           ${MEMTESTER_DIR}/output.c)
  set(CFLAGS)
  if(CONFIG_UTILS_MEMTESTER_NARROW)
    list(APPEND CFLAGS -DTEST_NARROW_WRITES)
  endif()

  nuttx_add_application(
    NAME
    ${CONFIG_UTILS_MEMTESTER_PROGNAME}
    PRIORITY
    ${CONFIG_UTILS_MEMTESTER_PRIORITY}
    STACKSIZE
    ${CONFIG_UTILS_MEMTESTER_STACKSIZE}
    MODULE
    ${CONFIG_UTILS_MEMTESTER}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})

endif()
