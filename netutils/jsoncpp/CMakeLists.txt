# ##############################################################################
# apps/netutils/jsoncpp/CMakeLists.txt
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

if(CONFIG_NETUTILS_JSONCPP)
  set(JSONCPP_DIR ${CMAKE_CURRENT_LIST_DIR}/jsoncpp)

  if(NOT EXISTS ${JSONCPP_DIR})
    set(JSONCPP_URL
        https://github.com/open-source-parsers/jsoncpp/archive/refs/tags)
    set(JSONCPP_VERSION ${CONFIG_JSONCPP_VERSION})
    FetchContent_Declare(
      jsoncpp_fetch
      URL ${JSONCPP_URL}/${JSONCPP_VERSION}.zip SOURCE_DIR ${JSONCPP_DIR}
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/netutils/jsoncpp
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(jsoncpp_fetch)

    if(NOT jsoncpp_fetch_POPULATED)
      FetchContent_Populate(jsoncpp_fetch)
    endif()
  endif()

  target_include_directories(apps PUBLIC jsoncpp/include)
  target_compile_options(apps PRIVATE -DJSON_USE_EXCEPTION=0)
  target_sources(
    apps
    PRIVATE jsoncpp/src/lib_json/json_writer.cpp
            jsoncpp/src/lib_json/json_value.cpp
            jsoncpp/src/lib_json/json_reader.cpp)
endif()
