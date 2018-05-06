############################################################################
# apps/system/embedlog/Makefile
#
#   Copyright (C) Michał Łyszczek. All rights reserved.
#   Author: Michał Łyszczek <michal.lyszczek@bofc.pl>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)/Make.defs

ROOTDEPPATH = --dep-path .

WGET = wget
CP = cp -R
UNPACK = tar -xzf
PACKEXT = .tar.gz

EMBEDLOG_URL = https://distfiles.kurwinet.pl/embedlog
EMBEDLOG_VERSION = 0.3.0
EMBEDLOG_EXT = tar.gz
EMBEDLOG_SOURCES = embedlog-$(EMBEDLOG_VERSION)
EMBEDLOG_TARBALL = $(EMBEDLOG_SOURCES).$(EMBEDLOG_EXT)

CFLAGS += -I$(APPDIR)/include/system

SRCS = embedlog/src/el-options.c \
	embedlog/src/el-perror.c \
	embedlog/src/el-pmemory.c \
	embedlog/src/el-print.c \
	embedlog/src/el-puts.c \
	embedlog/src/el-ts.c

# compile-time configuration of embedlog

ifeq ($(CONFIG_EMBEDLOG_ENABLE_TIMESTAMP),y)
	CFLAGS += -DENABLE_TIMESTAMP
	CFLAGS += -DENABLE_REALTIME

ifeq ($(CONFIG_CLOCK_MONOTONIC),y)
	CFLAGS += -DENABLE_MONOTONIC
else
	CFLAGS += -DENABLE_MONOTONIC=0
endif
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

ifeq ($(CONFIG_EMBEDLOG_ENABLE_FINFO),y)
	CFLAGS += -DENABLE_FINFO
	CFLAGS += -DNOFINFO=0
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
	SRCS += embedlog/src/el-file.c
else
	CFLAGS += -DENABLE_OUT_FILE=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_BINARY_LOGS),y)
	CFLAGS += -DENABLE_BINARY_LOGS
	SRCS += embedlog/src/el-pbinary.c
	SRCS += embedlog/src/el-encode-number.c
else
	CFLAGS += -DENABLE_BINARY_LOGS=0
endif

ifeq ($(CONFIG_EMBEDLOG_ENABLE_OUT_TTY),y)
	CFLAGS += -DENABLE_OUT_TTY
	SRCS += embedlog/src/el-tty.c
else
	CFLAGS += -DENABLE_OUT_TTY=0
endif

ifeq ($(CONFIG_SERIAL_TERMIOS),y)
	CFLAGS += -DHAVE_TERMIOS_H
else
	CFLAGS += -DHAVE_TERMIOS_H=0
endif

# config.h is available only when building with autotools

CFLAGS += -DHAVE_CONFIG_H=0

# embedlog uses access(path, F_OK) to determin if file exists or not and since
# nuttx always returns OK here, we mark access as not working and make embedlog
# to use stat() for that purpose

CFLAGS += -DHAVE_ACCESS=0
CFLAGS += -DEL_LOG_MAX=$(CONFIG_EMBEDLOG_LOG_MAX)
CFLAGS += -DEL_MEM_LINE_SIZE=$(CONFIG_EMBEDLOG_MEM_LINE_SIZE)
CFLAGS += -DENABLE_REENTRANT
CFLAGS += -DENABLE_OUT_SYSLOG
CFLAGS += -DHAVE_STAT
CFLAGS += -DHAVE_SNPRINTF
CFLAGS += -DHAVE_UNISTD_H
CFLAGS += -DHAVE_FSYNC
CFLAGS += -DHAVE_FILENO
CFLAGS += -DENABLE_COLORS_EXTENDED=0

# not yet implemented features - silent compiler warnings

CFLAGS += -DENABLE_OUT_NET=0

# nuttx does not implement clock() function

CFLAGS += -DENABLE_CLOCK=0

# building of embedlog

CCEXT = .c
COBJS = $(SRCS:$(CCEXT)=$(OBJEXT))

ifeq ($(WINTOOL),y)
	BIN = "${shell cygpath -w $(APPDIR)/libapps$(LIBEXT)}"
else
	BIN = $(APPDIR)/libapps$(LIBEXT)
endif
DEPBIN = $(APPDIR)/libapps$(LIBEXT)

all: .built
.PHONY: clean depend distclean preconfig context

$(EMBEDLOG_TARBALL):
	@echo "Downloading: $@"
	$(Q) $(WGET) -O $@ $(EMBEDLOG_URL)/$@
	$(Q) touch $@

$(EMBEDLOG_SOURCES): $(EMBEDLOG_TARBALL)
	@echo "Unpacking $< -> $@"
	$(Q) $(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

embedlog: $(EMBEDLOG_SOURCES)
	$(call DELDIR, $@)
	$(Q) $(CP) $< $@
	$(Q) touch $@

create_includes:
	$(Q) $(CP) $(EMBEDLOG_SOURCES)/include/embedlog.h $(APPDIR)/include/system

$(COBJS): %$(OBJEXT): %$(CCEXT)
	$(Q) $(call COMPILE, $<, $@)

.built: $(COBJS)
	$(Q) $(call ARCHIVE, $(BIN), $(COBJS))
	$(Q) touch $@

context: embedlog
	$(Q) $(MAKE) create_includes

.depend: Makefile $(SRCS)
	$(Q) $(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(CSRCS) >Make.dep
	$(Q) touch $@

depend: .depend

install:

clean:
	$(Q) $(call DELFILE, .built)
	$(Q) $(call DELFILE, $(APPDIR)/include/system/embedlog.h)
	$(Q) $(foreach COBJ, $(COBJS), $(call DELFILE, $(COBJ)))

distclean: clean
	$(Q) $(call DELFILE, Make.dep)
	$(Q) $(call DELFILE, .depend)
	$(Q) $(call DELDIR, embedlog)
	$(Q) $(call DELDIR, $(EMBEDLOG_SOURCES))
	$(Q) $(call DELDIR, $(EMBEDLOG_TARBALL))

preconfig:

-include Make.dep
