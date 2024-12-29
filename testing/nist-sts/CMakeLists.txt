# ##############################################################################
# apps/testing/nist-sts/CMakeLists.txt
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

if(CONFIG_TESTING_NIST_STS)

  set(NIST_DIR ${CMAKE_CURRENT_LIST_DIR}/nist-sts)

  if(NOT EXISTS ${NIST_DIR})
    set(CONFIG_NIST_URL
        https://csrc.nist.gov/CSRC/media/Projects/Random-Bit-Generation/documents/sts-2_1_2.zip
    )

    FetchContent_Declare(
      nist-sts_fetch
      URL ${CONFIG_NIST_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/nist-sts
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/testing/nist-sts/nist-sts
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/nist-sts <
        ${CMAKE_CURRENT_LIST_DIR}/0001-Solve-the-memory-out-of-bounds-problem-in-sts.patch
        && patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/nist-sts <
        ${CMAKE_CURRENT_LIST_DIR}/0002-Fix-bug-that-crash-in-running-a-single-test-after-ru.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(nist-sts_fetch)

    if(NOT nist-sts_fetch_POPULATED)
      FetchContent_Populate(nist-sts_fetch)
    endif()
  endif()

  file(GLOB NIST_SRCS ${NIST_DIR}/sts/src/*.c)
  list(REMOVE_ITEM NIST_SRCS ${NIST_DIR}/sts/src/assess.c)

  set(SRCS ${NIST_DIR}/sts/src/assess.c ${NIST_SRCS})

  set(CFLAGS
      -Wno-misleading-indentation
      -Wno-unused-but-set-variable
      -Wno-strict-prototypes
      -Wno-undef
      -Wno-shadow
      -Wno-unused-variable
      -Wno-maybe-uninitialized)

  nuttx_add_application(
    NAME
    ${CONFIG_TESTING_NIST_STS_PROGNAME}
    PRIORITY
    ${CONFIG_TESTING_NIST_STS_PRIORITY}
    STACKSIZE
    ${CONFIG_TESTING_NIST_STS_STACKSIZE}
    MODULE
    ${CONFIG_TESTING_NIST_STS}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})

endif()
