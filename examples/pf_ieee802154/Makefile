############################################################################
# apps/examples/pf_ieee802154/Makefile
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

# PF_IEEE802154 Socket Test

TARGCMN_CSRCS = pf_cmdline.c
TARGCMN_COBJS = $(TARGCMN_CSRCS:.c=$(OBJEXT))

# Target 1

TARG1_CRCS =
TARG1_MAINSRC = pf_server.c

TARG1_COBJS = $(TARG1_CRCS:.c=$(OBJEXT))
TARG1_MAINOBJ = $(TARG1_MAINSRC:.c=$(OBJEXT))

CONFIG_EXAMPLES_PFIEEE802154_PROGNAME1 ?= pfserver
APPNAME1 = pfserver

CONFIG_EXAMPLES_PFIEEE802154_PRIORITY1 ?= 100
CONFIG_EXAMPLES_PFIEEE802154_STACKSIZE1 ?= 2048

PROGNAME1 = $(CONFIG_EXAMPLES_PFIEEE802154_PROGNAME1)
PRIORITY1 = $(CONFIG_EXAMPLES_PFIEEE802154_PRIORITY1)
STACKSIZE1 = $(CONFIG_EXAMPLES_PFIEEE802154_STACKSIZE1)

# Target 2

TARG2_CSRCS =
TARG2_MAINSRC = pf_client.c

TARG2_COBJS = $(TARG2_CSRCS:.c=$(OBJEXT))
TARG2_MAINOBJ = $(TARG2_MAINSRC:.c=$(OBJEXT))

CONFIG_EXAMPLES_PFIEEE802154_PROGNAME2 ?= pfclient
APPNAME2 = pfclient

CONFIG_EXAMPLES_PFIEEE802154_PRIORITY2 ?= 100
CONFIG_EXAMPLES_PFIEEE802154_STACKSIZE2 ?= 2048

PROGNAME2 = $(CONFIG_EXAMPLES_PFIEEE802154_PROGNAME2)
PRIORITY2 = $(CONFIG_EXAMPLES_PFIEEE802154_PRIORITY2)
STACKSIZE2 = $(CONFIG_EXAMPLES_PFIEEE802154_STACKSIZE2)

SRCS = $(TARG1_CRCS) $(TARG1_MAINSRC) $(TARG2_CSRCS) $(TARG2_MAINSRC) $(TARGCMN_CSRCS)
OBJS = $(TARG1_COBJS) $(TARG2_COBJS) $(TARGCMN_COBJS)
MAINOBJS = $(TARG1_MAINOBJ) $(TARG2_MAINOBJ)

MAINNAME1 = pfserver_main
MAINNAME2 = pfclient_main

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

$(OBJS) $(MAINOBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

$(BIN): $(OBJS) $(MAINOBJS)
	$(call ARCHIVE, $@, $(OBJS) $(MAINOBJS))

.built: $(BIN)
	$(Q) touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(PROGNAME1): $(OBJS) $(TARG1_MAINOBJ)
	@echo "LD: $(PROGNAME1)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME1) $(ARCHCRT0OBJ) $(TARG1_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME1)

$(BIN_DIR)$(DELIM)$(PROGNAME2): $(OBJS) $(TARG2_MAINOBJ)
	@echo "LD: $(PROGNAME2)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME2) $(ARCHCRT0OBJ) $(TARG2_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME2)

install: $(BIN_DIR)$(DELIM)$(PROGNAME1) $(BIN_DIR)$(DELIM)$(PROGNAME2)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME1)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME1),$(PRIORITY1),$(STACKSIZE1),$(if $(CONFIG_BUILD_LOADABLE),,$(MAINNAME1)))

$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME2)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME2),$(PRIORITY2),$(STACKSIZE2),$(if $(CONFIG_BUILD_LOADABLE),,$(MAINNAME2)))

context: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME1)_main.bdat \
	$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME2)_main.bdat
else
context:
endif

.depend: Makefile config.h $(SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call DELFILE, *.dSYM)
	$(call DELFILE, config.h)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

preconfig:

-include Make.dep
