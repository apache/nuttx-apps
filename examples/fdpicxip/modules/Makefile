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
# nuttx/tools/fdpic/README.md and Documentation/components/fdpic.rst.
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

FDPICDIR  = $(NUTTX_DIR)/tools/fdpic
MODULE_MK = $(FDPICDIR)/nuttx-fdpic.mk
EMBED     = $(FDPICDIR)/fdpic-embed.py

# Where each generated header goes, and its path from the repository root --
# fdpic-embed.py puts that on line 2, which is what nxstyle wants.

DEMODIR = ..
DEMOREL = apps/examples/fdpicxip
TESTDIR = ../../../testing/fs/xipfs
TESTREL = apps/testing/fs/xipfs

BUILD = $(MAKE) -f $(MODULE_MK) NUTTX_DIR=$(NUTTX_DIR) CPU=$(CPU)

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

qsorter.fdpic: qsorter.c
	$(BUILD) MODULE=qsorter SRCS=qsorter.c

callback.fdpic: callback.c
	$(BUILD) MODULE=callback SRCS=callback.c

funcdesc.fdpic: funcdesc.c
	$(BUILD) MODULE=funcdesc SRCS=funcdesc.c

libcounter.so: libcounter.c
	$(BUILD) MODULE=libcounter SRCS=libcounter.c ENTRY=0 \
	    EXTRA_LDFLAGS="-soname libcounter.so"
	mv libcounter.fdpic libcounter.so

user.fdpic: user.c libcounter.so
	$(BUILD) MODULE=user SRCS=user.c LIBS=libcounter.so

# C++ compiles with the stock arm-none-eabi-g++; only the link is FDPIC.

libshape.so: libshape.cpp
	$(BUILD) MODULE=libshape CXXSRCS=libshape.cpp ENTRY=0 \
	    EXTRA_LDFLAGS="-soname libshape.so"
	mv libshape.fdpic libshape.so

cxxuser.fdpic: cxxuser.cpp libshape.so
	$(BUILD) MODULE=cxxuser CXXSRCS=cxxuser.cpp LIBS=libshape.so

# BINDNOW is emptied so the imported descriptors stay in the lazy binding
# table, which is the case this module exists to cover.

lazymod.fdpic: lazymod.c
	$(BUILD) MODULE=lazymod SRCS=lazymod.c BINDNOW=

# missingsym is linked with the bare .fdpic target rather than the default
# 'verify' one, because it is exactly what fdpic-verify.sh is meant to catch.

missingsym.fdpic: missingsym.c
	$(BUILD) MODULE=missingsym SRCS=missingsym.c missingsym.fdpic

need%.so:
	echo "int need_leaf_$*(void){return $*;}" > need$*.c
	$(BUILD) MODULE=need$* SRCS=need$*.c ENTRY=0 \
	    EXTRA_LDFLAGS="-soname need$*.so"
	mv need$*.fdpic need$*.so

manyneeded.c: $(NEEDLIBS)
	{ for n in 0 1 2 3 4 5 6 7 8; do \
	    echo "extern int need_leaf_$$n(void);"; done; \
	  echo "int main(int argc, char *argv[]) {"; \
	  echo "  return need_leaf_0() + need_leaf_1() + need_leaf_2() +"; \
	  echo "         need_leaf_3() + need_leaf_4() + need_leaf_5() +"; \
	  echo "         need_leaf_6() + need_leaf_7() + need_leaf_8();"; \
	  echo "}"; } > manyneeded.c

manyneeded.fdpic: manyneeded.c $(NEEDLIBS)
	$(BUILD) MODULE=manyneeded SRCS=manyneeded.c LIBS="$(NEEDLIBS)" \
	    manyneeded.fdpic

# The headers.  One artifact can be embedded under more than one name: the
# demo's user_bin.h and the suite's counteruser_bin.h are the same module.

$(DEMODIR)/qsorter_bin.h: qsorter.fdpic
	$(EMBED) $< g_qsorter_nxf $(DEMOREL)/$(@F) > $@

$(DEMODIR)/libcounter_bin.h: libcounter.so
	$(EMBED) $< g_libcounter $(DEMOREL)/$(@F) > $@

$(DEMODIR)/user_bin.h: user.fdpic
	$(EMBED) $< g_user $(DEMOREL)/$(@F) > $@

$(DEMODIR)/libshape_bin.h: libshape.so
	$(EMBED) $< g_libshape $(DEMOREL)/$(@F) > $@

$(DEMODIR)/cxxuser_bin.h: cxxuser.fdpic
	$(EMBED) $< g_cxxuser $(DEMOREL)/$(@F) > $@

$(DEMODIR)/lazymod_bin.h: lazymod.fdpic
	$(EMBED) $< g_lazymod $(DEMOREL)/$(@F) > $@

$(TESTDIR)/callback_bin.h: callback.fdpic
	$(EMBED) $< g_callback $(TESTREL)/$(@F) > $@

$(TESTDIR)/counteruser_bin.h: user.fdpic
	$(EMBED) $< g_counteruser $(TESTREL)/$(@F) > $@

$(TESTDIR)/cxxuser_bin.h: cxxuser.fdpic
	$(EMBED) $< g_cxxuser $(TESTREL)/$(@F) > $@

$(TESTDIR)/funcdesc_bin.h: funcdesc.fdpic
	$(EMBED) $< g_funcdesc $(TESTREL)/$(@F) > $@

$(TESTDIR)/lazymod_bin.h: lazymod.fdpic
	$(EMBED) $< g_lazymod $(TESTREL)/$(@F) > $@

$(TESTDIR)/libcounter_bin.h: libcounter.so
	$(EMBED) $< g_libcounter $(TESTREL)/$(@F) > $@

$(TESTDIR)/libshape_bin.h: libshape.so
	$(EMBED) $< g_libshape $(TESTREL)/$(@F) > $@

$(TESTDIR)/manyneeded_bin.h: manyneeded.fdpic
	$(EMBED) $< g_manyneeded $(TESTREL)/$(@F) > $@

$(TESTDIR)/missingsym_bin.h: missingsym.fdpic
	$(EMBED) $< g_missingsym $(TESTREL)/$(@F) > $@

# No NUTTX_DIR needed to clean.

clean:
	$(Q) rm -f *.o *.fdpic *.so .nuttx-exports need*.c manyneeded.c
