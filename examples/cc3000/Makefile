############################################################################
# apps/examples/cc3000/Makefile
#
#   Copyright (C) 2008, 2010-2013 Gregory Nutt. All rights reserved.
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

# Hello, World! built-in application info

APPNAME = c3b
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = 608

APPNAME1 = shell
PRIORITY1 = SCHED_PRIORITY_DEFAULT
STACKSIZE1 = 980

# Hello, World! Example

ASRCS =
CSRCS = board.c telnetd_driver.c telnetd_daemon.c

CC3000SRC = cc3000basic.c
SHELLSRC = shell.c
MAINSRC = $(CC3000SRC) $(SHELLSRC)

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

CC3000OBJ = $(CC3000SRC:.c=$(OBJEXT))
SHELLOBJ = $(SHELLSRC:.c=$(OBJEXT))
MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

ifneq ($(CONFIG_BUILD_KERNEL),y)
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

CONFIG_XYZ_PROGNAME1 ?= cc3000$(EXEEXT)
PROGNAME1 = $(CONFIG_XYZ_PROGNAME1)

CONFIG_XYZ_PROGNAME2 ?= shell$(EXEEXT)
PROGNAME2 = $(CONFIG_XYZ_PROGNAME2)

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

ifeq ($(CONFIG_BUILD_KERNEL),y)
$(BIN_DIR)$(DELIM)$(PROGNAME1): $(OBJS) $(CC3000OBJ)
	@echo "LD: $(PROGNAME1)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME1) $(ARCHCRT0OBJ) $(MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME1)

$(BIN_DIR)$(DELIM)$(PROGNAME2): $(OBJS) $(SHELLOBJ)
	@echo "LD: $(PROGNAME2)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME2) $(ARCHCRT0OBJ) $(MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME2)

install: $(BIN_DIR)$(DELIM)$(PROGNAME1)  $(BIN_DIR)$(DELIM)$(PROGNAME2)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME),$(PRIORITY),$(STACKSIZE),$(APPNAME)_main)
	$(call REGISTER,$(APPNAME1),$(PRIORITY1),$(STACKSIZE1),$(APPNAME1)_main)

context: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat
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
