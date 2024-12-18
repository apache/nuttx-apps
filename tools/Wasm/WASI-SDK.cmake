# ##############################################################################
# apps/tools/Wasm/WASI-SDK.cmake
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

# setup the flags for the compiler and linker

include_directories(${TOPDIR}/include ${TOPBINDIR}/include)

if(CONFIG_DEBUG_FULLOPT)
  add_compile_options(-Oz)
elseif(CONFIG_DEBUG_CUSTOMOPT)
  add_compile_options(${CONFIG_DEBUG_OPTLEVEL})
endif()

if(CONFIG_LTO_FULL OR CONFIG_LTO_THIN)
  add_compile_options(-flto)
  add_link_options(-flto)
endif()

add_compile_options(--sysroot=${TOPDIR})
add_compile_options(-nostdlib)
add_link_options(-nostdlib)
add_compile_options(-D__NuttX__)

if(NOT CONFIG_LIBM)
  add_compile_options(-DCONFIG_LIBM=1)
  include_directories(${APPDIR}/include/wasm)
endif()

add_link_options(-Wl,--export=main)
add_link_options(-Wl,--export=__main_argc_argv)
add_link_options(-Wl,--export=__heap_base)
add_link_options(-Wl,--export=__data_end)
add_link_options(-Wl,--no-entry)
add_link_options(-Wl,--strip-all)
add_link_options(-Wl,--allow-undefined)

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --print-libgcc-file-name
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE WCC_COMPILER_RT_LIB)

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
  set(APP_STACK_SIZE "")
  set(APP_INITIAL_MEMORY_SIZE "")
  set(APP_INSTALL_NAME "")
  set(APP_WAMR_MODE INT)
  set(APP_WCFLAGS "")
  set(APP_WLDFLAGS "")
  set(APP_WINCLUDES "")

  cmake_parse_arguments(
    APP "" "NAME;STACK_SIZE;INITIAL_MEMORY_SIZE;WAMR_MODE;INSTALL_NAME"
    "SRCS;WLDFLAGS;WCFLAGS;WINCLUDES" ${ARGN})

  # Check if the APP_NAME (NAME) is provided
  if(NOT APP_NAME)
    message(FATAL_ERROR "NAME is not provided.")
  endif()

  # Check if the APP_SRCS (SRCS) is provided
  if(NOT APP_SRCS)
    message(FATAL_ERROR "SRCS is not provided.")
  endif()

  if(NOT APP_STACK_SIZE)
    set(APP_STACK_SIZE 2048)
  endif()

  if(NOT APP_INITIAL_MEMORY_SIZE)
    set(APP_INITIAL_MEMORY_SIZE 65536)
  endif()

  # Create the executable target for the application
  add_executable(${APP_NAME} ${APP_SRCS})

  target_link_libraries(${APP_NAME} PRIVATE wasm_interface)
  target_include_directories(${APP_NAME} PRIVATE ${APP_WINCLUDES})
  target_compile_options(${APP_NAME} PRIVATE ${APP_WCFLAGS})
  target_link_options(${APP_NAME} PRIVATE -z stack-size=${APP_STACK_SIZE})
  target_link_options(${APP_NAME} PRIVATE
                      -Wl,--initial-memory=${APP_INITIAL_MEMORY_SIZE})
  target_link_options(${APP_NAME} PRIVATE ${APP_WLDFLAGS})

  target_link_libraries(${APP_NAME} PRIVATE ${WCC_COMPILER_RT_LIB})
  # Set the target properties
  set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME ${APP_NAME}.wasm)

  # do WASM OPTIMIZATION
  add_custom_target(
    ${APP_NAME}_OPT ALL
    COMMAND ${WASI_SDK_PATH}/wasm-opt -Oz --enable-bulk-memory -o
            ${APP_NAME}.wasm ${APP_NAME}.wasm
    DEPENDS ${APP_NAME}
    COMMENT "WASM build:Optimizing ${APP_NAME}")

  # armv7a
  if(CONFIG_ARCH_ARMV7A)
    if(CONFIG_ARCH_CORTEXA5)
      set(LLVM_CPUTYPE cortex-a5)
    elseif(CONFIG_ARCH_CORTEXA7)
      set(LLVM_CPUTYPE cortex-a7)
    elseif(CONFIG_ARCH_CORTEXA8)
      set(LLVM_CPUTYPE cortex-a8)
    elseif(CONFIG_ARCH_CORTEXA9)
      set(LLVM_CPUTYPE cortex-a9)
    endif()
    if(CONFIG_ARM_THUMB)
      set(LLVM_ARCHTYPE thumbv7)
    else()
      set(LLVM_ARCHTYPE armv7a)
    endif()
    if(CONFIG_ARCH_FPU)
      set(LLVM_ABITYPE eabihf)
    else()
      set(LLVM_ABITYPE eabi)
    endif()
  endif()

  # armv7m
  if(CONFIG_ARCH_ARMV7M)
    if(CONFIG_ARCH_CORTEXM4)
      set(LLVM_CPUTYPE cortex-m4)
    elseif(CONFIG_ARCH_CORTEXM7)
      set(LLVM_CPUTYPE cortex-m7)
    else()
      set(LLVM_CPUTYPE cortex-m3)
    endif()
    if(CONFIG_ARCH_CORTEXM3)
      set(LLVM_ARCHTYPE thumbv7m)
    else()
      set(LLVM_ARCHTYPE thumbv7em)
    endif()
    if(CONFIG_ARCH_FPU)
      set(LLVM_ABITYPE eabihf)
    else()
      set(LLVM_ABITYPE eabi)
    endif()
  endif()

  # armv8m
  if(CONFIG_ARCH_ARMV8M)
    if(CONFIG_ARM_DS)
      set(EXTCPUFLAGS +dsp)
    endif()
    if(CONFIG_ARM_PACBTI)
      set(EXTCPUFLAGS ${EXTCPUFLAGS}+pacbti)
    endif()
    if(CONFIG_ARM_HAVE_MVE)
      set(EXTCPUFLAGS ${EXTCPUFLAGS}+mve.fp+fp.dp)
    endif()
    if(CONFIG_ARCH_CORTEXM23)
      set(LLVM_CPUTYPE cortex-m23)
    elseif(CONFIG_ARCH_CORTEXM33)
      set(LLVM_CPUTYPE cortex-m33)
    elseif(CONFIG_ARCH_CORTEXM35P)
      set(LLVM_CPUTYPE cortex-m35p)
    elseif(CONFIG_ARCH_CORTEXM55)
      set(LLVM_CPUTYPE cortex-m55)
    elseif(CONFIG_ARCH_CORTEXM85)
      set(LLVM_CPUTYPE cortex-m85)
    endif()
    set(LLVM_ARCHTYPE thumbv8m.main${EXTCPUFLAGS})
    if(CONFIG_ARCH_FPU)
      set(LLVM_ABITYPE eabihf)
    else()
      set(LLVM_ABITYPE eabi)
    endif()
  endif()

  # armv7r
  if(CONFIG_ARCH_ARMV7R)
    if(CONFIG_ARCH_CORTEXR4)
      set(LLVM_CPUTYPE cortex-r4)
    elseif(CONFIG_ARCH_CORTEXR5)
      set(LLVM_CPUTYPE cortex-r5)
    elseif(CONFIG_ARCH_CORTEXR7)
      set(LLVM_CPUTYPE cortex-r7)
    endif()

    if(CONFIG_ARM_THUMB)
      set(LLVM_ARCHTYPE thumbv7r)
    else()
      set(LLVM_ARCHTYPE armv7r)
    endif()
    if(CONFIG_ARCH_FPU)
      set(LLVM_ABITYPE eabihf)
    else()
      set(LLVM_ABITYPE eabi)
    endif()
  endif()

  # armv6m
  if(CONFIG_ARCH_ARMV6M)
    set(LLVM_CPUTYPE cortex-m0)
    set(LLVM_ARCHTYPE thumbv6m)
    set(LLVM_ABITYPE eabi)
  endif()

  set(RCFLAGS)
  set(WRC wamrc)

  if(CONFIG_ARCH_XTENSA)
    set(WTARGET "xtensa")
  elseif(CONFIG_ARCH_X86_64)
    set(WTARGET "x86_64")
  elseif(CONFIG_ARCH_X86)
    set(WTARGET "i386")
  elseif(CONFIG_ARCH_MIPS)
    set(WTARGET "mips")
  elseif(CONFIG_ARCH_SIM)
    list(APPEND RCFLAGS --disable-simd)
    if(CONFIG_SIM_M32)
      set(WTARGET "i386")
    else()
      set(WTARGET "x86_64")
    endif()
  elseif(LLVM_ARCHTYPE MATCHES "thumb")
    string(FIND "${LLVM_ARCHTYPE}" "+" PLUS_INDEX)
    if(PLUS_INDEX EQUAL -1)
      set(WTARGET "${LLVM_ARCHTYPE}")
    else()
      string(SUBSTRING "${LLVM_ARCHTYPE}" 0 ${PLUS_INDEX} WTARGET)
    endif()
  else()
    set(WTARGET ${LLVM_ARCHTYPE})
  endif()

  set(WCPU ${LLVM_CPUTYPE})

  if("${LLVM_ABITYPE}" STREQUAL "eabihf")
    set(WABITYPE "gnueabihf")
  else()
    set(WABITYPE "${LLVM_ABITYPE}")
  endif()

  list(APPEND RCFLAGS --target=${WTARGET})
  list(APPEND RCFLAGS --cpu=${WCPU})
  list(APPEND RCFLAGS --target-abi=${WABITYPE})

  if(CONFIG_INTERPRETERS_WAMR_AOT)
    if("${APP_WAMR_MODE}" STREQUAL "AOT")
      # generate AoT
      add_custom_target(
        ${APP_NAME}_AOT ALL
        COMMAND ${WRC} ${RCFLAGS} -o ${APP_NAME}.aot ${APP_NAME}.wasm
        DEPENDS ${APP_NAME}_OPT
        COMMENT "Wamrc Generate AoT: ${APP_NAME}.aot")
      set(APP_INSTALL_BIN ${APP_NAME}.aot)
      if(NOT APP_INSTALL_NAME)
        set(APP_INSTALL_NAME ${APP_NAME}.aot)
      endif()
    elseif("${APP_WAMR_MODE}" STREQUAL "XIP")
      # generate XIP
      add_custom_target(
        ${APP_NAME}_AOT ALL
        COMMAND ${WRC} ${RCFLAGS} --enable-indirect-mode
                --disable-llvm-intrinsics -o ${APP_NAME}.xip ${APP_NAME}.wasm
        DEPENDS ${APP_NAME}_OPT
        COMMENT "Wamrc Generate XIP: ${APP_NAME}.xip")
      set(APP_INSTALL_BIN ${APP_NAME}.xip)
      if(NOT APP_INSTALL_NAME)
        set(APP_INSTALL_NAME ${APP_NAME}.xip)
      endif()
    endif()
  endif()

  if(NOT APP_INSTALL_BIN)
    set(APP_INSTALL_BIN ${APP_NAME}.wasm)
  endif()
  if(NOT APP_INSTALL_NAME)
    set(APP_INSTALL_NAME ${APP_NAME}.wasm)
  endif()
  # install WASM BIN
  add_custom_target(
    ${APP_NAME}_INSTALL ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${APP_INSTALL_BIN}
            ${TOPBINDIR}/wasm/${APP_INSTALL_NAME}
    DEPENDS ${APP_NAME}_OPT
    COMMENT "Install WASM BIN: ${APP_INSTALL_NAME}")
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
  set(APP_WCFLAGS "")
  set(APP_WINCLUDES "")

  cmake_parse_arguments(LIB "" "NAME" "SRCS;WCFLAGS;WINCLUDES" ${ARGN})

  # Check if the LIB_NAME (NAME) is provided
  if(NOT LIB_NAME)
    message(FATAL_ERROR "NAME is not provided.")
  endif()

  # Check if the LIB_NAME (NAME) is already declared If it is, then skip the
  # rest of the function
  if(TARGET ${LIB_NAME})
    message(STATUS "Target ${LIB_NAME} already declared.")
    return()
  endif()

  # Check if the LIB_SRCS (SRCS) is provided
  if(NOT LIB_SRCS)
    message(FATAL_ERROR "SRCS is not provided.")
  endif()

  # Create the static library target for the library
  add_library(${LIB_NAME} STATIC ${LIB_SRCS})

  target_include_directories(${LIB_NAME} PRIVATE ${LIB_WINCLUDES})
  target_compile_options(${LIB_NAME} PRIVATE ${LIB_WCFLAGS})

  add_dependencies(wasm_interface ${LIB_NAME})
  target_link_libraries(wasm_interface INTERFACE ${LIB_NAME})
  # Set the target properties
  set_target_properties(${LIB_NAME} PROPERTIES OUTPUT_NAME ${LIB_NAME})

endfunction()
