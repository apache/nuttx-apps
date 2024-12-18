############################################################################
# apps/benchmarks/tinymembench/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

if(CONFIG_BENCHMARK_TINY_MEMBENCH)

  set(TINYMB_UNPACK ${CMAKE_CURRENT_LIST_DIR}/tinymembench)
  set(TINYMB_URL https://github.com/ssvb/tinymembench/archive)
  set(TINYMB_ZIP master.zip)

  if(NOT EXISTS ${TINYMB_UNPACK})

    FetchContent_Declare(
      tinymb_fetch
      URL ${TINYMB_URL}/${TINYMB_ZIP} SOURCE_DIR ${TINYMB_UNPACK} BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/benchmarks/tinymembench/tinymembench
      PATCH_COMMAND
        patch -p0 -d ${CMAKE_CURRENT_LIST_DIR} <
        ${CMAKE_CURRENT_LIST_DIR}/0001-tinymembench-fix-compiling-error-for-NuttX.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(tinymb_fetch)
    if(NOT tinymb_fetch_POPULATED)
      FetchContent_Populate(tinymb_fetch)
    endif()

  endif()

  list(APPEND CFLAGS -Wno-unused-but-set-variable -Wno-unused-variable
       -Wno-strict-prototypes)

  set(SRCS tinymembench/main.c)

  list(
    APPEND
    SRCS
    tinymembench/aarch64-asm.S
    tinymembench/arm-neon.S
    tinymembench/x86-sse2.S
    tinymembench/mips-32.S
    tinymembench/asm-opt.c
    tinymembench/util.c)

  nuttx_add_application(
    NAME
    tinymembench
    PRIORITY
    ${CONFIG_BENCHMARK_TINY_MEMBENCH_PRIORITY}
    STACKSIZE
    ${CONFIG_BENCHMARK_TINY_MEMBENCH_STACKSIZE}
    MODULE
    ${CONFIG_BENCHMARK_TINY_MEMBENCH}
    COMPILE_FLAGS
    ${CFLAGS}
    SRCS
    ${SRCS})
endif()
