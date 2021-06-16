############################################################################
# apps/interpreters/wasm3/Makefile
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

WASM3_VERSION  = 0.4.7
WASM3_UNPACK   = wasm3
WASM3_TARBALL  = v$(WASM3_VERSION).tar.gz
WASM3_URL_BASE = https://github.com/wasm3/wasm3/archive
WASM3_URL      = $(WASM3_URL_BASE)/$(WASM3_TARBALL)

MAINSRC = main.c

CSRCS += m3_bind.c m3_code.c m3_compile.c m3_core.c
CSRCS += m3_emit.c m3_env.c m3_exec.c m3_info.c
CSRCS += m3_module.c m3_parse.c m3_api_libc.c

VPATH += $(WASM3_UNPACK)/source
VPATH += $(WASM3_UNPACK)/platforms/app
CFLAGS += -Wno-unused-function -Wno-unused-variable
CFLAGS += -Wno-strict-prototypes

ifneq ($(CONFIG_INTERPRETERS_WASM3_FIXEDHEAP),)
CFLAGS += -Dd_m3FixedHeap=$(CONFIG_INTERPRETERS_WASM3_FIXEDHEAP)
endif

ifneq ($(CONFIG_INTERPRETERS_WASM3_CODEPAGE),)
CFLAGS += -Dd_m3CodePageAlignSize=$(CONFIG_INTERPRETERS_WASM3_CODEPAGE)
endif

ifeq ($(CONFIG_INTERPRETERS_WASM3_LOG),)
CFLAGS += -Dd_m3LogOutput=0
endif

ifeq ($(CONFIG_INTERPRETERS_WASM3_LOG_VERBOSE),)
CFLAGS += -Dd_m3VerboseLogs=0
endif


PROGNAME  = wasm3
PRIORITY  = $(CONFIG_INTERPRETERS_WASM3_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_WASM3_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_WASM3)

$(WASM3_TARBALL):
	$(Q) echo "Downloading $(WASM3_TARBALL)"
	$(Q) curl -O -L $(WASM3_URL)

$(WASM3_UNPACK): $(WASM3_TARBALL)
	$(Q) echo "Unpacking $(WASM3_TARBALL) to $(WASM3_UNPACK)"
	$(Q) tar xzvf $(WASM3_TARBALL)
	$(Q) mv wasm3-$(WASM3_VERSION) $(WASM3_UNPACK)

$(WASM3_UNPACK)/.patch: $(WASM3_UNPACK)
	$(Q) touch $(WASM3_UNPACK)/.patch

context:: $(WASM3_UNPACK)/.patch

distclean::
	$(call DELDIR, $(WASM3_UNPACK))
	$(call DELFILE, $(WASM3_TARBALL))

include $(APPDIR)/Application.mk
