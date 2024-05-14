# ##############################################################################
# apps/netutils/libwebsockets/CMakeLists.txt
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

if(CONFIG_NETUTILS_LIBWEBSOCKETS)

  # ############################################################################
  # Config and Fetch MbedTLS lib
  # ############################################################################

  set(LIBWEBSOCKETS_DIR ${CMAKE_CURRENT_LIST_DIR}/libwebsockets)

  if(NOT EXISTS ${LIBWEBSOCKETS_DIR})
    set(LIBWEBSOCKETS_URL "https://github.com/warmcat/libwebsockets/archive")
    FetchContent_Declare(
      libwebsockets_fetch
      URL ${LIBWEBSOCKETS_URL}/v${CONFIG_NETUTILS_LIBWEBSOCKETS_VERSION}.zip
          SOURCE_DIR ${LIBWEBSOCKETS_DIR} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/netutils/libwebsockets/libwebsockets
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(libwebsockets_fetch)

    if(NOT libwebsockets_fetch_POPULATED)
      FetchContent_Populate(libwebsockets_fetch)
    endif()

    execute_process(
      COMMAND sh -c "patch -p0 < ${CMAKE_CURRENT_LIST_DIR}/libwebsockets.patch"
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    message("patching done")
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  string(REGEX MATCHALL "[0-9]" versions
               "${CONFIG_NETUTILS_LIBWEBSOCKETS_VERSION}")
  list(GET versions 0 VERSION_MAJOR)
  list(GET versions 1 VERSION_MINOR)
  list(GET versions 2 VERSION_PATCH)
  set(FLAGS
      -DLWS_LIBRARY_VERSION_MAJOR=${VERSION_MAJOR}
      -DLWS_LIBRARY_VERSION_MINOR=${VERSION_MINOR}
      -DLWS_LIBRARY_VERSION_PATCH=${VERSION_PATCH}
      -DLWS_LIBRARY_VERSION_PATCH_ELABORATED=${VERSION_PATCH}-unknown
      -DLWS_LIBRARY_VERSION="${CONFIG_NETUTILS_LIBWEBSOCKETS_VERSION}-unknown"
      -Wno-shadow)

  set(INCDIR
      .
      ${LIBWEBSOCKETS_DIR}/lib/core
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix
      ${LIBWEBSOCKETS_DIR}/lib/event-libs
      ${LIBWEBSOCKETS_DIR}/lib/system/smd
      ${LIBWEBSOCKETS_DIR}/lib/system/metrics
      ${LIBWEBSOCKETS_DIR}/lib/core-net
      ${LIBWEBSOCKETS_DIR}/lib/roles
      ${LIBWEBSOCKETS_DIR}/lib/roles/http
      ${LIBWEBSOCKETS_DIR}/lib/roles/h1
      ${LIBWEBSOCKETS_DIR}/lib/roles/h2
      ${LIBWEBSOCKETS_DIR}/lib/roles/ws
      ${LIBWEBSOCKETS_DIR}/lib/tls
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/wrapper/include
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/wrapper/include/internal)

  set(CSRCS
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-caps.c
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-misc.c
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-init.c
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-file.c
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-pipe.c
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-service.c
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-sockets.c
      ${LIBWEBSOCKETS_DIR}/lib/plat/unix/unix-fds.c
      ${LIBWEBSOCKETS_DIR}/lib/core/alloc.c
      ${LIBWEBSOCKETS_DIR}/lib/core/buflist.c
      ${LIBWEBSOCKETS_DIR}/lib/core/context.c
      ${LIBWEBSOCKETS_DIR}/lib/core/lws_dll2.c
      ${LIBWEBSOCKETS_DIR}/lib/core/lws_map.c
      ${LIBWEBSOCKETS_DIR}/lib/core/libwebsockets.c
      ${LIBWEBSOCKETS_DIR}/lib/core/logs.c
      ${LIBWEBSOCKETS_DIR}/lib/core/vfs.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/base64-decode.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/cache-ttl/lws-cache-ttl.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/cache-ttl/heap.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/cache-ttl/file.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/dir.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/prng.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/lws-ring.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/lwsac/lwsac.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/lwsac/cached-file.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/lejp.c
      ${LIBWEBSOCKETS_DIR}/lib/misc/sha-1.c
      ${LIBWEBSOCKETS_DIR}/lib/system/system.c
      ${LIBWEBSOCKETS_DIR}/lib/system/smd/smd.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/dummy-callback.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/output.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/close.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/network.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/vhost.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/pollfd.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/service.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/sorted-usec-list.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/wsi.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/wsi-timeout.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/adopt.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/state.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/client/client.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/client/connect.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/client/connect2.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/client/connect3.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/client/connect4.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/client/sort-dns.c
      ${LIBWEBSOCKETS_DIR}/lib/core-net/client/conmon.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/pipe/ops-pipe.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/http/header.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/http/date.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/http/parsers.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/http/cookie.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/h1/ops-h1.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/h2/http2.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/h2/hpack.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/h2/ops-h2.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/ws/ops-ws.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/ws/client-ws.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/ws/client-parser-ws.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/raw-skt/ops-raw-skt.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/raw-file/ops-raw-file.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/http/client/client-http.c
      ${LIBWEBSOCKETS_DIR}/lib/event-libs/poll/poll.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/tls.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/tls-network.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/tls-sessions.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/tls-client.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/mbedtls-tls.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/mbedtls-extensions.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/mbedtls-x509.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/mbedtls-ssl.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/mbedtls-session.c
      ${LIBWEBSOCKETS_DIR}/lib/tls/mbedtls/mbedtls-client.c)

  if(CONFIG_NETUTILS_MQTTC)
    list(APPEND INCDIR ${LIBWEBSOCKETS_DIR}/lib/roles/mqtt)
    list(
      APPEND
      CSRCS
      ${LIBWEBSOCKETS_DIR}/lib/roles/mqtt/mqtt.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/mqtt/ops-mqtt.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/mqtt/primitives.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/mqtt/client/client-mqtt.c
      ${LIBWEBSOCKETS_DIR}/lib/roles/mqtt/client/client-mqtt-handshake.c)

  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${LIBWEBSOCKETS_DIR}/include)

  nuttx_add_library(libwebsockets STATIC)
  target_sources(libwebsockets PRIVATE ${CSRCS})
  target_compile_options(libwebsockets PRIVATE ${FLAGS})
  target_include_directories(libwebsockets PRIVATE ${INCDIR})

endif()
