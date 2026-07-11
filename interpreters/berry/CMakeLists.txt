# ##############################################################################
# apps/interpreters/berry/CMakeLists.txt
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

if(CONFIG_INTERPRETERS_BERRY)
  set(BERRY_COMMIT_ID 4db341475df9e9ad18e6741ef978b4c467690e09)
  set(BERRY_DIR ${CMAKE_CURRENT_LIST_DIR}/berry)

  if(NOT EXISTS ${BERRY_DIR})
    set(BERRY_URL_BASE https://github.com/berry-lang/berry/archive/)

    FetchContent_Declare(
      berry_fetch
      URL ${BERRY_URL_BASE}/${BERRY_COMMIT_ID}.zip SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/berry BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/interpreters/berry/berry
      PATCH_COMMAND
        patch -l -p1 -d ${CMAKE_CURRENT_LIST_DIR}/berry -i
        ${CMAKE_CURRENT_LIST_DIR}/0001-Fix-Berry-default-port-for-NuttX.patch
        COMMAND patch -l -p1 -d ${CMAKE_CURRENT_LIST_DIR}/berry -i
        ${CMAKE_CURRENT_LIST_DIR}/0002-Use-NuttX-console-reader.patch
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(berry_fetch)

    if(NOT berry_fetch_POPULATED)
      FetchContent_Populate(berry_fetch)
    endif()
  endif()

  find_package(
    Python3
    COMPONENTS Interpreter
    REQUIRED)

  set(BERRY_CONFIG_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
  set(BERRY_CONFIG ${BERRY_CONFIG_DIR}/berry_conf.h)
  set(BERRY_GENERATE ${BERRY_DIR}/generate)
  set(BERRY_GEN_STAMP ${BERRY_GENERATE}/.generated)
  set(BERRY_FLAGS -Wno-unused-but-set-variable -Wno-shadow -DUSE_NUTTX_CONSOLE)
  set(BERRY_INCDIR ${BERRY_CONFIG_DIR} ${BERRY_DIR}/src ${BERRY_DIR}/default)

  set(BERRY_CSRCS
      ${BERRY_DIR}/src/be_api.c
      ${BERRY_DIR}/src/be_baselib.c
      ${BERRY_DIR}/src/be_bytecode.c
      ${BERRY_DIR}/src/be_byteslib.c
      ${BERRY_DIR}/src/be_class.c
      ${BERRY_DIR}/src/be_code.c
      ${BERRY_DIR}/src/be_debug.c
      ${BERRY_DIR}/src/be_debuglib.c
      ${BERRY_DIR}/src/be_exec.c
      ${BERRY_DIR}/src/be_filelib.c
      ${BERRY_DIR}/src/be_func.c
      ${BERRY_DIR}/src/be_gc.c
      ${BERRY_DIR}/src/be_gclib.c
      ${BERRY_DIR}/src/be_globallib.c
      ${BERRY_DIR}/src/be_introspectlib.c
      ${BERRY_DIR}/src/be_jsonlib.c
      ${BERRY_DIR}/src/be_lexer.c
      ${BERRY_DIR}/src/be_libs.c
      ${BERRY_DIR}/src/be_list.c
      ${BERRY_DIR}/src/be_listlib.c
      ${BERRY_DIR}/src/be_map.c
      ${BERRY_DIR}/src/be_maplib.c
      ${BERRY_DIR}/src/be_mathlib.c
      ${BERRY_DIR}/src/be_mem.c
      ${BERRY_DIR}/src/be_module.c
      ${BERRY_DIR}/src/be_object.c
      ${BERRY_DIR}/src/be_oslib.c
      ${BERRY_DIR}/src/be_parser.c
      ${BERRY_DIR}/src/be_rangelib.c
      ${BERRY_DIR}/src/be_repl.c
      ${BERRY_DIR}/src/be_solidifylib.c
      ${BERRY_DIR}/src/be_strictlib.c
      ${BERRY_DIR}/src/be_string.c
      ${BERRY_DIR}/src/be_strlib.c
      ${BERRY_DIR}/src/be_syslib.c
      ${BERRY_DIR}/src/be_timelib.c
      ${BERRY_DIR}/src/be_undefinedlib.c
      ${BERRY_DIR}/src/be_var.c
      ${BERRY_DIR}/src/be_vector.c
      ${BERRY_DIR}/src/be_vm.c
      ${BERRY_DIR}/default/be_modtab.c
      ${BERRY_DIR}/default/be_port.c)

  add_custom_command(
    OUTPUT ${BERRY_GEN_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BERRY_GENERATE}
    COMMAND
      ${Python3_EXECUTABLE} ${BERRY_DIR}/tools/coc/coc -o ${BERRY_GENERATE}
      ${BERRY_DIR}/src ${BERRY_DIR}/default -c ${BERRY_CONFIG}
    COMMAND ${CMAKE_COMMAND} -E touch ${BERRY_GEN_STAMP}
    DEPENDS ${BERRY_CSRCS} ${BERRY_CONFIG}
    WORKING_DIRECTORY ${BERRY_DIR}
    COMMENT "Generating Berry constant objects")

  add_custom_target(berry_coc DEPENDS ${BERRY_GEN_STAMP})

  nuttx_add_library(libberry)
  target_sources(libberry PRIVATE ${BERRY_CSRCS})
  target_include_directories(libberry PRIVATE ${BERRY_INCDIR})
  target_compile_options(libberry PRIVATE ${BERRY_FLAGS})
  add_dependencies(libberry berry_coc)
  nuttx_export_header(TARGET libberry INCLUDE_DIRECTORIES ${BERRY_DIR}/src)

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${BERRY_DIR}/src)

  nuttx_add_application(
    MODULE
    ${CONFIG_INTERPRETERS_BERRY}
    NAME
    ${CONFIG_INTERPRETERS_BERRY_PROGNAME}
    STACKSIZE
    ${CONFIG_INTERPRETERS_BERRY_STACKSIZE}
    PRIORITY
    ${CONFIG_INTERPRETERS_BERRY_PRIORITY}
    SRCS
    ${BERRY_DIR}/default/berry.c
    INCLUDE_DIRECTORIES
    ${BERRY_INCDIR}
    COMPILE_FLAGS
    ${BERRY_FLAGS}
    DEPENDS
    libberry
    berry_coc)

endif()
