# ##############################################################################
# apps/testing/nettest/CMakeLists.txt
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

if(CONFIG_TESTING_NET_TEST)
  if(CONFIG_TESTING_NET_TCP)
    set(SRCS
        ${CMAKE_CURRENT_LIST_DIR}/tcp/test_tcp.c
        ${CMAKE_CURRENT_LIST_DIR}/tcp/test_tcp_common.c
        ${CMAKE_CURRENT_LIST_DIR}/utils/nettest_tcpserver.c)

    if(CONFIG_NET_IPv4)
      list(APPEND SRCS ${CMAKE_CURRENT_LIST_DIR}/tcp/test_tcp_connect_ipv4.c)
    endif()

    if(CONFIG_NET_IPv6)
      list(APPEND SRCS ${CMAKE_CURRENT_LIST_DIR}/tcp/test_tcp_connect_ipv6.c)
    endif()

    nuttx_add_application(
      NAME
      cmocka_net_tcp
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_LIST_DIR}/utils
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_OTHERS)
    set(SRCS
        ${CMAKE_CURRENT_LIST_DIR}/others/test_others.c
        ${CMAKE_CURRENT_LIST_DIR}/others/test_others_common.c
        ${CMAKE_CURRENT_LIST_DIR}/others/test_others_bufpool.c)

    nuttx_add_application(
      NAME
      cmocka_net_others
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_LIST_DIR}/utils
      SRCS
      ${SRCS})
  endif()

endif()
