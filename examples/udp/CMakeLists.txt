# ##############################################################################
# apps/examples/udp/CMakeLists.txt
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

if(CONFIG_EXAMPLES_UDP)

  # Basic UDP networking test

  set(CSRCS udp_cmdline.c)

  if(CONFIG_EXAMPLES_UDP_NETINIT)
    list(APPEND CSRCS udp_netinit.c)
  endif()

  # Target 1 Files

  if(CONFIG_EXAMPLES_UDP_SERVER1)
    list(APPEND CSRCS udp_server.c)
  else()
    list(APPEND CSRCS udp_client.c)
  endif()

  nuttx_add_application(
    NAME
    ${CONFIG_EXAMPLES_UDP_PROGNAME1}
    PRIORITY
    ${CONFIG_EXAMPLES_UDP_PRIORITY1}
    STACKSIZE
    ${CONFIG_EXAMPLES_UDP_STACKSIZE1}
    MODULE
    ${CONFIG_EXAMPLES_UDP}
    INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}/include/nuttx
    SRCS
    udp_target1.c
    ${CSRCS})

  # Target 2 Files

  if(CONFIG_EXAMPLES_UDP_TARGET2)
    if(CONFIG_EXAMPLES_UDP_SERVER1)
      list(APPEND CSRCS udp_client.c)
    else()
      list(APPEND CSRCS udp_server.c)
    endif()

    nuttx_add_application(
      NAME
      ${CONFIG_EXAMPLES_UDP_PROGNAME2}
      PRIORITY
      ${CONFIG_EXAMPLES_UDP_PRIORITY2}
      STACKSIZE
      ${CONFIG_EXAMPLES_UDP_STACKSIZE2}
      MODULE
      ${CONFIG_EXAMPLES_UDP}
      INCLUDE_DIRECTORIES
      ${CMAKE_BINARY_DIR}/include/nuttx
      SRCS
      udp_target2.c
      ${CSRCS})
  endif()
endif()
