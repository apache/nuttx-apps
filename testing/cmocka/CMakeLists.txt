# ##############################################################################
# apps/testing/cmocka/CMakeLists.txt
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

if(CONFIG_TESTING_CMOCKA)

  set(CMOCKA_DIR ${CMAKE_CURRENT_LIST_DIR}/cmocka)

  if(NOT EXISTS ${CMOCKA_DIR})
    set(CONFIG_CMOCKA_URL https://github.com/clibs/cmocka/archive)

    FetchContent_Declare(
      cmocka_fetch
      URL ${CONFIG_CMOCKA_URL}/1.1.5.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/cmocka BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/testing/cmocka/cmocka
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/cmocka <
        ${CMAKE_CURRENT_LIST_DIR}/0001-cmocka.c-Reduce-the-call-stack-consumption-of-printf.patch
        && patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/cmocka <
        ${CMAKE_CURRENT_LIST_DIR}/0002-cmocka-feature-to-forwarding-cmocka-log-message-to-c.patch
        && patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/cmocka <
        ${CMAKE_CURRENT_LIST_DIR}/0003-cmocka-update-method-for-strmatch-to-regex-and-add-list-all-testcases-function.patch
        && patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/cmocka <
        ${CMAKE_CURRENT_LIST_DIR}/0004-cmocka-xml-report.patch && patch -p0 -d
        ${CMAKE_CURRENT_LIST_DIR}/cmocka <
        ${CMAKE_CURRENT_LIST_DIR}/0005-cmocka-cmocka_private-fix-warning-in-cmocka_private.patch
        && patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/cmocka <
        ${CMAKE_CURRENT_LIST_DIR}/0006-fix-linux-risc-v-compile-error-list_initialize.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(cmocka_fetch)

    if(NOT cmocka_fetch_POPULATED)
      FetchContent_Populate(cmocka_fetch)
    endif()
  endif()

  nuttx_add_library(cmocka)

  file(GLOB SRCS ${CMOCKA_DIR}/src/*.c)

  set(EXPORT_INCDIR ${CMOCKA_DIR}/include)
  nuttx_export_header(TARGET cmocka INCLUDE_DIRECTORIES ${EXPORT_INCDIR})

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR} ${EXPORT_INCDIR})

  set(CFLAGS -Dprint_error=nx_print_error -DCMOCKA_PLATFORM_INCLUDE
             -DHAVE_CLOCK_REALTIME=1 -DHAVE_STRUCT_TIMESPEC=1)
  if(CONFIG_TESTING_CMOCKA_LEAKDETECT)
    list(APPEND CFLAGS -DUNIT_TESTING=ON)
  endif()

  target_sources(cmocka PRIVATE ${SRCS})
  target_include_directories(cmocka PRIVATE ${INCDIR})
  target_compile_options(cmocka PRIVATE ${CFLAGS})

  nuttx_add_application(
    NAME
    ${CONFIG_TESTING_CMOCKA_PROGNAME}
    PRIORITY
    ${CONFIG_TESTING_CMOCKA_PRIORITY}
    STACKSIZE
    ${CONFIG_TESTING_CMOCKA_STACKSIZE}
    MODULE
    ${CONFIG_TESTING_CMOCKA}
    INCLUDE_DIRECTORIES
    ${INCDIR}
    SRCS
    cmocka_main.c
    DEPENDS
    cmocka)

endif()
