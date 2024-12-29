# ##############################################################################
# apps/examples/tcpblaster/CMakeLists.txt
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

if(CONFIG_EXAMPLES_TCPBLASTER)

  if(NOT CONFIG_EXAMPLES_TCPBLASTER_TARGET2
     AND NOT CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK)
    include(tcpblaster_host.cmake)
  endif()

  # Basic TCP networking test

  set(CSRCS tcpblaster_cmdline.c)

  if(CONFIG_EXAMPLES_TCPBLASTER_INIT)
    list(APPEND CSRCS tcpblaster_netinit.c)
  endif()

  # Target 1 Files

  if(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK)
    list(APPEND CSRCS tcpblaster_server.c tcpblaster_client.c)
  elseif(CONFIG_EXAMPLES_TCPBLASTER_SERVER)
    list(APPEND CSRCS tcpblaster_server.c)
  else()
    list(APPEND CSRCS tcpblaster_client.c)
  endif()

  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_TCPBLASTER_PROGNAME1}
    PRIORITY
    ${CONFIG_EXAMPLES_TCPBLASTER_PRIORITY1}
    STACKSIZE
    ${CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE1}
    MODULE
    ${CONFIG_EXAMPLES_TCPBLASTER}
    INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}/include/nuttx
    SRCS
    tcpblaster_target1.c
    ${CSRCS})

  # Target 2 Files

  if(CONFIG_EXAMPLES_TCPBLASTER_TARGET2)
    if(CONFIG_EXAMPLES_TCPBLASTER_SERVER)
      list(APPEND CSRCS tcpblaster_client.c)
    else()
      list(APPEND CSRCS tcpblaster_server.c)
    endif()
    nuttx_add_application(
      NAME
      ${CONFIG_EXAMPLES_TCPBLASTER_PROGNAME2}
      PRIORITY
      ${CONFIG_EXAMPLES_TCPBLASTER_PRIORITY2}
      STACKSIZE
      ${CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE2}
      MODULE
      ${CONFIG_EXAMPLES_TCPBLASTER}
      INCLUDE_DIRECTORIES
      ${CMAKE_BINARY_DIR}/include/nuttx
      SRCS
      tcpblaster_target2.c
      ${CSRCS})
  endif()

endif()
