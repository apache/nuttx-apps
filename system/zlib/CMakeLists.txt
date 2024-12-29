# ##############################################################################
# apps/system/zlib/CMakeLists.txt
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

if(CONFIG_LIB_ZLIB)

  # ############################################################################
  # Config and Fetch zlib lib
  # ############################################################################

  set(ZLIB_DIR ${CMAKE_CURRENT_LIST_DIR}/zlib)

  if(NOT EXISTS ${ZLIB_DIR})
    set(ZLIB_URL
        https://github.com/madler/zlib/releases/download/v1.3/zlib13.zip)
    FetchContent_Declare(
      zlib_fetch
      URL ${ZLIB_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/zlib BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/system/zlib/zlib
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(zlib_fetch)

    if(NOT zlib_fetch_POPULATED)
      FetchContent_Populate(zlib_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -Dunix -Wno-shadow -Wno-strict-prototypes -Wno-undef)

  if(NOT CONFIG_FS_LARGEFILE)
    list(APPEND CFLAGS -DIOAPI_NO_64 -DMINIZIP_FOPEN_NO_64)
  endif()

  # ############################################################################
  # Sources
  # ############################################################################

  file(GLOB ZLIB_CSRCS ${ZLIB_DIR}/*.c)
  set(CSRCS
      ${ZLIB_DIR}/contrib/minizip/ioapi.c ${ZLIB_DIR}/contrib/minizip/mztools.c
      ${ZLIB_DIR}/contrib/minizip/unzip.c ${ZLIB_DIR}/contrib/minizip/zip.c
      ${ZLIB_CSRCS})

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR ${CMAKE_CURRENT_LIST_DIR}/zlib)

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(zlib STATIC)
  target_sources(zlib PRIVATE ${CSRCS})
  target_include_directories(zlib PRIVATE ${INCDIR})

  target_compile_options(zlib PRIVATE ${CFLAGS})

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_UTILS_GZIP)
    nuttx_add_application(
      MODULE
      ${CONFIG_LIB_ZLIB}
      NAME
      ${CONFIG_UTILS_GZIP_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_GZIP_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_GZIP_PRIORITY}
      SRCS
      ${ZLIB_DIR}/test/minigzip.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      zlib)
  endif()

  if(CONFIG_UTILS_ZIP)
    nuttx_add_application(
      MODULE
      ${CONFIG_LIB_ZLIB}
      NAME
      ${CONFIG_UTILS_ZIP_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_ZIP_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_ZIP_PRIORITY}
      SRCS
      ${ZLIB_DIR}/contrib/minizip/minizip.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      zlib)
  endif()

  if(CONFIG_UTILS_UNZIP)
    nuttx_add_application(
      MODULE
      ${CONFIG_LIB_ZLIB}
      NAME
      ${CONFIG_UTILS_UNZIP_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_UNZIP_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_UNZIP_PRIORITY}
      SRCS
      ${ZLIB_DIR}/contrib/minizip/miniunz.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      zlib)
  endif()

  if(CONFIG_LIB_ZLIB_TEST)
    nuttx_add_application(
      MODULE
      ${CONFIG_LIB_ZLIB}
      NAME
      zlib_infcover_test
      STACKSIZE
      ${CONFIG_LIB_ZLIB_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_LIB_ZLIB_TEST_PRIORITY}
      SRCS
      ${ZLIB_DIR}/test/infcover.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      zlib)
  endif()

endif()
