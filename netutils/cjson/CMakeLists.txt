# ##############################################################################
# apps/netutils/cjson/CMakeLists.txt
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

if(CONFIG_NETUTILS_CJSON)

  if(NOT CONFIG_NETUTILS_CJSON_URL)
    set(CONFIG_NETUTILS_CJSON_URL "https://github.com/DaveGamble/cJSON/archive")
  endif()

  if(NOT CONFIG_NETUTILS_CJSON_VERSION)
    set(CONFIG_NETUTILS_CJSON_VERSION "1.7.10")
  endif()

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/cJSON)
    FetchContent_Declare(
      cJSON
      DOWNLOAD_NAME "v${CONFIG_NETUTILS_CJSON_VERSION}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "${CONFIG_NETUTILS_CJSON_URL}/v${CONFIG_NETUTILS_CJSON_VERSION}.tar.gz"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/cJSON
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/netutils/cjson/cJSON
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(cJSON)

    if(NOT cJSON_POPULATED)
      FetchContent_Populate(cJSON)
    endif()
  endif()

  if(NOT EXISTS ${CMAKE_BINARY_DIR}/apps/include/netutils/cJSON.h
     OR NOT EXISTS ${CMAKE_BINARY_DIR}/apps/include/netutils/cJSON_Utils.h)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/apps/include/netutils)
    file(CREATE_LINK ${CMAKE_CURRENT_LIST_DIR}/cJSON/cJSON.h
         ${CMAKE_BINARY_DIR}/apps/include/netutils/cJSON.h SYMBOLIC)
    file(CREATE_LINK ${CMAKE_CURRENT_LIST_DIR}/cJSON/cJSON_Utils.h
         ${CMAKE_BINARY_DIR}/apps/include/netutils/cJSON_Utils.h SYMBOLIC)
  endif()

  target_sources(apps PRIVATE cJSON/cJSON.c cJSON/cJSON_Utils.c)

endif()
