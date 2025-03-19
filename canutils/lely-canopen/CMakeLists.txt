# ##############################################################################
# apps/canutils/lely-canopen/CMakeLists.txt
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

if(CONFIG_CANUTILS_LELYCANOPEN)

  # ############################################################################
  # Config and Fetch lelycanopen lib
  # ############################################################################

  set(LELYCANOPEN_DIR ${CMAKE_CURRENT_LIST_DIR}/lely-core)

  if(NOT EXISTS ${LELYCANOPEN_DIR})
    FetchContent_Declare(
      lelycanopen_fetch
      URL ${CONFIG_CANUTILS_LELYCANOPEN_URL}/lely-core-${CONFIG_CANUTILS_LELYCANOPEN_VERSION}.tar.gz
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/lely-core
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/canutils/lely-canopen/lely-core
      PATCH_COMMAND
        patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/lely-core <
        ${CMAKE_CURRENT_LIST_DIR}/0001-tools-eliminate-multiple-definitions-of-poll-compile.patch
        && patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/lely-core <
        ${CMAKE_CURRENT_LIST_DIR}/0002-tools-coctl.c-fix-printf-issues.patch &&
        patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/lely-core <
        ${CMAKE_CURRENT_LIST_DIR}/0003-src-co-nmt.c-fix-compilation.patch &&
        patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/lely-core <
        ${CMAKE_CURRENT_LIST_DIR}/0004-tools-coctl.c-add-missing-mutex-init.patch
        && patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/lely-core <
        ${CMAKE_CURRENT_LIST_DIR}/0005-add-NuttX-support.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(lelycanopen_fetch)

    if(NOT lelycanopen_fetch_POPULATED)
      FetchContent_Populate(lelycanopen_fetch)
    endif()
  endif()

  nuttx_add_library(lelycanopen STATIC)

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -Wno-shadow -Wno-undef -DHAVE_CONFIG_H=1)

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS)

  # CAN network object
  list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/can/buf.c
       ${LELYCANOPEN_DIR}/src/can/msg.c ${LELYCANOPEN_DIR}/src/can/vci.c
       ${LELYCANOPEN_DIR}/src/can/net.c)
  # CANopen library
  list(
    APPEND
    CSRCS
    ${LELYCANOPEN_DIR}/src/co/crc.c
    ${LELYCANOPEN_DIR}/src/co/dev.c
    ${LELYCANOPEN_DIR}/src/co/nmt.c
    ${LELYCANOPEN_DIR}/src/co/nmt_hb.c
    ${LELYCANOPEN_DIR}/src/co/nmt_srv.c
    ${LELYCANOPEN_DIR}/src/co/obj.c
    ${LELYCANOPEN_DIR}/src/co/pdo.c
    ${LELYCANOPEN_DIR}/src/co/sdo.c
    ${LELYCANOPEN_DIR}/src/co/ssdo.c
    ${LELYCANOPEN_DIR}/src/co/type.c
    ${LELYCANOPEN_DIR}/src/co/val.c)

  if(CONFIG_CANUTILS_LELYCANOPEN_TIME)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/time.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_CSDO)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/csdo.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_DCF)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/dcf.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_EMCY)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/emcy.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_GW)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/gw.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_GW_TXT)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/gw_txt.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_LSS)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/lss.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_NMTBOOT)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/nmt_boot.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_NMTCFG)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/nmt_cfg.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_RPDO)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/rpdo.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_SDEV)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/sdev.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_SYNC)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/sync.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_TPDO)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/tpdo.c)
  endif()

  if(CONFIG_CANUTILS_LELYCANOPEN_WTM)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/co/wtm.c)
  endif()

  # utils

  if(CONFIG_CANUTILS_LELYCANOPEN_DIAG)
    list(APPEND CSRCS ${LELYCANOPEN_DIR}/src/util/diag.c)
  endif()

  list(
    APPEND
    CSRCS
    ${LELYCANOPEN_DIR}/src/util/bits.c
    ${LELYCANOPEN_DIR}/src/util/bitset.c
    ${LELYCANOPEN_DIR}/src/util/cmp.c
    ${LELYCANOPEN_DIR}/src/util/config.c
    ${LELYCANOPEN_DIR}/src/util/config_ini.c
    ${LELYCANOPEN_DIR}/src/util/dllist.c
    ${LELYCANOPEN_DIR}/src/util/endian.c
    ${LELYCANOPEN_DIR}/src/util/errnum.c
    ${LELYCANOPEN_DIR}/src/util/frbuf.c
    ${LELYCANOPEN_DIR}/src/util/fwbuf.c
    ${LELYCANOPEN_DIR}/src/util/lex.c
    ${LELYCANOPEN_DIR}/src/util/membuf.c
    ${LELYCANOPEN_DIR}/src/util/pheap.c
    ${LELYCANOPEN_DIR}/src/util/print.c
    ${LELYCANOPEN_DIR}/src/util/rbtree.c
    ${LELYCANOPEN_DIR}/src/util/stop.c
    ${LELYCANOPEN_DIR}/src/util/time.c
    ${LELYCANOPEN_DIR}/src/util/ustring.c)

  # Lely IO lib
  if(CONFIG_CANUTILS_LELYCANOPEN_IOLIB)
    list(
      APPEND
      CSRCS
      ${LELYCANOPEN_DIR}/src/io/handle.c
      ${LELYCANOPEN_DIR}/src/io/io.c
      ${LELYCANOPEN_DIR}/src/io/pipe.c
      ${LELYCANOPEN_DIR}/src/io/poll.c
      ${LELYCANOPEN_DIR}/src/io2/posix/poll.c
      ${LELYCANOPEN_DIR}/src/io2/ctx.c
      ${LELYCANOPEN_DIR}/src/io/can.c
      ${LELYCANOPEN_DIR}/src/can/socket.c)
  endif()

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${LELYCANOPEN_DIR}/include ${NUTTX_APPS_DIR}/include/canutils/lely)

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_CANUTILS_LELYCANOPEN_TOOLS_COCTL)
    nuttx_add_application(
      NAME
      coctl
      PRIORITY
      ${CONFIG_CANUTILS_LELYCANOPEN_TOOLS_COCTL_PRIORITY}
      STACKSIZE
      ${CONFIG_CANUTILS_LELYCANOPEN_TOOLS_COCTL_STACKSIZE}
      COMPILE_FLAGS
      ${CFLAGS}
      SRCS
      ${LELYCANOPEN_DIR}/tools/coctl.c
      MODULE
      ${CONFIG_CANUTILS_LELYCANOPEN}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      lelycanopen)
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  target_compile_options(lelycanopen PRIVATE ${CFLAGS})
  target_sources(lelycanopen PRIVATE ${CSRCS})
  target_include_directories(lelycanopen PRIVATE ${INCDIR})

endif()
