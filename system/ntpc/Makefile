############################################################################
# apps/system/ntpc/Makefile
#
#   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

# NTPC address renewal built-in application info

CONFIG_SYSTEM_NTPC_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_SYSTEM_NTPC_STACKSIZE ?= 2048

APPNAME1 = ntpstart
APPNAME2 = ntpstop
PRIORITY = $(CONFIG_SYSTEM_NTPC_PRIORITY)
STACKSIZE = $(CONFIG_SYSTEM_NTPC_STACKSIZE)

PROGNAME1 = ntpstart$(EXEEXT)
PROGNAME2 = ntpstop$(EXEEXT)

# NTPC address renewal

ASRCS =
CSRCS =
MAINSRC1 = ntpcstart_main.c
MAINSRC2 = ntpcstop_main.c
MAINSRC = $(MAINSRC1) $(MAINSRC2)

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
MAINOBJ1 = $(MAINSRC1:.c=$(OBJEXT))
MAINOBJ2 = $(MAINSRC2:.c=$(OBJEXT))
MAINOBJ = $(MAINOBJ1) $(MAINOBJ2)

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
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
.PHONY: clean depend distclean preconfig
.PRECIOUS: ../../libapps$(LIBEXT)

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	@touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(PROGNAME1): $(OBJS) $(MAINOBJ1)
	@echo "LD: $(PROGNAME1)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME1) $(ARCHCRT0OBJ) $(MAINOBJ1) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME1)

$(BIN_DIR)$(DELIM)$(PROGNAME2): $(OBJS) $(MAINOBJ2)
	@echo "LD: $(PROGNAME2)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME2) $(ARCHCRT0OBJ) $(MAINOBJ2) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME2)

install: $(BIN_DIR)$(DELIM)$(PROGNAME1)  $(BIN_DIR)$(DELIM)$(PROGNAME2)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME1)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME1),$(PRIORITY),$(STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,$(APPNAME1)_main))

$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME2)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME2),$(PRIORITY),$(STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,$(APPNAME2)_main))

context: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME1)_main.bdat $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME2)_main.bdat
else
context:
endif

.depend: Makefile $(SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

preconfig:

-include Make.dep
