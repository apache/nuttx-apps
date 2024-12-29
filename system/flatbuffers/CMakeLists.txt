# ##############################################################################
# apps/system/flatbuffers/CMakeLists.txt
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

if(CONFIG_SYSTEM_FLATBUFFERS)

  # ############################################################################
  # Config and Fetch flatbuffers lib
  # ############################################################################

  set(FLATBUFFERS_DIR ${CMAKE_CURRENT_LIST_DIR}/flatbuffers)

  if(NOT EXISTS ${FLATBUFFERS_DIR})
    set(FLATBUFFERS_URL
        https://github.com/google/flatbuffers/archive/v23.5.26.tar.gz)
    FetchContent_Declare(
      flatbuffers_fetch
      URL ${FLATBUFFERS_URL} SOURCE_DIR ${FLATBUFFERS_DIR} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/math/flatbuffers/flatbuffers
      PATCH_COMMAND patch -d ${FLATBUFFERS_DIR} -p1 <
                    ${CMAKE_CURRENT_LIST_DIR}/flatbuffers.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(flatbuffers_fetch)

    if(NOT flatbuffers_fetch_POPULATED)
      FetchContent_Populate(flatbuffers_fetch)
    endif()
  endif()

endif()
