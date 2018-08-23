############################################################################
# apps/examples/udp/Makefile
#
#   Copyright (C) 2007-2008, 2011-2012, 2014, 2017 Gregory Nutt. All rights reserved.
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

# UDP Test

TARGCMN_CSRCS = udp_cmdline.c
ifeq ($(CONFIG_EXAMPLES_UDP_NETINIT),y)
TARGCMN_CSRCS += udp_netinit.c
endif

TARGCMN_COBJS = $(TARGCMN_CSRCS:.c=$(OBJEXT))

# Target 1

TARG1_CRCS =
ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
TARG1_CRCS += udp_server.c
else
TARG1_CRCS += udp_client.c
endif
TARG1_MAINSRC = udp_target1.c

TARG1_COBJS = $(TARG1_CRCS:.c=$(OBJEXT))
TARG1_MAINOBJ = $(TARG1_MAINSRC:.c=$(OBJEXT))

ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
CONFIG_EXAMPLES_UDP_PROGNAME1 ?= udpserver
APPNAME1 = udpserver
else
CONFIG_EXAMPLES_UDP_PROGNAME1 ?= udpclient
APPNAME1 = udpclient
endif
CONFIG_EXAMPLES_UDP_PRIORITY1 ?= 100
CONFIG_EXAMPLES_UDP_STACKSIZE1 ?= 2048

PROGNAME1 = $(CONFIG_EXAMPLES_UDP_PROGNAME1)
PRIORITY1 = $(CONFIG_EXAMPLES_UDP_PRIORITY1)
STACKSIZE1 = $(CONFIG_EXAMPLES_UDP_STACKSIZE1)

# Target 2

ifeq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)

TARG2_CSRCS =
ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
TARG2_CSRCS += udp_client.c
else
TARG2_CSRCS += udp_server.c
endif
TARG2_MAINSRC = udp_target2.c

TARG2_COBJS = $(TARG2_CSRCS:.c=$(OBJEXT))
TARG2_MAINOBJ = $(TARG2_MAINSRC:.c=$(OBJEXT))

ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
CONFIG_EXAMPLES_UDP_PROGNAME2 ?= udpclient
APPNAME2 = udpclient
else
CONFIG_EXAMPLES_UDP_PROGNAME2 ?= udpserver
APPNAME2 = udpserver
endif
CONFIG_EXAMPLES_UDP_PRIORITY2 ?= 100
CONFIG_EXAMPLES_UDP_STACKSIZE2 ?= 2048

PROGNAME2 = $(CONFIG_EXAMPLES_UDP_PROGNAME2)
PRIORITY2 = $(CONFIG_EXAMPLES_UDP_PRIORITY2)
STACKSIZE2 = $(CONFIG_EXAMPLES_UDP_STACKSIZE2)

endif

TARG_SRCS = $(TARG1_CRCS) $(TARG1_MAINSRC) $(TARG2_CSRCS) $(TARG2_MAINSRC) $(TARGCMN_CSRCS)
TARG_OBJS = $(TARG1_COBJS) $(TARG2_COBJS) $(TARGCMN_COBJS)

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  TARG_OBJS += $(TARG1_MAINOBJ) $(TARG2_MAINOBJ)
endif

# Host

ifneq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)

HOSTCFLAGS += -DEXAMPLES_UDP_HOST=1
HOSTOBJSEXT ?= hobj

HOST_SRCS = udp_host.c udp_cmdline.c
ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
HOST_SRCS += udp_client.c
HOST_BIN = udpclient$(EXEEXT)
else
HOST_SRCS += udp_server.c
HOST_BIN = udpserver$(EXEEXT)
endif

HOST_OBJS = $(HOST_SRCS:.c=.$(HOSTOBJSEXT))
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

ifeq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
MAINNAME1 = udp1_main
MAINNAME2 = udp2_main
else
MAINNAME1 = udp_main
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

$(TARG1_COBJS) $(TARG1_MAINOBJ) $(TARG2_COBJS) $(TARG2_MAINOBJ) $(TARGCMN_COBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

$(TARG_BIN): $(TARG_OBJS) $(HOST_BIN)
	$(call ARCHIVE, $@, $(TARG_OBJS))

ifneq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
$(HOST_OBJS): %.$(HOSTOBJSEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@
endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
$(HOST_BIN): config.h $(HOST_OBJS)
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@
endif

.built: config.h $(TARG_BIN) $(HOST_BIN)
	$(Q) touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
$(BIN_DIR)$(DELIM)$(PROGNAME1): $(OBJS) $(TARG1_MAINOBJ)
	@echo "LD: $(PROGNAME1)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME1) $(ARCHCRT0OBJ) $(TARG1_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME1)

install: $(BIN_DIR)$(DELIM)$(PROGNAME1)

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME1)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME1),$(PRIORITY1),$(STACKSIZE1),$(if $(CONFIG_BUILD_LOADABLE),,$(MAINNAME1)))

ifeq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME2)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME2),$(PRIORITY2),$(STACKSIZE2),$(if $(CONFIG_BUILD_LOADABLE),,$(MAINNAME2)))

context: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME1)_main.bdat \
	$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME2)_main.bdat
else
context: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME1)_main.bdat
endif
else
context:
endif

.depend: Makefile config.h $(TARG_SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(TARG_SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
ifneq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.$(HOSTOBJSEXT))
endif
	$(call DELFILE, *.dSYM)
	$(call DELFILE, config.h)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

preconfig:

-include Make.dep
