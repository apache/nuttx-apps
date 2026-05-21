############################################################################
# apps/system/libffi/Makefile
#
# SPDX-License-Identifier: Apache-2.0
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

include $(APPDIR)/Make.defs

LIBFFI_URL        = https://github.com/libffi/libffi/archive
LIBFFI_COMMIT     = $(patsubst "%",%,$(strip $(CONFIG_LIB_LIBFFI_VERSION)))
LIBFFI_ZIP        = libffi-$(LIBFFI_COMMIT).zip
LIBFFI_SRC        = libffi
LIBFFI_BUILDDIR   = $(CURDIR)/build
LIBFFI_INSTALLDIR = $(CURDIR)/install
APPS_INCDIR       = $(APPDIR)$(DELIM)include

UNPACK ?= unzip -q -o

# ---------------------------------------------------------------------------
# Derive the autotools --host triplet from the NuttX configuration.
#
# libffi's configure.host matches on patterns such as:
#   riscv*-*-*   arm*-*-*   aarch64*-*-*   xtensa*-*-*
#   mips*-*-*    i?86-*-*   x86_64-*-*
#
# We map CONFIG_ARCH (and, for RISC-V, the word-size selectors) to the
# CPU part that libffi expects, then append "-nuttx-elf" as vendor-os.
# ---------------------------------------------------------------------------

ifeq ($(CONFIG_ARCH),risc-v)
  ifeq ($(CONFIG_ARCH_RV64),y)
    LIBFFI_CPU = riscv64
  else
    LIBFFI_CPU = riscv32
  endif
else ifeq ($(CONFIG_ARCH),arm64)
  LIBFFI_CPU = aarch64
else ifeq ($(CONFIG_ARCH),arm)
  LIBFFI_CPU = arm
else ifeq ($(CONFIG_ARCH),xtensa)
  LIBFFI_CPU = xtensa
else ifeq ($(CONFIG_ARCH),mips)
  LIBFFI_CPU = mips
else ifeq ($(CONFIG_ARCH),x86_64)
  LIBFFI_CPU = x86_64
else ifeq ($(CONFIG_ARCH),x86)
  LIBFFI_CPU = i686
else
  LIBFFI_CPU = $(subst -,,$(CONFIG_ARCH))
endif

LIBFFI_HOST = $(LIBFFI_CPU)-nuttx-elf

# ---------------------------------------------------------------------------
# CFLAGS for the libffi build itself.
#
# We start from the NuttX-provided CFLAGS (which already carry -march,
# -mabi, -isystem paths, -D__NuttX__, etc.) and add a few flags that the
# library needs.  We deliberately strip -nostdlib because autoconf link
# checks need the default startfiles/libs to succeed.
# ---------------------------------------------------------------------------

LIBFFI_CFLAGS = $(filter-out -nostdlib -Werror -Werror=% -Wno-cpp,$(CFLAGS))
LIBFFI_CFLAGS += -ffunction-sections -fdata-sections -O2

# ---------------------------------------------------------------------------
# Download, autoreconf, configure, build, install
# ---------------------------------------------------------------------------

ifeq ($(wildcard $(LIBFFI_SRC)/.git),)

$(LIBFFI_ZIP):
	@echo "Downloading: $(LIBFFI_URL)/$(LIBFFI_COMMIT).zip"
	$(Q) curl -L $(LIBFFI_URL)/$(LIBFFI_COMMIT).zip -o $(LIBFFI_ZIP)

$(LIBFFI_SRC): $(LIBFFI_ZIP)
	@echo "Unpacking: $(LIBFFI_ZIP) -> $(LIBFFI_SRC)"
	$(Q) $(UNPACK) $(LIBFFI_ZIP)
	$(Q) mv libffi-$(LIBFFI_COMMIT) $(LIBFFI_SRC)

endif

$(LIBFFI_SRC)/configure: $(LIBFFI_SRC)
	@echo "Generating libffi configure script (autoreconf)"
	$(Q) (cd $(LIBFFI_SRC) && autoreconf -v -i)

$(LIBFFI_INSTALLDIR)/lib/libffi.a: $(LIBFFI_SRC)/configure
	$(Q) mkdir -p $(LIBFFI_BUILDDIR)
	$(Q) ( \
		cd $(LIBFFI_BUILDDIR) && \
		$(CURDIR)/$(LIBFFI_SRC)/configure \
		  --host=$(LIBFFI_HOST) \
		  --prefix=$(LIBFFI_INSTALLDIR) \
		  --disable-shared \
		  --enable-static \
		  --disable-docs \
		  --disable-exec-static-tramp \
		  --disable-multi-os-directory \
		  CC="$(CC)" \
		  CFLAGS="$(LIBFFI_CFLAGS)" \
		  AR="$(CROSSDEV)ar" \
		  RANLIB="$(CROSSDEV)ranlib" \
	)
	$(MAKE) -C $(LIBFFI_BUILDDIR) -j$(shell nproc)
	$(MAKE) -C $(LIBFFI_BUILDDIR) install

context:: $(LIBFFI_INSTALLDIR)/lib/libffi.a
	$(Q) mkdir -p $(APPS_INCDIR)
	$(Q) cp $(LIBFFI_INSTALLDIR)/include/*.h $(APPS_INCDIR)

distclean::
	$(call DELDIR, $(LIBFFI_BUILDDIR))
	$(call DELDIR, $(LIBFFI_INSTALLDIR))
	$(call DELDIR, $(LIBFFI_SRC))
	$(call DELFILE, $(LIBFFI_ZIP))
	$(call DELFILE, $(APPS_INCDIR)/ffi.h)
	$(call DELFILE, $(APPS_INCDIR)/ffitarget.h)

include $(APPDIR)/Application.mk
