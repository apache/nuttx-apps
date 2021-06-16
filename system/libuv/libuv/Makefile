############################################################################
# apps/system/libuv/Makefile
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

LIBUV_PATCHS ?= $(sort $(wildcard $(LIBUV_DIR)/000*.patch))
UNPACK ?= unzip -q

$(LIBUV_DIR)/$(CONFIG_LIBUV_TARBALL):
	@echo "Downloading: $(CONFIG_LIBUV_TARBALL)"
	$(Q) curl -L $(CONFIG_LIBUV_URL)/$(CONFIG_LIBUV_TARBALL) \
		-o $(LIBUV_DIR)/$(CONFIG_LIBUV_TARBALL)

$(LIBUV_UNPACKDIR): $(LIBUV_DIR)/$(CONFIG_LIBUV_TARBALL)
	@echo "Unpacking: $(CONFIG_LIBUV_TARBALL) -> $(LIBUV_UNPACKDIR)"
	$(call DELDIR, $(LIBUV_UNPACKDIR))
	$(Q) unzip -q -d "$(LIBUV_DIR)" "$(LIBUV_DIR)/$(CONFIG_LIBUV_TARBALL)"
	$(Q) cat $(LIBUV_PATCHS) | \
		patch -s -N -d $(LIBUV_UNPACKDIR) -p1
	$(Q) touch $(LIBUV_UNPACKDIR)

# Build libuv library

CFLAGS += -I$(LIBUV_UNPACKDIR)/src
CFLAGS += -I$(LIBUV_UNPACKDIR)/src/unix
CFLAGS += -D__NUTTX__

CSRCS += $(LIBUV_UNPACKDIR)/src/unix/core.c
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/poll.c
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/loop.c
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/thread.c
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/no-proctitle.c

ifeq ($(CONFIG_DEV_URANDOM),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/random-devurandom.c
CSRCS += $(LIBUV_UNPACKDIR)/src/random.c
endif

ifeq ($(CONFIG_LIBUV_LOOP_WATCHERS),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/loop-watcher.c
endif

# FIXME signal does not work yet
ifeq ($(CONFIG_LIBUV_SIGNAL),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/signal.c
endif

# FIXME process does not work yet
ifeq ($(CONFIG_LIBUV_PROCESS),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/process.c
endif

CSRCS += $(LIBUV_UNPACKDIR)/src/unix/nuttx.c

ifeq ($(CONFIG_LIBUV_STREAM),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/nuttx_stream.c
endif

ifeq ($(CONFIG_LIBUV_TCP),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/nuttx_tcp.c
endif

ifeq ($(CONFIG_LIBUV_WQ),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/nuttx_threadpool.c
endif

ifeq ($(CONFIG_LIBUV_ASYNC),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/async.c
endif

ifeq ($(CONFIG_LIBUV_PIPE),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/pipe.c
endif

ifeq ($(CONFIG_LIBUV_FS),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/fs.c
endif

ifeq ($(CONFIG_LIBUV_FS_POLL),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/fs-poll.c
endif

ifeq ($(CONFIG_LIBUV_TIMER),y)
ifeq ($(CONFIG_LIBUV_TIMER_NUTTX),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/unix/nuttx_timer.c
else
CSRCS += $(LIBUV_UNPACKDIR)/src/timer.c
endif
endif

CSRCS += $(LIBUV_UNPACKDIR)/src/uv-common.c
CSRCS += $(LIBUV_UNPACKDIR)/src/strscpy.c

ifeq ($(CONFIG_LIBUV_NET),y)
CSRCS += $(LIBUV_UNPACKDIR)/src/inet.c
endif

context:: $(LIBUV_UNPACKDIR)

clean::
	$(call DELFILE, $(OBJS))

distclean::
	$(call DELDIR, $(LIBUV_UNPACKDIR))
	$(call DELFILE, $(LIBUV_DIR)/$(CONFIG_LIBUV_TARBALL))

include $(APPDIR)/Application.mk
