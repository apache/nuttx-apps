# ##############################################################################
# apps/crypto/mbedtls/CMakeLists.txt
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

if(CONFIG_CRYPTO_MBEDTLS)

  # ############################################################################
  # Config and Fetch MbedTLS lib
  # ############################################################################

  set(MBEDTLS_DIR ${CMAKE_CURRENT_LIST_DIR}/mbedtls)

  if(NOT EXISTS ${MBEDTLS_DIR})
    set(MBEDTLS_URL https://github.com/ARMmbed/mbedtls/archive)
    FetchContent_Declare(
      mbedtls_fetch
      URL ${MBEDTLS_URL}/v${CONFIG_MBEDTLS_VERSION}.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/mbedtls BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/crypto/mbedtls/mbedtls
      PATCH_COMMAND
        patch -p1 -d ${MBEDTLS_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-mbedtls-entropy_poll-use-getrandom-to-get-the-system.patch
        && patch -p1 -d ${MBEDTLS_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0002-mbedtls-add-mbedtls-x509-crt-pool.patch
        && patch -p1 -d ${MBEDTLS_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0003-Fix-MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT-warning.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(mbedtls_fetch)

    if(NOT mbedtls_fetch_POPULATED)
      FetchContent_Populate(mbedtls_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set_source_files_properties(${MBEDTLS_DIR}/library/bignum.c
                              PROPERTIES COMPILE_FLAGS -fno-lto)

  if(CONFIG_FRAME_POINTER)
    if(CONFIG_DEBUG_OPTLEVEL STREQUAL -O3)
      set_source_files_properties(${MBEDTLS_DIR}/library/sha246.c
                                  PROPERTIES COMPILE_FLAGS -O2)
      set_source_files_properties(${MBEDTLS_DIR}/library/cmac.c
                                  PROPERTIES COMPILE_FLAGS -O2)
    endif()
  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  file(GLOB CSRCS ${MBEDTLS_DIR}/library/*.c)

  if(CONFIG_MBEDTLS_ENTROPY_HARDWARE_ALT
     OR CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
    list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/entropy_alt.c)
  endif()

  if(CONFIG_MBEDTLS_ALT)
    list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/dev_alt.c)

    if(CONFIG_MBEDTLS_AES_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/aes_alt.c)
    endif()

    if(CONFIG_MBEDTLS_CMAC_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/cmac_alt.c)
    endif()

    if(CONFIG_MBEDTLS_MD5_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/md5_alt.c)
    endif()

    if(CONFIG_MBEDTLS_POLY1305_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/poly1305_alt.c)
    endif()

    if(CONFIG_MBEDTLS_RIPEMD160_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/ripemd160_alt.c)
    endif()

    if(CONFIG_MBEDTLS_SHA1_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/sha1_alt.c)
    endif()

    if(CONFIG_MBEDTLS_SHA256_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/sha256_alt.c)
    endif()

    if(CONFIG_MBEDTLS_SHA512_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/sha512_alt.c)
    endif()

    if(CONFIG_MBEDTLS_BIGNUM_ALT)
      list(APPEND CSRCS ${CMAKE_CURRENT_LIST_DIR}/source/bignum_alt.c)
    endif()

  endif()

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR}/include ${MBEDTLS_DIR}/include)
  # ############################################################################
  # Library Configuration
  # ############################################################################

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${INCDIR})
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_CXX_INCLUDE_DIRECTORIES ${INCDIR})

  nuttx_add_library(mbedtls STATIC)
  target_sources(mbedtls PRIVATE ${CSRCS})
  target_include_directories(mbedtls PRIVATE ${INCDIR})
  target_compile_definitions(mbedtls PRIVATE unix)

  if(CONFIG_ARCH_SIM)
    target_compile_options(mbedtls PRIVATE -O0)
  endif()

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_MBEDTLS_APPS)

    if(CONFIG_MBEDTLS_APP_BENCHMARK)
      nuttx_add_application(
        MODULE
        ${CONFIG_MBEDTLS_APPS}
        NAME
        ${CONFIG_MBEDTLS_APP_BENCHMARK_PROGNAME}
        STACKSIZE
        ${CONFIG_MBEDTLS_APP_BENCHMARK_STACKSIZE}
        PRIORITY
        ${CONFIG_MBEDTLS_APP_BENCHMARK_PRIORITY}
        SRCS
        ${MBEDTLS_DIR}/programs/test/benchmark.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        DEPENDS
        mbedtls)
    endif()

    if(CONFIG_MBEDTLS_APP_SELFTEST)
      nuttx_add_application(
        MODULE
        ${CONFIG_MBEDTLS_APPS}
        NAME
        ${CONFIG_MBEDTLS_APP_SELFTEST_PROGNAME}
        STACKSIZE
        ${CONFIG_MBEDTLS_APP_SELFTEST_STACKSIZE}
        PRIORITY
        ${CONFIG_MBEDTLS_APP_SELFTEST_PRIORITY}
        SRCS
        ${MBEDTLS_DIR}/programs/test/selftest.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        DEPENDS
        mbedtls)
    endif()

  endif()

endif()
