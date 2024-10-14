# ##############################################################################
# apps/system/libarchive/CMakeLists.txt
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

if(CONFIG_UTILS_LIBARCHIVE)

  # ############################################################################
  # Config and Fetch libarchive
  # ############################################################################

  set(LIBARCHIVE_DIR ${CMAKE_CURRENT_LIST_DIR}/libarchive)

  if(NOT EXISTS ${LIBARCHIVE_DIR})
    set(LIBARCHIVE_URL
        https://github.com/libarchive/libarchive/releases/download/v3.7.7/libarchive-3.7.7.zip
    )

    FetchContent_Declare(
      libarchive_fetch
      URL ${LIBARCHIVE_URL} SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libarchive
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/system/libarchive/libarchive
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30
      PATCH_COMMAND
        patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/libarchive <
        ${CMAKE_CURRENT_LIST_DIR}/0001-fix-conflict-libarchive-for-nuttx.patch)

    FetchContent_GetProperties(libarchive_fetch)

    if(NOT libarchive_fetch_POPULATED)
      FetchContent_Populate(libarchive_fetch)
    endif()
  endif()

  # ############################################################################
  # Flags
  # ############################################################################

  set(CFLAGS -Wno-undef -DHAVE_CONFIG_H=1 -D__LIBARCHIVE_CONFIG_H_INCLUDED=1
             -UMAX_SIGNO)

  # ############################################################################
  # Sources
  # ############################################################################

  file(GLOB LIBARCHIVE_CSRCS ${LIBARCHIVE_DIR}/libarchive/*.c
       ${LIBARCHIVE_DIR}/libarchive_fe/*.c)
  set(CSRCS ${LIBARCHIVE_CSRCS})

  # ############################################################################
  # Include Directory
  # ############################################################################

  set(INCDIR
      ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/libarchive/libarchive
      ${CMAKE_CURRENT_LIST_DIR}/libarchive/libarchive_fe)

  if(CONFIG_UTILS_XZ)
    list(APPEND INCDIR ${NUTTX_APPS_DIR}/system/xz/xz/src/liblzma/api/)
  endif()

  if(CONFIG_LIB_ZLIB)
    list(APPEND INCDIR INCDIR ${NUTTX_APPS_DIR}/system/zlib/zlib)
  endif()

  if(CONFIG_CRYPTO_MBEDTLS)
    list(APPEND INCDIR INCDIR ${NUTTX_APPS_DIR}/crypto/mbedtls/mbedtls/include)
  endif()

  # ############################################################################
  # Library Configuration
  # ############################################################################

  nuttx_add_library(libarchive STATIC)
  target_sources(libarchive PRIVATE ${CSRCS})
  target_include_directories(libarchive PRIVATE ${INCDIR})

  target_compile_options(libarchive PRIVATE ${CFLAGS})

  # ############################################################################
  # Applications Configuration
  # ############################################################################

  if(CONFIG_UTILS_LIBARCHIVE_BSDUNZIP)

    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_LIBARCHIVE}
      NAME
      ${CONFIG_UTILS_LIBARCHIVE_BSDUNZIP_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_LIBARCHIVE_BSDUNZIP_STACKSIZE}
      PRIORITY
      ${CCONFIG_UTILS_LIBARCHIVE_BSDUNZIP_PRIORITY}
      SRCS
      ${LIBARCHIVE_DIR}/unzip/bsdunzip.c
      ${LIBARCHIVE_DIR}/unzip/cmdline.c
      ${LIBARCHIVE_DIR}/unzip/la_getline.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libarchive)

  endif()

  if(CONFIG_UTILS_LIBARCHIVE_BSDTAR)

    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_LIBARCHIVE}
      NAME
      ${CONFIG_UTILS_LIBARCHIVE_BSDTAR_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_LIBARCHIVE_BSDTAR_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_LIBARCHIVE_BSDTAR_PRIORITY}
      SRCS
      ${LIBARCHIVE_DIR}/tar/bsdtar.c
      ${LIBARCHIVE_DIR}/tar/creation_set.c
      ${LIBARCHIVE_DIR}/tar/cmdline.c
      ${LIBARCHIVE_DIR}/tar/read.c
      ${LIBARCHIVE_DIR}/tar/subst.c
      ${LIBARCHIVE_DIR}/tar/util.c
      ${LIBARCHIVE_DIR}/tar/write.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libarchive)

  endif()

  if(CONFIG_UTILS_LIBARCHIVE_BSDCAT)

    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_LIBARCHIVE}
      NAME
      ${CONFIG_UTILS_LIBARCHIVE_BSDCAT_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_LIBARCHIVE_BSDCAT_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_LIBARCHIVE_BSDCAT_PRIORITY}
      SRCS
      ${LIBARCHIVE_DIR}/cat/bsdcat.c
      ${LIBARCHIVE_DIR}/cat/cmdline.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libarchive)

  endif()

  if(CONFIG_UTILS_LIBARCHIVE_CPIO)

    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_LIBARCHIVE}
      NAME
      ${CONFIG_UTILS_LIBARCHIVE_CPIO_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_LIBARCHIVE_CPIO_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_LIBARCHIVE_CPIO_PRIORITY}
      SRCS
      ${LIBARCHIVE_DIR}/cpio/cpio.c
      ${LIBARCHIVE_DIR}/cpio/cmdline.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libarchive)

  endif()

  if(CONFIG_UTILS_LIBARCHIVE_MINITAR)

    list(APPEND CFLAGS -Wno-shadow)
    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_LIBARCHIVE}
      NAME
      ${CONFIG_UTILS_LIBARCHIVE_MINITAR_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_LIBARCHIVE_MINITAR_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_LIBARCHIVE_MINITAR_PRIORITY}
      SRCS
      ${LIBARCHIVE_DIR}/examples/minitar/minitar.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libarchive)

  endif()

  if(CONFIG_UTILS_LIBARCHIVE_TARFILTER)

    list(APPEND CFLAGS -Wno-deprecated-declarations)
    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_LIBARCHIVE}
      NAME
      ${CONFIG_UTILS_LIBARCHIVE_TARFILTER_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_LIBARCHIVE_TARFILTER_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_LIBARCHIVE_TARFILTER_PRIORITY}
      SRCS
      ${LIBARCHIVE_DIR}/examples/tarfilter.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libarchive)

  endif()

  if(CONFIG_UTILS_LIBARCHIVE_UNTAR)

    list(APPEND CFLAGS -Wno-unused-but-set-variable)
    nuttx_add_application(
      MODULE
      ${CONFIG_UTILS_LIBARCHIVE}
      NAME
      ${CONFIG_UTILS_LIBARCHIVE_UNTAR_PROGNAME}
      STACKSIZE
      ${CONFIG_UTILS_LIBARCHIVE_UNTAR_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_LIBARCHIVE_UNTAR_PRIORITY}
      SRCS
      ${LIBARCHIVE_DIR}/examples/untar.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libarchive)

  endif()
endif()
