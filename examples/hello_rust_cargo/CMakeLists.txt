# ##############################################################################
# apps/examples/hello_rust_cargo/CMakeLists.txt
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

include(nuttx_add_library)

if(CONFIG_EXAMPLES_HELLO_RUST_CARGO)

# Call cargo build from CMakeLists and add it to the build system.
# Notice we should call cargo build with add_custom_target, otherwise
# cargo will be called every time when cmake is configured.

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/hello_rust_cargo/riscv32imac-unknown-nuttx-elf/release/libhello.a
  COMMAND cargo build --release
    -Zbuild-std=core
    --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/hello/Cargo.toml
    --target riscv32imac-unknown-nuttx-elf
    --target-dir ${CMAKE_BINARY_DIR}/hello_rust_cargo)

add_custom_target(hello_rust_cargo
  DEPENDS ${CMAKE_BINARY_DIR}/hello_rust_cargo/riscv32imac-unknown-nuttx-elf/release/libhello.a)

# Call cargo_build target each time when cmake is configured or generated.

# Add static library to the build system.
nuttx_library_import(rust_hello ${CMAKE_BINARY_DIR}/hello_rust_cargo/riscv32imac-unknown-nuttx-elf/release/libhello.a)
nuttx_add_extra_library(rust_hello)

nuttx_add_application(
  NAME
  ${CONFIG_EXAMPLES_HELLO_RUST_CARGO_PROGNAME}
  SRCS
  proxy_main.c
  STACKSIZE
  ${CONFIG_EXAMPLES_HELLO_STACKSIZE}
  PRIORITY
  ${CONFIG_EXAMPLES_HELLO_PRIORITY})

endif() # CONFIG_EXAMPLES_HELLO_RUST_CARGO
