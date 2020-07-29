############################################################################
# apps/platform/Makefile
#
#   Copyright (C) 2013-2014, 2016-2018 Gregory Nutt. All rights reserved.
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

include $(APPDIR)/Make.defs

CONFIG_ARCH_BOARD ?= dummy

# Directories

PLATFORMDIR = board
DUMMYDIR    = $(APPDIR)/platform/dummy

ifeq ($(CONFIG_ARCH_BOARD_CUSTOM),y)
  LINKDIR   = $(DUMMYDIR)
else
  BOARDDIR  = $(APPDIR)$(DELIM)platform$(DELIM)$(CONFIG_ARCH_BOARD)
  LINKDIR   = $(if $(wildcard $(BOARDDIR)$(DELIM)Make.defs),$(BOARDDIR),$(DUMMYDIR))
endif

VPATH       += board
DEPPATH += --obj-path bin
DEPPATH += --obj-suffix $(OBJEXT)
DEPPATH += --dep-path $(PLATFORMDIR)

# Binaries

-include $(PLATFORMDIR)$(DELIM)Make.defs

# Build context setup

$(TOPDIR)$(DELIM).config:

$(PLATFORMDIR): $(TOPDIR)$(DELIM).config
	@echo "LN: platform$(DELIM)board to $(LINKDIR)"
	$(Q) $(DIRLINK) $(LINKDIR) $(PLATFORMDIR)

dirlinks: $(PLATFORMDIR)

context:: dirlinks

clean_context:
	$(Q) $(DIRUNLINK) $(PLATFORMDIR)

clean::
	$(Q) $(MAKE) -C bin TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)" clean

include $(APPDIR)/Application.mk
