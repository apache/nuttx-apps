############################################################################
# apps/netutils/nng/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

if(CONFIG_NETUTILS_NNG)

  # ############################################################################
  # Config and Fetch NNG lib
  # ############################################################################

  set(NNG_DIR ${CMAKE_CURRENT_LIST_DIR}/nng)

  if(NOT EXISTS ${NNG_DIR})
    set(NNG_URL "https://github.com/nanomsg/nng/archive")
    FetchContent_Declare(
      nng_fetch
      URL ${NNG_URL}/v${CONFIG_NETUTILS_NNG_VERSION}.tar.gz SOURCE_DIR
          ${NNG_DIR} BINARY_DIR ${CMAKE_BINARY_DIR}/apps/netutils/nng/nng
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(nng_fetch)

    if(NOT nng_fetch)
      FetchContent_Populate(nng_fetch)
    endif()

    execute_process(COMMAND sh -c "patch -p0 < nng_porting_for_nuttx.patch"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  endif()

  set(CFLAGS
      -DNNG_PLATFORM_POSIX
      -DNNG_HAVE_GETRANDOM
      -DNNG_TRANSPORT_INPROC
      -DNNG_TRANSPORT_IPC
      -DNNG_TRANSPORT_TCP
      -DNNG_TRANSPORT_TLS
      -DNNG_TRANSPORT_WS
      -DNNG_TRANSPORT_WSS
      -DNNG_SETSTACKSIZE
      -DNNG_HAVE_PTHREAD_SETNAME_NP
      -DNNG_USE_EVENTFD
      -Wno-unused-value
      -Wno-shadow)

  if(CONFIG_SMP_NCPUS)
    list(APPEND CFLAGS -DNNG_MAX_TASKQ_THREADS=$(CONFIG_SMP_NCPUS))
  else()
    list(APPEND CFLAGS -DNNG_MAX_TASKQ_THREADS=2)
  endif()
  if(CONFIG_NETUTILS_NNG_HAVE_EPOLL)
    list(APPEND CFLAGS -DNNG_HAVE_EPOLL -DNNG_HAVE_EPOLL_CREATE1)
  endif()
  if(CONFIG_NETUTILS_NNG_USE_MBEDTLS)
    list(APPEND CFLAGS -DNNG_TLS_ENGINE_INIT=nng_tls_engine_init_mbed
         -DNNG_TLS_ENGINE_FINI=nng_tls_engine_fini_mbed -DNNG_SUPP_TLS)
  endif()

  set(CSRCS
      ${NNG_DIR}/src/nng.c
      ${NNG_DIR}/src/nng_legacy.c
      ${NNG_DIR}/src/core/aio.c
      ${NNG_DIR}/src/core/device.c
      ${NNG_DIR}/src/core/dialer.c
      ${NNG_DIR}/src/core/file.c
      ${NNG_DIR}/src/core/idhash.c
      ${NNG_DIR}/src/core/init.c
      ${NNG_DIR}/src/core/list.c
      ${NNG_DIR}/src/core/listener.c
      ${NNG_DIR}/src/core/lmq.c
      ${NNG_DIR}/src/core/message.c
      ${NNG_DIR}/src/core/msgqueue.c
      ${NNG_DIR}/src/core/options.c
      ${NNG_DIR}/src/core/pollable.c
      ${NNG_DIR}/src/core/panic.c
      ${NNG_DIR}/src/core/pipe.c
      ${NNG_DIR}/src/core/protocol.c
      ${NNG_DIR}/src/core/reap.c
      ${NNG_DIR}/src/core/socket.c
      ${NNG_DIR}/src/core/stats.c
      ${NNG_DIR}/src/core/stream.c
      ${NNG_DIR}/src/core/strs.c
      ${NNG_DIR}/src/core/taskq.c
      ${NNG_DIR}/src/core/tcp.c
      ${NNG_DIR}/src/core/thread.c
      ${NNG_DIR}/src/core/timer.c
      ${NNG_DIR}/src/core/url.c
      ${NNG_DIR}/src/platform/posix/posix_alloc.c
      ${NNG_DIR}/src/platform/posix/posix_atomic.c
      ${NNG_DIR}/src/platform/posix/posix_clock.c
      ${NNG_DIR}/src/platform/posix/posix_debug.c
      ${NNG_DIR}/src/platform/posix/posix_file.c
      ${NNG_DIR}/src/platform/posix/posix_ipcconn.c
      ${NNG_DIR}/src/platform/posix/posix_ipcdial.c
      ${NNG_DIR}/src/platform/posix/posix_ipclisten.c
      ${NNG_DIR}/src/platform/posix/posix_pipe.c
      ${NNG_DIR}/src/platform/posix/posix_resolv_gai.c
      ${NNG_DIR}/src/platform/posix/posix_sockaddr.c
      ${NNG_DIR}/src/platform/posix/posix_tcpconn.c
      ${NNG_DIR}/src/platform/posix/posix_tcpdial.c
      ${NNG_DIR}/src/platform/posix/posix_tcplisten.c
      ${NNG_DIR}/src/platform/posix/posix_thread.c
      ${NNG_DIR}/src/platform/posix/posix_udp.c
      ${NNG_DIR}/src/platform/posix/posix_rand_getrandom.c
      ${NNG_DIR}/src/sp/transport.c
      ${NNG_DIR}/src/sp/protocol/bus0/bus.c
      ${NNG_DIR}/src/sp/protocol/pair0/pair.c
      ${NNG_DIR}/src/sp/protocol/pair1/pair.c
      ${NNG_DIR}/src/sp/protocol/pair1/pair1_poly.c
      ${NNG_DIR}/src/sp/protocol/pipeline0/pull.c
      ${NNG_DIR}/src/sp/protocol/pipeline0/push.c
      ${NNG_DIR}/src/sp/protocol/pubsub0/pub.c
      ${NNG_DIR}/src/sp/protocol/pubsub0/sub.c
      ${NNG_DIR}/src/sp/protocol/pubsub0/xsub.c
      ${NNG_DIR}/src/sp/protocol/reqrep0/rep.c
      ${NNG_DIR}/src/sp/protocol/reqrep0/req.c
      ${NNG_DIR}/src/sp/protocol/reqrep0/xrep.c
      ${NNG_DIR}/src/sp/protocol/reqrep0/xreq.c
      ${NNG_DIR}/src/sp/protocol/survey0/respond.c
      ${NNG_DIR}/src/sp/protocol/survey0/survey.c
      ${NNG_DIR}/src/sp/protocol/survey0/xrespond.c
      ${NNG_DIR}/src/sp/protocol/survey0/xsurvey.c
      ${NNG_DIR}/src/sp/transport/inproc/inproc.c
      ${NNG_DIR}/src/sp/transport/ipc/ipc.c
      ${NNG_DIR}/src/sp/transport/tcp/tcp.c
      ${NNG_DIR}/src/sp/transport/tls/tls.c
      ${NNG_DIR}/src/sp/transport/ws/websocket.c
      ${NNG_DIR}/src/supplemental/base64/base64.c
      ${NNG_DIR}/src/supplemental/http/http_chunk.c
      ${NNG_DIR}/src/supplemental/http/http_client.c
      ${NNG_DIR}/src/supplemental/http/http_conn.c
      ${NNG_DIR}/src/supplemental/http/http_msg.c
      ${NNG_DIR}/src/supplemental/http/http_public.c
      ${NNG_DIR}/src/supplemental/http/http_schemes.c
      ${NNG_DIR}/src/supplemental/http/http_server.c
      ${NNG_DIR}/src/supplemental/sha1/sha1.c
      ${NNG_DIR}/src/supplemental/tls/tls_common.c
      ${NNG_DIR}/src/supplemental/util/platform.c
      ${NNG_DIR}/src/supplemental/websocket/websocket.c)

  if(CONFIG_NETUTILS_NNG_HAVE_EPOLL)
    list(APPEND CSRCS ${NNG_DIR}/src/platform/posix/posix_pollq_epoll.c)
  else()
    list(APPEND CSRCS ${NNG_DIR}/src/platform/posix/posix_pollq_poll.c)
  endif()
  if(CONFIG_NETUTILS_NNG_USE_MBEDTLS)
    list(APPEND CSRCS ${NNG_DIR}/src/supplemental/tls/mbedtls/tls.c)
  endif()

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${NNG_DIR}/include)
  set(INCDIR ${NNG_DIR}/src)

  nuttx_add_library(nng STATIC)
  target_sources(nng PRIVATE ${CSRCS})
  target_compile_options(nng PRIVATE ${CFLAGS})
  target_include_directories(nng PRIVATE ${INCDIR})
endif()
