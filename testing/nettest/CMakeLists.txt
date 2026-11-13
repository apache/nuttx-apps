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

  if(CONFIG_TESTING_NET_UDP)
    set(SRCS
        ${CMAKE_CURRENT_LIST_DIR}/udp/test_udp.c
        ${CMAKE_CURRENT_LIST_DIR}/udp/test_udp_common.c
        ${CMAKE_CURRENT_LIST_DIR}/udp/test_udp_v6only.c
        ${CMAKE_CURRENT_LIST_DIR}/udp/test_udp_family.c
        ${CMAKE_CURRENT_LIST_DIR}/udp/test_udp_bind.c)

    nuttx_add_application(
      NAME
      cmocka_net_udp
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_LOCAL)
    set(SRCS
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_common.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_scm_rights.c)

    nuttx_add_application(
      NAME
      cmocka_net_local
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

  if(CONFIG_TESTING_NET_LOCAL_DGRAM)
    set(SRCS
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_dgram.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_dgram_common.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_dgram_connect.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_dgram_poll.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_dgram_pollext.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_dgram_regress.c
        ${CMAKE_CURRENT_LIST_DIR}/local/test_local_dgram_follow.c)

    nuttx_add_application(
      NAME
      cmocka_net_local_dgram
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_OTHERS)
    set(SRCS
        ${CMAKE_CURRENT_LIST_DIR}/others/test_others.c
        ${CMAKE_CURRENT_LIST_DIR}/others/test_others_common.c
        ${CMAKE_CURRENT_LIST_DIR}/others/test_others_bufpool.c
        ${CMAKE_CURRENT_LIST_DIR}/others/test_others_chksum.c)

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
      ${NUTTX_DIR}/net
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_MCAST)
    set(SRCS ${CMAKE_CURRENT_LIST_DIR}/mcast/test_mcast.c
             ${CMAKE_CURRENT_LIST_DIR}/mcast/test_mcast_common.c)

    if(CONFIG_NET_IGMP)
      list(APPEND SRCS ${CMAKE_CURRENT_LIST_DIR}/mcast/test_mcast_ipv4.c)
    endif()

    if(CONFIG_NET_MLD)
      list(APPEND SRCS ${CMAKE_CURRENT_LIST_DIR}/mcast/test_mcast_ipv6.c)
    endif()

    nuttx_add_application(
      NAME
      cmocka_net_mcast
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_RADIO)
    set(SRCS ${CMAKE_CURRENT_LIST_DIR}/radio/test_radio.c
             ${CMAKE_CURRENT_LIST_DIR}/radio/test_radio_common.c)

    if(CONFIG_NET_BLUETOOTH)
      list(APPEND SRCS ${CMAKE_CURRENT_LIST_DIR}/radio/test_radio_bluetooth.c)
    endif()

    if(CONFIG_NET_IEEE802154)
      list(APPEND SRCS ${CMAKE_CURRENT_LIST_DIR}/radio/test_radio_ieee802154.c)
    endif()

    nuttx_add_application(
      NAME
      cmocka_net_radio
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_TIMESTAMP)
    set(SRCS ${CMAKE_CURRENT_LIST_DIR}/timestamp/test_timestamp.c
             ${CMAKE_CURRENT_LIST_DIR}/timestamp/test_timestamp_common.c)

    nuttx_add_application(
      NAME
      cmocka_net_timestamp
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_CAN)
    set(SRCS ${CMAKE_CURRENT_LIST_DIR}/can/test_can.c
             ${CMAKE_CURRENT_LIST_DIR}/can/test_can_bind.c)

    nuttx_add_application(
      NAME
      cmocka_net_can
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      SRCS
      ${SRCS})
  endif()

  if(CONFIG_TESTING_NET_PKT)
    set(SRCS ${CMAKE_CURRENT_LIST_DIR}/pkt/test_pkt.c
             ${CMAKE_CURRENT_LIST_DIR}/pkt/test_pkt_lo.c)

    nuttx_add_application(
      NAME
      cmocka_net_pkt
      PRIORITY
      ${CONFIG_TESTING_NET_TEST_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_NET_TEST_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_NET_TEST}
      DEPENDS
      cmocka
      SRCS
      ${SRCS})
  endif()

endif()
