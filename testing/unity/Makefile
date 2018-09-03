############################################################################
# apps/ testing/unity/Makefile
#
#   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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
###########################################################################

# Standard includes

-include $(TOPDIR)/Make.defs

# Set up build configuration and environment

WD := ${shell pwd | sed -e 's/ /\\ /g'}

CONFIG_TESTING_UNITY_URL ?= "https://github.com/ThrowTheSwitch/Unity/archive"
CONFIG_TESTING_UNITY_VERSION ?= "2.4.3"
UNITY_VERSION = $(patsubst "%",%,$(strip $(CONFIG_TESTING_UNITY_VERSION)))

UNITY_TARBALL = v$(UNITY_VERSION).tar.gz
WGET ?= wget

UNITY_UNPACKNAME = Unity-$(UNITY_VERSION)
UNPACK ?= tar -zxf

UNITY_UNPACKDIR =  $(WD)/$(UNITY_UNPACKNAME)
UNITY_SRCDIR = $(UNITY_UNPACKDIR)$(DELIM)src

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)testing

CFLAGS += -I$(APPS_INCDIR) -DUNITY_INCLUDE_CONFIG_H

CSRCS = $(UNITY_SRCDIR)$(DELIM)unity.c

$(UNITY_TARBALL):
	@echo "Downloading: $(UNITY_TARBALL)"
	$(Q) $(WGET) $(CONFIG_TESTING_UNITY_URL)/$(UNITY_TARBALL)

$(UNITY_UNPACKNAME): $(UNITY_TARBALL)
	@echo "Unpacking: $(UNITY_TARBALL) -> $(UNITY_UNPACKNAME)"
	$(Q) $(UNPACK) $(UNITY_TARBALL)
	@touch $(UNITY_UNPACKNAME)

$(UNITY_SRCDIR)$(DELIM)unity.h: $(UNITY_UNPACKNAME)

$(UNITY_SRCDIR)$(DELIM)unity_internals.h: $(UNITY_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)unity.h: $(UNITY_SRCDIR)$(DELIM)unity.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)unity_internals.h: $(UNITY_SRCDIR)$(DELIM)unity_internals.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)unity.h $(APPS_INCDIR)$(DELIM)unity_internals.h

clean::
	$(call DELDIR, build)

distclean::
	$(call DELDIR, $(UNITY_UNPACKNAME))
	$(call DELFILE, .downloaded)
	$(call DELFILE, $(UNITY_TARBALL))
	$(call DELFILE, $(APPDIR)/include/testing/unity.h)
	$(call DELFILE, $(APPDIR)/include/testing/unity_internals.h)

include $(APPDIR)/Application.mk
