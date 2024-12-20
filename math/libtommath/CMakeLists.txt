# ##############################################################################
# apps/math/libtommath/CMakeLists.txt
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

if(CONFIG_MATH_LIBTOMMATH)

  # ############################################################################
  # Config and Fetch Tommath lib
  # ############################################################################

  set(LIBTOMMATH_DIR ${CMAKE_CURRENT_LIST_DIR}/libtommath)

  if(NOT EXISTS ${LIBTOMMATH_DIR})
    set(CONFIG_LIBTOMMATH_URL https://github.com/libtom/libtommath/archive)
    FetchContent_Declare(
      libtommath_fetch
      URL ${CONFIG_LIBTOMMATH_URL}/v${CONFIG_LIBTOMMATH_VERSION}.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/libtommath BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/math/libtommath/libtommath
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(libtommath_fetch)
    if(NOT libtommath_fetch_POPULATED)
      FetchContent_Populate(libtommath_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -Wno-format)

  # ############################################################################
  # Sources
  # ############################################################################

  file(GLOB CSRCS ${LIBTOMMATH_DIR}/*.c)

  if(CONFIG_LIBTOMMATH_DEMOS)
    list(APPEND CSRCS ${LIBTOMMATH_DIR}/demo/shared.c)
  endif()

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${LIBTOMMATH_DIR})

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(libtommath STATIC)
  target_compile_options(libtommath PRIVATE ${CFLAGS})
  target_sources(libtommath PRIVATE ${CSRCS})
  target_include_directories(libtommath PRIVATE ${INCDIR})

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_LIBTOMMATH_TEST)
    nuttx_add_application(
      NAME
      ${CONFIG_LIBTOMMATH_TEST_PROGNAME}
      STACKSIZE
      ${CONFIG_LIBTOMMATH_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_LIBTOMMATH_TEST_PRIORITY}
      SRCS
      ${LIBTOMMATH_DIR}/demo/test.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      libtommath)
  endif()

  if(CONFIG_LIBTOMMATH_MTEST_OPPONENT)
    nuttx_add_application(
      NAME
      ${CONFIG_LIBTOMMATH_MTEST_OPPONENT_PROGNAME}
      STACKSIZE
      ${CONFIG_LIBTOMMATH_MTEST_OPPONENT_STACKSIZE}
      PRIORITY
      ${CONFIG_LIBTOMMATH_MTEST_OPPONENT_PRIORITY}
      SRCS
      ${LIBTOMMATH_DIR}/demo/mtest_opponent.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      libtommath)
  endif()

  if(CONFIG_LIBTOMMATH_TIMING)
    nuttx_add_application(
      NAME
      ${CONFIG_LIBTOMMATH_TIMING_PROGNAME}
      STACKSIZE
      ${CONFIG_LIBTOMMATH_TIMING_STACKSIZE}
      PRIORITY
      ${CONFIG_LIBTOMMATH_TIMING_PRIORITY}
      SRCS
      ${LIBTOMMATH_DIR}/demo/timing.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      libtommath)

  endif()

  if(CONFIG_LIBTOMMATH_MTEST)
    nuttx_add_application(
      NAME
      ${CONFIG_LIBTOMMATH_MTEST_PROGNAME}
      STACKSIZE
      ${CONFIG_LIBTOMMATH_MTEST_STACKSIZE}
      PRIORITY
      ${CONFIG_LIBTOMMATH_MTEST_PRIORITY}
      SRCS
      ${LIBTOMMATH_DIR}/mtest/mtest.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      libtommath)
  endif()

endif()
