# ##############################################################################
# apps/crypto/mbedtls/CMakeLists.txt
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
    endif()
  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  file(GLOB CSRCS ${MBEDTLS_DIR}/library/*.c)

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${MBEDTLS_DIR}/include)

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(mbedtls STATIC)
  nuttx_export_header(TARGET mbedtls INCLUDE_DIRECTORIES ${MBEDTLS_DIR}/include)
  target_sources(mbedtls PRIVATE ${CSRCS})
  target_include_directories(mbedtls PRIVATE ${INCDIR})
  target_compile_definitions(mbedtls PRIVATE __unix__)

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
        mbedtls_nuttx)
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
        mbedtls_nuttx)
    endif()

  endif()

endif()
