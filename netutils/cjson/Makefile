#############################################################################
# apps/ netutils/cjson/Makefile
#
#   Copyright (C) 2019 Gregory Nutt. All rights reserved.
#   Author: Mateusz Szafoni <raiden00@railab.me>
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
#############################################################################

# Standard includes

include $(APPDIR)/Make.defs

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CONFIG_NETUTILS_CJSON_URL ?= "https://github.com/DaveGamble/cJSON/archive"
CONFIG_NETUTILS_CJSON_VERSION ?= "1.7.10"
CJSON_VERSION = $(patsubst "%",%,$(strip $(CONFIG_NETUTILS_CJSON_VERSION)))

CJSON_TARBALL = v$(CJSON_VERSION).tar.gz
WGET ?= wget

CJSON_UNPACKNAME = cJSON-$(CJSON_VERSION)
UNPACK ?= tar -zxf

CJSON_UNPACKDIR =  $(WD)/$(CJSON_UNPACKNAME)
CJSON_SRCDIR = $(CJSON_UNPACKNAME)

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)netutils

CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPS_INCDIR)}
CFLAGS += -DCJSON_INCLUDE_CONFIG_H

CSRCS = $(CJSON_SRCDIR)$(DELIM)cJSON.c
CSRCS += $(CJSON_SRCDIR)$(DELIM)cJSON_Utils.c

$(CJSON_TARBALL):
	@echo "Downloading: $(CJSON_TARBALL)"
	$(Q) $(WGET) $(CONFIG_NETUTILS_CJSON_URL)/$(CJSON_TARBALL)

$(CJSON_UNPACKNAME): $(CJSON_TARBALL)
	@echo "Unpacking: $(CJSON_TARBALL) -> $(CJSON_UNPACKNAME)"
	$(Q) $(UNPACK) $(CJSON_TARBALL)
	$(Q) touch $(CJSON_UNPACKNAME)

$(CJSON_SRCDIR)$(DELIM)cJSON.h: $(CJSON_UNPACKNAME)

$(CJSON_SRCDIR)$(DELIM)cJSON_Utils.h: $(CJSON_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)cJSON.h: $(CJSON_SRCDIR)$(DELIM)cJSON.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)cJSON_Utils.h: $(CJSON_SRCDIR)$(DELIM)cJSON_Utils.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)cJSON.h $(APPS_INCDIR)$(DELIM)cJSON_Utils.h

distclean::
	$(call DELDIR, $(CJSON_UNPACKNAME))
	$(call DELFILE, $(CJSON_TARBALL))
	$(call DELFILE, $(APPDIR)/include/netutils/cJSON.h)
	$(call DELFILE, $(APPDIR)/include/netutils/cJSON_Utils.h)

include $(APPDIR)/Application.mk
