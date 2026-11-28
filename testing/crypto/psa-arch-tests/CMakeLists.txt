# ##############################################################################
# apps/testing/crypto/psa-arch-tests/CMakeLists.txt
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

if(CONFIG_PSA_ARCH_TESTS)
  set(PSA_DIR ${CMAKE_CURRENT_LIST_DIR}/psa-arch-tests)
  set(PSA_API_DIR ${PSA_DIR}/api-tests)

  if(NOT EXISTS ${PSA_DIR})
    set(PSA_ARCH_TESTS_URL
        "https://github.com/ARM-software/psa-arch-tests/archive/main.zip")
    set(PSA_ARCH_TESTS_ZIP "psa-arch-tests-main.zip")
    set(PSA_ARCH_TESTS_PACKNAME "psa-arch-tests-main")

    file(
      DOWNLOAD ${PSA_ARCH_TESTS_URL}
      ${CMAKE_CURRENT_LIST_DIR}/${PSA_ARCH_TESTS_ZIP}
      TIMEOUT 30
      SHOW_PROGRESS)

    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xzf
              ${CMAKE_CURRENT_LIST_DIR}/${PSA_ARCH_TESTS_ZIP}
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

    execute_process(
      COMMAND ${CMAKE_COMMAND} -E rename ${PSA_ARCH_TESTS_PACKNAME} ${PSA_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

    file(REMOVE ${CMAKE_CURRENT_LIST_DIR}/${PSA_ARCH_TESTS_ZIP})
  endif()

  file(GLOB_RECURSE PSA_VAL_NSPE_SRCS ${PSA_API_DIR}/val/nspe/*.c)
  file(GLOB_RECURSE PSA_VAL_COMMON_SRCS ${PSA_API_DIR}/val/common/*.c)

  set(PSA_SRCS
      ${PSA_VAL_NSPE_SRCS} ${PSA_VAL_COMMON_SRCS}
      ${PSA_API_DIR}/platform/targets/tgt_dev_apis_linux/nspe/pal_driver_intf.c
      ${PSA_API_DIR}/platform/targets/common/nspe/pal_weak.c)

  set(PSA_INCLUDES
      ${NUTTX_APPS_DIR}/crypto/mbedtls/mbedtls/include
      ${NUTTX_APPS_DIR}/crypto/mbedtls/mbedtls/library
      ${NUTTX_APPS_DIR}/testing/crypto/psa-arch-tests/include
      ${PSA_API_DIR}/val/nspe
      ${PSA_API_DIR}/val/common
      ${PSA_API_DIR}/platform/targets/common/nspe
      ${PSA_API_DIR}/platform/targets/common/nspe/crypto
      ${PSA_API_DIR}/platform/targets/tgt_dev_apis_linux/nspe/crypto)

  set(PSA_WARNING_FLAGS -Wno-undef -Wno-shadow -Wno-unused-variable
                        -Wno-misleading-indentation -Wno-uninitialized)

  set(PSA_DEFINES "-Dmd5_hash=psa_md5_hash" "-Div=psa_iv"
                  "-Dplaintext=psa_plaintext")

  if(CONFIG_PSA_ARCH_CRYPTO_TESTS)
    file(GLOB_RECURSE PSA_CRYPTO_SRCS ${PSA_API_DIR}/dev_apis/crypto/*.c)
    file(GLOB_RECURSE PSA_CRYPTO_PLATFORM_SRCS
         ${PSA_API_DIR}/platform/targets/common/nspe/crypto/*.c)
    list(APPEND PSA_CRYPTO_SRCS src/pal_crypto.c)
    list(REMOVE_ITEM PSA_CRYPTO_SRCS
         ${PSA_API_DIR}/dev_apis/crypto/test_c078/test_c078.c)
    list(APPEND PSA_SRCS ${PSA_CRYPTO_SRCS} ${PSA_CRYPTO_PLATFORM_SRCS})
    list(APPEND PSA_DEFINES -DCRYPTO)
    list(APPEND PSA_INCLUDES ${PSA_API_DIR}/dev_apis/crypto/common)
  endif()

  if(CONFIG_PSA_ARCH_ITS_TESTS)
    file(GLOB_RECURSE PSA_STORAGE_SRCS ${PSA_API_DIR}/dev_apis/storage/*.c)
    file(
      GLOB_RECURSE PSA_STORAGE_PLATFORM_SRCS
      ${PSA_API_DIR}/platform/targets/common/nspe/internal_trusted_storage/*.c)
    list(REMOVE_ITEM PSA_STORAGE_SRCS
         ${PSA_API_DIR}/dev_apis/storage/test_s016/test_s016.c)
    list(REMOVE_ITEM PSA_STORAGE_SRCS
         ${PSA_API_DIR}/dev_apis/storage/test_s017/test_s017.c)
    list(APPEND PSA_SRCS ${PSA_STORAGE_SRCS} ${PSA_STORAGE_PLATFORM_SRCS})
    list(APPEND PSA_DEFINES -DINTERNAL_TRUSTED_STORAGE)
    list(APPEND PSA_INCLUDES ${PSA_API_DIR}/dev_apis/storage/common)
  endif()

  set(SRCS ${PSA_API_DIR}/platform/targets/tgt_dev_apis_linux/nspe/main.c
           ${PSA_SRCS})

  nuttx_add_application(
    MODULE
    ${CONFIG_PSA_ARCH_TESTS}
    NAME
    ${CONFIG_PSA_ARCH_TESTS_PROGNAME}
    SRCS
    ${SRCS}
    STACKSIZE
    ${CONFIG_PSA_ARCH_TESTS_STACKSIZE}
    PRIORITY
    ${CONFIG_PSA_ARCH_TESTS_PRIORITY}
    INCLUDE_DIRECTORIES
    ${PSA_INCLUDES}
    COMPILE_FLAGS
    ${PSA_WARNING_FLAGS}
    DEFINITIONS
    ${PSA_DEFINES})
endif()
