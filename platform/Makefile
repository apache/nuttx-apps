############################################################################
# apps/platform/Makefile
#
#   Copyright (C) 2013-2014 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
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

DELIM ?= $(strip /)
include $(APPDIR)$(DELIM)Make.defs

# Platform-specific Appliction Support
# Tool stuff

ifeq ($(DIRLINK),)
ifeq ($(CONFIG_WINDOWS_NATIVE),y)
DIRLINK = $(TOPDIR)/tools/link.bat
DIRUNLINK = $(TOPDIR)/tools/unlink.bat
else
DIRLINK = $(TOPDIR)/tools/link.sh
DIRUNLINK = $(TOPDIR)/tools/unlink.sh
endif
endif

ifeq ($(WINTOOL),y)
INCDIROPT = -w
endif

# Directories

PLATFORMDIR = $(APPDIR)$(DELIM)platform$(DELIM)board
DUMMYDIR    = $(APPDIR)$(DELIM)platform$(DELIM)dummy

ifeq ($(CONFIG_ARCH_BOARD_CUSTOM),y)
  LINKDIR   = $(DUMMYDIR)
else
  BOARDDIR  = $(APPDIR)$(DELIM)platform$(DELIM)$(CONFIG_ARCH_BOARD)
  LINKDIR   = $(if $(wildcard $(BOARDDIR)$(DELIM)Make.defs),$(BOARDDIR),$(DUMMYDIR))
endif

VPATH       = board

# Binaries

ASRCS =
CSRCS =

-include $(PLATFORMDIR)$(DELIM)Make.defs

AOBJS = $(patsubst %.S, bin$(DELIM)%$(OBJEXT), $(ASRCS))
COBJS = $(patsubst %.c, bin$(DELIM)%$(OBJEXT), $(CSRCS))

SRCS = $(ASRCS) $(CSRCS)
OBJS = $(AOBJS) $(COBJS)

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  BIN = ..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  BIN = ..\\libapps$(LIBEXT)
else
  BIN = ../libapps$(LIBEXT)
endif
endif

# Build targets

all: .built
.PHONY: context .depend depend clean distclean

ifneq ($(ASRCS),)
$(AOBJS): bin$(DELIM)%$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)
endif

ifneq ($(CSRCS),)
$(COBJS): bin$(DELIM)%$(OBJEXT): %.c
	$(call COMPILE, $<, $@)
endif

.built: $(OBJS)
ifneq ($(SRCS),)
	$(call ARCHIVE, $(BIN), $(OBJS))
endif
	$(Q) touch .built

# Build context setup

$(PLATFORMDIR): $(TOPDIR)$(DELIM).config
	@echo "LN: platform$(DELIM)board to $(LINKDIR)"
	$(Q) $(DIRUNLINK) $(PLATFORMDIR)
	$(Q) $(DIRLINK) $(LINKDIR) $(PLATFORMDIR)

install:

context: $(PLATFORMDIR)

# Dependencies

.depend: Makefile $(SRCS) $(PLATFORMDIR)
	$(Q) $(MKDEP) --obj-path bin --obj-suffix  $(OBJEXT) --dep-path $(PLATFORMDIR) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	$(Q) touch $@

depend: .depend

# Clean targets

clean: $(PLATFORMDIR)
	$(call DELFILE, .built)
	$(Q) $(MAKE) -C bin TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" clean

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)
	$(Q) $(DIRUNLINK) $(PLATFORMDIR)

-include Make.dep

