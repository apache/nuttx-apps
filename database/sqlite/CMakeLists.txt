# ##############################################################################
# apps/database/sqlite/CMakeLists.txt
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

if(CONFIG_LIB_SQLITE)

  set(SQLITE_DIR ${CMAKE_CURRENT_LIST_DIR}/sqlite)

  if(NOT EXISTS ${SQLITE_DIR})
    string(CONCAT SQLITE_URL
                  "https://codeload.github.com/sqlite/sqlite/tar.gz/"
                  "refs/tags/vesion-${CONFIG_LIB_SQLITE_VERSION}")
    FetchContent_Declare(
      sqlite_fetch
      URL ${SQLITE_URL} SOURCE_DIR ${SQLITE_DIR} BINARY_DIR
          ${CMAKE_CURRENT_BINARY_DIR}/sqlite_fetch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(sqlite_fetch)
    if(NOT sqlite_fetch_POPULATED)
      FetchContent_Populate(sqlite_fetch)
    endif()
  endif()

  set(BUILDDIR ${CMAKE_CURRENT_BINARY_DIR}/build)
  file(MAKE_DIRECTORY ${BUILDDIR})

  set(SQLITE_OUTPUTS ${BUILDDIR}/sqlite3.c ${BUILDDIR}/sqlite3.h)
  set(SQLITE_TARGETS sqlite3.c)

  if(CONFIG_UTILS_SQLITE)
    list(APPEND SQLITE_OUTPUTS ${BUILDDIR}/shell.c)
    list(APPEND SQLITE_TARGETS shell.c)
  endif()

  find_program(SH_PROGRAM sh)
  if(NOT SH_PROGRAM)
    set(SH_PROGRAM sh)
  endif()

  find_program(MAKE_PROGRAM NAMES make gmake)
  if(NOT MAKE_PROGRAM)
    set(MAKE_PROGRAM make)
  endif()

  add_custom_command(
    OUTPUT ${SQLITE_OUTPUTS}
    COMMAND ${SH_PROGRAM} ${SQLITE_DIR}/configure
    COMMAND ${MAKE_PROGRAM} ${SQLITE_TARGETS} -j
    WORKING_DIRECTORY ${BUILDDIR}
    DEPENDS ${SQLITE_DIR}/configure
    COMMENT "Generating SQLite amalgamation"
    VERBATIM)

  add_custom_target(
    sqlite_amalgamation
    DEPENDS ${SQLITE_OUTPUTS}
    COMMENT "Build SQLite amalgamation")

  # SQLite library

  nuttx_add_library(sqlite3 STATIC)
  add_dependencies(sqlite3 sqlite_amalgamation)
  target_sources(sqlite3 PRIVATE ${BUILDDIR}/sqlite3.c)
  target_include_directories(
    sqlite3
    PUBLIC ${BUILDDIR}
    PRIVATE ${CMAKE_CURRENT_LIST_DIR})
  target_compile_definitions(sqlite3 PRIVATE _HAVE_SQLITE_CONFIG_H NDEBUG)
  target_compile_options(sqlite3 PRIVATE -Wno-unused-variable -Wno-undef
                                         -Wno-unused-function -Wno-shadow)

  # SQLite CLI application

  if(CONFIG_UTILS_SQLITE)
    nuttx_add_application(
      NAME
      sqlite3
      PRIORITY
      100
      STACKSIZE
      ${CONFIG_UTILS_SQLITE_STACKSIZE}
      MODULE
      ${CONFIG_UTILS_SQLITE}
      SRCS
      ${BUILDDIR}/shell.c
      INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_LIST_DIR}
      ${BUILDDIR}
      DEFINITIONS
      _HAVE_SQLITE_CONFIG_H
      NDEBUG
      COMPILE_FLAGS
      -Wno-unused-variable
      -Wno-undef
      -Wno-unused-function
      -Wno-shadow
      DEPENDS
      sqlite3
      sqlite_amalgamation)
  endif()

  set_property(
    DIRECTORY
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${BUILDDIR})

endif()
