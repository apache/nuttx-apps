# ##############################################################################
# apps/crypto/tinydtls/CMakeLists.txt
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

if(CONFIG_CRYPTO_TINYDTLS)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/tinydtls)
    FetchContent_Declare(
      tinydtls
      DOWNLOAD_NAME "${CONFIG_TINYDTLS_VERSION}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "https://github.com/eclipse/tinydtls/archive/${CONFIG_TINYDTLS_VERSION}.tar.gz"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/tinydtls
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/crypto/tinydtls/tinydtls
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(tinydtls)

    if(NOT tinydtls_POPULATED)
      FetchContent_Populate(tinydtls)
    endif()
  endif()

  add_subdirectory(tinydtls)

  target_compile_definitions(
    tinydtls
    PRIVATE HAVE_ARPA_INET_H
            HAVE_ASSERT_H
            HAVE_FCNTL_H
            HAVE_GETRANDOM
            HAVE_INTTYPES_H
            HAVE_MEMSET
            HAVE_NETDB_H
            HAVE_NETINET_IN_H
            HAVE_RANDOM
            HAVE_SELECT
            HAVE_SOCKET
            HAVE_STDDEF_H
            HAVE_STDINT_H
            HAVE_STDLIB_H
            HAVE_STRDUP
            HAVE_STRERROR
            HAVE_STRINGS_H
            HAVE_STRING_H
            HAVE_STRNLEN
            HAVE_SYS_SOCKET_H
            HAVE_SYS_STAT_H
            HAVE_SYS_TIME_H
            HAVE_SYS_TYPES_H
            HAVE_TIME_H
            HAVE_UNISTD_H
            HAVE_VPRINTF
            HAVE_SNPRINTF)

  target_compile_options(tinydtls PRIVATE -Wno-unused -Wno-inline)

  # TODO: this should be function in nuttx/cmake/nuttx_add_library.cmake
  set_property(GLOBAL APPEND PROPERTY NUTTX_EXTRA_LIBRARIES tinydtls)
  nuttx_add_library_internal(tinydtls)

  # allow #include "tinydtls/xxx.h"
  target_include_directories(tinydtls PUBLIC .)

endif()
