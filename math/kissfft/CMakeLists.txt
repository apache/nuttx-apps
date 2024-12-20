# ##############################################################################
# apps/math/kissfft/CMakeLists.txt
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

if(CONFIG_MATH_KISSFFT)

  # ############################################################################
  # Config and Fetch kissfft lib
  # ############################################################################

  set(KISSFFT_DIR ${CMAKE_CURRENT_LIST_DIR}/kissfft)

  if(NOT EXISTS ${KISSFFT_DIR})
    set(KISSFFT_URL
        https://github.com/mborgerding/kissfft/archive/refs/tags/v130.zip)
    FetchContent_Declare(
      kissfft_fetch
      URL ${KISSFFT_URL} SOURCE_DIR ${KISSFFT_DIR} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/math/kissfft/kissfft
      PATCH_COMMAND patch -d ${KISSFFT_DIR} -p1 <
                    ${CMAKE_CURRENT_LIST_DIR}/kissfft.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(kissfft_fetch)

    if(NOT kissfft_fetch_POPULATED)
      FetchContent_Populate(kissfft_fetch)
    endif()
  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS ${KISSFFT_DIR}/kiss_fft.c ${KISSFFT_DIR}/tools/kiss_fftr.c)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR}/kissfft)

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(kissfft STATIC)
  target_sources(kissfft PRIVATE ${CSRCS})
  target_include_directories(kissfft PUBLIC ${INCDIR})

endif()
