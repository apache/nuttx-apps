############################################################################
# apps/system/zmodem/Makefile
#
#   Copyright (C) 2013, 2016 Gregory Nutt. All rights reserved.
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

# Zmodem sz and rz commands

CONFIG_SYSTEM_SZPROGNAME ?= sz$(EXEEXT)
SZ_PROGNAME = $(CONFIG_SYSTEM_SZPROGNAME)

CONFIG_SYSTEM_RZPROGNAME ?= rz$(EXEEXT)
RZ_PROGNAME = $(CONFIG_SYSTEM_RZPROGNAME)

CONFIG_SYSTEM_ZMODEM_PRIORITY ?= 100
CONFIG_SYSTEM_ZMODEM_STACKSIZE ?= 2048

PRIORITY = $(CONFIG_SYSTEM_ZMODEM_PRIORITY)
STACKSIZE = $(CONFIG_SYSTEM_ZMODEM_STACKSIZE)

# Source/object files

ASRCS  =

CSRCS  = zm_send.c zm_receive.c zm_state.c zm_proto.c zm_watchdog.c
CSRCS += zm_utils.c
SZ_MAINSRC = sz_main.c
RZ_MAINSRC = rz_main.c

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
SZ_MAINOBJ = $(SZ_MAINSRC:.c=$(OBJEXT))
RZ_MAINOBJ = $(RZ_MAINSRC:.c=$(OBJEXT))
MAINOBJ = $(SZ_MAINOBJ) $(RZ_MAINOBJ)

SRCS = $(ASRCS) $(CSRCS) $(SZ_MAINSRC) $(RZ_MAINSRC)
OBJS = $(AOBJS) $(COBJS)

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

ROOTDEPPATH = --dep-path .

# Common build

VPATH =

all: .built
.PHONY: context depend clean distclean preconfig
.PRECIOUS: ../../libapps$(LIBEXT)

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	$(Q) touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(SZ_PROGNAME): $(OBJS) $(SZ_MAINOBJ)
	@echo "LD: $(SZ_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(SZ_PROGNAME) $(ARCHCRT0OBJ) $(SZ_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(SZ_PROGNAME)

$(BIN_DIR)$(DELIM)$(RZ_PROGNAME): $(OBJS) $(RZ_MAINOBJ)
	@echo "LD: $(RZ_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(RZ_PROGNAME) $(ARCHCRT0OBJ) $(RZ_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(RZ_PROGNAME)

install: $(BIN_DIR)$(DELIM)$(SZ_PROGNAME) $(BIN_DIR)$(DELIM)$(RZ_PROGNAME)

else
install:

endif

# Register application

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)sz.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,"sz",$(PRIORITY),$(STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,sz_main))

$(BUILTIN_REGISTRY)$(DELIM)rz.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,"rz",$(PRIORITY),$(STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,rz_main))

context: $(BUILTIN_REGISTRY)$(DELIM)sz.bdat $(BUILTIN_REGISTRY)$(DELIM)rz.bdat
else
context:
endif

# Create dependencies

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
