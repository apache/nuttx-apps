# ##############################################################################
# apps/tee/optee_supplicant/CMakeLists.txt
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

if(CONFIG_OPTEE_SUPPLICANT)

  # Versions must match
  set(LIBTEEC_VERSION ${CONFIG_LIBTEEC_VERSION})
  set(OPTEE_SUPPLICANT_VERSION ${CONFIG_OPTEE_SUPPLICANT_VERSION})
  if(NOT "${OPTEE_SUPPLICANT_VERSION}" STREQUAL "${LIBTEEC_VERSION}")
    message(
      FATAL_ERROR
        "OPTEE supplicant version (${OPTEE_SUPPLICANT_VERSION}) does not match libteec version (${LIBTEEC_VERSION}).\n"
        "Please set CONFIG_OPTEE_SUPPLICANT_VERSION to ${LIBTEEC_VERSION} in your config."
    )
  endif()

  # Determine paths
  set(OPTEE_CLIENT_DIR ${CMAKE_CURRENT_LIST_DIR}/optee_client)
  set(OPTEE_CLIENT_ZIP
      ${CMAKE_CURRENT_LIST_DIR}/../libteec/${LIBTEEC_VERSION}.zip)
  set(PATCH_FILE
      ${CMAKE_CURRENT_LIST_DIR}/0001-tee-supplicant-port-to-nuttx.patch)

  # Unpack only if not already present
  if(NOT EXISTS "${OPTEE_CLIENT_DIR}")
    message(
      STATUS
        "Unpacking OP-TEE client from ${OPTEE_CLIENT_ZIP} to ${OPTEE_CLIENT_DIR}"
    )
    # Ensure output dir exists
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/_optee_unpack)
    # Extract zip
    file(ARCHIVE_EXTRACT INPUT ${OPTEE_CLIENT_ZIP} OUTPUT_DIR
         ${CMAKE_CURRENT_LIST_DIR}/_optee_unpack)
    # Rename unpacked folder to optee_client
    file(RENAME
         ${CMAKE_CURRENT_LIST_DIR}/_optee_unpack/optee_client-${LIBTEEC_VERSION}
         ${OPTEE_CLIENT_DIR})
    file(REMOVE_RECURSE ${CMAKE_CURRENT_LIST_DIR}/_optee_unpack)
    # Apply patch
    execute_process(
      COMMAND patch -p1 -d ${OPTEE_CLIENT_DIR} -i ${PATCH_FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
      RESULT_VARIABLE patch_result)
    if(NOT patch_result EQUAL 0)
      message(
        FATAL_ERROR
          "Failed to apply patch ${PATCH_FILE} to optee_client sources.")
    endif()
  endif()

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

  if(CONFIG_OPTEE_GP_SOCKETS)
    list(APPEND SUPPLICANT_SRCS
         ${OPTEE_CLIENT_DIR}/tee-supplicant/src/tee_socket.c)
    add_compile_definitions(CFG_GP_SOCKETS=1)
  endif()

  # Compiler flags
  set(SUPPLICANT_CFLAGS
      ${INCDIR_PREFIX}${OPTEE_CLIENT_DIR}/tee-supplicant/src
      ${DEFINE_PREFIX}TEEC_LOAD_PATH=\"/bin\"
      ${DEFINE_PREFIX}TEE_FS_PARENT_PATH=\"/data/tee\"
      ${DEFINE_PREFIX}DEBUGLEVEL_${CONFIG_OPTEE_SUPPLICANT_LOG_LEVEL})
  add_compile_options($<$<COMPILE_LANGUAGE:C>:${SUPPLICANT_CFLAGS}>)

  # Expose public headers for other apps
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${OPTEE_CLIENT_DIR}/libteec/include)

  # Add the application
  nuttx_add_application(
    NAME
    ${CONFIG_OPTEE_SUPPLICANT_PROGNAME}
    SRCS
    ${SUPPLICANT_SRCS}
    INCLUDES
    ${OPTEE_CLIENT_DIR}/libteec/include
    ${OPTEE_CLIENT_DIR}/tee-supplicant/src
    STACKSIZE
    ${CONFIG_OPTEE_SUPPLICANT_STACKSIZE}
    PRIORITY
    ${CONFIG_OPTEE_SUPPLICANT_PRIORITY}
    REQUIRES
    libteec)

endif()
