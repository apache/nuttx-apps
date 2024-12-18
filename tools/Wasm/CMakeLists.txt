# ##############################################################################
# apps/tools/Wasm/CMakeLists.txt
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

# ~~~
# This file is used to manage the WebAssembly (Wasm) build process as entry
# point. All the wasm apps are built using this file as a sub target.
#
# Necessary input for this file is the APPDIR, TOPDIR and KCONFIG_FILE_PATH.
# The APPDIR is the directory where the Wasm apps are located.
# The TOPDIR is the root directory of the NuttX source code.
# The KCONFIG_FILE_PATH is the path to the .config file of the NuttX build.
# ~~~

cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0079 NEW)

# Include the NuttX kconfig parser to shared the configuration between the NuttX
# build and the Wasm build. And then parse the input KCONFIG_FILE_PATH to get
# the configuration.
include(${TOPDIR}/cmake/nuttx_kconfig.cmake)

# Parse the input KCONFIG_FILE_PATH to get the configuration.
nuttx_export_kconfig(${KCONFIG_FILE_PATH})

# Include the WASI-SDK.cmake file to setup the necessary flags for building
include(WASI-SDK.cmake)

project(WasmApps)

# Add the wasm_interface library to hold all the Wasm libraries.
add_library(wasm_interface INTERFACE)

# Check whether the APPDIR is defined or not. If not, then set it to the parent
# directory of the current CMakeLists.txt file.
if(NOT DEFINED APPDIR)
  message(FATAL_ERROR "APPDIR is not defined")
endif()

# Check wether the TOPDIR is defined or not. If not, then raise an error.
if(NOT DEFINED TOPDIR)
  message(FATAL_ERROR "TOPDIR is not defined")
endif()

# Check wether the TOPBINDIR is defined or not. If not, then raise an error.
if(NOT DEFINED TOPBINDIR)
  message(FATAL_ERROR "TOPBINDIR is not defined")
endif()

if(NOT EXISTS ${TOPBINDIR}/wasm)
  file(MAKE_DIRECTORY ${TOPBINDIR}/wasm)
endif()

# Check wether the KCONFIG_FILE_PATH is defined or not. If not, then raise an
# error.
if(NOT DEFINED KCONFIG_FILE_PATH)
  message(FATAL_ERROR "KCONFIG_FILE_PATH is not defined")
endif()

# Provide FAR macro from command line since it is not supported in wasi-sdk, but
# it is used in NuttX code.
# ~~~
# #define FAR
# ~~~
# It usually defined in nuttx/compiler.h.
set(NUTTX_MACRO_FAR "")
add_definitions(-DFAR=${NUTTX_MACRO_FAR})

# Fake nuttx_add_application to avoid error in the Wasm build process.
function(nuttx_add_application)

endfunction()

# Fake nuttx_add_library to avoid error in the Wasm build process.
function(nuttx_add_library)

endfunction()

separate_arguments(WASM_DIRS)

# ~~~
# Add all the Wasm apps to the build process.
foreach(WASM_APP ${WASM_DIRS})
  string(REPLACE ${APPDIR} "" WASM_APP_BUILD_DIR ${WASM_APP})
  add_subdirectory(${WASM_APP}
                   ${CMAKE_CURRENT_BINARY_DIR}/Wasm/${WASM_APP_BUILD_DIR})
endforeach()
# ~~~
