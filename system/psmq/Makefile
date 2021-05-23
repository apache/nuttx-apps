############################################################################
# apps/system/psmq/Makefile
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

PSMQ_URL = https://distfiles.bofc.pl/psmq
PSMQ_VERSION = 0.2.1
PSMQ_SRC_SHA256 = b029fa06752ad7dce89f6b899f27a08a1bd1ee0a4e1d9df407274f0322bf1946
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
	$(PSMQ_SOURCES)/src/topic-list.c \
	$(PSMQ_SOURCES)/src/utils.c

# compile-time options from Kconfig
CFLAGS += -DPSMQ_MAX_CLIENTS=$(CONFIG_PSMQ_MAX_CLIENTS)

# build psmq as library and disable standalone applications (apps will
# still be compiled but into shared library rather than standalone application
CFLAGS += -DPSMQ_LIBRARY=1
CFLAGS += -DPSMQ_STANDALONE=0

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
	$(Q) curl -L -o $@ $(PSMQ_URL)/$@
	$(Q) $(NXTOOLSDIR)/check-hash.sh sha256 $(PSMQ_SRC_SHA256) $@

$(PSMQ_SOURCES): $(PSMQ_TARBALL)
	@echo "Unpacking $< -> $@"
	$(Q) $(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(PSMQ_SOURCES)/inc/psmq.h: $(PSMQ_SOURCES)/inc/psmq.h.in
	@echo "Generating: $@"
	$(Q) sed -e "s/@PSMQ_MSG_MAX@/$(CONFIG_PSMQ_MSG_MAX)/" $< > $@

create_includes: $(PSMQ_SOURCES)/inc/psmq.h
	$(Q) $(CP) $< $(APPDIR)/include/system

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
