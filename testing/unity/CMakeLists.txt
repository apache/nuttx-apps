# ##############################################################################
# apps/testing/unity/CMakeLists.txt
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

if(CONFIG_TESTING_UNITY)
  nuttx_add_library(unity)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/unity)
    FetchContent_Declare(
      unity
      DOWNLOAD_NAME "v${CONFIG_TESTING_UNITY_VERSION}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "${CONFIG_TESTING_UNITY_URL}/v${CONFIG_TESTING_UNITY_VERSION}.tar.gz"
          SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/Unity BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/testing/unity/unity
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(unity)

    if(NOT unity_POPULATED)
      FetchContent_Populate(unity)
    endif()
  endif()

  set(SRCS Unity/src/unity.c)
  target_compile_options(unity PRIVATE -DUNITY_INCLUDE_CONFIG_H
                                       -DUNITY_INCLUDE_DOUBLE)

  target_include_directories(unity PRIVATE ${NUTTX_APPS_DIR}/include/testing)

  # allow use of <testing/unity.h> like for make build
  file(COPY ${CMAKE_CURRENT_LIST_DIR}/Unity/src/unity.h
            ${CMAKE_CURRENT_LIST_DIR}/Unity/src/unity_internals.h
       DESTINATION ${CMAKE_BINARY_DIR}/include/testing/)

  target_sources(unity PRIVATE ${SRCS})

endif()
