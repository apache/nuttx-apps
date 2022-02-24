############################################################################
# apps/system/embedlog/Makefile
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

CP = cp -R
UNPACK = tar -xzf
PACKEXT = .tar.gz
NXTOOLSDIR = $(APPDIR)/tools

EMBEDLOG_URL = https://distfiles.bofc.pl/embedlog
EMBEDLOG_VERSION = 0.6.0
EMBEDLOG_SRC_SHA256 = f5f15dd124d113c7c8cc286f17fc633e1ade90acb6633dc98277968ab2a149d5
EMBEDLOG_EXT = tar.gz
EMBEDLOG_SOURCES = embedlog-$(EMBEDLOG_VERSION)
EMBEDLOG_TARBALL = $(EMBEDLOG_SOURCES).$(EMBEDLOG_EXT)

CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPDIR)$(DELIM)include$(DELIM)system}

CSRCS = $(EMBEDLOG_SOURCES)/src/el-options.c \
	$(EMBEDLOG_SOURCES)/src/el-perror.c \
	$(EMBEDLOG_SOURCES)/src/el-pmemory.c \
	$(EMBEDLOG_SOURCES)/src/el-print.c \
	$(EMBEDLOG_SOURCES)/src/el-puts.c \
	$(EMBEDLOG_SOURCES)/src/el-ts.c \
	$(EMBEDLOG_SOURCES)/src/el-flush.c \
	$(EMBEDLOG_SOURCES)/src/el-utils.c \

# compile-time configuration of embedlog

ifeq ($(CONFIG_EMBEDLOG_ENABLE_TIMESTAMP),y)
	CFLAGS += -DENABLE_TIMESTAMP
	CFLAGS += -DENABLE_REALTIME
	CFLAGS += -DENABLE_MONOTONIC
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_FRACTIONS),y)
	CFLAGS += -DENABLE_FRACTIONS
else
	CFLAGS += -DENABLE_FRACTIONS=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_COLORS),y)
	CFLAGS += -DENABLE_COLORS
else
	CFLAGS += -DENABLE_COLORS=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_PREFIX),y)
	CFLAGS += -DENABLE_PREFIX
	CFLAGS += -DEL_PREFIX_MAX=$(CONFIG_EMBEDLOG_PREFIX_MAX)
else
	CFLAGS += -DENABLE_PREFIX=0
	CFLAGS += -DEL_PREFIX_MAX=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_FUNCINFO),y)
	CFLAGS += -DENABLE_FUNCINFO
	CFLAGS += -DEL_FUNCLEN_MAX=$(CONFIG_EMBEDLOG_FUNCLEN_MAX)
else
	CFLAGS += -DENABLE_FUNCINFO=0
	CFLAGS += -DEL_FUNCLEN_MAX=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_FINFO),y)
	CFLAGS += -DENABLE_FINFO
	CFLAGS += -DEL_FLEN_MAX=$(CONFIG_EMBEDLOG_FLEN_MAX)
else
	CFLAGS += -DENABLE_FINFO=0
	CFLAGS += -DNOFINFO
	CFLAGS += -DEL_FLEN_MAX=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_OUT_CUSTOM),y)
	CFLAGS += -DENABLE_OUT_CUSTOM
else
	CFLAGS += -DENABLE_OUT_CUSTOM=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_OUT_STDERR),y)
	CFLAGS += -DENABLE_OUT_STDERR
else
	CFLAGS += -DENABLE_OUT_STDERR=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_OUT_FILE),y)
	CFLAGS += -DENABLE_OUT_FILE
	CSRCS += $(EMBEDLOG_SOURCES)/src/el-file.c
else
	CFLAGS += -DENABLE_OUT_FILE=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_BINARY_LOGS),y)
	CFLAGS += -DENABLE_BINARY_LOGS
	CSRCS += $(EMBEDLOG_SOURCES)/src/el-pbinary.c
	CSRCS += $(EMBEDLOG_SOURCES)/src/el-encode-number.c
	CSRCS += $(EMBEDLOG_SOURCES)/src/el-decode-number.c
else
	CFLAGS += -DENABLE_BINARY_LOGS=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_OUT_TTY),y)
	CFLAGS += -DENABLE_OUT_TTY
	CSRCS += $(EMBEDLOG_SOURCES)/src/el-tty.c
else
	CFLAGS += -DENABLE_OUT_TTY=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_PTHREAD),y)
	CFLAGS += -DENABLE_PTHREAD
	CSRCS += $(EMBEDLOG_SOURCES)/src/el-lock.c
else
	CFLAGS += -DENABLE_PTHREAD=0
endif

ifeq ($(CONFIG_SERIAL_TERMIOS),y)
	CFLAGS += -DHAVE_TERMIOS_H
else
	CFLAGS += -DHAVE_TERMIOS_H=0
endif

CFLAGS += -DEL_LOG_MAX=$(CONFIG_EMBEDLOG_LOG_MAX)
CFLAGS += -DEL_MEM_LINE_SIZE=$(CONFIG_EMBEDLOG_MEM_LINE_SIZE)

# config.h is available only when building with autotools
CFLAGS += -DHAVE_CONFIG_H=0

# embedlog uses access(path, F_OK) to determine if file exists or not and since
# nuttx always returns OK here, we mark access as not working and make embedlog
# to use stat() for that purpose
CFLAGS += -DHAVE_ACCESS=0

# tell embedlog which features are available in nuttx os
CFLAGS += -DENABLE_REENTRANT
CFLAGS += -DENABLE_OUT_SYSLOG
CFLAGS += -DHAVE_STAT
CFLAGS += -DHAVE_UNISTD_H
CFLAGS += -DHAVE_FSYNC
CFLAGS += -DHAVE_FILENO
CFLAGS += -DENABLE_COLORS_EXTENDED=0
# symlinks are only supported on pseudofs, where nobody is going to
# store logfiles, thus symlinks are useless on nuttx
CFLAGS += -DHAVE_SYMLINK=0

# nuttx has its own snprintf implementation, disable internal snprintf
# implementation
CFLAGS += -DPREFER_PORTABLE_SNPRINTF=0
CFLAGS += -DNEED_SNPRINTF_ONLY=0
CFLAGS += -DHAVE_SNPRINTF

# not yet implemented features - silent compiler warnings
CFLAGS += -DENABLE_OUT_NET=0

# nuttx does not implement clock() function
CFLAGS += -DENABLE_CLOCK=0

ifeq ($(CONFIG_EMBEDLOG_DEMO_PROGRAMS),y)
	# build demo programs as library instead of standalone programs
	CFLAGS += -DEMBEDLOG_DEMO_LIBRARY

	PROGNAME   = el_demo_print_memory
	PRIORITY   = $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_PRIORITY)
	STACKSIZE  = $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_STACKSIZE)
	MAINSRC    = $(EMBEDLOG_SOURCES)/examples/print-memory.c

	PROGNAME  += el_demo_print_options
	PRIORITY  += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_PRIORITY)
	STACKSIZE += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_STACKSIZE)
	MAINSRC   += $(EMBEDLOG_SOURCES)/examples/print-options.c

	PROGNAME  += el_demo_print_simple
	PRIORITY  += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_PRIORITY)
	STACKSIZE += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_STACKSIZE)
	MAINSRC   += $(EMBEDLOG_SOURCES)/examples/print-simple.c

	PROGNAME  += el_demo_print_thread_safe
	PRIORITY  += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_PRIORITY)
	STACKSIZE += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_STACKSIZE)
	MAINSRC   += $(EMBEDLOG_SOURCES)/examples/print-thread-safe.c

	PROGNAME  += el_demo_print_to_file
	PRIORITY  += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_PRIORITY)
	STACKSIZE += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_STACKSIZE)
	MAINSRC   += $(EMBEDLOG_SOURCES)/examples/print-to-file.c

	PROGNAME  += el_demo_print_tty
	PRIORITY  += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_PRIORITY)
	STACKSIZE += $(CONFIG_EMBEDLOG_DEMO_PROGRAMS_STACKSIZE)
	MAINSRC   += $(EMBEDLOG_SOURCES)/examples/print-tty.c
endif

# building of embedlog

$(EMBEDLOG_TARBALL):
	@echo "Downloading: $@"
	$(Q) curl -L -o $@ $(EMBEDLOG_URL)/$@
	${Q} $(NXTOOLSDIR)/check-hash.sh sha256 $(EMBEDLOG_SRC_SHA256) $@

$(EMBEDLOG_SOURCES): $(EMBEDLOG_TARBALL)
	@echo "Unpacking $< -> $@"
	$(Q) $(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(EMBEDLOG_SOURCES)/include/embedlog.h: $(EMBEDLOG_SOURCES)/include/embedlog.h.in
	@echo "Generating: $@"
	$(Q) sed -e "s/@ENABLE_OUT_FILE@/$(CONFIG_EMBEDLOG_ENABLE_OUT_FILE)/" \
	    -e "s/@ENABLE_OUT_TTY@/$(CONFIG_EMBEDLOG_ENABLE_OUT_TTY)/" \
	    -e "s/@ENABLE_PREFIX@/$(CONFIG_EMBEDLOG_ENABLE_PREFIX)/" \
	    -e "s/@ENABLE_OUT_CUSTOM@/$(CONFIG_EMBEDLOG_ENABLE_OUT_CUSTOM)/" \
	    -e "s/@ENABLE_PTHREAD@/$(CONFIG_EMBEDLOG_ENABLE_PTHREAD)/" \
	    -e "s/^#if y$$/#if 1/" \
	    -e "s/^#if $$/#if 0/" \
	    -e "s/^#if n$$/#if 0/" $< > $@

create_includes: $(EMBEDLOG_SOURCES)/include/embedlog.h
	$(Q) $(CP) $< $(APPDIR)/include/system

context:: $(EMBEDLOG_SOURCES)
	$(Q) $(MAKE) create_includes

clean::
	$(Q) $(call DELFILE, $(APPDIR)/include/system/embedlog.h)
	$(Q) $(foreach COBJ, $(COBJS), $(call DELFILE, $(COBJ)))

distclean::
	$(Q) $(call DELDIR, $(EMBEDLOG_SOURCES))
	$(Q) $(call DELDIR, $(EMBEDLOG_TARBALL))

include $(APPDIR)/Application.mk
