# ##############################################################################
# apps/tools/CMakeLists.txt
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

if(CONFIG_TOOLS_WASM_BUILD OR CONFIG_INTERPRETERS_WAMR_BUILD_MODULES_FOR_NUTTX)

  include(ExternalProject)

  set(TOPDIR
      ${CMAKE_SOURCE_DIR}
      CACHE INTERNAL "")

  set(KCONFIG_FILE_PATH
      ${CMAKE_BINARY_DIR}/.config
      CACHE INTERNAL "")

  # Get parent dir of CMAKE_CURRENT_SOURCE_DIR
  get_filename_component(APPDIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

  # Get the Wasm build dirs from the wasm_interface target
  get_property(
    WASM_DIRS
    TARGET wasm_interface
    PROPERTY WASM_DIR)

  # ensure the Wasm build dirs are unique
  list(REMOVE_DUPLICATES WASM_DIRS)
  # Configure and build the Wasm based application
  add_custom_target(
    configure_wasm_build
    COMMAND
      ${CMAKE_COMMAND} -B${CMAKE_BINARY_DIR}/Wasm
      ${CMAKE_CURRENT_SOURCE_DIR}/Wasm -DAPPDIR=${APPDIR} -DTOPDIR=${TOPDIR}
      -DTOPBINDIR=${CMAKE_BINARY_DIR} -DKCONFIG_FILE_PATH=${KCONFIG_FILE_PATH}
      -DWASI_SDK_PATH=$ENV{WASI_SDK_PATH} -DWASM_DIRS="${WASM_DIRS}")

  add_custom_target(wasm_build COMMAND ${CMAKE_COMMAND} --build
                                       ${CMAKE_BINARY_DIR}/Wasm)

  add_dependencies(wasm_build configure_wasm_build)

  # Add the Wasm based application to the build. Notice: Wasm build will be
  # triggered by the native build process each time, but it's ok since the
  # incremental build is very fast in CMake.
  add_dependencies(apps wasm_build)

endif()
