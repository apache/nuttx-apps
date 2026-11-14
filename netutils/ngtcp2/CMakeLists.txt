# ##############################################################################
# apps/netutils/ngtcp2/CMakeLists.txt
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

if(CONFIG_NETUTILS_QUIC)
  set(NGTCP2_DIR ${CMAKE_CURRENT_LIST_DIR}/ngtcp2)
  set(NGTCP2_VERSION ${CONFIG_NETUTILS_QUIC_VERSION})

  if(NOT EXISTS ${NGTCP2_DIR})
    set(NGTCP2_URL https://github.com/ngtcp2/ngtcp2/releases/download)
    FetchContent_Declare(
      ngtcp2_fetch
      URL ${NGTCP2_URL}/v${NGTCP2_VERSION}/ngtcp2-${NGTCP2_VERSION}.tar.bz2
      SOURCE_DIR ${NGTCP2_DIR} BINARY_DIR
      ${CMAKE_BINARY_DIR}/apps/netutils/ngtcp2
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(ngtcp2_fetch)

    if(NOT ngtcp2_fetch_POPULATED)
      FetchContent_Populate(ngtcp2_fetch)
    endif()
  endif()

  set(NGTCP2_INCDIR
      ${CMAKE_CURRENT_LIST_DIR}
      ${NGTCP2_DIR}/lib
      ${NGTCP2_DIR}/lib/includes
      ${NGTCP2_DIR}/crypto
      ${NGTCP2_DIR}/crypto/includes
      ${NUTTX_APPS_DIR}/../nuttx/include
      ${NUTTX_APPS_DIR}/crypto/wolfssl
      ${NUTTX_APPS_DIR}/crypto/wolfssl/wolfssl
      ${NUTTX_APPS_DIR}/crypto/wolfssl/openssl
      ${NUTTX_APPS_DIR}/include)

  set(NGTCP2_FLAGS
      -Wno-undef
      -DNO_INT128
      -DHAVE_WOLFSSL
      -DBUILDING_NGTCP2
      -DWOLFSSL_QUIC
      -DOPENSSL_EXTRA
      -DHAVE_AESGCM
      -DHAVE_SESSION_TICKET)

  target_include_directories(apps PUBLIC ${NGTCP2_INCDIR})
  target_compile_options(apps PRIVATE ${NGTCP2_FLAGS})
  target_sources(
    apps
    PRIVATE ${NGTCP2_DIR}/lib/ngtcp2_pkt.c
            ${NGTCP2_DIR}/lib/ngtcp2_conv.c
            ${NGTCP2_DIR}/lib/ngtcp2_str.c
            ${NGTCP2_DIR}/lib/ngtcp2_vec.c
            ${NGTCP2_DIR}/lib/ngtcp2_buf.c
            ${NGTCP2_DIR}/lib/ngtcp2_conn.c
            ${NGTCP2_DIR}/lib/ngtcp2_mem.c
            ${NGTCP2_DIR}/lib/ngtcp2_pq.c
            ${NGTCP2_DIR}/lib/ngtcp2_map.c
            ${NGTCP2_DIR}/lib/ngtcp2_rob.c
            ${NGTCP2_DIR}/lib/ngtcp2_ppe.c
            ${NGTCP2_DIR}/lib/ngtcp2_crypto.c
            ${NGTCP2_DIR}/lib/ngtcp2_err.c
            ${NGTCP2_DIR}/lib/ngtcp2_range.c
            ${NGTCP2_DIR}/lib/ngtcp2_acktr.c
            ${NGTCP2_DIR}/lib/ngtcp2_rtb.c
            ${NGTCP2_DIR}/lib/ngtcp2_frame_chain.c
            ${NGTCP2_DIR}/lib/ngtcp2_strm.c
            ${NGTCP2_DIR}/lib/ngtcp2_idtr.c
            ${NGTCP2_DIR}/lib/ngtcp2_gaptr.c
            ${NGTCP2_DIR}/lib/ngtcp2_ringbuf.c
            ${NGTCP2_DIR}/lib/ngtcp2_log.c
            ${NGTCP2_DIR}/lib/ngtcp2_qlog.c
            ${NGTCP2_DIR}/lib/ngtcp2_cid.c
            ${NGTCP2_DIR}/lib/ngtcp2_ksl.c
            ${NGTCP2_DIR}/lib/ngtcp2_cc.c
            ${NGTCP2_DIR}/lib/ngtcp2_bbr.c
            ${NGTCP2_DIR}/lib/ngtcp2_addr.c
            ${NGTCP2_DIR}/lib/ngtcp2_path.c
            ${NGTCP2_DIR}/lib/ngtcp2_pv.c
            ${NGTCP2_DIR}/lib/ngtcp2_pmtud.c
            ${NGTCP2_DIR}/lib/ngtcp2_version.c
            ${NGTCP2_DIR}/lib/ngtcp2_rst.c
            ${NGTCP2_DIR}/lib/ngtcp2_window_filter.c
            ${NGTCP2_DIR}/lib/ngtcp2_opl.c
            ${NGTCP2_DIR}/lib/ngtcp2_balloc.c
            ${NGTCP2_DIR}/lib/ngtcp2_objalloc.c
            ${NGTCP2_DIR}/lib/ngtcp2_unreachable.c
            ${NGTCP2_DIR}/lib/ngtcp2_transport_params.c
            ${NGTCP2_DIR}/lib/ngtcp2_settings.c
            ${NGTCP2_DIR}/lib/ngtcp2_callbacks.c
            ${NGTCP2_DIR}/lib/ngtcp2_dcidtr.c
            ${NGTCP2_DIR}/lib/ngtcp2_pcg.c
            ${NGTCP2_DIR}/lib/ngtcp2_ratelim.c
            ${NGTCP2_DIR}/lib/ngtcp2_conn_info.c
            ${NGTCP2_DIR}/crypto/shared.c
            ${NGTCP2_DIR}/crypto/wolfssl/wolfssl.c)
endif()
