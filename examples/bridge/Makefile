############################################################################
# apps/examples/bridge/Makefile
#
#   Copyright (C) 2014 Gregory Nutt. All rights reserved.
#
#   Authors: Gregory Nutt <gnutt@nuttx.org>
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

# Discover built-in application info

TARG_APPNAME = bridge
TARG_PRIORITY = SCHED_PRIORITY_DEFAULT
TARG_STACKSIZE = 2048

TARG_ASRCS =
TARG_CSRCS =
MAINSRC = bridge_main.c

TARG_AOBJS = $(TARG_ASRCS:.S=$(OBJEXT))
TARG_COBJS = $(TARG_CSRCS:.c=$(OBJEXT))
TARG_MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

TARG_SRCS = $(TARG_ASRCS) $(TARG_CSRCS) $(MAINSRC)
TARG_OBJS = $(TARG_AOBJS) $(TARG_COBJS)

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  TARG_OBJS += $(TARG_MAINOBJ)
endif

HOST_SRCS1 = host_net1.c
HOST_SRCS2 = host_net2.c
HOST_SRCS  = $(HOST_SRCS1) $(HOST_SRCS2)

HOST_OBJS1 = $(HOST_SRCS1:.c=.o)
HOST_OBJS2 = $(HOST_SRCS2:.c=.o)
HOST_OBJS  = $(HOST_OBJS1) $(HOST_OBJS2)

HOST_BIN1 = host1
HOST_BIN2 = host2

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  TARG_BIN = ..\..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  TARG_BIN = ..\\..\\libapps$(LIBEXT)
else
  TARG_BIN = ../../libapps$(LIBEXT)
endif
endif

ifeq ($(WINTOOL),y)
  TARG_INSTALL_DIR = "${shell cygpath -w $(BIN_DIR)}"
else
  TARG_INSTALL_DIR = $(BIN_DIR)
endif

CONFIG_EXAMPLES_BRIDGE_PROGNAME ?= bridge$(EXEEXT)
TARG_PROGNAME = $(CONFIG_EXAMPLES_BRIDGE_PROGNAME)

ROOTDEPPATH = --dep-path .

# Common build

VPATH =

all: .built
.PHONY: clean depend distclean preconfig
.PRECIOUS: ../../libapps$(LIBEXT)

$(TARG_AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(TARG_COBJS) $(TARG_MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

$(HOST_OBJS): %.o: %.c
	@echo "CC: $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

$(TARG_BIN): $(TARG_OBJS)
	$(call ARCHIVE, $(TARG_BIN), $(TARG_OBJS))

bridge_config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP: brigetconfig.h"
	$(Q) cp $(TOPDIR)/include/nuttx/config.h bridge_config.h

$(HOST_BIN1): bridge_config.h $(HOST_OBJS1)
	@echo "LD: $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS1) -o $@

$(HOST_BIN2): bridge_config.h $(HOST_OBJS2)
	@echo "LD: $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS2) -o $@

.built: $(TARG_BIN) $(HOST_BIN1) $(HOST_BIN2)
	@touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(TARG_PROGNAME): $(TARG_OBJS) $(TARG_MAINOBJ)
	@echo "LD: $(TARG_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(TARG_INSTALL_DIR)$(DELIM)$(TARG_PROGNAME) $(ARCHCRT0OBJ) $(TARG_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(TARG_INSTALL_DIR)$(DELIM)$(TARG_PROGNAME)

install: $(BIN_DIR)$(DELIM)$(TARG_PROGNAME)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(TARG_APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(TARG_APPNAME),$(TARG_PRIORITY),$(TARG_STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,$(TARG_APPNAME)_main))

context: $(BUILTIN_REGISTRY)$(DELIM)$(TARG_APPNAME)_main.bdat
else
context:
endif

.depend: Makefile $(TARG_SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(TARG_SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call DELFILE, $(HOST_BIN1))
	$(call DELFILE, $(HOST_BIN2))
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)
	$(call DELFILE, bridge_config.h)

preconfig:

-include Make.dep
