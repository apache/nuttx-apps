############################################################################
# examples/udpblaster/Makefile
#
#   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

# Basic TCP networking test

TARG_ASRCS =
TARG_AOBJS = $(TARG_ASRCS:.S=$(OBJEXT))

TARG_CSRCS = udpblaster_text.c
TARG_MAINSRC = udpblaster_target.c

TARG_COBJS = $(TARG_CSRCS:.c=$(OBJEXT))
TARG_MAINOBJ = $(TARG_MAINSRC:.c=$(OBJEXT))

TARG_SRCS = $(TARG_ASRCS) $(TARG_CSRCS) $(TARG_CSRCS)
TARG_OBJS = $(TARG_AOBJS) $(TARG_COBJS)

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  TARG_OBJS += $(TARG_MAINOBJ)
endif

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  TARG_BIN = ..\..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  TARG_BIN = ..\\..\\libapps$(LIBEXT)
else
  TARG_BIN = ../../libapps$(LIBEXT)
endif
endif

HOSTCFLAGS += -DUDPBLASTER_HOST=1

HOST_SRCS = udpblaster_host.c udpblaster_text.c

HOSTOBJEXT ?= .hobj
HOST_OBJS = $(HOST_SRCS:.c=$(HOSTOBJEXT))
HOST_BIN = host

ifeq ($(WINTOOL),y)
  INSTALL_DIR = "${shell cygpath -w $(BIN_DIR)}"
else
  INSTALL_DIR = $(BIN_DIR)
endif

CONFIG_UPBLASTER_PROGNAME ?= udpblaster$(EXEEXT)
PROGNAME = $(CONFIG_UPBLASTER_PROGNAME)

ROOTDEPPATH = --dep-path .

# NET test built-in application info

CONFIG_EXAMPLES_UDPBLASTER_STACKSIZE ?= 2048
CONFIG_EXAMPLES_UDPBLASTER_PRIORITY ?= 100

APPNAME = udpblaster
PRIORITY = $(CONFIG_EXAMPLES_UDPBLASTER_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_UDPBLASTER_STACKSIZE)

# Common build

VPATH =

all: .built $(HOST_BIN)
.PHONY: clean depend distclean preconfig
.PRECIOUS: ../../libapps$(LIBEXT)

$(TARG_AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(TARG_COBJS) $(TARG_MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
$(HOST_OBJS): %$(HOSTOBJEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@
endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
$(HOST_BIN): config.h $(HOST_OBJS)
	@echo "LD:  $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@
endif

.built: config.h $(TARG_OBJS)
	$(call ARCHIVE, $(TARG_BIN), $(TARG_OBJS))
	$(Q) touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(PROGNAME): $(OBJS) $(TARG_MAINOBJ)
	@echo "LD: $(PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME) $(ARCHCRT0OBJ) $(TARG_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME)

install: $(BIN_DIR)$(DELIM)$(PROGNAME)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME),$(PRIORITY),$(STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,$(APPNAME)_main))

context: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat
else
context:
endif

.depend: Makefile config.h $(TARG_SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(TARG_SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
	$(call DELFILE, *$(HOSTOBJEXT))
	$(call DELFILE, $(HOST_BIN))
endif
	$(call DELFILE, .built)
	$(call DELFILE, *.dSYM)
	$(call DELFILE, config.h)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

preconfig:

-include Make.dep
