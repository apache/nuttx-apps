# ##############################################################################
# apps/examples/nettest/CMakeLists.txt
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

if(CONFIG_EXAMPLES_NETTEST)

  set(CSRCS)

  # Basic TCP networking test

  set(SRCS nettest_cmdline.c)
  if(CONFIG_EXAMPLES_NETTEST_INIT)
    list(APPEND SRCS nettest_netinit.c)
  endif()

  # Target 1 Files

  if(CONFIG_EXAMPLES_NETTEST_LOOPBACK)
    list(APPEND SRCS nettest_server.c nettest_client.c)
  elseif(CONFIG_EXAMPLES_NETTEST_SERVER)
    list(APPEND SRCS nettest_server.c)
  else()
    list(APPEND SRCS nettest_client.c)
  endif()

  # Target 1 Application Info

  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_NETTEST_PROGNAME1}
    SRCS
    nettest_target1.c
    INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}/include/nuttx
    STACKSIZE
    ${CONFIG_EXAMPLES_NETTEST_STACKSIZE1}
    PRIORITY
    ${CONFIG_EXAMPLES_NETTEST_PRIORITY1})

  # Target 2

  if(CONFIG_EXAMPLES_NETTEST_TARGET2)

    if(CONFIG_EXAMPLES_NETTEST_SERVER)
      list(APPEND SRCS nettest_client.c)
    else()
      list(APPEND SRCS nettest_server.c)
    endif()

    # Target 2 Application Info

    nuttx_add_application(
      NAME
      ${CONFIG_EXAMPLES_NETTEST_PROGNAME2}
      SRCS
      nettest_target2.c
      INCLUDE_DIRECTORIES
      ${CMAKE_BINARY_DIR}/include/nuttx
      STACKSIZE
      ${CONFIG_EXAMPLES_NETTEST_STACKSIZE2}
      PRIORITY
      ${CONFIG_EXAMPLES_NETTEST_PRIORITY2})

  endif()

  target_include_directories(apps PRIVATE ${CMAKE_BINARY_DIR}/include/nuttx)
  target_sources(apps PRIVATE ${SRCS})

  # Host

  if(NOT CONFIG_EXAMPLES_NETTEST_TARGET2 AND NOT
                                             CONFIG_EXAMPLES_NETTEST_LOOPBACK)
    ExternalProject_Add(
      nettest
      SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/host
      INSTALL_DIR ${CMAKE_BINARY_DIR}/apps/examples/nettest/host
      CMAKE_ARGS
        -DNUTTX_DIR=${NUTTX_DIR} -DNUTTX_BINARY_DIR=${CMAKE_BINARY_DIR}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/apps/examples/nettest/host
      USES_TERMINAL_CONFIGURE true
      USES_TERMINAL_BUILD true
      USES_TERMINAL_INSTALL true)
  endif()

endif()
