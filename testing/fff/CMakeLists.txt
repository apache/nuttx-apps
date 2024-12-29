# ##############################################################################
# apps/testing/fff/CMakeLists.txt
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

if(CONFIG_TESTING_FFF)

  set(FFF_UNPACK ${CMAKE_CURRENT_LIST_DIR}/fff)
  set(FFF_URL https://github.com/meekrosoft/fff/archive/refs/heads/master.zip)

  if(NOT EXISTS ${FFF_UNPACK})

    FetchContent_Declare(
      fff_fetch
      URL ${FFF_URL} SOURCE_DIR ${FFF_UNPACK} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/testing/fff/fff
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(fff_fetch)
    if(NOT fff_fetch_POPULATED)
      FetchContent_Populate(fff_fetch)
    endif()

  endif()

  set(INCDIR ${FFF_UNPACK})

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${INCDIR})

endif()
