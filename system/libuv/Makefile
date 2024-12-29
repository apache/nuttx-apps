############################################################################
# apps/system/libuv/Makefile
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

LIBUV_PATCHS ?= $(sort $(wildcard 000*.patch))

LIBUV_VERSION  = 1.46.0
LIBUV_UNPACK   = libuv
LIBUV_TARBALL  = v$(LIBUV_VERSION).zip
LIBUV_URL_BASE = https://github.com/libuv/libuv/archive/refs/tags
LIBUV_URL      = $(LIBUV_URL_BASE)/$(LIBUV_TARBALL)

$(LIBUV_TARBALL):
	@echo "Downloading: $(LIBUV_TARBALL)"
	$(Q) curl -L $(LIBUV_URL) -o $(LIBUV_TARBALL)

$(LIBUV_UNPACK): $(LIBUV_TARBALL)
	@echo "Unpacking: $(LIBUV_TARBALL) -> $(LIBUV_UNPACK)"
	$(call DELDIR, $(LIBUV_UNPACK))
	$(Q) unzip $(LIBUV_TARBALL)
	$(Q) mv libuv-$(LIBUV_VERSION) $(LIBUV_UNPACK)
	$(Q) cat $(LIBUV_PATCHS) | patch -s -N -d $(LIBUV_UNPACK) -p1

$(LIBUV_UNPACK)/.patch: $(LIBUV_UNPACK)
	$(Q) touch $(LIBUV_UNPACK)/.patch

# Build libuv library

CFLAGS += -I$(LIBUV_UNPACK)/src
CFLAGS += -I$(LIBUV_UNPACK)/src/unix
CFLAGS += -I$(LIBUV_UNPACK)/test
CFLAGS += -Wno-shadow
CFLAGS += -DDEF_THREADPOOL_SIZE=CONFIG_LIBUV_THREADPOOL_SIZE
CFLAGS += -DDEF_THREADPOOL_STACKSIZE=CONFIG_LIBUV_THREAD_STACKSIZE
CFLAGS += -DDEF_THREADPOOL_PRIORITY=CONFIG_LIBUV_THREADPOOL_PRIORITY
CFLAGS += -DMAX_EPOLL_EVENTS=CONFIG_LIBUV_MAX_EPOLL_EVENTS
CFLAGS += -DPREP_EVENT_SIZE=CONFIG_LIBUV_PREP_EVENT_SIZE
CFLAGS += -DDEF_STREAM_READ_BUF_SIZE=CONFIG_LIBUV_STREAM_READ_BUF_SIZE

ifeq ($(shell expr "$(GCCVER)" \>= 12), 1)
  CFLAGS += -Wno-dangling-pointer
endif

VPATH += $(LIBUV_UNPACK)/src
VPATH += $(LIBUV_UNPACK)/src/unix
VPATH += $(LIBUV_UNPACK)/test

DEPPATH += --dep-path $(LIBUV_UNPACK)/src
DEPPATH += --dep-path $(LIBUV_UNPACK)/src/unix
DEPPATH += --dep-path $(LIBUV_UNPACK)/test

CSRCS += core.c
CSRCS += poll.c
CSRCS += loop.c
CSRCS += thread.c
CSRCS += thread-common.c
CSRCS += posix-hrtime.c
ifneq ($(CONFIG_LIBUV_BACKEND_EPOLL),)
CSRCS += linux.c
else
CSRCS += posix-poll.c
CSRCS += no-fsevents.c
endif
CSRCS += uv-data-getter-setters.c
CSRCS += version.c
ifeq ($(CONFIG_LIBUV_UTILS_TEST),)
CSRCS += idna.c
CSRCS += strscpy.c
CSRCS += strtok.c
endif
CSRCS += uv-common.c
CSRCS += random-devurandom.c
CSRCS += random.c
CSRCS += nuttx.c
CSRCS += tty.c
CSRCS += loop-watcher.c
CSRCS += signal.c
CSRCS += stream.c
CSRCS += threadpool.c
CSRCS += async.c
CSRCS += pipe.c
CSRCS += fs.c
CSRCS += fs-poll.c
CSRCS += timer.c
ifneq ($(CONFIG_LIBC_EXECFUNCS),)
CSRCS += process-spawn.c
endif
CSRCS += sysinfo-loadavg.c
CSRCS += sysinfo-memory.c

ifneq ($(CONFIG_LIBC_DLFCN),)
CSRCS += dl.c
endif

ifneq ($(CONFIG_LIBC_NETDB),)
CSRCS += getaddrinfo.c
CSRCS += getnameinfo.c
endif

ifneq ($(CONFIG_NET),)
CSRCS += inet.c
endif

ifneq ($(CONFIG_NET_TCP),)
CSRCS += tcp.c
endif

ifneq ($(CONFIG_NET_UDP),)
CSRCS += udp.c
endif

ifeq ($(findstring y,$(CONFIG_LIBUV_UTILS_TEST)$(CONFIG_LIBUV_UTILS_BENCHMARK)), y)
PRIORITY  = $(CONFIG_LIBUV_UTILS_PRIORITY)
STACKSIZE = $(CONFIG_LIBUV_UTILS_STACKSIZE)
endif

ifneq ($(CONFIG_LIBUV_UTILS_TEST),)

PROGNAME = uv_run_tests
MAINSRC  = run-tests.c

CSRCS += runner.c
CSRCS += runner-unix.c
CSRCS += echo-server.c

CSRCS += $(wildcard libuv/test/test-*.c)
endif

ifneq ($(CONFIG_LIBUV_UTILS_BENCHMARK),)

PROGNAME = uv_run_benchmarks
MAINSRC  = run-benchmarks.c

CSRCS += runner.c
CSRCS += runner-unix.c

CSRCS += benchmark-async-pummel.c
CSRCS += benchmark-async.c
CSRCS += benchmark-fs-stat.c
CSRCS += benchmark-getaddrinfo.c
CSRCS += benchmark-loop-count.c
CSRCS += benchmark-million-async.c
CSRCS += benchmark-million-timers.c
CSRCS += benchmark-multi-accept.c
CSRCS += benchmark-ping-pongs.c
CSRCS += benchmark-ping-udp.c
CSRCS += benchmark-pound.c
CSRCS += benchmark-pump.c
CSRCS += benchmark-sizes.c
CSRCS += benchmark-spawn.c
CSRCS += benchmark-tcp-write-batch.c
CSRCS += benchmark-thread.c
CSRCS += benchmark-udp-pummel.c

CSRCS += blackhole-server.c
CSRCS += echo-server.c

endif

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(LIBUV_UNPACK)/.git),)
context:: $(LIBUV_UNPACK)/.patch

distclean::
	$(call DELDIR, $(LIBUV_UNPACK))
	$(call DELFILE, $(LIBUV_TARBALL))
endif

include $(APPDIR)/Application.mk
