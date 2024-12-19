# ##############################################################################
# apps/crypto/tinycrypt/CMakeLists.txt
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

if(CONFIG_TINYCRYPT)

  # ############################################################################
  # Config and Fetch tinycrypt
  # ############################################################################
  set(TINYCRYPT_DIR ${CMAKE_CURRENT_LIST_DIR}/tinycrypt)

  if(NOT EXISTS ${TINYCRYPT_DIR})
    set(TINYCRYPT_VERSION ${CONFIG_TINYCRYPT_VERSION})

    set(TINYCRYPT_URL
        https://github.com/intel/tinycrypt/archive/refs/tags/v${TINYCRYPT_VERSION}.zip
    )
    FetchContent_Declare(
      tinycrypt_fetch
      URL ${TINYCRYPT_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/tinycrypt
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/crypto/tinycrypt/tinycrypt
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR}/tinycrypt <
        ${CMAKE_CURRENT_LIST_DIR}/0001-TinyCrypt-test-resolve-compile-error.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(tinycrypt_fetch)

    if(NOT tinycrypt_fetch_POPULATED)
      FetchContent_Populate(tinycrypt_fetch)
    endif()
  endif()

  nuttx_add_library(tinycrypt STATIC)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${TINYCRYPT_DIR}/lib/include)

  nuttx_export_header(TARGET tinycrypt INCLUDE_DIRECTORIES ${INCDIR})

  # ############################################################################
  # Sources
  # ############################################################################

  set(CSRCS
      ${TINYCRYPT_DIR}/lib/source/utils.c ${TINYCRYPT_DIR}/lib/source/ecc.c
      ${TINYCRYPT_DIR}/lib/source/ecc_platform_specific.c)

  if(CONFIG_TINYCRYPT_ECC_DH)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/ecc_dh.c)
  endif()

  if(CONFIG_TINYCRYPT_ECC_DSA)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/ecc_dsa.c)
  endif()

  if(CONFIG_TINYCRYPT_AES)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/aes_encrypt.c
         ${TINYCRYPT_DIR}/lib/source/aes_decrypt.c)
  endif()

  if(CONFIG_TINYCRYPT_AES_CBC)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/cbc_mode.c)
  endif()

  if(CONFIG_TINYCRYPT_AES_CTR)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/ctr_mode.c)
  endif()

  if(CONFIG_TINYCRYPT_AES_CCM)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/ccm_mode.c)
  endif()

  if(CONFIG_TINYCRYPT_AES_CMAC)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/cmac_mode.c)
  endif()

  if(CONFIG_TINYCRYPT_SHA256)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/sha256.c)
  endif()

  if(CONFIG_TINYCRYPT_SHA256_HMAC)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/hmac.c)
  endif()

  if(CONFIG_TINYCRYPT_SHA256_HMAC_PRNG)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/hmac_prng.c)
  endif()

  if(CONFIG_TINYCRYPT_CTR_PRNG)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/lib/source/ctr_prng.c)
  endif()

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_TINYCRYPT_TEST)
    list(APPEND CSRCS ${TINYCRYPT_DIR}/tests/test_ecc_utils.c)
    list(APPEND INCDIR ${TINYCRYPT_DIR}/tests/include)
    set(CFLAGS -Dhex2bin=ltp_hex2bin -DENABLE_TESTS)

    nuttx_add_application(
      NAME
      tinycrypt_test_aes
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_aes.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_cbc_mode
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_cbc_mode.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_ccm_mode
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_ccm_mode.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_cmac_mode
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_cmac_mode.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_ctr_mode
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_ctr_mode.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_ctr_prng
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_ctr_prng.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_ecc_dh
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_ecc_dh.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_ecc_dsa
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_ecc_dsa.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_hmac
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_hmac.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_hmac_prng
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_hmac_prng.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)

    nuttx_add_application(
      NAME
      tinycrypt_test_sha256
      STACKSIZE
      ${CONFIG_TINYCRYPT_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_TINYCRYPT_TEST_PRIORITY}
      SRCS
      ${TINYCRYPT_DIR}/tests/test_sha256.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      tinycrypt)
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  target_sources(tinycrypt PRIVATE ${CSRCS})
  target_include_directories(tinycrypt PRIVATE ${INCDIR})
  if(CFLAGS)
    target_compile_options(tinycrypt PRIVATE ${CFLAGS})
  endif()
endif()
