# ##############################################################################
# apps/canutils/libopencyphal/CMakeLists.txt
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

if(CONFIG_CANUTILS_LIBOPENCYPHAL)

  # ############################################################################
  # Config and Fetch libopencyphal o1heap lib
  # ############################################################################

  set(LIBOPENCYPHAL_DIR ${CMAKE_CURRENT_LIST_DIR}/libcanard)
  if(NOT EXISTS ${LIBOPENCYPHAL_DIR})
    FetchContent_Declare(
      libcanard_fetch
      URL ${CONFIG_LIBOPENCYPHAL_URL}/${CONFIG_LIBOPENCYPHAL_VERSION}.zip
          SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libcanard BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/canutils/libopencyphal/libcanard
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)
    FetchContent_GetProperties(libcanard_fetch)
    if(NOT libcanard_fetch_POPULATED)
      FetchContent_Populate(libcanard_fetch)
    endif()
  endif()

  set(O1HEAP_DIR ${CMAKE_CURRENT_LIST_DIR}/o1heap)
  if(NOT EXISTS ${O1HEAP_DIR})
    FetchContent_Declare(
      o1heap_fetch
      URL ${CONFIG_O1HEAP_URL}/${CONFIG_O1HEAP_VERSION}.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/o1heap BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/canutils/libopencyphal/o1heap
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)
    FetchContent_GetProperties(o1heap_fetch)
    if(NOT o1heap_fetch_POPULATED)
      FetchContent_Populate(o1heap_fetch)
    endif()
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_user_library(libopencyphal)

  target_sources(
    libopencyphal
    PRIVATE ${LIBOPENCYPHAL_DIR}/libcanard/canard.c
            ${LIBOPENCYPHAL_DIR}/libcanard/canard_dsdl.c
            ${O1HEAP_DIR}/o1heap/o1heap.c)
  target_compile_options(
    libopencyphal PRIVATE -std=c11 -DCANARD_ASSERT=DEBUGASSERT
                          -DCANARD_DSDL_CONFIG_LITTLE_ENDIAN=1)
  target_include_directories(libopencyphal PRIVATE ${O1HEAP_DIR}/o1heap)

endif()
