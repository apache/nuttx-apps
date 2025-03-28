# ##############################################################################
# apps/netutils/wakaama/CMakeLists.txt
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

if(CONFIG_NETUTILS_WAKAAMA)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/wakaama)
    FetchContent_Declare(
      wakaama
      DOWNLOAD_NAME "${CONFIG_WAKAAMA_VERSION}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "https://github.com/eclipse/wakaama/archive/${CONFIG_WAKAAMA_VERSION}.tar.gz"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/wakaama
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/netutils/wakaama/wakaama
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(wakaama)

    if(NOT wakaama_POPULATED)
      FetchContent_Populate(wakaama)
    endif()
  endif()

  include(wakaama/wakaama.cmake)

  set(WAKAAMA_OPTIONS -Wno-format -Wno-type-limits -Wno-cast-align
                      -Wno-unused-but-set-variable)
  set(WAKAAMA_DEFINITIONS)

  if(CONFIG_ENDIAN_BIG)
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_BIG_ENDIAN)
  else()
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_LITTLE_ENDIAN)
  endif()

  if(CONFIG_WAKAAMA_CLIENT_MODE)
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_CLIENT_MODE)
  endif()

  if(CONFIG_WAKAAMA_SERVER_MODE)
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_SERVER_MODE)
  endif()

  if(CONFIG_WAKAAMA_BOOTSTRAP)
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_BOOTSTRAP)
  endif()

  if(CONFIG_WAKAAMA_BOOTSTRAP_SERVER_MODE)
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_BOOTSTRAP_SERVER_MODE)
  endif()

  if(CONFIG_WAKAAMA_JSON)
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_SUPPORT_JSON)
  endif()

  if(CONFIG_WAKAAMA_SENML_JSON)
    list(APPEND WAKAAMA_DEFINITIONS LWM2M_SUPPORT_SENML_JSON)
  endif()

  list(APPEND WAKAAMA_DEFINITIONS
       LWM2M_COAP_DEFAULT_BLOCK_SIZE=${CONFIG_WAKAAMA_COAP_DEFAULT_BLOCK_SIZE})

  nuttx_add_library(wakaama)
  target_include_directories(wakaama PUBLIC wakaama/include)
  target_compile_options(wakaama PUBLIC ${WAKAAMA_OPTIONS})
  target_compile_definitions(wakaama PUBLIC ${WAKAAMA_DEFINITIONS})
  target_sources_wakaama(wakaama)

  include(examples.cmake)

endif()
