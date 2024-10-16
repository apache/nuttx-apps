# ##############################################################################
# apps/system/xz/CMakeLists.txt
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

if(CONFIG_UTILS_XZ)

  # ############################################################################
  # Config and Fetch libarchive
  # ############################################################################

  set(XZ_DIR ${CMAKE_CURRENT_LIST_DIR}/xz)

  if(NOT EXISTS ${XZ_DIR})
    set(XZ_URL
        https://github.com/tukaani-project/xz/releases/download/v5.6.3/xz-5.6.3.tar.gz
    )

    FetchContent_Declare(
      xz_fetch
      URL ${XZ_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/xz BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/system/xz/xz
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(xz_fetch)

    if(NOT xz_fetch_POPULATED)
      FetchContent_Populate(xz_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -DHAVE__BOOL=1 -DHAVE_CONFIG_H -DMYTHREAD_POSIX)

  # ############################################################################
  # Sources
  # ############################################################################

  set(XZ_PREFIX ${XZ_DIR}/src/liblzma)
  file(
    GLOB
    XZ_CSRCS
    ${XZ_PREFIX}/check/*_fast.c
    ${XZ_PREFIX}/check/check.c
    ${XZ_PREFIX}/check/crc32_table.c
    ${XZ_PREFIX}/check/crc64_table.c
    ${XZ_PREFIX}/check/sha256.c
    ${XZ_PREFIX}/check/sha256.c
    ${XZ_PREFIX}/rangecoder/price_table.c
    ${XZ_PREFIX}/lz/*.c
    ${XZ_PREFIX}/lzma/*.c
    ${XZ_PREFIX}/common/*.c
    ${XZ_PREFIX}/simple/*.c
    ${XZ_PREFIX}/delta/*.c
    ${XZ_DIR}/src/common/*.c)

  set(CSRCS ${XZ_CSRCS})

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR
      ${CMAKE_CURRENT_LIST_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/xz/src/common
      ${XZ_PREFIX}/api
      ${XZ_PREFIX}/check
      ${XZ_PREFIX}/common
      ${XZ_PREFIX}/delta
      ${XZ_PREFIX}/lz
      ${XZ_PREFIX}/lzma
      ${XZ_PREFIX}/rangecoder
      ${XZ_PREFIX}/simple)

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(xz STATIC)
  target_sources(xz PRIVATE ${CSRCS})
  target_include_directories(xz PRIVATE ${INCDIR})

  target_compile_options(xz PRIVATE ${CFLAGS})

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_UTILS_XZ_PROGNAME)

    file(GLOB XZ_MAIN_CSRCS ${XZ_DIR}/src/xz/*.c)
    list(REMOVE_ITEM ${XZ_MAIN_CSRCS} "${XZ_DIR}/src/xz/main.c")

    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_XZ}
      NAME
      ${CONFIG_UTILS_XZ_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_XZ_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_XZ_PRIORITY}
      SRCS
      ${XZ_DIR}/src/xz/main.c
      ${XZ_MAIN_CSRCS}
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      xz)

  endif()

  if(CONFIG_UTILS_XZDEC_PROGNAME)

    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_XZ}
      NAME
      ${CONFIG_UTILS_XZDEC_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_XZDEC_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_XZDEC_PRIORITY}
      SRCS
      ${XZ_DIR}/src/xzdec/xzdec.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      xz)

  endif()

endif()
