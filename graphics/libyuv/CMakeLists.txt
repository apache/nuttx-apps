# ##############################################################################
# apps/graphics/libyuv/CMakeLists.txt
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

if(CONFIG_LIBYUV)

  set(LIBYUV_DIR ${CMAKE_CURRENT_LIST_DIR}/libyuv)

  if(NOT EXISTS ${LIBYUV_DIR})
    FetchContent_Declare(
      libyuv_fetch
      URL https://chromium.googlesource.com/libyuv/libyuv/+archive/refs/heads/${CONFIG_LIBYUV_BRANCH}.tar.gz
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/libyuv
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/graphics/libyuv/libyuv
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-include-libyuv-fix-strict-prototype-warning.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(libyuv_fetch)

    if(NOT libyuv_fetch_POPULATED)
      FetchContent_Populate(llibyuv_fetch)
    endif()
  endif()

  nuttx_add_library(libyuv STATIC)
  file(GLOB SRCS ${LIBYUV_DIR}/source/*.cc)
  target_sources(libyuv PRIVATE ${SRCS})
  target_compile_options(libyuv PRIVATE -Wno-shadow -Wno-undef)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_SOURCE_DIR}/libyuv/include)
endif()
