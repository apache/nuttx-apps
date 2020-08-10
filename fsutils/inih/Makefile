############################################################################
# apps/fsutils/inih/Makefile
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

INIH_GITHUB_NAME = benhoyt/inih
INIH_VERSION = r42
INIH_SRC_SHA256 = 53ac3591be39a004ddc7874d0a57e7b9bc92be14192808bbef510c5c533cb1d8
INIH_EXT = tar.gz
INIH_SOURCES = inih-$(INIH_VERSION)
INIH_TARBALL = $(INIH_VERSION).$(INIH_EXT)
INIH_URL = https://github.com/$(INIH_GITHUB_NAME)/archive

CSRCS = $(INIH_SOURCES)/ini.c

# inih configuration from Kconfig

ifeq ($(CONFIG_INIH_MULTI_LINE_ENTRIES),y)
	CFLAGS += -DINI_ALLOW_MULTILINE=1
else
	CFLAGS += -DINI_ALLOW_MULTILINE=0
endif

ifeq ($(CONFIG_INIH_USE_MALLOC),y)
	CFLAGS += -DINI_USE_STACK=0
	CFLAGS += -DINI_ALLOW_REALLOC=1
	CFLAGS += -DINI_INITIAL_ALLOC=$(CONFIG_INIH_INITIAL_ALLOC)
else
	CFLAGS += -DINI_USE_STACK=1
	CFLAGS += -DINI_ALLOW_REALLOC=0
endif

CFLAGS += -DINI_MAX_LINE=$(CONFIG_INIH_MAX_LINE)

# compile-time configuration of inih

CFLAGS += -DINI_ALLOW_BOM=0
CFLAGS += -DINI_ALLOW_INLINE_COMMENTS=1
CFLAGS += -DINI_STOP_ON_FIRST_ERROR=0
CFLAGS += -DINI_HANDLER_LINENO=1

# building of inih

$(INIH_TARBALL):
	@echo "Downloading: $@"
	$(Q) $(WGET) -O $@ $(INIH_URL)/$@
	${Q} $(NXTOOLSDIR)/check-hash.sh sha256 $(INIH_SRC_SHA256) $@

$(INIH_SOURCES): $(INIH_TARBALL)
	@echo "Unpacking $< -> $@"
	$(Q) $(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

create_includes:
	$(Q) $(CP) $(INIH_SOURCES)/ini.h $(APPDIR)/include/fsutils

context:: $(INIH_SOURCES)
	$(Q) $(MAKE) create_includes

clean::
	$(Q) $(call DELFILE, $(APPDIR)/include/fsutils/ini.h)
	$(Q) $(foreach COBJ, $(COBJS), $(call DELFILE, $(COBJ)))

distclean::
	$(Q) $(call DELDIR, $(INIH_SOURCES))
	$(Q) $(call DELDIR, $(INIH_TARBALL))

include $(APPDIR)/Application.mk
