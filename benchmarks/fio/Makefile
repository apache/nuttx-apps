############################################################################
# apps/benchmarks/fio/Makefile
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

PROGNAME  = fio
PRIORITY  = $(CONFIG_BENCHMARK_FIO_PRIORITY)
STACKSIZE = $(CONFIG_BENCHMARK_FIO_STACKSIZE)
MODULE    = $(CONFIG_BENCHMARK_FIO)
MAINSRC   = $(CURDIR)/fio/fio.c

CFLAGS += -DCONFIG_GETTIMEOFDAY -DCONFIG_CLOCK_GETTIME -DCONFIG_HAVE_GETTID
CFLAGS += -DCONFIG_STATIC_ASSERT -DCONFIG_LITTLE_ENDIAN -DCONFIG_POSIX_FALLOCATE
CFLAGS += -DCONFIG_HAVE_BOOL -DFIO_INTERNAL -DCONFIG_NO_SHM
CFLAGS += -DFIO_VERSION='"1.2.0"' -DFIO_USE_GENERIC_SWAP
CFLAGS += -DFIO_USE_GENERIC_INIT_RANDOM_STATE -DFIO_USE_GENERIC_BDEV_SIZE

CFLAGS += -DBLOOM_SIZE=1024*1024 -DBUF_SIZE=512*1024
CFLAGS += -DMAX_POOLS=4 -DINITIAL_POOLS=2 -DINITIAL_SIZE=512*1024
CFLAGS += -DCONFIG_SEED_BUCKETS=8 -DBITS_PER_LONG=32
CFLAGS += -Ifio -Ifio/arch -Ifio/crc -Ifio/os
CFLAGS += -DXXH32_digest=fio_XXH32_digest -DXXH32_update=fio_XXH32_update -DXXH32=fio_XXH32 \
          -Dparse_options=fio_parse_options -Dparse_option=fio_parse_option

ifneq ($(CONFIG_ARCH_ARM),)
CFLAGS += -D__ARM_ARCH_6__
endif

CSRCS += $(wildcard fio/lib/*.c)
CSRCS += $(wildcard fio/crc/*.c)

CSRCS += fio/gettime.c fio/ioengines.c fio/init.c fio/stat.c fio/log.c fio/time.c fio/filesetup.c \
         fio/eta.c fio/verify.c fio/memory.c fio/io_u.c fio/parse.c fio/fio_sem.c fio/rwlock.c \
         fio/pshared.c fio/options.c fio/smalloc.c fio/filehash.c fio/profile.c fio/debug.c \
         fio/server.c fio/client.c fio/iolog.c fio/backend.c fio/libfio.c fio/flow.c fio/cconv.c \
         fio/gettime-thread.c fio/helpers.c fio/json.c fio/idletime.c fio/td_error.c fio/zbd.c \
         fio/profiles/tiobench.c fio/profiles/act.c fio/io_u_queue.c fio/filelock.c fio/steadystate.c \
         fio/workqueue.c fio/rate-submit.c fio/optgroup.c fio/helper_thread.c fio/zone-dist.c \
         fio/dedupe.c

CSRCS += fio/engines/exec.c fio/engines/cpu.c fio/engines/sync.c fio/engines/ftruncate.c \
         fio/engines/falloc.c fio/engines/fileoperations.c fio/engines/mmap.c \
         fio/engines/null.c fio/engines/net.c

ifeq ($(wildcard fio/.git),)
VERSION ?= master
fio.zip:
	$(Q) curl -L https://github.com/ldorau/fio/archive/refs/heads/master.zip -o fio.zip
	$(Q) unzip -o fio.zip
	$(Q) mv fio-$(VERSION) fio
	$(Q) patch -p0 < 0001-external-fio-fix-compile-warning.patch
	$(Q) patch -p0 < 0002-external-fio-add-os-nuttx-support.patch
	$(Q) patch -p0 < 0003-external-fio-modify-smaller-size-for-nuttx.patch
	$(Q) patch -p0 < 0004-external-fio-fix-runtime-error-by-asan.patch
	$(Q) patch -p0 < 0005-external-fio-fix-compile-warning.patch
	$(Q) patch -p0 < 0006-external-fio-reinit-global-var-issue-when-run-multip.patch
	$(Q) patch -p0 < 0007-external-fio-add-engine-init.patch
	$(Q) patch -p0 < 0008-fio-fix-memory-leak-run-cpuio.fio.patch
	$(Q) patch -p0 < 0009-fio-fix-memory-leak-ioengine-filecreate.patch
	$(Q) patch -p0 < 0010-fio-fix-memory-leak-ioengine-exec.patch

context:: fio.zip

distclean::
	$(call DELDIR, fio)
	$(call DELFILE, fio.zip)

endif

include $(APPDIR)/Application.mk
