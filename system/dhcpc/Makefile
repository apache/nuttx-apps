############################################################################
# apps/system/dhcpc/Makefile
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

# DHCPC address renewal built-in application info

CONFIG_DHCPC_RENEW_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_DHCPC_RENEW_STACKSIZE ?= 2048

RENEW_APPNAME = renew
RENEW_PRIORITY = $(CONFIG_DHCPC_RENEW_PRIORITY)
RENEW_STACKSIZE = $(CONFIG_DHCPC_RENEW_STACKSIZE)

CONFIG_DHCPC_RENEW_PROGNAME ?= renew$(EXEEXT)
RENEW_PROGNAME = $(CONFIG_DHCPC_RENEW_PROGNAME)

# DHCPC address renewal

RENEW_SRCS =
RENEW_MAINSRC = renew_main.c

RENEW_OBJS = $(RENEW_SRCS:.c=$(OBJEXT))
RENEW_MAINOBJ = $(RENEW_MAINSRC:.c=$(OBJEXT))

# Other DHCPC commands go here

SRCS = $(RENEW_SRCS) $(RENEW_MAINSRC)
OBJS = $(RENEW_OBJS)

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  OBJS += $(RENEW_MAINOBJ)
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

$(RENEW_OBJS) $(RENEW_MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	@touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(RENEW_PROGNAME): $(OBJS) $(RENEW_MAINOBJ)
	@echo "LD: $(RENEW_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(RENEW_PROGNAME) $(ARCHCRT0OBJ) $(RENEW_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(RENEW_PROGNAME)

install: $(BIN_DIR)$(DELIM)$(RENEW_PROGNAME)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(RENEW_APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(RENEW_APPNAME),$(RENEW_PRIORITY),$(RENEW_STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,$(RENEW_APPNAME)_main))

context: $(BUILTIN_REGISTRY)$(DELIM)$(RENEW_APPNAME)_main.bdat
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
