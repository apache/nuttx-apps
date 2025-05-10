# ##############################################################################
# apps/tee/libteec/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2023 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_LIBTEEC)

  set(OPTEE_CLIENT_DIR ${CMAKE_CURRENT_LIST_DIR}/optee_client)

  if(NOT EXISTS ${OPTEE_CLIENT_DIR})
    FetchContent_Declare(
      optee_client_fetch
      URL https://github.com/OP-TEE/optee_client/archive/refs/tags/${CONFIG_LIBTEEC_VERSION}.tar.gz
          SOURCE_DIR
          ${OPTEE_CLIENT_DIR}
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/tee/libteec/optee_client
      PATCH_COMMAND patch -p1 -d ${OPTEE_CLIENT_DIR} <
                    ${CMAKE_CURRENT_LIST_DIR}/0001-libteec-NuttX.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(optee_client_fetch)

    if(NOT optee_client_fetch_POPULATED)
      FetchContent_Populate(optee_client_fetch)
    endif()
  endif()

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${OPTEE_CLIENT_DIR}/libteec/include)

  nuttx_add_library(libteec STATIC)

  target_sources(libteec PRIVATE optee_client/libteec/src/tee_client_api.c
                                 optee_client/libteec/src/teec_trace.c)
  target_include_directories(libteec
                             PRIVATE ${OPTEE_CLIENT_DIR}/libteec/include)
  target_compile_definitions(libteec PRIVATE BINARY_PREFIX=\"TEEC\")

endif()
