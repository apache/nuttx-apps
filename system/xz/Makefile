############################################################################
# apps/system/xz/Makefile
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

XZ_TARGZ = xz-5.6.3.tar.gz
XZ_SRC = xz

ifeq ($(wildcard $(XZ_SRC)/.git),)
$(XZ_TARGZ):
	@echo "Downloading $(XZ_TARGZ)"
	$(Q) curl -O -L https://github.com/tukaani-project/xz/releases/download/v5.6.3/xz-5.6.3.tar.gz

$(XZ_SRC): $(XZ_TARGZ)
	tar -zxvf $(XZ_TARGZ)
	mv xz-5.6.3 $(XZ_SRC)

context:: $(XZ_SRC)

distclean::
	$(call DELDIR, $(XZ_SRC), $(XZ_TARGZ))

endif

XZ_PREFIX = $(XZ_SRC)/src/liblzma

CFLAGS += -I.
CFLAGS += -Ixz/src/common
CFLAGS += -I$(XZ_PREFIX)/api
CFLAGS += -I$(XZ_PREFIX)/check
CFLAGS += -I$(XZ_PREFIX)/common
CFLAGS += -I$(XZ_PREFIX)/delta
CFLAGS += -I$(XZ_PREFIX)/lz
CFLAGS += -I$(XZ_PREFIX)/lzma
CFLAGS += -I$(XZ_PREFIX)/rangecoder
CFLAGS += -I$(XZ_PREFIX)/simple

CFLAGS += -DHAVE__BOOL=1
CFLAGS += -DHAVE_CONFIG_H
CFLAGS += -DMYTHREAD_POSIX

CSRCS += $(wildcard $(XZ_PREFIX)/check/*_fast.c)
CSRCS += $(XZ_PREFIX)/check/check.c
CSRCS += $(XZ_PREFIX)/check/crc32_table.c
CSRCS += $(XZ_PREFIX)/check/crc64_table.c
CSRCS += $(XZ_PREFIX)/check/sha256.c
CSRCS += $(XZ_PREFIX)/rangecoder/price_table.c

CSRCS += $(wildcard xz/src/common/*.c)
CSRCS += $(wildcard $(XZ_PREFIX)/common/*.c)
CSRCS += $(wildcard $(XZ_PREFIX)/delta/*.c)
CSRCS += $(wildcard $(XZ_PREFIX)/lz/*.c)
CSRCS += $(wildcard $(XZ_PREFIX)/lzma/*.c)
CSRCS += $(wildcard $(XZ_PREFIX)/simple/*.c)

ifneq ($(CONFIG_UTILS_XZ_PROGNAME),)
CSRCS += $(filter-out xz/src/xz/main.c, $(wildcard xz/src/xz/*.c))
PROGNAME += $(CONFIG_UTILS_XZ_PROGNAME)
PRIORITY += $(CONFIG_UTILS_XZ_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_XZ_STACKSIZE)
MAINSRC += xz/src/xz/main.c
endif

ifneq ($(CONFIG_UTILS_XZDEC_PROGNAME),)
PROGNAME += $(CONFIG_UTILS_XZDEC_PROGNAME)
PRIORITY += $(CONFIG_UTILS_XZDEC_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_XZDEC_STACKSIZE)
MAINSRC += xz/src/xzdec/xzdec.c
endif

include $(APPDIR)/Application.mk
