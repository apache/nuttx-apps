# ##############################################################################
# apps/interpreters/mquickjs/CMakeLists.txt
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

if(CONFIG_INTERPRETERS_MQJS)
  set(MQJS_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/mquickjs)

  # Prefer local git repo if it exists, otherwise download tarball
  if(EXISTS ${MQJS_SOURCE_DIR}/.git)
    # Use local git-managed mquickjs
    set(MQJS_DIR ${MQJS_SOURCE_DIR})
    message(STATUS "Using local mquickjs git repo: ${MQJS_DIR}")
  else()
    # Download mquickjs to build directory
    FetchContent_Declare(
      mquickjs_fetch
      URL https://github.com/bellard/mquickjs/archive/refs/heads/main.zip
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_MakeAvailable(mquickjs_fetch)

    # The source directory after FetchContent_MakeAvailable
    set(MQJS_DIR ${mquickjs_fetch_SOURCE_DIR})
    message(STATUS "Using downloaded mquickjs: ${MQJS_DIR}")
  endif()

  # Detect target architecture for header generation
  if(CONFIG_ARCH_64BIT)
    set(MQJS_BUILD_FLAGS "-m64")
  else()
    set(MQJS_BUILD_FLAGS "-m32")
  endif()

  include(ExternalProject)

  # Build mqjs_stdlib host tool using mquickjs Makefile
  ExternalProject_Add(
    mqjs_stdlib_host
    SOURCE_DIR ${MQJS_DIR}
    BINARY_DIR ${MQJS_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make mqjs_stdlib
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${MQJS_DIR}/mqjs_stdlib)

  # Generate mqjs_stdlib.h
  add_custom_command(
    OUTPUT ${MQJS_DIR}/mqjs_stdlib.h
    COMMAND ${MQJS_DIR}/mqjs_stdlib ${MQJS_BUILD_FLAGS} >
            ${MQJS_DIR}/mqjs_stdlib.h
    DEPENDS mqjs_stdlib_host
    COMMENT "Generating mqjs_stdlib.h")

  # Generate mquickjs_atom.h
  add_custom_command(
    OUTPUT ${MQJS_DIR}/mquickjs_atom.h
    COMMAND ${MQJS_DIR}/mqjs_stdlib -a ${MQJS_BUILD_FLAGS} >
            ${MQJS_DIR}/mquickjs_atom.h
    DEPENDS mqjs_stdlib_host
    COMMENT "Generating mquickjs_atom.h")

  # Custom target for header generation
  add_custom_target(mqjs_headers_gen DEPENDS ${MQJS_DIR}/mqjs_stdlib.h
                                             ${MQJS_DIR}/mquickjs_atom.h)

  # Create mquickjs library
  nuttx_add_library(libmquickjs)
  target_sources(
    libmquickjs
    PRIVATE ${MQJS_DIR}/mquickjs.c ${MQJS_DIR}/dtoa.c ${MQJS_DIR}/libm.c
            ${MQJS_DIR}/cutils.c ${MQJS_DIR}/readline.c
            ${MQJS_DIR}/readline_tty.c)
  target_include_directories(libmquickjs PRIVATE ${MQJS_DIR})
  nuttx_export_header(TARGET libmquickjs INCLUDE_DIRECTORIES ${MQJS_DIR})

  # Ensure headers are generated before building library
  add_dependencies(libmquickjs mqjs_headers_gen)

  # Add mquickjs include directory to global include paths
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/interpreters/mquickjs)

  # Create mqjs NSH application
  set(MQJS_PRIORITY ${CONFIG_INTERPRETERS_MQJS_PRIORITY})
  set(MQJS_STACKSIZE ${CONFIG_INTERPRETERS_MQJS_STACKSIZE})

  nuttx_add_application(
    NAME
    mqjs
    PRIORITY
    ${MQJS_PRIORITY}
    STACKSIZE
    ${MQJS_STACKSIZE}
    SRCS
    ${MQJS_DIR}/mqjs.c
    INCLUDE_DIRECTORIES
    ${MQJS_DIR}
    DEPENDS
    libmquickjs
    mqjs_headers_gen)

endif()
