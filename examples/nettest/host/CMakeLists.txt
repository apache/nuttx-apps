# ##############################################################################
# apps/examples/nettest/host/CMakeLists.txt
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
project(nuttx_tools LANGUAGES C)

include(${NUTTX_DIR}/cmake/nuttx_kconfig.cmake)
nuttx_export_kconfig(${NUTTX_BINARY_DIR}/.config)

set(HOST_SRCS ../nettest_host.c)
if(CONFIG_EXAMPLES_NETTEST_SERVER)
  set(HOST_TARGET tcpclient)
  add_executable(tcpclient ../nettest_client.c ${HOST_SRCS})
  install(TARGETS tcpclient DESTINATION bin)
else()
  set(HOST_TARGET tcpserver)
  add_executable(tcpserver ../nettest_server.c ${HOST_SRCS})
  install(TARGETS tcpserver DESTINATION bin)
endif()

target_compile_definitions(${HOST_TARGET} PRIVATE NETTEST_HOST=1)
if(CONFIG_EXAMPLES_NETTEST_SERVER)
  target_compile_definitions(
    ${HOST_TARGET}
    PRIVATE
      CONFIG_EXAMPLES_NETTEST_SERVER=1
      CONFIG_EXAMPLES_NETTEST_SERVERIP="${CONFIG_EXAMPLES_NETTEST_SERVERIP}")
endif()
if(CONFIG_EXAMPLES_NETTEST_PERFORMANCE)
  target_compile_definitions(${HOST_TARGET}
                             PRIVATE CONFIG_EXAMPLES_NETTEST_PERFORMANCE=1)
endif()

target_include_directories(${HOST_TARGET}
                           PRIVATE ${NUTTX_BINARY_DIR}/include/nuttx)
