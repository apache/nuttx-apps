# ##############################################################################
# apps/math/ruy/CMakeLists.txt
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

if(CONFIG_MATH_RUY)

  # ############################################################################
  # Config and Fetch ruy lib
  # ############################################################################

  set(RUY_DIR ${CMAKE_CURRENT_LIST_DIR}/ruy)

  if(NOT EXISTS ${RUY_DIR})
    set(RUY_URL
        https://github.com/google/ruy/archive/d37128311b445e758136b8602d1bbd2a755e115d.zip
    )
    FetchContent_Declare(
      ruy_fetch
      URL ${RUY_URL} SOURCE_DIR ${RUY_DIR} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/math/ruy/ruy
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(ruy_fetch)

    if(NOT ruy_fetch_POPULATED)
      FetchContent_Populate(ruy_fetch)
    endif()
  endif()

endif()
