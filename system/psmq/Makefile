############################################################################
# apps/system/psmq/Makefile
#
#   Copyright (C) 2019 Michał Łyszczek. All rights reserved.
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

include $(APPDIR)/Make.defs

WGET = wget
CP = cp -R
UNPACK = tar -xzf
PACKEXT = .tar.gz
NXTOOLSDIR = $(APPDIR)/tools

PSMQ_URL = https://distfiles.kurwinet.pl/psmq
PSMQ_VERSION = 0.1.0
PSMQ_SRC_SHA256 = 52dce7d6fcbf84243e11afdb953eea6a6eb405fd81c8e71bc556bb63090e1284
PSMQ_EXT = tar.gz
PSMQ_SOURCES = psmq-$(PSMQ_VERSION)
PSMQ_TARBALL = $(PSMQ_SOURCES).$(PSMQ_EXT)

CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPDIR)$(DELIM)include$(DELIM)system}
CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPDIR)$(DELIM)system$(DELIM)psmq$(DELIM)$(PSMQ_SOURCES)}

# mandatory source files to compile
CSRCS = $(PSMQ_SOURCES)/lib/psmq.c \
	$(PSMQ_SOURCES)/src/broker.c \
	$(PSMQ_SOURCES)/src/cfg.c \
	$(PSMQ_SOURCES)/src/globals.c \
	$(PSMQ_SOURCES)/src/topic-list.c

# compile-time options from Kconfig
CFLAGS += -DPSMQ_MAX_CLIENTS=$(CONFIG_PSMQ_MAX_CLIENTS)
CFLAGS += -DPSMQ_PAYLOAD_MAX=$(CONFIG_PSMQ_PAYLOAD_MAX)
CFLAGS += -DPSMQ_TOPIC_MAX=$(CONFIG_PSMQ_TOPIC_MAX)

# build psmq as library and disable standalone applications (apps will
# still be compiled but into shared library rather than standalone application
CFLAGS += -DPSMQ_LIBRARY=1
CFLAGS += -DPSMQ_STANDALONE=0

# psmq cannot be damonized in nuttx
CFLAGS += -DPSMQ_ENABLE_DAEMON=0

# nuttx has no opterr variable
CFLAGS += -DPSMQ_NO_OPTERR=1

# nuttx does not really support signals, so disable them. There will be no way
# to gently kill programs.
CFLAGS += -DPSMQ_NO_SIGNALS=1

# package string needed by tools when passing '-v' option
CFLAGS += -DPACKAGE_STRING="\"psmq $(PSMQ_VERSION)\""
CFLAGS += -DPACKAGE_VERSION=\"$(PSMQ_VERSION)\"

# register psmqd broker daemon application
PROGNAME = psmqd
PRIORITY = $(CONFIG_PSMQD_PRIORITY)
STACKSIZE = $(CONFIG_PSMQD_STACKSIZE)
MAINSRC = $(PSMQ_SOURCES)/src/psmqd.c

# register psmq_pub application if it was enabled
ifeq ($(CONFIG_PSMQ_TOOLS_PUB),y)
	PROGNAME += psmq_pub
	PRIORITY += $(CONFIG_PSMQ_PUB_PRIORITY)
	STACKSIZE += $(CONFIG_PSMQ_PUB_STACKSIZE)
	MAINSRC += $(PSMQ_SOURCES)/src/psmq-pub.c
endif

# register psmq_sub application if it was enabled
ifeq ($(CONFIG_PSMQ_TOOLS_SUB),y)
	PROGNAME += psmq_sub
	PRIORITY += $(CONFIG_PSMQ_SUB_PRIORITY)
	STACKSIZE += $(CONFIG_PSMQ_SUB_STACKSIZE)
	MAINSRC += $(PSMQ_SOURCES)/src/psmq-sub.c
endif

# download and build psmq

$(PSMQ_TARBALL):
	@echo "Downloading: $@"
	$(Q) $(WGET) -O $@ $(PSMQ_URL)/$@
	$(Q) $(NXTOOLSDIR)/check-hash.sh sha256 $(PSMQ_SRC_SHA256) $@

$(PSMQ_SOURCES): $(PSMQ_TARBALL)
	@echo "Unpacking $< -> $@"
	$(Q) $(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

create_includes:
	$(Q) $(CP) $(PSMQ_SOURCES)/inc/psmq.h $(APPDIR)/include/system

context:: $(PSMQ_SOURCES)
	$(Q) $(MAKE) create_includes

clean::
	$(Q) $(call DELFILE, $(APPDIR)/include/system/psmq.h)
	$(Q) $(foreach COBJ, $(COBJS), $(call DELFILE, $(COBJ)))

distclean::
	$(Q) $(call DELDIR, $(PSMQ_SOURCES))
	$(Q) $(call DELDIR, $(PSMQ_TARBALL))

MODULE = $(CONFIG_SYSTEM_PSMQ)

include $(APPDIR)/Application.mk
