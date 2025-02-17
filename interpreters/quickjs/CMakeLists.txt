# ##############################################################################
# apps/interpreters/quickjs/CMakeLists.txt
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

if(CONFIG_INTERPRETERS_QUICKJS)
  set(QUICKJS_DIR ${CMAKE_CURRENT_LIST_DIR}/quickjs)

  if(NOT EXISTS ${QUICKJS_DIR})
    set(QUICKJS_URL_BASE https://github.com/bellard/quickjs/archive/)

    FetchContent_Declare(
      quickjs_fetch
      URL ${QUICKJS_URL_BASE}/6e2e68fd0896957f92eb6c242a2e048c1ef3cae0.zip
          SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/quickjs BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/interpreters/quickjs/quickjs
      PATCH_COMMAND
        patch -p1 -d ${CMAKE_CURRENT_LIST_DIR}/quickjs <
        ${CMAKE_CURRENT_LIST_DIR}/0001-Disabled-unsupported-feature-on-NuttX.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(quickjs)

    if(NOT quickjs_fetch_POPULATED)
      FetchContent_Populate(quickjs_fetch)
    endif()
  endif()

  set(CMAKE_C_STANDARD 11)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  include(ExternalProject)

  # add quickjs to global INCLUDE DIR
  list(APPEND CFLAGS ${INCDIR_PREFIX}$(APPDIR)/interpreters/quickjs)
  list(APPEND CXXFLAGS ${INCDIR_PREFIX}$(APPDIR)/interpreters/quickjs)
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/interpreters/quickjs)
  set(QUICKJS_VERSION "\"2024-02-14\"")
  set(QUICKJS_FLAGS)
  set(QUICKJS_INCDIR)
  set(QUICKJS_CSRCS)
  set(QUICKJS_SRCS)

  if(CONFIG_INTERPRETERS_QUICKJS_MINI)
    list(APPEND QUICKJS_SRCS qjsmini.c)
  endif()

  list(
    APPEND
    QUICKJS_FLAGS
    -Wno-shadow
    -Wno-array-bounds
    -Wno-incompatible-pointer-types
    -Wno-implicit-function-declaration
    -Wno-unused-function
    -Wno-format
    -D__linux__
    -DCONFIG_VERSION=${QUICKJS_VERSION}
    -include
    alloca.h)

  list(APPEND QUICKJS_FLAGS -Dmp_add=qjs_mp_add -Dmp_sub=qjs_mp_sub
       -Dmp_mul=qjs_mp_mul)

  list(APPEND QUICKJS_INCDIR ${QUICKJS_DIR})

  list(
    APPEND
    QUICKJS_CSRCS
    ${QUICKJS_DIR}/quickjs.c
    ${QUICKJS_DIR}/libregexp.c
    ${QUICKJS_DIR}/libbf.c
    ${QUICKJS_DIR}/libunicode.c
    ${QUICKJS_DIR}/cutils.c)

  if(CONFIG_ARCH_ARM)
    list(APPEND QUICKJS_FLAGS -DFE_TONEAREST=0x00000000)
    list(APPEND QUICKJS_FLAGS -DFE_UPWARD=0x00400000)
    list(APPEND QUICKJS_FLAGS -DFE_DOWNWARD=0x00800000)
    list(APPEND QUICKJS_FLAGS -DFE_TOWARDZERO=0x00c00000)
  endif()

  if(CONFIG_INTERPRETERS_QUICKJS_BIGNUM)
    list(APPEND QUICKJS_FLAGS -DCONFIG_BIGNUM)
    list(APPEND QUICKJS_CSRCS ${CMAKE_CURRENT_BINARY_DIR}/qjscalc.c)
  endif()

  if(CONFIG_INTERPRETERS_QUICKJS_FULL)
    add_custom_command(
      PRE_BUILD
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/qjsc -c -o
              ${CMAKE_CURRENT_BINARY_DIR}/repl.c -m ${QUICKJS_DIR}/repl.js
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/repl.c
      DEPENDS qjsc)
    add_custom_command(
      PRE_BUILD
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/qjsc -fbignum -c -o
              ${CMAKE_CURRENT_BINARY_DIR}/qjscalc.c ${QUICKJS_DIR}/qjscalc.js
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/qjscalc.c
      DEPENDS qjsc)
    list(APPEND QUICKJS_SRCS ${QUICKJS_DIR}/qjs.c
         ${CMAKE_CURRENT_BINARY_DIR}/repl.c)
    list(APPEND QUICKJS_CSRCS ${QUICKJS_DIR}/quickjs-libc.c)
  endif()

  nuttx_add_library(libqjs)
  target_sources(libqjs PRIVATE ${QUICKJS_CSRCS})
  target_include_directories(libqjs PRIVATE ${QUICKJS_INCDIR})
  target_compile_options(libqjs PRIVATE ${QUICKJS_FLAGS})
  nuttx_export_header(TARGET libqjs INCLUDE_DIRECTORIES ${QUICKJS_DIR})

  if(NOT CONFIG_INTERPRETERS_QUICKJS_NONE)
    set(QJS_PRIORITY ${CONFIG_INTERPRETERS_QUICKJS_PRIORITY})
    set(QJS_STACKSIZE ${CONFIG_INTERPRETERS_QUICKJS_STACKSIZE})
    nuttx_add_application(
      NAME
      qjs
      PRIORITY
      ${QJS_PRIORITY}
      STACKSIZE
      ${QJS_STACKSIZE}
      SRCS
      ${QUICKJS_SRCS}
      INCLUDE_DIRECTORIES
      ${QUICKJS_INCDIR}
      COMPILE_FLAGS
      ${QUICKJS_FLAGS}
      DEPENDS
      libqjs)
  endif()

  set(QUICKJS_LIB
      ${CMAKE_CURRENT_BINARY_DIR}
      CACHE INTERNAL "")

  ExternalProject_Add(
    qjsc
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/quickjs
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/
    CMAKE_ARGS -DCONFIG_BIGNUM=${CONFIG_INTERPRETERS_QUICKJS_BIGNUM}
               -DQUICKJS_VERSION=${QUICKJS_VERSION} -DAPPDIR=${NUTTX_APPS_DIR}
    TEST_COMMAND ""
    INSTALL_COMMAND "")

endif()
