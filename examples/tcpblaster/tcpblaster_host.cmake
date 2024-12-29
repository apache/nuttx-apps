# ##############################################################################
# apps/examples/tcpblaster/tcpblaster_host.cmake
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
# Configure project
cmake_minimum_required(VERSION 3.16)
project(tcpblaster_host LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Build type" FORCE)
endif()

message(STATUS "NuttX apps examples tcpblaster host")

include_directories(${CMAKE_BINARY_DIR}/include/nuttx)

add_compile_definitions(TCPBLASTER_HOST=1)

if(CONFIG_EXAMPLES_TCPBLASTER_SERVER)
  add_compile_definitions(CONFIG_EXAMPLES_TCPBLASTER_SERVER=1)
  add_compile_definitions(
    CONFIG_EXAMPLES_TCPBLASTER_SERVERIP=${CONFIG_EXAMPLES_TCPBLASTER_SERVERIP})

endif()

add_library(tcpblaster)
target_sources(tcpblaster PRIVATE tcpblaster_cmdline.c)
if(CONFIG_EXAMPLES_TCPBLASTER_SERVER)
  target_sources(tcpblaster PRIVATE tcpblaster_client.c)
  add_executable(tcpclient tcpblaster_host.c)
  target_link_libraries(tcpclient PRIVATE tcpblaster)
  install(TARGETS tcpclient DESTINATION bin)
else()
  target_sources(tcpblaster PRIVATE tcpblaster_server.c)
  add_executable(tcpserver tcpblaster_host.c)
  target_link_libraries(tcpserver PRIVATE tcpblaster)
  install(TARGETS tcpserver DESTINATION bin)
endif()
