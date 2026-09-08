############################################################################
# apps/examples/fdpicxip/modules/Makefile
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

# The FDPIC modules that examples/fdpicxip and testing/fs/xipfs carry inside
# their images, and the sources they are built from.
#
# This is NOT part of the application build.  Nothing here runs unless it is
# asked for: the *_bin.h headers are committed, so both apps build with a
# plain toolchain and CI covers them, while the link needs
# arm-uclinuxfdpiceabi binutils, which the tree does not require.  See
# Documentation/components/fdpic.rst.
#
# To rebuild the headers from these sources:
#
#   make -C apps/examples/fdpicxip/modules regen NUTTX_DIR=/path/to/nuttx
#
# NUTTX_DIR must be a configured, built tree with CONFIG_FDPIC and a system
# symbol table: the compile needs its headers and the verify step needs
# libs/libc/exec_symtab.c.
#
# CPU is cortex-m3 rather than the toolchain default, deliberately.  A v7-M
# module runs on both the v7-M and v8-M targets, so one set of blobs serves
# the RP2350 (Cortex-M33) and mps2-an500 (Cortex-M7) alike.  Building for
# cortex-m33 produces blobs the M7 cannot execute at all.

CPU ?= cortex-m3

# make has built-in defaults for CC and CXX, so ?= never fires for them and
# the host compiler silently gets the job.  Test the origin instead.

ifeq ($(origin CC),default)
  CC := arm-none-eabi-gcc
endif

ifeq ($(origin CXX),default)
  CXX := arm-none-eabi-g++
endif

ifeq ($(origin LD),default)
  LD := arm-uclinuxfdpiceabi-ld
endif

# -mfdpic -fPIC is the whole of what makes an FDPIC object; the rest is what
# a module needs anywhere.  -fno-use-cxa-atexit puts a static object's
# destructor in .fini_array, which is where the loader looks for it.

MODCFLAGS   = -mcpu=$(CPU) -mthumb -mfdpic -fPIC -Os -fno-builtin -Wall \
              -Wa,--noexecstack -D__STDC_NO_ATOMICS__ -D__NuttX__ \
              -I$(NUTTX_DIR)/include
MODCXXFLAGS = $(MODCFLAGS) -fno-exceptions -fno-rtti -fno-use-cxa-atexit \
              -I$(NUTTX_DIR)/include/cxx

# The crt0 and the linker script are the tree's own, the ones the in-tree
# module build uses.  crt0 is compiled here rather than taken built, because
# these are built for a different CPU from the firmware.

MODLDFLAGS  = -m armelf_linux_fdpiceabi -shared -z now \
              -T $(NUTTX_DIR)/libs/libc/elf/gnu-elf.ld

DEMODIR = ..
DEMOREL = apps/examples/fdpicxip
TESTDIR = ../../../testing/fs/xipfs
TESTREL = apps/testing/fs/xipfs

%.o: %.c
	$(CC) $(MODCFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(MODCXXFLAGS) -c $< -o $@

crt0.o: $(NUTTX_DIR)/arch/arm/src/common/crt0.c
	$(CC) $(MODCFLAGS) -c $< -o $@

# A module is entered at _start and carries crt0.  A library is never
# entered, so it has neither, and it is named by its SONAME, which is what a
# consumer records in DT_NEEDED and the loader searches for.

LINKMOD = $(LD) $(MODLDFLAGS) -e _start -o $@ crt0.o
LINKLIB = $(LD) $(MODLDFLAGS) -e 0 -soname $(@F) -o $@

# EMBED turns a built module into the committed header that carries it: $1
# the artifact, $2 the symbol, $3 the header's path from the repository root,
# which nxstyle wants on line 2.

define EMBED
	{ sed -e "s|@PATH@|$3|" header.template; \
	  echo "static const unsigned char $2[] ="; echo "{"; \
	  xxd -i < $1 | sed -e 's/^  /  /'; echo "};"; echo; \
	  echo "static const unsigned int $2_len = $$(wc -c < $1 | tr -d ' ');"; \
	} > $@
endef

# manyneeded needs one library per DT_NEEDED entry, one more than the loader
# will follow.  They exist only to make the linker record nine entries; the
# module is refused while its dynamic section is parsed, before any of them
# would be loaded, so none of them is embedded.

NEEDLIBS := $(foreach n,0 1 2 3 4 5 6 7 8,need$(n).so)

DEMO_HDRS = $(DEMODIR)/qsorter_bin.h    $(DEMODIR)/libcounter_bin.h \
            $(DEMODIR)/user_bin.h       $(DEMODIR)/libshape_bin.h \
            $(DEMODIR)/cxxuser_bin.h    $(DEMODIR)/lazymod_bin.h

TEST_HDRS = $(TESTDIR)/callback_bin.h   $(TESTDIR)/counteruser_bin.h \
            $(TESTDIR)/cxxuser_bin.h    $(TESTDIR)/funcdesc_bin.h \
            $(TESTDIR)/lazymod_bin.h    $(TESTDIR)/libcounter_bin.h \
            $(TESTDIR)/libshape_bin.h   $(TESTDIR)/manyneeded_bin.h \
            $(TESTDIR)/missingsym_bin.h

.PHONY: all regen check-nuttx-dir clean

all:
	@echo "Nothing to do: the *_bin.h headers are committed."
	@echo "To rebuild them:  make regen NUTTX_DIR=/path/to/nuttx"

regen: check-nuttx-dir $(DEMO_HDRS) $(TEST_HDRS)
	@echo "Regenerated the embedded modules for $(DEMOREL) and $(TESTREL)."

check-nuttx-dir:
ifeq ($(NUTTX_DIR),)
	$(error Set NUTTX_DIR to a configured, built NuttX tree)
endif

# The modules.  Libraries have no entry point and are named by their SONAME,
# which is what a consumer records in DT_NEEDED and what the loader searches
# for at run time.

qsorter.fdpic: qsorter.o crt0.o
	$(LINKMOD) qsorter.o

callback.fdpic: callback.o crt0.o
	$(LINKMOD) callback.o

funcdesc.fdpic: funcdesc.o crt0.o
	$(LINKMOD) funcdesc.o

libcounter.so: libcounter.o
	$(LINKLIB) libcounter.o

user.fdpic: user.o crt0.o libcounter.so
	$(LINKMOD) user.o libcounter.so

# C++ compiles with the stock arm-none-eabi-g++; only the link is FDPIC.

libshape.so: libshape.o
	$(LINKLIB) libshape.o

cxxuser.fdpic: cxxuser.o crt0.o libshape.so
	$(LINKMOD) cxxuser.o libshape.so

# BINDNOW is emptied so the imported descriptors stay in the lazy binding
# table, which is the case this module exists to cover.

lazymod.fdpic: lazymod.o crt0.o
	$(LD) $(filter-out -z now,$(MODLDFLAGS)) -e _start -o $@ crt0.o lazymod.o

# missingsym is linked with the bare .fdpic target rather than the default
# 'verify' one, because it is exactly what fdpic-verify.sh is meant to catch.

missingsym.fdpic: missingsym.o crt0.o
	$(LINKMOD) missingsym.o

need%.so:
	echo "int need_leaf_$*(void){return $*;}" > need$*.c
	$(CC) $(MODCFLAGS) -c need$*.c -o need$*.o
	$(LINKLIB) need$*.o

manyneeded.c: $(NEEDLIBS)
	{ for n in 0 1 2 3 4 5 6 7 8; do \
	    echo "extern int need_leaf_$$n(void);"; done; \
	  echo "int main(int argc, char *argv[]) {"; \
	  echo "  return need_leaf_0() + need_leaf_1() + need_leaf_2() +"; \
	  echo "         need_leaf_3() + need_leaf_4() + need_leaf_5() +"; \
	  echo "         need_leaf_6() + need_leaf_7() + need_leaf_8();"; \
	  echo "}"; } > manyneeded.c

manyneeded.fdpic: manyneeded.o crt0.o $(NEEDLIBS)
	$(LINKMOD) manyneeded.o $(NEEDLIBS)

# The headers.  One artifact can be embedded under more than one name: the
# demo's user_bin.h and the suite's counteruser_bin.h are the same module.

$(DEMODIR)/qsorter_bin.h: qsorter.fdpic
	$(call EMBED,$<,g_qsorter_nxf,$(DEMOREL)/$(@F))

$(DEMODIR)/libcounter_bin.h: libcounter.so
	$(call EMBED,$<,g_libcounter,$(DEMOREL)/$(@F))

$(DEMODIR)/user_bin.h: user.fdpic
	$(call EMBED,$<,g_user,$(DEMOREL)/$(@F))

$(DEMODIR)/libshape_bin.h: libshape.so
	$(call EMBED,$<,g_libshape,$(DEMOREL)/$(@F))

$(DEMODIR)/cxxuser_bin.h: cxxuser.fdpic
	$(call EMBED,$<,g_cxxuser,$(DEMOREL)/$(@F))

$(DEMODIR)/lazymod_bin.h: lazymod.fdpic
	$(call EMBED,$<,g_lazymod,$(DEMOREL)/$(@F))

$(TESTDIR)/callback_bin.h: callback.fdpic
	$(call EMBED,$<,g_callback,$(TESTREL)/$(@F))

$(TESTDIR)/counteruser_bin.h: user.fdpic
	$(call EMBED,$<,g_counteruser,$(TESTREL)/$(@F))

$(TESTDIR)/cxxuser_bin.h: cxxuser.fdpic
	$(call EMBED,$<,g_cxxuser,$(TESTREL)/$(@F))

$(TESTDIR)/funcdesc_bin.h: funcdesc.fdpic
	$(call EMBED,$<,g_funcdesc,$(TESTREL)/$(@F))

$(TESTDIR)/lazymod_bin.h: lazymod.fdpic
	$(call EMBED,$<,g_lazymod,$(TESTREL)/$(@F))

$(TESTDIR)/libcounter_bin.h: libcounter.so
	$(call EMBED,$<,g_libcounter,$(TESTREL)/$(@F))

$(TESTDIR)/libshape_bin.h: libshape.so
	$(call EMBED,$<,g_libshape,$(TESTREL)/$(@F))

$(TESTDIR)/manyneeded_bin.h: manyneeded.fdpic
	$(call EMBED,$<,g_manyneeded,$(TESTREL)/$(@F))

$(TESTDIR)/missingsym_bin.h: missingsym.fdpic
	$(call EMBED,$<,g_missingsym,$(TESTREL)/$(@F))

# No NUTTX_DIR needed to clean.

clean:
	$(Q) rm -f *.o *.fdpic *.so .nuttx-exports need*.c manyneeded.c
