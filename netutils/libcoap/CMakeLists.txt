# ##############################################################################
# apps/netutils/libcoap/CMakeLists.txt
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

if(CONFIG_NETUTILS_LIBCOAP)
  set(LIBCOAP_DIR ${CMAKE_CURRENT_LIST_DIR}/libcoap)

  if(NOT EXISTS ${LIBCOAP_DIR})
    set(LIBCOAP_URL https://codeload.github.com/obgm/libcoap/zip/refs/tags)
    set(LIBCOAP_VERSION ${CONFIG_LIBCOAP_VERSION})
    FetchContent_Declare(
      libcoap_fetch
      URL ${LIBCOAP_URL}/v${LIBCOAP_VERSION} SOURCE_DIR ${LIBCOAP_DIR}
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/netutils/libcoap
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(libcoap_fetch)

    if(NOT libcoap_fetch_POPULATED)
      FetchContent_Populate(libcoap_fetch)
    endif()
  endif()

  set(LIBCOAP_FLAGS -Wno-undef)
  set(LIBCOAP_INCDIR ${CMAKE_CURRENT_LIST_DIR} ${LIBCOAP_DIR}/include
                     ${NUTTX_APPS_DIR}/crypto/mbedtls/mbedtls/include)

  target_include_directories(apps PUBLIC ${LIBCOAP_INCDIR})
  target_compile_options(apps PRIVATE ${LIBCOAP_FLAGS})
  target_sources(
    apps
    PRIVATE libcoap/src/coap_address.c
            libcoap/src/coap_asn1.c
            libcoap/src/coap_async.c
            libcoap/src/coap_block.c
            libcoap/src/coap_cache.c
            libcoap/src/coap_debug.c
            libcoap/src/coap_dtls.c
            libcoap/src/coap_encode.c
            libcoap/src/coap_event.c
            libcoap/src/coap_hashkey.c
            libcoap/src/coap_gnutls.c
            libcoap/src/coap_io.c
            libcoap/src/coap_layers.c
            libcoap/src/coap_mbedtls.c
            libcoap/src/coap_mem.c
            libcoap/src/coap_net.c
            libcoap/src/coap_netif.c
            libcoap/src/coap_notls.c
            libcoap/src/coap_openssl.c
            libcoap/src/coap_option.c
            libcoap/src/coap_oscore.c
            libcoap/src/coap_pdu.c
            libcoap/src/coap_prng.c
            libcoap/src/coap_resource.c
            libcoap/src/coap_session.c
            libcoap/src/coap_str.c
            libcoap/src/coap_subscribe.c
            libcoap/src/coap_tcp.c
            libcoap/src/coap_time.c
            libcoap/src/coap_tinydtls.c
            libcoap/src/coap_uri.c
            libcoap/src/coap_ws.c
            libcoap/src/oscore/oscore.c
            libcoap/src/oscore/oscore_cbor.c
            libcoap/src/oscore/oscore_context.c
            libcoap/src/oscore/oscore_cose.c
            libcoap/src/oscore/oscore_crypto.c)

  set(LIBCOAP_API_VERSION 3)
  set(LIBCOAP_PACKAGE_BUGREPORT "libcoap-developers@lists.sourceforge.net")
  set(LIBCOAP_PACKAGE_NAME "libcoap")
  set(LIBCOAP_PACKAGE_VERSION "${CONFIG_LIBCOAP_VERSION}")
  set(LIBCOAP_PACKAGE_STRING
      "${LIBCOAP_PACKAGE_NAME} ${LIBCOAP_PACKAGE_VERSION}")
  set(LIBCOAP_PACKAGE_URL "https://libcoap.net/")
  configure_file(${LIBCOAP_DIR}/include/coap3/coap.h.in
                 ${LIBCOAP_DIR}/include/coap3/coap.h)

  if(CONFIG_NETUTILS_LIBCOAP_EXAMPLE)
    nuttx_add_application(
      NAME
      coap_server
      SRCS
      libcoap/examples/coap-server.c
      INCLUDE_DIRECTORIES
      ${LIBCOAP_INCDIR}
      COMPILE_FLAGS
      ${LIBCOAP_FLAGS}
      DEPENDS
      mbedtls
      STACKSIZE
      ${CONFIG_NETUTILS_LIBCOAP_EXAMPLE_STACKSIZE}
      PRIORITY
      ${CONFIG_NETUTILS_LIBCOAP_EXAMPLE_PRIORITY})

    nuttx_add_application(
      NAME
      coap_client
      SRCS
      libcoap/examples/coap-client.c
      INCLUDE_DIRECTORIES
      ${LIBCOAP_INCDIR}
      COMPILE_FLAGS
      ${LIBCOAP_FLAGS}
      DEPENDS
      mbedtls
      STACKSIZE
      ${CONFIG_NETUTILS_LIBCOAP_EXAMPLE_STACKSIZE}
      PRIORITY
      ${CONFIG_NETUTILS_LIBCOAP_EXAMPLE_PRIORITY})
  endif()
endif()
