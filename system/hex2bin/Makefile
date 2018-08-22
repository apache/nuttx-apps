############################################################################
# apps/system/hex2bin/Makefile
#
#   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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
include $(APPDIR)/Make.defs

# Hex2bin utility

ASRCS =
CSRCS =

ifeq ($(CONFIG_BUILD_LOADABLE),y)

HEX2BIN_MAINSRC = hex2bin_main.c
HEX2MEM_MAINSRC = hex2mem_main.c

else

ifeq ($(CONFIG_SYSTEM_HEX2BIN_BUILTIN),y)
HEX2BIN_MAINSRC = hex2bin_main.c
endif

ifeq ($(CONFIG_SYSTEM_HEX2MEM_BUILTIN),y)
HEX2MEM_MAINSRC = hex2mem_main.c
endif

endif

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
HEX2BIN_MAINOBJ = $(HEX2BIN_MAINSRC:.c=$(OBJEXT))
HEX2MEM_MAINOBJ = $(HEX2MEM_MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)
MAINOBJ = $(HEX2BIN_MAINOBJ) $(HEX2MEM_MAINOBJ)

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  OBJS += $(MAINOBJ)
endif

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  BIN = ..\..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  BIN = ..\\..\\libapps$(LIBEXT)
else
  BIN = ../../libapps$(LIBEXT)
endif
endif

ifeq ($(WINTOOL),y)
  INSTALL_DIR = "${shell cygpath -w $(BIN_DIR)}"
else
  INSTALL_DIR = $(BIN_DIR)
endif

CONFIG_SYSTEM_HEX2BIN_PROGNAME ?= hex2bin$(EXEEXT)
HEX2BIN_PROGNAME = $(CONFIG_SYSTEM_HEX2BIN_PROGNAME)

CONFIG_SYSTEM_HEX2MEM_PROGNAME ?= hex2mem$(EXEEXT)
HEX2MEM_PROGNAME = $(CONFIG_SYSTEM_HEX2MEM_PROGNAME)

ROOTDEPPATH = --dep-path .
VPATH =

CONFIG_SYSTEM_HEX2BIN_STACKSIZE ?= 1536
CONFIG_SYSTEM_HEX2BIN_PRIORITY ?= 100

HEX2BIN_STACKSIZE = $(CONFIG_SYSTEM_HEX2BIN_STACKSIZE)
HEX2BIN_PRIORITY = $(CONFIG_SYSTEM_HEX2BIN_PRIORITY)

CONFIG_SYSTEM_HEX2MEM_STACKSIZE ?= 1536
CONFIG_SYSTEM_HEX2MEM_PRIORITY ?= 100

HEX2MEM_STACKSIZE = $(CONFIG_SYSTEM_HEX2MEM_STACKSIZE)
HEX2MEM_PRIORITY = $(CONFIG_SYSTEM_HEX2MEM_PRIORITY)

# Build targets

all: .built
.PHONY: context .depend depend clean distclean preconfig
.PRECIOUS: ../../libapps$(LIBEXT)

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	$(Q) touch .built

# Register application(s)

ifeq ($(CONFIG_SYSTEM_HEX2BIN_BUILTIN),y)
$(BUILTIN_REGISTRY)$(DELIM)hex2bin.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,"hex2bin",$(HEX2BIN_PRIORITY),$(HEX2BIN_STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,hex2bin_main))

rhex2bin: $(BUILTIN_REGISTRY)$(DELIM)hex2bin.bdat
else
rhex2bin:
endif

ifeq ($(CONFIG_SYSTEM_HEX2MEM_BUILTIN),y)
$(BUILTIN_REGISTRY)$(DELIM)hex2mem.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,"hex2mem",$(HEX2MEM_PRIORITY),$(HEX2MEM_STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,hex2mem_main))

rhex2mem: $(BUILTIN_REGISTRY)$(DELIM)hex2mem.bdat
else
rhex2mem:
endif

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(HEX2BIN_PROGNAME): $(OBJS) $(HEX2BIN_MAINOBJ)
	@echo "LD: $(HEX2BIN_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(HEX2BIN_PROGNAME) $(ARCHCRT0OBJ) $(HEX2BIN_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(HEX2BIN_PROGNAME)

$(BIN_DIR)$(DELIM)$(HEX2MEM_PROGNAME): $(OBJS) $(HEX2MEM_MAINOBJ)
	@echo "LD: $(HEX2MEM_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(HEX2MEM_PROGNAME) $(ARCHCRT0OBJ) $(HEX2MEM_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(HEX2MEM_PROGNAME)

install: $(BIN_DIR)$(DELIM)$(HEX2BIN_PROGNAME) $(BIN_DIR)$(DELIM)$(HEX2MEM_PROGNAME)

else
install:

endif

context: rhex2bin rhex2mem

.depend: Makefile $(SRCS)
	$(Q) $(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	$(Q) touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

preconfig:

-include Make.dep
