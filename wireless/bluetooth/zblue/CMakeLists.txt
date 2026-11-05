# ##############################################################################
# apps/wireless/bluetooth/zblue/CMakeLists.txt
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

if(CONFIG_ZBLUE)
  set(ZBLUE_DIR ${CMAKE_CURRENT_LIST_DIR})
  set(ZEPHYR ${ZBLUE_DIR}/zephyr)
  set(ZBLUE_GEN ${CMAKE_CURRENT_BINARY_DIR}/gen)

  # Zephyr source, downloaded from the release tag. Only the parts used by the
  # Bluetooth host are unpacked, as in the Makefile.

  string(REGEX REPLACE "^v" "" ZEPHYR_VERSION ${CONFIG_ZBLUE_REF})
  set(ZEPHYR_TAR ${ZBLUE_DIR}/zephyr-${CONFIG_ZBLUE_REF}.tar.gz)
  set(ZEPHYR_PATHS
      LICENSE
      Kconfig.constants
      include
      lib/net_buf
      lib/utils
      lib/uuid
      modules/mbedtls
      scripts/kconfig
      subsys/bluetooth
      subsys/crc
      subsys/logging
      subsys/settings)

  if(NOT EXISTS ${ZEPHYR})
    if(NOT EXISTS ${ZEPHYR_TAR})
      file(
        DOWNLOAD
        "https://github.com/zephyrproject-rtos/zephyr/archive/${CONFIG_ZBLUE_REF}.tar.gz"
        ${ZEPHYR_TAR}
        STATUS status)
      list(GET status 0 rc)
      if(NOT rc EQUAL 0)
        file(REMOVE ${ZEPHYR_TAR})
        message(FATAL_ERROR "zblue: download failed: ${status}")
      endif()
    endif()

    if(NOT "${CONFIG_ZBLUE_SHA256}" STREQUAL "")
      file(SHA256 ${ZEPHYR_TAR} sha256)
      if(NOT sha256 STREQUAL CONFIG_ZBLUE_SHA256)
        file(REMOVE ${ZEPHYR_TAR})
        message(FATAL_ERROR "zblue: ${ZEPHYR_TAR} has SHA-256 ${sha256}, "
                            "expected ${CONFIG_ZBLUE_SHA256}")
      endif()
    else()
      message(WARNING "zblue: hash check skipped for ${ZEPHYR_TAR}")
    endif()

    list(TRANSFORM ZEPHYR_PATHS PREPEND "zephyr-${ZEPHYR_VERSION}/"
                                        OUTPUT_VARIABLE members)
    execute_process(
      COMMAND tar xzf ${ZEPHYR_TAR} ${members}
      WORKING_DIRECTORY ${ZBLUE_DIR}
      RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
      message(FATAL_ERROR "zblue: cannot unpack ${ZEPHYR_TAR}")
    endif()

    file(RENAME ${ZBLUE_DIR}/zephyr-${ZEPHYR_VERSION} ${ZEPHYR})
  endif()

  # Zephyr configuration, resolved by the Zephyr Kconfig

  set(ZBLUE_CONF ${CONFIG_ZBLUE_CONF})
  if(NOT IS_ABSOLUTE ${ZBLUE_CONF})
    set(ZBLUE_CONF ${ZBLUE_DIR}/${ZBLUE_CONF})
  endif()

  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  execute_process(
    COMMAND
      ${Python3_EXECUTABLE} ${ZBLUE_DIR}/tools/zkconfig.py --zephyr ${ZEPHYR}
      --kconfig ${ZBLUE_DIR}/Kconfig.zephyr --nuttx-config
      ${CMAKE_BINARY_DIR}/.config --outdir ${ZBLUE_GEN} ${ZBLUE_CONF}
    RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "zblue: Zephyr Kconfig failed")
  endif()

  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${ZBLUE_DIR}/tools/gen_syscalls.py
            ${ZEPHYR}/include ${ZBLUE_GEN}/zephyr/syscalls RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "zblue: syscall generation failed")
  endif()

  set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${ZBLUE_CONF} ${CMAKE_BINARY_DIR}/.config
             ${ZBLUE_DIR}/Kconfig.zephyr ${ZBLUE_DIR}/tools/zkconfig.py
             ${ZBLUE_DIR}/tools/gen_syscalls.py)

  file(STRINGS ${ZBLUE_GEN}/zblue.config lines REGEX "^ZCONFIG_")
  foreach(line ${lines})
    string(REGEX MATCH "^([A-Za-z0-9_]+)=(.*)$" _ ${line})
    set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
  endforeach()

  # Upstream sources, selected as in the upstream CMakeLists.txt files

  set(BT ${ZEPHYR}/subsys/bluetooth)
  file(GLOB SRCS ${ZBLUE_DIR}/port/src/*.c)

  list(
    APPEND
    SRCS
    ${BT}/common/addr.c
    ${BT}/common/dummy.c
    ${BT}/common/bt_str.c
    ${BT}/common/hci_pkt.c
    ${BT}/common/hci_lockstep.c)

  macro(zblue_sources_ifdef option)
    if("${${option}}" STREQUAL "y")
      list(APPEND SRCS ${ARGN})
    endif()
  endmacro()

  zblue_sources_ifdef(ZCONFIG_BT_HCI_SET_PUBLIC_ADDR
                      ${BT}/common/hci_public_addr.c)
  zblue_sources_ifdef(ZCONFIG_BT_RPA ${BT}/common/rpa.c)
  zblue_sources_ifdef(ZCONFIG_BT_LONG_WQ ${BT}/common/long_wq.c)

  if("${ZCONFIG_BT_HCI_HOST}" STREQUAL "y")
    list(
      APPEND
      SRCS
      ${BT}/host/uuid.c
      ${BT}/host/addr.c
      ${BT}/host/buf.c
      ${BT}/host/data.c
      ${BT}/host/hci_core.c
      ${BT}/host/hci_common.c
      ${BT}/host/id.c)
    zblue_sources_ifdef(ZCONFIG_BT_BROADCASTER ${BT}/host/adv.c)
    zblue_sources_ifdef(ZCONFIG_BT_OBSERVER ${BT}/host/scan.c)
    zblue_sources_ifdef(ZCONFIG_BT_HOST_CRYPTO ${BT}/host/crypto_psa.c)
    zblue_sources_ifdef(ZCONFIG_BT_ECC ${BT}/host/ecc.c)
    if("${ZCONFIG_BT_CONN}" STREQUAL "y")
      list(APPEND SRCS ${BT}/host/conn.c ${BT}/host/l2cap.c ${BT}/host/att.c
           ${BT}/host/gatt.c)
      zblue_sources_ifdef(ZCONFIG_BT_GATT_GAP_SVC_VALIDATE
                          ${BT}/host/gatt_gap_svc_validate.c)
      if("${ZCONFIG_BT_SMP}" STREQUAL "y")
        list(APPEND SRCS ${BT}/host/smp.c ${BT}/host/keys.c)
      else()
        list(APPEND SRCS ${BT}/host/smp_null.c)
      endif()
    endif()
  endif()

  zblue_sources_ifdef(ZCONFIG_BT_HOST_CCM ${BT}/host/aes_ccm.c)
  zblue_sources_ifdef(ZCONFIG_BT_CRYPTO ${BT}/crypto/bt_crypto.c
                      ${BT}/crypto/bt_crypto_psa.c)
  zblue_sources_ifdef(ZCONFIG_BT_GAP_SVC ${BT}/services/gap_svc.c)
  zblue_sources_ifdef(ZCONFIG_NET_BUF ${ZEPHYR}/lib/net_buf/buf.c
                      ${ZEPHYR}/lib/net_buf/buf_simple.c)
  zblue_sources_ifdef(ZCONFIG_UUID ${ZEPHYR}/lib/uuid/uuid.c)
  list(APPEND SRCS ${ZEPHYR}/subsys/crc/crc8_sw.c
       ${ZEPHYR}/subsys/crc/crc16_sw.c ${ZEPHYR}/lib/utils/hex.c)

  # Flags for the Zephyr sources, the port and the applications using zblue

  set(ZBLUE_INCDIRS ${ZBLUE_DIR}/port/include ${ZBLUE_GEN} ${ZEPHYR}/include
                    ${ZEPHYR}/subsys/bluetooth)
  set(ZBLUE_FLAGS
      -imacros
      ${ZBLUE_GEN}/zephyr/autoconf.h
      -include
      ${ZBLUE_DIR}/port/include/zblue_prefix.h
      -D__ZEPHYR__=1
      -std=gnu11
      -Wno-undef
      -Wno-shadow
      -Wno-unused-but-set-variable)

  # The host relies on the compiler removing code under IS_ENABLED(n)

  if(CONFIG_DEBUG_NOOPT)
    list(APPEND ZBLUE_FLAGS -Og)
  endif()

  # The objects are linked into one relocatable object.  Some of them (e.g.
  # services/gap_svc.c) only add entries to an iterable section and have no
  # symbol referenced from elsewhere, so they would not be pulled out of a
  # static library.

  # Same flags as nuttx_add_library() gives to an application library

  add_library(zblue_objs OBJECT ${SRCS})
  add_dependencies(zblue_objs nuttx_context apps_context)
  target_compile_options(
    zblue_objs
    PRIVATE
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx,NUTTX_COMPILE_OPTIONS>>
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx_apps_interface,APPS_COMPILE_OPTIONS>>
      ${ZBLUE_FLAGS})
  target_compile_definitions(
    zblue_objs
    PRIVATE
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx,NUTTX_DEFINITIONS>>
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx_apps_interface,APPS_COMPILE_DEFINITIONS>>
  )
  target_include_directories(
    zblue_objs
    PRIVATE
      ${ZBLUE_INCDIRS}
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx,NUTTX_INCLUDE_DIRECTORIES>>
      $<GENEX_EVAL:$<TARGET_PROPERTY:nuttx_apps_interface,APPS_INCLUDE_DIRECTORIES>>
      $<TARGET_PROPERTY:mbedtls,INTERFACE_INCLUDE_DIRECTORIES>)
  target_include_directories(
    zblue_objs SYSTEM PRIVATE ${NUTTX_DIR}/include ${NUTTX_BINARY_DIR}/include)

  add_custom_command(
    OUTPUT ${ZBLUE_GEN}/zblue.o
    COMMAND ${CMAKE_C_COMPILER} -r -nostdlib -o ${ZBLUE_GEN}/zblue.o
            $<TARGET_OBJECTS:zblue_objs>
    DEPENDS $<TARGET_OBJECTS:zblue_objs>
    COMMAND_EXPAND_LISTS
    COMMENT "LD: zblue.o")

  set_source_files_properties(${ZBLUE_GEN}/zblue.o
                              PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
  nuttx_add_library(zblue STATIC)
  target_sources(zblue PRIVATE ${ZBLUE_GEN}/zblue.o)
  set_target_properties(zblue PROPERTIES LINKER_LANGUAGE C)
endif()
