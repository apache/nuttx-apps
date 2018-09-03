############################################################################
# apps/canutils/libcanard/Makefile
#
#   Copyright (C) 2016 Omni Hoverboards Inc. All rights reserved.
#   Authors: Paul Alexander Patience <paul-a.patience@polymtl.ca>
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

-include $(TOPDIR)/Make.defs

WGET = wget
UNPACK = unzip
PACKEXT = .zip

LIBCANARD_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARD_URL)))
LIBCANARD_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARD_VERSION)))
LIBCANARD_UNPACKNAME = libcanard-$(LIBCANARD_VERSION)
LIBCANARD_PACKNAME = $(LIBCANARD_UNPACKNAME)$(PACKEXT)
LIBCANARD_SRCDIR = $(LIBCANARD_UNPACKNAME)
LIBCANARD_DRVDIR = $(LIBCANARD_SRCDIR)$(DELIM)drivers$(DELIM)nuttx

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)canutils

CFLAGS += -std=c99 -I$(APPS_INCDIR) -DCANARD_ASSERT=DEBUGASSERT

CSRCS = $(LIBCANARD_SRCDIR)$(DELIM)canard.c $(LIBCANARD_DRVDIR)$(DELIM)canard_nuttx.c

$(LIBCANARD_PACKNAME):
	@echo "Downloading: $@"
	$(Q) $(WGET) -O $@ $(LIBCANARD_URL)$(DELIM)$(LIBCANARD_VERSION)$(PACKEXT)

$(LIBCANARD_UNPACKNAME): $(LIBCANARD_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(LIBCANARD_SRCDIR)$(DELIM)canard.h: $(LIBCANARD_UNPACKNAME)

$(LIBCANARD_DRVDIR)$(DELIM)canard_nuttx.h: $(LIBCANARD_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)canard.h: $(LIBCANARD_SRCDIR)$(DELIM)canard.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)canard_nuttx.h: $(LIBCANARD_DRVDIR)$(DELIM)canard_nuttx.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)canard.h $(APPS_INCDIR)$(DELIM)canard_nuttx.h

clean::
	$(foreach OBJ, $(OBJS), $(call DELFILE, $(OBJ)))

distclean::
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard.h)
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard_nuttx.h)
	$(call DELDIR, $(LIBCANARD_UNPACKNAME))
	$(call DELFILE, $(LIBCANARD_PACKNAME))

include $(APPDIR)/Application.mk
