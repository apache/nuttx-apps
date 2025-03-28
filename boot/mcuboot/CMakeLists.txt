# ##############################################################################
# apps/boot/mcuboot/CMakeLists.txt
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

if(CONFIG_BOOT_MCUBOOT)
  nuttx_add_library(mcuboot)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/mcuboot)
    FetchContent_Declare(
      mcuboot
      DOWNLOAD_NAME "${CONFIG_MCUBOOT_VERSION}.tar.gz"
      DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}
      URL "https://github.com/mcu-tools/mcuboot/archive/${CONFIG_MCUBOOT_VERSION}.tar.gz"
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/mcuboot
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/boot/mcuboot/mcuboot
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(mcuboot)

    if(NOT mcuboot_POPULATED)
      FetchContent_Populate(mcuboot)
    endif()
  endif()

  set(SRCS
      mcuboot/boot/bootutil/src/boot_record.c
      mcuboot/boot/bootutil/src/bootutil_misc.c
      mcuboot/boot/bootutil/src/bootutil_public.c
      mcuboot/boot/bootutil/src/caps.c
      mcuboot/boot/bootutil/src/encrypted.c
      mcuboot/boot/bootutil/src/fault_injection_hardening.c
      mcuboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c
      mcuboot/boot/bootutil/src/image_ed25519.c
      mcuboot/boot/bootutil/src/image_rsa.c
      mcuboot/boot/bootutil/src/image_validate.c
      mcuboot/boot/bootutil/src/loader.c
      mcuboot/boot/bootutil/src/swap_misc.c
      mcuboot/boot/bootutil/src/swap_move.c
      mcuboot/boot/bootutil/src/swap_scratch.c
      mcuboot/boot/bootutil/src/tlv.c)

  list(APPEND SRCS mcuboot/boot/nuttx/src/flash_map_backend/flash_map_backend.c)

  if(CONFIG_MCUBOOT_BOOTLOADER)
    nuttx_add_application(
      NAME
      mcuboot_loader
      SRCS
      ${CMAKE_CURRENT_LIST_DIR}/mcuboot/boot/nuttx/main.c
      INCLUDE_DIRECTORIES
      mcuboot/boot/bootutil/include
      mcuboot/boot/nuttx/include
      STACKSIZE
      ${CONFIG_DEFAULT_TASK_STACKSIZE}
      PRIORITY
      SCHED_PRIORITY_DEFAULT)
  endif()

  if(CONFIG_MCUBOOT_WATCHDOG)
    list(APPEND SRCS mcuboot/boot/nuttx/src/watchdog/watchdog.c)
  endif()

  if(CONFIG_MCUBOOT_USE_TINYCRYPT)
    list(
      APPEND
      SRCS
      mcuboot/ext/tinycrypt/lib/source/aes_encrypt.c
      mcuboot/ext/tinycrypt/lib/source/aes_decrypt.c
      mcuboot/ext/tinycrypt/lib/source/ctr_mode.c
      mcuboot/ext/tinycrypt/lib/source/hmac.c
      mcuboot/ext/tinycrypt/lib/source/ecc_dh.c
      mcuboot/ext/tinycrypt/lib/source/sha256.c
      mcuboot/ext/tinycrypt/lib/source/utils.c)
    target_include_directories(mcuboot
                               PRIVATE mcuboot/ext/tinycrypt/lib/include)
  endif()

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR}/mcuboot/boot/nuttx/include
             ${CMAKE_CURRENT_LIST_DIR}/mcuboot/boot/bootutil/include)
  target_include_directories(mcuboot PRIVATE ${INCDIR})

  target_compile_options(mcuboot PRIVATE -Wno-undef)
  target_sources(mcuboot PRIVATE ${SRCS})

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${INCDIR})
endif()
