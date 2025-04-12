############################################################################
# apps/interpreters/toywasm/Makefile
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

PROGNAME  = toywasm
PRIORITY  = $(CONFIG_INTERPRETERS_TOYWASM_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_TOYWASM_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_TOYWASM)

# cli
MAINSRC = main.c
CSRCS += str_to_uint.c
CSRCS += repl.c

# lib
CSRCS += bitmap.c
CSRCS += cconv.c
CSRCS += cell.c
CSRCS += context.c
CSRCS += decode.c
CSRCS += dummy_host_func.c
CSRCS += endian.c
CSRCS += escape.c
CSRCS += exec.c
CSRCS += exec_debug.c
CSRCS += exec_insn_subr.c
CSRCS += expr.c
CSRCS += expr_parser.c
CSRCS += fileio.c
CSRCS += host_instance.c
CSRCS += idalloc.c
CSRCS += import_object.c
CSRCS += insn.c
CSRCS += instance.c
CSRCS += leb128.c
CSRCS += list.c
CSRCS += load_context.c
CSRCS += mem.c
CSRCS += module.c
CSRCS += module_writer.c
CSRCS += name.c
CSRCS += nbio.c
CSRCS += options.c
CSRCS += report.c
CSRCS += restart.c
CSRCS += shared_memory.c
CSRCS += slist.c
CSRCS += timeutil.c
CSRCS += toywasm_config.c
CSRCS += type.c
CSRCS += util.c
CSRCS += validation.c
CSRCS += vec.c
CSRCS += xlog.c

# TOYWASM_ENABLE_WASM_THREADS
CSRCS += cluster.c
CSRCS += lock.c
CSRCS += suspend.c
CSRCS += waitlist.c
#CSRCS += usched.c

# libwasi
CSRCS += wasi.c
CSRCS += wasi_abi_clock.c
CSRCS += wasi_abi_environ.c
CSRCS += wasi_abi_fd.c
CSRCS += wasi_abi_path.c
CSRCS += wasi_abi_poll.c
CSRCS += wasi_abi_prestat.c
CSRCS += wasi_abi_proc.c
CSRCS += wasi_abi_random.c
CSRCS += wasi_abi_sched.c
CSRCS += wasi_abi_sock.c
CSRCS += wasi_fdinfo.c
CSRCS += wasi_fdtable.c
CSRCS += wasi_host_dirent.c
CSRCS += wasi_host_fdop.c
CSRCS += wasi_host_pathop.c
CSRCS += wasi_host_subr.c
CSRCS += wasi_path_subr.c
CSRCS += wasi_poll_subr.c
CSRCS += wasi_subr.c
CSRCS += wasi_table.c
CSRCS += wasi_uio.c
CSRCS += wasi_vfs.c
CSRCS += wasi_vfs_impl_host.c

# TOYWASM_ENABLE_WASI_THREADS
CSRCS += wasi_threads.c

# libwasi_littlefs TOYWASM_ENABLE_WASI_LITTLEFS
#CSRCS += wasi_littlefs.c
#CSRCS += wasi_littlefs_impl.c
#CSRCS += wasi_littlefs_mount.c
#CSRCS += wasi_littlefs_ops.c
#CSRCS += wasi_vfs_impl_littlefs.c
#CSRCS += lfs.c
#CSRCS += lfs_util.c
#toywasm/libwasi_littlefs/littlefs/lfs.c_CFLAGS += -Wno-shadow
#CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/toywasm/toywasm/libwasi_littlefs
#CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/toywasm/toywasm/libwasi_littlefs/littlefs
#CFLAGS += -DLFS_THREADSAFE
#VPATH += $(TOYWASM_UNPACK)/libwasi_littlefs
#VPATH += $(TOYWASM_UNPACK)/libwasi_littlefs/littlefs

# TOYWASM_ENABLE_DYLD
CSRCS += dyld.c
CSRCS += dyld_plt.c
CSRCS += dyld_stats.c

# TOYWASM_ENABLE_DYLD_DLFCN
CSRCS += dyld_dlfcn.c

ifeq ($(shell $(CC) --version | grep -i clang),)
CFLAGS += -Wno-maybe-uninitialized
endif
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/toywasm/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/toywasm/toywasm/lib
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/toywasm/toywasm/libwasi
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/toywasm/toywasm/libwasi_threads
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/interpreters/toywasm/toywasm/libdyld

# Keep the compilation parameters consistent with toywasm mainline
# https://github.com/yamt/toywasm/blob/master/cmake/ToywasmConfig.cmake#L298-L302

CFLAGS += -Wno-unknown-warning-option -Wno-unused-but-set-variable -Wno-unused-variable -Wno-return-type

TOYWASM_VERSION  = 2e4474d1af3bf9bc5a4b571be7ee69694f4a8aef
TOYWASM_UNPACK   = toywasm
TOYWASM_TARBALL  = $(TOYWASM_VERSION).zip
TOYWASM_URL_BASE = https://github.com/yamt/toywasm/archive/
TOYWASM_URL      = $(TOYWASM_URL_BASE)/$(TOYWASM_TARBALL)

VPATH += $(TOYWASM_UNPACK)/cli
VPATH += $(TOYWASM_UNPACK)/lib
VPATH += $(TOYWASM_UNPACK)/libwasi
VPATH += $(TOYWASM_UNPACK)/libwasi_threads
VPATH += $(TOYWASM_UNPACK)/libdyld
VPATH += src

$(TOYWASM_TARBALL):
	$(Q) echo "Downloading $(TOYWASM_TARBALL)"
	$(Q) curl -O -L $(TOYWASM_URL)

$(TOYWASM_UNPACK): $(TOYWASM_TARBALL)
	$(Q) echo "Unpacking $(TOYWASM_TARBALL) to $(TOYWASM_UNPACK)"
	$(Q) unzip $(TOYWASM_TARBALL)
	$(Q) mv toywasm-$(TOYWASM_VERSION) $(TOYWASM_UNPACK)
	$(Q) touch $(TOYWASM_UNPACK)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(TOYWASM_UNPACK)/.git),)
context:: $(TOYWASM_UNPACK)

distclean::
	$(call DELDIR, $(TOYWASM_UNPACK))
	$(call DELFILE, $(TOYWASM_TARBALL))
endif

include $(APPDIR)/Application.mk
