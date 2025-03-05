############################################################################
# apps/tools/Rust.mk
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

# Generate Rust target triple based on LLVM architecture configuration
#
# Uses the following LLVM variables directly:
#   - LLVM_ARCHTYPE: Architecture type (e.g. thumbv7m, riscv32)
#   - LLVM_ABITYPE: ABI type (e.g. eabi, eabihf)
#   - LLVM_CPUTYPE: CPU type (e.g. cortex-m23, sifive-e20)
#
# Supported architectures and their target triples:
#   - x86: i686-unknown-nuttx
#   - x86_64: x86_64-unknown-nuttx
#   - armv7a: armv7a-nuttx-eabi, armv7a-nuttx-eabihf
#   - thumbv6m: thumbv6m-nuttx-eabi
#   - thumbv7a: thumbv7a-nuttx-eabi, thumbv7a-nuttx-eabihf
#   - thumbv7m: thumbv7m-nuttx-eabi
#   - thumbv7em: thumbv7em-nuttx-eabihf
#   - thumbv8m.main: thumbv8m.main-nuttx-eabi, thumbv8m.main-nuttx-eabihf
#   - thumbv8m.base: thumbv8m.base-nuttx-eabi, thumbv8m.base-nuttx-eabihf
#   - riscv32: riscv32imc/imac/imafc-unknown-nuttx-elf
#   - riscv64: riscv64imac/imafdc-unknown-nuttx-elf
#
# Usage:   $(call RUST_TARGET_TRIPLE)
#
# Output:
#   Rust target triple (e.g. riscv32imac-unknown-nuttx-elf,
#   thumbv7m-nuttx-eabi, thumbv7em-nuttx-eabihf)

define RUST_TARGET_TRIPLE
$(or \
  $(and $(filter x86_64,$(LLVM_ARCHTYPE)), \
    x86_64-unknown-nuttx \
  ), \
  $(and $(filter x86,$(LLVM_ARCHTYPE)), \
    i686-unknown-nuttx \
  ), \
  $(and $(filter thumb%,$(LLVM_ARCHTYPE)), \
    $(if $(filter thumbv8m%,$(LLVM_ARCHTYPE)), \
      $(if $(filter cortex-m23,$(LLVM_CPUTYPE)),thumbv8m.base,thumbv8m.main)-nuttx-$(LLVM_ABITYPE), \
      $(LLVM_ARCHTYPE)-nuttx-$(LLVM_ABITYPE) \
    ) \
  ), \
  $(and $(filter riscv32,$(LLVM_ARCHTYPE)), \
    riscv32$(or \
      $(and $(filter sifive-e20,$(LLVM_CPUTYPE)),imc), \
      $(and $(filter sifive-e31,$(LLVM_CPUTYPE)),imac), \
      $(and $(filter sifive-e76,$(LLVM_CPUTYPE)),imafc), \
      imc \
    )-unknown-nuttx-elf \
  ), \
  $(and $(filter riscv64,$(LLVM_ARCHTYPE)), \
    riscv64$(or \
      $(and $(filter sifive-s51,$(LLVM_CPUTYPE)),imac), \
      $(and $(filter sifive-u54,$(LLVM_CPUTYPE)),imafdc), \
      imac \
    )-unknown-nuttx-elf \
  ) \
)
endef

# Build Rust project using cargo
#
# Usage:   $(call RUST_CARGO_BUILD,cratename,prefix)
#
# Inputs:
#   cratename - Name of the Rust crate (e.g. hello)
#   prefix    - Path prefix to the crate (e.g. path/to/project)
#
# Output:
#   None, builds the Rust project

ifeq ($(CONFIG_DEBUG_FULLOPT),y)
define RUST_CARGO_BUILD
	NUTTX_INCLUDE_DIR=$(TOPDIR)/include:$(TOPDIR)/include/arch \
    cargo build --release -Zbuild-std=std,panic_abort \
    -Zbuild-std-features=panic_immediate_abort \
		--manifest-path $(2)/$(1)/Cargo.toml \
		--target $(call RUST_TARGET_TRIPLE)
endef
else
define RUST_CARGO_BUILD
	@echo "Building Rust code with cargo..."
	NUTTX_INCLUDE_DIR=$(TOPDIR)/include:$(TOPDIR)/include/arch \
    cargo build -Zbuild-std=std,panic_abort \
		--manifest-path $(2)/$(1)/Cargo.toml \
		--target $(call RUST_TARGET_TRIPLE)
endef
endif

# Clean Rust project using cargo
#
# Usage:   $(call RUST_CARGO_CLEAN,cratename,prefix)
#
# Inputs:
#   cratename - Name of the Rust crate (e.g. hello)
#   prefix    - Path prefix to the crate (e.g. path/to/project)
#
# Output:
#   None, cleans the Rust project

define RUST_CARGO_CLEAN
	cargo clean --manifest-path $(2)/$(1)/Cargo.toml
endef

# Get Rust binary path for given crate and path prefix
#
# Usage:   $(call RUST_GET_BINDIR,cratename,prefix)
#
# Inputs:
#   cratename - Name of the Rust crate (e.g. hello)
#   prefix    - Path prefix to the crate (e.g. path/to/project)
#
# Output:
#   Path to the Rust binary (e.g. path/to/project/target/riscv32imac-unknown-nuttx-elf/release/libhello.a)

define RUST_GET_BINDIR
$(2)/$(1)/target/$(strip $(call RUST_TARGET_TRIPLE))/$(if $(CONFIG_DEBUG_FULLOPT),release,debug)/lib$(1).a
endef
