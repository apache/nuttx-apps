# ##############################################################################
# apps/system/argtable3/CMakeLists.txt
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

if(CONFIG_SYSTEM_ARGTABLE3)
  nuttx_add_library(argtable3)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/argtable3)

    if(NOT CONFIG_SYSTEM_ARGTABLE3_URL)
      set(CONFIG_SYSTEM_ARGTABLE3_URL
          "https://github.com/argtable/argtable3/archive")
    endif()

    if(NOT CONFIG_SYSTEM_ARGTABLE3_VERSION)
      set(CONFIG_SYSTEM_ARGTABLE3_VERSION "3.2.0.7402e6e")
    endif()

    FetchContent_Declare(
      argtable3
      DOWNLOAD_NAME "v${CONFIG_SYSTEM_ARGTABLE3_VERSION}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "${CONFIG_SYSTEM_ARGTABLE3_URL}/v${CONFIG_SYSTEM_ARGTABLE3_VERSION}.tar.gz"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/argtable3
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/system/argtable3/argtable3
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(argtable3)

    if(NOT argtable3_POPULATED)
      FetchContent_Populate(argtable3)
    endif()
  endif()

  set(SRCS)
  file(GLOB SRCS ${CMAKE_CURRENT_LIST_DIR}/argtable3/src/*.c)

  if(CONFIG_SYSTEM_ARGTABLE3_TEST)
    file(GLOB testsrcs ${CMAKE_CURRENT_LIST_DIR}/argtable3/tests/*.c)

    nuttx_add_application(
      NAME
      ${CONFIG_SYSTEM_ARGTABLE3_TEST_PROGNAME}
      SRCS
      ${CMAKE_CURRENT_LIST_DIR}/argtable3/tests/testall.c
      ${testsrcs}
      INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_LIST_DIR}/argtable3/src
      STACKSIZE
      ${CONFIG_SYSTEM_ARGTABLE3_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_SYSTEM_ARGTABLE3_TEST_PRIORITY})
  endif()

  target_sources(argtable3 PRIVATE ${SRCS})

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES
             ${NUTTX_APPS_DIR}/system/argtable3/argtable3/src)

  target_compile_options(argtable3 PRIVATE -DARG_REPLACE_GETOPT=0)

endif()
