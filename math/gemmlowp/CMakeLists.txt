# ##############################################################################
# apps/math/gemmlowp/CMakeLists.txt
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

if(CONFIG_MATH_GEMMLOWP)

  # ############################################################################
  # Config and Fetch gemmlowp lib
  # ############################################################################

  set(GEMMLOWP_DIR ${CMAKE_CURRENT_LIST_DIR}/gemmlowp)

  if(NOT EXISTS ${GEMMLOWP_DIR})
    set(GEMMLOWP_URL
        https://github.com/google/gemmlowp/archive/719139ce755a0f31cbf1c37f7f98adcc7fc9f425.zip
    )
    FetchContent_Declare(
      gemmlowp_fetch
      URL ${GEMMLOWP_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/gemmlowp
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/math/gemmlowp/gemmlowp
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(gemmlowp_fetch)

    if(NOT gemmlowp_fetch_POPULATED)
      FetchContent_Populate(gemmlowp_fetch)
    endif()
  endif()
endif()
