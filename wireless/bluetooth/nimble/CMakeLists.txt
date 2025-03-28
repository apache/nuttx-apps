# ##############################################################################
# apps/wireless/bluetooth/nimble/CMakeLists.txt
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

if(CONFIG_NIMBLE)
  nuttx_add_library(nimble)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/mynewt-nimble)
    FetchContent_Declare(
      mynewt-nimble
      DOWNLOAD_NAME "${CONFIG_NIMBLE_REF}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "https://github.com/apache/mynewt-nimble/archive/${CONFIG_NIMBLE_REF}.tar.gz"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/mynewt-nimble
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/wireless/bluetooth/nimble/mynewt-nimble
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(mynewt-nimble)

    if(NOT mynewt-nimble_POPULATED)
      FetchContent_Populate(mynewt-nimble)
    endif()
  endif()

  set(SRCS_PORTING
      mynewt-nimble/porting/nimble/src/endian.c
      mynewt-nimble/porting/nimble/src/mem.c
      mynewt-nimble/porting/nimble/src/nimble_port.c
      mynewt-nimble/porting/nimble/src/os_mbuf.c
      mynewt-nimble/porting/nimble/src/os_mempool.c
      mynewt-nimble/porting/nimble/src/os_msys_init.c)

  set(SRCS_NPL
      mynewt-nimble/porting/npl/nuttx/src/os_atomic.c
      mynewt-nimble/porting/npl/nuttx/src/os_callout.c
      mynewt-nimble/porting/npl/nuttx/src/os_eventq.c
      mynewt-nimble/porting/npl/nuttx/src/os_mutex.c
      mynewt-nimble/porting/npl/nuttx/src/os_sem.c
      mynewt-nimble/porting/npl/nuttx/src/os_task.c
      mynewt-nimble/porting/npl/nuttx/src/os_time.c)

  set(SRCS_TRANSPORT
      mynewt-nimble/nimble/transport/socket/src/ble_hci_socket.c
      mynewt-nimble/nimble/transport/src/monitor.c
      mynewt-nimble/nimble/transport/src/transport.c)

  set(SRCS_SERVICES
      mynewt-nimble/nimble/host/services/ans/src/ble_svc_ans.c
      mynewt-nimble/nimble/host/services/bas/src/ble_svc_bas.c
      mynewt-nimble/nimble/host/services/dis/src/ble_svc_dis.c
      mynewt-nimble/nimble/host/services/gap/src/ble_svc_gap.c
      mynewt-nimble/nimble/host/services/gatt/src/ble_svc_gatt.c
      mynewt-nimble/nimble/host/services/ias/src/ble_svc_ias.c
      mynewt-nimble/nimble/host/services/ipss/src/ble_svc_ipss.c
      mynewt-nimble/nimble/host/services/lls/src/ble_svc_lls.c
      mynewt-nimble/nimble/host/services/tps/src/ble_svc_tps.c)

  set(SRCS_HOST
      mynewt-nimble/nimble/host/src/ble_att.c
      mynewt-nimble/nimble/host/src/ble_att_clt.c
      mynewt-nimble/nimble/host/src/ble_att_cmd.c
      mynewt-nimble/nimble/host/src/ble_att_svr.c
      mynewt-nimble/nimble/host/src/ble_dtm.c
      mynewt-nimble/nimble/host/src/ble_eddystone.c
      mynewt-nimble/nimble/host/src/ble_gap.c
      mynewt-nimble/nimble/host/src/ble_gattc.c
      mynewt-nimble/nimble/host/src/ble_gatts.c
      mynewt-nimble/nimble/host/src/ble_gatts_lcl.c
      mynewt-nimble/nimble/host/src/ble_hs_adv.c
      mynewt-nimble/nimble/host/src/ble_hs_atomic.c
      mynewt-nimble/nimble/host/src/ble_hs.c
      mynewt-nimble/nimble/host/src/ble_hs_cfg.c
      mynewt-nimble/nimble/host/src/ble_hs_conn.c
      mynewt-nimble/nimble/host/src/ble_hs_flow.c
      mynewt-nimble/nimble/host/src/ble_hs_hci.c
      mynewt-nimble/nimble/host/src/ble_hs_hci_cmd.c
      mynewt-nimble/nimble/host/src/ble_hs_hci_evt.c
      mynewt-nimble/nimble/host/src/ble_hs_hci_util.c
      mynewt-nimble/nimble/host/src/ble_hs_id.c
      mynewt-nimble/nimble/host/src/ble_hs_log.c
      mynewt-nimble/nimble/host/src/ble_hs_mbuf.c
      mynewt-nimble/nimble/host/src/ble_hs_misc.c
      mynewt-nimble/nimble/host/src/ble_hs_mqueue.c
      mynewt-nimble/nimble/host/src/ble_hs_periodic_sync.c
      mynewt-nimble/nimble/host/src/ble_hs_pvcy.c
      mynewt-nimble/nimble/host/src/ble_hs_shutdown.c
      mynewt-nimble/nimble/host/src/ble_hs_startup.c
      mynewt-nimble/nimble/host/src/ble_hs_stop.c
      mynewt-nimble/nimble/host/src/ble_ibeacon.c
      mynewt-nimble/nimble/host/src/ble_l2cap.c
      mynewt-nimble/nimble/host/src/ble_l2cap_coc.c
      mynewt-nimble/nimble/host/src/ble_l2cap_sig.c
      mynewt-nimble/nimble/host/src/ble_l2cap_sig_cmd.c
      mynewt-nimble/nimble/host/src/ble_sm_alg.c
      mynewt-nimble/nimble/host/src/ble_sm.c
      mynewt-nimble/nimble/host/src/ble_sm_cmd.c
      mynewt-nimble/nimble/host/src/ble_sm_lgcy.c
      mynewt-nimble/nimble/host/src/ble_sm_sc.c
      mynewt-nimble/nimble/host/src/ble_store.c
      mynewt-nimble/nimble/host/src/ble_store_util.c
      mynewt-nimble/nimble/host/src/ble_uuid.c
      mynewt-nimble/nimble/host/util/src/addr.c
      mynewt-nimble/nimble/host/store/ram/src/ble_store_ram.c)

  set(SRCS ${SRCS_PORTING} ${SRCS_NPL} ${SRCS_TRANSPORT} ${SRCS_SERVICES}
           ${SRCS_HOST})

  if(CONFIG_NIMBLE_TINYCRYPT)
    list(
      APPEND
      SRCS
      mynewt-nimble/ext/tinycrypt/src/aes_decrypt.c
      mynewt-nimble/ext/tinycrypt/src/aes_encrypt.c
      mynewt-nimble/ext/tinycrypt/src/cbc_mode.c
      mynewt-nimble/ext/tinycrypt/src/ccm_mode.c
      mynewt-nimble/ext/tinycrypt/src/cmac_mode.c
      mynewt-nimble/ext/tinycrypt/src/ctr_mode.c
      mynewt-nimble/ext/tinycrypt/src/ctr_prng.c
      mynewt-nimble/ext/tinycrypt/src/ecc.c
      mynewt-nimble/ext/tinycrypt/src/ecc_dh.c
      mynewt-nimble/ext/tinycrypt/src/ecc_dsa.c
      mynewt-nimble/ext/tinycrypt/src/ecc_platform_specific.c
      mynewt-nimble/ext/tinycrypt/src/hmac.c
      mynewt-nimble/ext/tinycrypt/src/hmac_prng.c
      mynewt-nimble/ext/tinycrypt/src/sha256.c
      mynewt-nimble/ext/tinycrypt/src/utils.c)
    target_include_directories(nimble
                               PRIVATE mynewt-nimble/ext/tinycrypt/include)
  endif()

  if(CONFIG_NIMBLE_MESH)
    # TODO
    list(
      APPEND
      SRCS
      mynewt-nimble/nimble/host/mesh/src/access.c
      mynewt-nimble/nimble/host/mesh/src/adv.c
      mynewt-nimble/nimble/host/mesh/src/adv_ext.c
      mynewt-nimble/nimble/host/mesh/src/adv_legacy.c
      mynewt-nimble/nimble/host/mesh/src/aes-ccm.c
      mynewt-nimble/nimble/host/mesh/src/app_keys.c)
    target_include_directories(nimble
                               PUBLIC mynewt-nimble/nimble/host/mesh/include)
  endif()

  set(INCLUDES
      mynewt-nimble/nimble/include
      mynewt-nimble/nimble/host/include
      mynewt-nimble/nimble/host/util/include
      mynewt-nimble/nimble/host/services/ans/include
      mynewt-nimble/nimble/host/services/bas/include
      mynewt-nimble/nimble/host/services/dis/include
      mynewt-nimble/nimble/host/services/gap/include
      mynewt-nimble/nimble/host/services/gatt/include
      mynewt-nimble/nimble/host/services/ias/include
      mynewt-nimble/nimble/host/services/ipss/include
      mynewt-nimble/nimble/host/services/lls/include
      mynewt-nimble/nimble/host/services/tps/include
      mynewt-nimble/nimble/host/store/ram/include
      mynewt-nimble/nimble/transport/include
      mynewt-nimble/porting/nimble/include)

  target_include_directories(
    nimble PUBLIC mynewt-nimble/porting/npl/nuttx/include
                  mynewt-nimble/nimble/transport/socket/include)

  target_include_directories(nimble PUBLIC include ${INCLUDES})

  target_compile_options(nimble PUBLIC -Wno-pointer-to-int-cast -Wno-undef
                                       -Wno-format -Wno-unused-but-set-variable)
  target_sources(nimble PRIVATE ${SRCS})
endif()
