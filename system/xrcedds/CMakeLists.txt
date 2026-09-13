# ##############################################################################
# apps/system/xrcedds/CMakeLists.txt
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

if(CONFIG_SYSTEM_XRCEDDS)
  set(XRCEDDS_VERSION 3.0.1)
  set(MICROCDR_VERSION 2.0.2)
  set(XRCEDDS_DIR ${CMAKE_CURRENT_LIST_DIR}/Micro-XRCE-DDS-Client)
  set(MICROCDR_DIR ${CMAKE_CURRENT_LIST_DIR}/Micro-CDR)
  set(XRCEDDS_URL https://github.com/eProsima/Micro-XRCE-DDS-Client/archive)
  set(MICROCDR_URL https://github.com/eProsima/Micro-CDR/archive)

  if(NOT EXISTS ${XRCEDDS_DIR})
    FetchContent_Declare(
      xrcedds_fetch
      URL ${XRCEDDS_URL}/refs/tags/v${XRCEDDS_VERSION}.tar.gz SOURCE_DIR
          ${XRCEDDS_DIR} BINARY_DIR
          ${NUTTX_APPS_BINDIR}/system/xrcedds/xrcedds-client
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)
    FetchContent_GetProperties(xrcedds_fetch)
    if(NOT xrcedds_fetch_POPULATED)
      FetchContent_Populate(xrcedds_fetch)
    endif()
  endif()

  if(NOT EXISTS ${MICROCDR_DIR})
    FetchContent_Declare(
      microcdr_fetch
      URL ${MICROCDR_URL}/refs/tags/v${MICROCDR_VERSION}.tar.gz SOURCE_DIR
          ${MICROCDR_DIR} BINARY_DIR
          ${NUTTX_APPS_BINDIR}/system/xrcedds/microcdr
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)
    FetchContent_GetProperties(microcdr_fetch)
    if(NOT microcdr_fetch_POPULATED)
      FetchContent_Populate(microcdr_fetch)
    endif()
  endif()

  file(
    GLOB XRCEDDS_SRCS ${XRCEDDS_DIR}/src/c/core/serialization/*.c
    ${XRCEDDS_DIR}/src/c/core/session/*.c
    ${XRCEDDS_DIR}/src/c/core/session/stream/*.c ${XRCEDDS_DIR}/src/c/util/*.c)

  list(APPEND XRCEDDS_SRCS
       ${XRCEDDS_DIR}/src/c/profile/transport/ip/udp/udp_transport.c
       ${XRCEDDS_DIR}/src/c/profile/transport/ip/udp/udp_transport_posix.c)

  if(CONFIG_SYSTEM_XRCEDDS_STREAM_FRAMING)
    file(GLOB XRCEDDS_FRAMING_SRCS
         ${XRCEDDS_DIR}/src/c/profile/transport/stream_framing/*.c)
    list(APPEND XRCEDDS_SRCS ${XRCEDDS_FRAMING_SRCS})
  endif()

  file(GLOB MICROCDR_SRCS ${MICROCDR_DIR}/src/c/*.c
       ${MICROCDR_DIR}/src/c/types/*.c)

  nuttx_add_library(xrcedds STATIC)
  target_sources(xrcedds PRIVATE ${XRCEDDS_SRCS} ${MICROCDR_SRCS})
  target_include_directories(
    xrcedds PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include ${XRCEDDS_DIR}/include
                    ${XRCEDDS_DIR}/src/c ${MICROCDR_DIR}/include)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include
             ${XRCEDDS_DIR}/include ${MICROCDR_DIR}/include)
endif()
