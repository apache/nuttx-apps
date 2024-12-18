# ##############################################################################
# apps/canutils/libdronecan/CMakeLists.txt
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

if(CONFIG_CANUTILS_LIBDRONECAN)

  # ############################################################################
  # Config and Fetch libdronecan lib
  # ############################################################################

  set(LIBDRONECAN_DIR ${CMAKE_CURRENT_LIST_DIR}/libcanard)

  if(NOT EXISTS ${LIBDRONECAN_DIR})
    FetchContent_Declare(
      libdronecan_fetch
      URL ${CONFIG_LIBDRONECAN_URL}/${CONFIG_LIBDRONECAN_VERSION}.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/libcanard BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/canutils/libdronecan/libcanard
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(libdronecan_fetch)

    if(NOT libdronecan_fetch_POPULATED)
      FetchContent_Populate(libdronecan_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -std=c99 -DCANARD_ASSERT=DEBUGASSERT)

  # Conflict with Cyphal's libcanard
  if(CONFIG_CANUTILS_LIBOPENCYPHAL)
    list(APPEND CFLAGS -DcanardInit=dronecanardInit)
  endif()

  if(CONFIG_LIBDRONECAN_CANFD)
    list(APPEND CFLAGS -DCANARD_ENABLE_CANFD=1)
  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS ${LIBDRONECAN_DIR}/canard.c)

  if(CONFIG_NET_CAN)
    list(APPEND CSRCS ${LIBDRONECAN_DIR}/drivers/socketcan/socketcan.c)
  else()
    list(APPEND CSRCS ${LIBDRONECAN_DIR}/drivers/nuttx/canard_nuttx.c)
  endif()

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${NUTTX_APPS_DIR}/canutils/libdronecan/libcanard)

  if(CONFIG_NET_CAN)
    list(APPEND INCDIR
         ${NUTTX_APPS_DIR}/canutils/libdronecan/libcanard/drivers/socketcan)
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_user_library(libdronecan)

  target_sources(libdronecan PRIVATE ${CSRCS})
  target_compile_options(libdronecan PRIVATE ${CFLAGS})
  target_include_directories(libdronecan PRIVATE ${INCDIR})

endif()
