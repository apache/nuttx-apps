# ##############################################################################
# apps/tools/Wasm/WASI-SDK.cmake
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

# This file is to used for finding the WASI SDK and setting up the necessary
# flags for building WebAssembly applications with CMake or Makefile in NuttX
# build system.
#
# This file is intended to be included in the top-level CMakeLists.txt file of
# the application.
#
# For legacy Makefile-based build system, CMake will be called in the Makefile
# to do actual build.

# If no WASI_SDK_PATH is provided, the raise an error and stop the build
if(NOT DEFINED WASI_SDK_PATH)
  message(FATAL_ERROR "WASI_SDK_PATH is not defined."
                      "Please set it to the path of the WASI SDK.")
endif()

# Set the system name, version and processor to WASI These are from original
# WASI SDK's cmake toolchain file
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

# Fetch the wasm compiler and linker from the WASI SDK
set(CMAKE_C_COMPILER ${WASI_SDK_PATH}/bin/clang)
set(CMAKE_CXX_COMPILER ${WASI_SDK_PATH}/bin/clang++)

# ~~~
# Function "wasm_add_application" to add a WebAssembly application to the
# build system.
#
# This function is used to add a WebAssembly application to the build system.
# It creates an executable target for the application and sets the necessary
# properties for building the application.
#
# Usage:
#   wasm_add_application(NAME <name> SRCS <source files>
#     [STACK_SIZE <stack size>] [INITIAL_MEMORY_SIZE <initial memory size>])
#
# Parameters:
#   NAME: The name of the application (NAME.wasm).
#   SRCS: The source files of the application.
#   STACK_SIZE: The stack size of the application. Default is 2048.
#   INITIAL_MEMORY_SIZE: The initial memory size of the application.
#     Default is 65536 (One page), and must be a multiple of 65536.
# ~~~

function(wasm_add_application)

  # Parse the APP_NAME and APP_SRCS from the arguments
  set(APP_NAME "")
  set(APP_SRCS "")
  set(APP_STACK_SIZE 2048)
  set(APP_INITIAL_MEMORY_SIZE 65536)

  cmake_parse_arguments(APP "" "NAME;STACK_SIZE;INITIAL_MEMORY_SIZE" "SRCS"
                        ${ARGN})

  # Check if the APP_NAME (NAME) is provided
  if(NOT APP_NAME)
    message(FATAL_ERROR "NAME is not provided.")
  endif()

  # Check if the APP_SRCS (SRCS) is provided
  if(NOT APP_SRCS)
    message(FATAL_ERROR "SRCS is not provided.")
  endif()

  # Create the executable target for the application
  add_executable(${APP_NAME} ${APP_SRCS})

  # Set the target properties
  set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME ${APP_NAME}.wasm)

endfunction()

# ~~~
# Function "wasm_add_library" to add a WebAssembly library to the build system.
#
# This function is used to add a WebAssembly library to the build system.
# It creates a static library target for the library and sets the necessary
# properties for building the library.
#
# Usage:
#   wasm_add_library(NAME <name> SRCS <source files>)
#
# Parameters:
#   NAME: The name of the library (libNAME.a).
#   SRCS: The source files of the library.
# ~~~

function(wasm_add_library)

  # Parse the LIB_NAME and LIB_SRCS from the arguments
  set(LIB_NAME "")
  set(LIB_SRCS "")

  cmake_parse_arguments(LIB "" "NAME" "SRCS" ${ARGN})

  # Check if the LIB_NAME (NAME) is provided
  if(NOT LIB_NAME)
    message(FATAL_ERROR "NAME is not provided.")
  endif()

  # Check if the LIB_SRCS (SRCS) is provided
  if(NOT LIB_SRCS)
    message(FATAL_ERROR "SRCS is not provided.")
  endif()

  # Create the static library target for the library
  add_library(${LIB_NAME} STATIC ${LIB_SRCS})

  # Set the target properties
  set_target_properties(${LIB_NAME} PROPERTIES OUTPUT_NAME lib${LIB_NAME}.a)

endfunction()
