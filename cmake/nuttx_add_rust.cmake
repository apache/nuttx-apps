# ##############################################################################
# cmake/nuttx_add_rust.cmake
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

include(nuttx_parse_function_args)

# ~~~
# Convert architecture type to Rust NuttX target
#
# Supported architectures:
#   - armv7a: armv7a-nuttx-eabi, armv7a-nuttx-eabihf
#   - thumbv6m: thumbv6m-nuttx-eabi
#   - thumbv7a: thumbv7a-nuttx-eabi, thumbv7a-nuttx-eabihf
#   - thumbv7m: thumbv7m-nuttx-eabi
#   - thumbv7em: thumbv7em-nuttx-eabihf
#   - thumbv8m.main: thumbv8m.main-nuttx-eabi, thumbv8m.main-nuttx-eabihf
#   - thumbv8m.base: thumbv8m.base-nuttx-eabi, thumbv8m.base-nuttx-eabihf
#   - riscv32: riscv32imc/imac/imafc-unknown-nuttx-elf
#   - riscv64: riscv64imac/imafdc-unknown-nuttx-elf
#   - x86: i686-unknown-nuttx
#   - x86_64: x86_64-unknown-nuttx
#
# Inputs:
#   ARCHTYPE - Architecture type (e.g. thumbv7m, riscv32)
#   ABITYPE  - ABI type (e.g. eabi, eabihf)
#   CPUTYPE  - CPU type (e.g. cortex-m4, sifive-e20)
#
# Output:
#   OUTPUT   - Rust target triple (e.g. riscv32imac-unknown-nuttx-elf,
#             thumbv7m-nuttx-eabi, thumbv7em-nuttx-eabihf)
# ~~~

function(nuttx_rust_target_triple ARCHTYPE ABITYPE CPUTYPE OUTPUT)
  if(ARCHTYPE STREQUAL "x86_64")
    set(TARGET_TRIPLE "x86_64-unknown-nuttx")
  elseif(ARCHTYPE STREQUAL "x86")
    set(TARGET_TRIPLE "i686-unknown-nuttx")
  elseif(ARCHTYPE MATCHES "thumb")
    if(ARCHTYPE MATCHES "thumbv8m")
      # Extract just the base architecture type (thumbv8m.main or thumbv8m.base)
      if(ARCHTYPE MATCHES "thumbv8m.main")
        set(ARCH_BASE "thumbv8m.main")
      elseif(ARCHTYPE MATCHES "thumbv8m.base")
        set(ARCH_BASE "thumbv8m.base")
      else()
        # Otherwise determine if we should use thumbv8m.main or thumbv8m.base
        # based on CPU type
        if(CPUTYPE MATCHES "cortex-m23")
          set(ARCH_BASE "thumbv8m.base")
        else()
          set(ARCH_BASE "thumbv8m.main")
        endif()
      endif()
      set(TARGET_TRIPLE "${ARCH_BASE}-nuttx-${ABITYPE}")
    else()
      set(TARGET_TRIPLE "${ARCHTYPE}-nuttx-${ABITYPE}")
    endif()
  elseif(ARCHTYPE STREQUAL "riscv32")
    if(CPUTYPE STREQUAL "sifive-e20")
      set(TARGET_TRIPLE "riscv32imc-unknown-nuttx-elf")
    elseif(CPUTYPE STREQUAL "sifive-e31")
      set(TARGET_TRIPLE "riscv32imac-unknown-nuttx-elf")
    elseif(CPUTYPE STREQUAL "sifive-e76")
      set(TARGET_TRIPLE "riscv32imafc-unknown-nuttx-elf")
    else()
      set(TARGET_TRIPLE "riscv32imc-unknown-nuttx-elf")
    endif()
  elseif(ARCHTYPE STREQUAL "riscv64")
    if(CPUTYPE STREQUAL "sifive-s51")
      set(TARGET_TRIPLE "riscv64imac-unknown-nuttx-elf")
    elseif(CPUTYPE STREQUAL "sifive-u54")
      set(TARGET_TRIPLE "riscv64imafdc-unknown-nuttx-elf")
    else()
      set(TARGET_TRIPLE "riscv64imac-unknown-nuttx-elf")
    endif()
  endif()
  set(${OUTPUT}
      ${TARGET_TRIPLE}
      PARENT_SCOPE)
endfunction()

# ~~~
# nuttx_add_rust
#
# Description:
#   Build a Rust crate and add it as a static library to the NuttX build system
#
# Example:
#  nuttx_add_rust(
#    CRATE_NAME
#    hello
#    CRATE_PATH
#    ${CMAKE_CURRENT_SOURCE_DIR}/hello
#  )
# ~~~

function(nuttx_add_rust)

  # parse arguments into variables
  nuttx_parse_function_args(
    FUNC
    nuttx_add_rust
    ONE_VALUE
    CRATE_NAME
    CRATE_PATH
    REQUIRED
    CRATE_NAME
    CRATE_PATH
    ARGN
    ${ARGN})

  # Determine build profile based on CONFIG_DEBUG_FULLOPT
  if(CONFIG_DEBUG_FULLOPT)
    set(RUST_PROFILE "release")
    set(RUST_DEBUG_FLAGS "-Zbuild-std-features=panic_immediate_abort")
  else()
    set(RUST_PROFILE "debug")
    set(RUST_DEBUG_FLAGS "")
  endif()

  # Get the Rust target triple
  nuttx_rust_target_triple(${LLVM_ARCHTYPE} ${LLVM_ABITYPE} ${LLVM_CPUTYPE}
                           RUST_TARGET)

  # Set up build directory in current binary dir
  set(RUST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CRATE_NAME})
  set(RUST_LIB_PATH
      ${RUST_BUILD_DIR}/${RUST_TARGET}/${RUST_PROFILE}/lib${CRATE_NAME}.a)

  # Create build directory
  file(MAKE_DIRECTORY ${RUST_BUILD_DIR})

  # Add a custom command to build the Rust crate
  add_custom_command(
    OUTPUT ${RUST_LIB_PATH}
    COMMAND
      ${CMAKE_COMMAND} -E env
      NUTTX_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/include:${CMAKE_BINARY_DIR}/include:${CMAKE_BINARY_DIR}/include/arch
      cargo build --${RUST_PROFILE} -Zbuild-std=std,panic_abort
      ${RUST_DEBUG_FLAGS} --manifest-path ${CRATE_PATH}/Cargo.toml --target
      ${RUST_TARGET} --target-dir ${RUST_BUILD_DIR}
    COMMENT "Building Rust crate ${CRATE_NAME}"
    VERBATIM)

  # Add a custom target that depends on the built library
  add_custom_target(${CRATE_NAME}_build ALL DEPENDS ${RUST_LIB_PATH})

  # Add imported library target
  add_library(${CRATE_NAME} STATIC IMPORTED GLOBAL)
  set_target_properties(${CRATE_NAME} PROPERTIES IMPORTED_LOCATION
                                                 ${RUST_LIB_PATH})

  # Add the Rust library to NuttX build
  nuttx_add_extra_library(${RUST_LIB_PATH})

  # Ensure the Rust library is built before linking
  add_dependencies(${CRATE_NAME} ${CRATE_NAME}_build)

endfunction()
