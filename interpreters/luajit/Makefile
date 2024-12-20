############################################################################
# apps/interpreters/luajit/Makefile
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

LUAJIT_PATCHS ?= $(sort $(wildcard 000*.patch))

LUAJIT_VERSION := $(patsubst "%",%,$(CONFIG_LUAJIT_VERSION))
LUAJIT_UNPACK   = LuaJIT
LUAJIT_TARBALL  = $(LUAJIT_VERSION).tar.gz
LUAJIT_URL_BASE = https://github.com/LuaJIT/LuaJIT/archive
LUAJIT_URL      = $(LUAJIT_URL_BASE)/$(LUAJIT_TARBALL)

MAINSRC = luajit.c

CSRCS += lib_aux.c
CSRCS += lib_base.c
CSRCS += lib_bit.c
CSRCS += lib_buffer.c
CSRCS += lib_debug.c
CSRCS += lib_ffi.c
CSRCS += lib_init.c
CSRCS += lib_io.c
CSRCS += lib_jit.c
CSRCS += lib_math.c
CSRCS += lib_os.c
CSRCS += lib_package.c
CSRCS += lib_string.c
CSRCS += lib_table.c
CSRCS += lj_alloc.c
CSRCS += lj_api.c
CSRCS += lj_asm.c
CSRCS += lj_assert.c
CSRCS += lj_bc.c
CSRCS += lj_bcread.c
CSRCS += lj_bcwrite.c
CSRCS += lj_buf.c
CSRCS += lj_carith.c
CSRCS += lj_ccall.c
CSRCS += lj_ccallback.c
CSRCS += lj_cconv.c
CSRCS += lj_cdata.c
CSRCS += lj_char.c
CSRCS += lj_clib.c
CSRCS += lj_cparse.c
CSRCS += lj_crecord.c
CSRCS += lj_ctype.c
CSRCS += lj_debug.c
CSRCS += lj_dispatch.c
CSRCS += lj_err.c
CSRCS += lj_ffrecord.c
CSRCS += lj_func.c
CSRCS += lj_gc.c
CSRCS += lj_gdbjit.c
CSRCS += lj_ir.c
CSRCS += lj_lex.c
CSRCS += lj_lib.c
CSRCS += lj_load.c
CSRCS += lj_mcode.c
CSRCS += lj_meta.c
CSRCS += lj_obj.c
CSRCS += lj_opt_dce.c
CSRCS += lj_opt_fold.c
CSRCS += lj_opt_loop.c
CSRCS += lj_opt_mem.c
CSRCS += lj_opt_narrow.c
CSRCS += lj_opt_sink.c
CSRCS += lj_opt_split.c
CSRCS += lj_parse.c
CSRCS += lj_prng.c
CSRCS += lj_profile.c
CSRCS += lj_record.c
CSRCS += lj_serialize.c
CSRCS += lj_snap.c
CSRCS += lj_state.c
CSRCS += lj_str.c
CSRCS += lj_strfmt.c
CSRCS += lj_strfmt_num.c
CSRCS += lj_strscan.c
CSRCS += lj_tab.c
CSRCS += lj_trace.c
CSRCS += lj_udata.c
CSRCS += lj_vmevent.c
CSRCS += lj_vmmath.c

ASRCS += lj_vm.S

VPATH += $(LUAJIT_UNPACK)/dynasm
VPATH += $(LUAJIT_UNPACK)/src
VPATH += $(LUAJIT_UNPACK)/src/host

CFLAGS += -fomit-frame-pointer
CFLAGS += -fno-short-enums
CFLAGS += -D__ARM_ARCH_7M__
CFLAGS += -DLUAJIT_DISABLE_PROFILE
CFLAGS += -DLUAJIT_NO_UNWIND
CFLAGS += -DLUAJIT_OS=LUAJIT_OS_NUTTX
CFLAGS += -DLUAJIT_SECURITY_MCODE=0
CFLAGS += -DLUAJIT_SECURITY_PRNG=0
CFLAGS += -DLUA_ROOT=CONFIG_LIBC_HOMEDIR

PROGNAME  = luajit
PRIORITY  = $(CONFIG_LUAJIT_PRIORITY)
STACKSIZE = $(CONFIG_LUAJIT_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_LUAJIT)

$(LUAJIT_TARBALL):
	$(call DOWNLOAD,$(LUAJIT_URL_BASE),$(LUAJIT_TARBALL),$(LUAJIT_TARBALL))

$(LUAJIT_UNPACK): $(LUAJIT_TARBALL)
	$(ECHO_BEGIN) "Unpacking $(LUAJIT_TARBALL) to $(LUAJIT_UNPACK)"
	$(Q) tar xzvf $(LUAJIT_TARBALL)
	$(Q) mv LuaJIT-$(LUAJIT_VERSION) $(LUAJIT_UNPACK)
	$(Q) cat $(LUAJIT_PATCHS) | \
		patch -s -N -d $(LUAJIT_UNPACK) -p1
	$(ECHO_END)

$(LUAJIT_UNPACK)/.patch: $(LUAJIT_UNPACK)
	$(Q) touch $(LUAJIT_UNPACK)/.patch

DASM_ARCH = armv7m

DASM_DASC = $(LUAJIT_UNPACK)/src/vm_armv7m.dasc

DASM_FLAGS += -D ENDIAN_LE
DASM_FLAGS += -D JIT
DASM_FLAGS += -D FFI
DASM_FLAGS += -D DUALNUM
DASM_FLAGS += -D FPU
DASM_FLAGS += -D HFABI
DASM_FLAGS += -D NO_UNWIND
DASM_FLAGS += -D VER=70

MINILUA_O = $(LUAJIT_UNPACK)/src/host/minilua.o

BUILDVM_O = $(LUAJIT_UNPACK)/src/host/buildvm.o \
            $(LUAJIT_UNPACK)/src/host/buildvm_asm.o \
            $(LUAJIT_UNPACK)/src/host/buildvm_fold.o \
            $(LUAJIT_UNPACK)/src/host/buildvm_lib.o \
            $(LUAJIT_UNPACK)/src/host/buildvm_peobj.o

HOSTCFLAGS += -D__ARM_ARCH_7M__=1
HOSTCFLAGS += -DLJ_ABI_SOFTFP=0
HOSTCFLAGS += -DLJ_ARCH_HASFPU=1
HOSTCFLAGS += -DLUAJIT_DISABLE_PROFILE
HOSTCFLAGS += -DLUAJIT_NO_UNWIND
HOSTCFLAGS += -DLUAJIT_TARGET=LUAJIT_ARCH_ARM
HOSTCFLAGS += -I$(LUAJIT_UNPACK)/src
HOSTCFLAGS += -I$(LUAJIT_UNPACK)/src/host

HOSTLDFLAGS += -lm

BUILDVM_BIN = $(LUAJIT_UNPACK)/src/host/buildvm

MINILUA_BIN = $(LUAJIT_UNPACK)/src/host/minilua

LJVM_S = $(LUAJIT_UNPACK)/src/lj_vm.S

LIB_VMDEF = $(LUAJIT_UNPACK)/src/jit/vmdef.lua

LIB_VMDEFP = $(LIB_VMDEF)

ALL_T = $(MINILUA_BIN)

ALL_HDRGEN = $(LUAJIT_UNPACK)/src/lj_bcdef.h
ALL_HDRGEN += $(LUAJIT_UNPACK)/src/lj_ffdef.h
ALL_HDRGEN += $(LUAJIT_UNPACK)/src/lj_folddef.h
ALL_HDRGEN += $(LUAJIT_UNPACK)/src/lj_libdef.h
ALL_HDRGEN += $(LUAJIT_UNPACK)/src/lj_recdef.h

ALL_GEN = $(LJVM_S) $(ALL_HDRGEN) $(LIB_VMDEFP)

HOST_OBJS = $(MINILUA_O) $(BUILDVM_O)

LJLIB_O = $(LUAJIT_UNPACK)/src/lib_base.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_bit.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_buffer.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_debug.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_ffi.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_io.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_jit.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_math.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_os.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_package.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_string.o
LJLIB_O += $(LUAJIT_UNPACK)/src/lib_table.o

LJLIB_C = $(LJLIB_O:.o=.c)

.NOTPARALLEL:

$(MINILUA_BIN): $(MINILUA_O)
	$(ECHO_BEGIN) "HOSTLINK $@"
	$(Q) $(HOSTCC) -m32 -o $@ $(MINILUA_O) $(HOSTLDFLAGS)
	$(ECHO_END)

$(LUAJIT_UNPACK)/src/host/buildvm_arch.h: $(DASM_DASC) $(MINILUA_BIN) $(LUAJIT_UNPACK)/src/lj_arch.h $(LUAJIT_UNPACK)/src/lua.h $(LUAJIT_UNPACK)/src/luaconf.h
	$(ECHO_BEGIN) "DYNASM $@"
	$(Q)$(MINILUA_BIN) $(LUAJIT_UNPACK)/dynasm/dynasm.lua $(DASM_FLAGS) -o $@ $(DASM_DASC)
	$(ECHO_END)

$(BUILDVM_BIN): $(BUILDVM_O)
	$(ECHO_BEGIN) "HOSTLINK $@"
	$(Q)$(HOSTCC) -m32 -o $@ $(BUILDVM_O)
	$(ECHO_END)

$(LJVM_S): $(BUILDVM_BIN)
	$(ECHO_BEGIN) "BUILDVM $@"
	$(Q)$(BUILDVM_BIN) -m elfasm -o $@
	$(ECHO_END)

$(LUAJIT_UNPACK)/src/lj_bcdef.h: $(BUILDVM_BIN) $(LJLIB_C)
	$(ECHO_BEGIN) "BUILDVM $@"
	$(Q)$(BUILDVM_BIN) -m bcdef -o $@ $(LJLIB_C)
	$(ECHO_END)

$(LUAJIT_UNPACK)/src/lj_ffdef.h: $(BUILDVM_BIN) $(LJLIB_C)
	$(ECHO_BEGIN) "BUILDVM $@"
	$(Q)$(BUILDVM_BIN) -m ffdef -o $@ $(LJLIB_C)
	$(ECHO_END)

$(LUAJIT_UNPACK)/src/lj_libdef.h: $(BUILDVM_BIN) $(LJLIB_C)
	$(ECHO_BEGIN) "BUILDVM $@"
	$(Q)$(BUILDVM_BIN) -m libdef -o $@ $(LJLIB_C)
	$(ECHO_END)

$(LUAJIT_UNPACK)/src/lj_recdef.h: $(BUILDVM_BIN) $(LJLIB_C)
	$(ECHO_BEGIN) "BUILDVM $@"
	$(Q)$(BUILDVM_BIN) -m recdef -o $@ $(LJLIB_C)
	$(ECHO_END)

$(LIB_VMDEF): $(BUILDVM_BIN) $(LJLIB_C)
	$(ECHO_BEGIN) "BUILDVM $@"
	$(Q)$(BUILDVM_BIN) -m vmdef -o $(LIB_VMDEFP) $(LJLIB_C)
	$(ECHO_END)

$(LUAJIT_UNPACK)/src/lj_folddef.h: $(BUILDVM_BIN) $(LUAJIT_UNPACK)/src/lj_opt_fold.c
	$(ECHO_BEGIN) "BUILDVM $@"
	$(Q)$(BUILDVM_BIN) -m folddef -o $@ $(LUAJIT_UNPACK)/src/lj_opt_fold.c
	$(ECHO_END)

$(HOST_OBJS): %.o: %.c
	$(ECHO_BEGIN) "CC: $<"
	$(Q) $(HOSTCC) -m32 -c $(HOSTCFLAGS) $< -o $@
	$(ECHO_END)

ifeq ($(wildcard $(LUAJIT_UNPACK)/.git),)
context:: $(LUAJIT_UNPACK)/.patch $(LUAJIT_UNPACK)/src/host/buildvm_arch.h $(ALL_GEN)

distclean::
	$(call DELDIR, $(LUAJIT_UNPACK))
	$(call DELFILE, $(LUAJIT_TARBALL))
endif

include $(APPDIR)/Application.mk
