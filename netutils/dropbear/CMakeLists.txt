# ##############################################################################
# apps/netutils/dropbear/CMakeLists.txt
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

if(CONFIG_NETUTILS_DROPBEAR)

  set(DROPBEAR_COMMIT "${CONFIG_NETUTILS_DROPBEAR_COMMIT}")
  string(REPLACE "\"" "" DROPBEAR_COMMIT "${DROPBEAR_COMMIT}")

  set(DROPBEAR_ZIP "${DROPBEAR_COMMIT}.zip")
  set(DROPBEAR_URL "https://github.com/mkj/dropbear/archive")
  set(DROPBEAR_UNPACKNAME "dropbear")

  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DROPBEAR_UNPACKNAME}")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DROPBEAR_ZIP}")
      message(STATUS "Downloading Dropbear: ${DROPBEAR_URL}/${DROPBEAR_ZIP}")
      file(DOWNLOAD "${DROPBEAR_URL}/${DROPBEAR_ZIP}"
           "${CMAKE_CURRENT_SOURCE_DIR}/${DROPBEAR_ZIP}")
    endif()
    message(STATUS "Unpacking Dropbear: ${DROPBEAR_ZIP}")
    execute_process(
      COMMAND unzip -q -o "${DROPBEAR_ZIP}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      RESULT_VARIABLE result)
    if(result EQUAL 0)
      file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/dropbear-${DROPBEAR_COMMIT}"
           "${CMAKE_CURRENT_SOURCE_DIR}/${DROPBEAR_UNPACKNAME}")
      execute_process(
        COMMAND
          patch -s -N -l -p1 -d "${DROPBEAR_UNPACKNAME}" -i
          "${CMAKE_CURRENT_SOURCE_DIR}/patch/0001-guard-platform-declarations.patch"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
      execute_process(
        COMMAND
          patch -s -N -l -p1 -d "${DROPBEAR_UNPACKNAME}" -i
          "${CMAKE_CURRENT_SOURCE_DIR}/patch/0002-use-nuttx-passwd-auth.patch"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
      execute_process(
        COMMAND
          patch -s -N -l -p1 -d "${DROPBEAR_UNPACKNAME}" -i
          "${CMAKE_CURRENT_SOURCE_DIR}/patch/0003-allow-localoptions-to-override-tracking-malloc.patch"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
      execute_process(
        COMMAND
          patch -s -N -l -p1 -d "${DROPBEAR_UNPACKNAME}" -i
          "${CMAKE_CURRENT_SOURCE_DIR}/patch/0004-use-nuttx-unused-macro.patch"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
      message(STATUS "Generating default_options_guard.h")
      execute_process(
        COMMAND
          sh
          "${CMAKE_CURRENT_SOURCE_DIR}/${DROPBEAR_UNPACKNAME}/src/ifndef_wrapper.sh"
        INPUT_FILE
          "${CMAKE_CURRENT_SOURCE_DIR}/${DROPBEAR_UNPACKNAME}/src/default_options.h"
        OUTPUT_FILE
          "${CMAKE_CURRENT_SOURCE_DIR}/${DROPBEAR_UNPACKNAME}/src/default_options_guard.h"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
  endif()

  set(PROGNAME "${CONFIG_NETUTILS_DROPBEAR_PROGNAME}")
  string(REPLACE "\"" "" PROGNAME "${PROGNAME}")

  set(DROPBEAR_SRCS
      dropbear_nshsession.c
      port/nuttx_auth.c
      port/dropbear_utils.c
      dropbear/src/dbutil.c
      dropbear/src/buffer.c
      dropbear/src/dbhelpers.c
      dropbear/src/bignum.c
      dropbear/src/signkey.c
      dropbear/src/dbrandom.c
      dropbear/src/queue.c
      dropbear/src/atomicio.c
      dropbear/src/compat.c
      dropbear/src/fake-rfc2553.c
      dropbear/src/curve25519.c
      dropbear/src/chachapoly.c
      dropbear/src/ltc_prng.c
      dropbear/src/ecc.c
      dropbear/src/ecdsa.c
      dropbear/src/crypto_desc.c
      dropbear/src/dbmalloc.c
      dropbear/src/gensignkey.c
      dropbear/src/common-session.c
      dropbear/src/packet.c
      dropbear/src/common-algo.c
      dropbear/src/common-kex.c
      dropbear/src/common-channel.c
      dropbear/src/common-chansession.c
      dropbear/src/termcodes.c
      dropbear/src/tcp-accept.c
      dropbear/src/listener.c
      dropbear/src/process-packet.c
      dropbear/src/common-runopts.c
      dropbear/src/circbuffer.c
      dropbear/src/list.c
      dropbear/src/netio.c
      dropbear/src/gcm.c
      dropbear/src/kex-x25519.c
      dropbear/src/svr-kex.c
      dropbear/src/svr-auth.c
      dropbear/src/svr-authpasswd.c
      dropbear/src/svr-session.c
      dropbear/src/svr-service.c
      dropbear/src/svr-runopts.c
      dropbear/src/svr-tcpfwd.c
      dropbear/src/svr-forward.c
      dropbear/src/svr-streamfwd.c
      dropbear/src/svr-authpam.c)

  file(GLOB LIBTOMMATH_SRCS CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/dropbear/libtommath/*.c")
  list(APPEND DROPBEAR_SRCS ${LIBTOMMATH_SRCS})

  file(GLOB_RECURSE LIBTOMCRYPT_SRCS CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/dropbear/libtomcrypt/src/*.c")
  list(FILTER LIBTOMCRYPT_SRCS EXCLUDE REGEX ".*/prngs/sober128tab\\.c$")

  # Drop the bundled libtomcrypt HMAC modules replaced by the NuttX adapter.
  list(FILTER LIBTOMCRYPT_SRCS EXCLUDE REGEX ".*/mac/hmac/hmac_init\\.c$")
  list(FILTER LIBTOMCRYPT_SRCS EXCLUDE REGEX ".*/mac/hmac/hmac_process\\.c$")
  list(FILTER LIBTOMCRYPT_SRCS EXCLUDE REGEX ".*/mac/hmac/hmac_done\\.c$")
  list(APPEND DROPBEAR_SRCS port/dropbear_ltc_hmac_sha256.c)

  list(APPEND DROPBEAR_SRCS ${LIBTOMCRYPT_SRCS})

  if(CONFIG_NETUTILS_DROPBEAR_SCP)
    list(APPEND DROPBEAR_SRCS dropbear/src/scpmisc.c port/nuttx_scp.c)
  endif()

  nuttx_add_application(
    NAME
    ${PROGNAME}
    SRCS
    ${DROPBEAR_SRCS}
    dropbear_main.c
    STACKSIZE
    ${CONFIG_NETUTILS_DROPBEAR_STACKSIZE}
    PRIORITY
    ${CONFIG_NETUTILS_DROPBEAR_PRIORITY}
    DEPENDS
    ${DROPBEAR_UNPACKNAME})

  if(CONFIG_NETUTILS_DROPBEAR_SCP)
    nuttx_add_application(
      NAME
      scp
      SRCS
      dropbear/src/scp.c
      STACKSIZE
      ${CONFIG_NETUTILS_DROPBEAR_SCP_STACKSIZE}
      PRIORITY
      ${CONFIG_NETUTILS_DROPBEAR_SCP_PRIORITY}
      DEPENDS
      ${DROPBEAR_UNPACKNAME})
  endif()

  target_include_directories(
    ${PROGNAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/port
            ${CMAKE_CURRENT_SOURCE_DIR}/dropbear
            ${CMAKE_CURRENT_SOURCE_DIR}/dropbear/src
            ${CMAKE_CURRENT_SOURCE_DIR}/dropbear/libtomcrypt/src/headers
            ${CMAKE_CURRENT_SOURCE_DIR}/dropbear/libtommath
            ${CMAKE_CURRENT_SOURCE_DIR}/../../nshlib)

  if(CONFIG_NETUTILS_DROPBEAR_SCP)
    target_include_directories(
      scp
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/port
              ${CMAKE_CURRENT_SOURCE_DIR}/dropbear
              ${CMAKE_CURRENT_SOURCE_DIR}/dropbear/src)
  endif()

  if(CONFIG_NETUTILS_DROPBEAR_COMPRESSION)
    target_include_directories(
      ${PROGNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../system/zlib/zlib)
  endif()

  target_compile_definitions(
    ${PROGNAME}
    PRIVATE LOCALOPTIONS_H_EXISTS=1
            DROPBEAR_NUTTX=1
            DROPBEAR_NUTTX_PASSWD=1
            base64_encode=dropbear_ltc_base64_encode
            base64_decode=dropbear_ltc_base64_decode
            ecc_make_key=dropbear_ltc_ecc_make_key)

  if(CONFIG_NETUTILS_DROPBEAR_SCP)
    target_compile_definitions(
      scp PRIVATE LOCALOPTIONS_H_EXISTS=1 DROPBEAR_NUTTX=1
                  DROPBEAR_NUTTX_PASSWD=1)
  endif()

  set_source_files_properties(
    dropbear_nshsession.c
    PROPERTIES COMPILE_DEFINITIONS
               "Channel=dropbear_channel;ChanType=dropbear_chantype")

  if(CONFIG_NETUTILS_DROPBEAR_SCP)
    set_source_files_properties(
      dropbear/src/scp.c
      PROPERTIES
        COMPILE_DEFINITIONS
        "xmalloc=dropbear_scp_xmalloc;xrealloc=dropbear_scp_xrealloc;xfree=dropbear_scp_xfree;execvp=dropbear_scp_execvp"
    )
    set_source_files_properties(
      dropbear/src/scpmisc.c
      PROPERTIES
        COMPILE_DEFINITIONS
        "xmalloc=dropbear_scp_xmalloc;xrealloc=dropbear_scp_xrealloc;xfree=dropbear_scp_xfree"
    )
  endif()

  # LTC_SOURCE must be set only for libtomcrypt sources.
  set_source_files_properties(${LIBTOMCRYPT_SRCS} PROPERTIES COMPILE_DEFINITIONS
                                                             LTC_SOURCE=1)

  target_compile_options(${PROGNAME} PRIVATE -Wno-pointer-sign -Wno-format)

  if(CONFIG_NETUTILS_DROPBEAR_SCP)
    target_compile_options(
      scp
      PRIVATE -Wno-pointer-sign -Wno-format -include
              ${CMAKE_CURRENT_SOURCE_DIR}/port/nuttx_scp.h
              -Wno-strict-prototypes)
  endif()

  target_sources(apps PRIVATE ${DROPBEAR_SRCS})

endif()
