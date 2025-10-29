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
    set(OPTEE_CLIENT_URL
        https://github.com/OP-TEE/optee_client/archive/refs/tags)
    set(OPTEE_CLIENT_VER ${CONFIG_OPTEE_CLIENT_VERSION})
    FetchContent_Declare(
      optee_client_fetch
      URL ${OPTEE_CLIENT_URL}/${OPTEE_CLIENT_VER}.zip SOURCE_DIR
          ${OPTEE_CLIENT_DIR} BINARY_DIR
          ${CMAKE_BINARY_DIR}/tee/optee_client/optee_client
      PATCH_COMMAND
        patch -p1 -d ${OPTEE_CLIENT_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-libteec-NuttX.patch && patch -p1 -d
        ${OPTEE_CLIENT_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0002-tee-supplicant-port-to-nuttx.patch
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
  target_include_directories(libteec PUBLIC ${OPTEE_CLIENT_DIR}/libteec/include)
  target_compile_definitions(libteec PUBLIC BINARY_PREFIX=\"TEEC\")

  if(CONFIG_DEBUG_INFO)
    target_compile_definitions(libteec PUBLIC DEBUGLEVEL=3)
  elseif(CONFIG_DEBUG_WARN)
    target_compile_definitions(libteec PUBLIC DEBUGLEVEL=2)
  elseif(CONFIG_DEBUG_ERROR)
    target_compile_definitions(libteec PUBLIC DEBUGLEVEL=1)
  else()
    # the default DEBUGLEVEL is 1 (with error level)
    target_compile_definitions(libteec PUBLIC DEBUGLEVEL=1)
  endif()

  if(CONFIG_OPTEE_SUPPLICANT)

    # Source files for supplicant
    set(SUPPLICANT_SRCS
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/tee_supplicant.c
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/handle.c
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/hmac_sha2.c
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/sha2.c
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/sd_notify.c
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/teec_ta_load.c
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/tee_supp_fs.c
        ${OPTEE_CLIENT_DIR}/tee-supplicant/src/rpmb.c)

    set(SUPPLICANT_DEFS
        TEEC_LOAD_PATH=\"/bin\" TEE_FS_PARENT_PATH=\"/data/tee\"
        DEBUGLEVEL_${CONFIG_OPTEE_SUPPLICANT_LOG_LEVEL})

    if(CONFIG_OPTEE_GP_SOCKETS)
      list(APPEND SUPPLICANT_SRCS
           ${OPTEE_CLIENT_DIR}/tee-supplicant/src/tee_socket.c)
      list(APPEND SUPPLICANT_DEFS CFG_GP_SOCKETS=1)
    endif()

    # Add the application
    nuttx_add_application(
      NAME
      ${CONFIG_OPTEE_SUPPLICANT_PROGNAME}
      SRCS
      ${SUPPLICANT_SRCS}
      INCLUDE_DIRECTORIES
      ${OPTEE_CLIENT_DIR}/tee-supplicant/src
      DEFINITIONS
      ${SUPPLICANT_DEFS}
      STACKSIZE
      ${CONFIG_OPTEE_SUPPLICANT_STACKSIZE}
      PRIORITY
      ${CONFIG_OPTEE_SUPPLICANT_PRIORITY}
      DEPENDS
      libteec)

  endif() # CONFIG_OPTEE_SUPPLICANT

endif() # CONFIG_LIBTEEC
