############################################################################
# examples/tcpblaster/Makefile
#
#   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

TARGCMN_CSRCS = tcpblaster_cmdline.c
ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_INIT),y)
TARGCMN_CSRCS += tcpblaster_netinit.c
endif

TARGCMN_COBJS = $(TARGCMN_CSRCS:.c=$(OBJEXT))

# Target 1 Files

TARG1_CSRCS =
ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)
TARG1_CSRCS += tcpblaster_server.c tcpblaster_client.c
else ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
TARG1_CSRCS += tcpblaster_server.c
else
TARG1_CSRCS += tcpblaster_client.c
endif
TARG1_MAINSRC = tcpblaster_target1.c

TARG1_COBJS = $(TARG1_CSRCS:.c=$(OBJEXT))
TARG1_MAINOBJ = $(TARG1_MAINSRC:.c=$(OBJEXT))

TARG_CSRCS = $(TARG1_CSRCS) $(TARG1_CSRCS) $(TARGCMN_CSRCS)
TARG_OBJS = $(TARG1_COBJS) $(TARGCMN_COBJS)

# Target 1 Application Info

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
CONFIG_EXAMPLES_TCPBLASTER_PROGNAME1 ?= tcpserver
APPNAME1 = tcpserver
else
CONFIG_EXAMPLES_TCPBLASTER_PROGNAME1 ?= tcpclient
APPNAME1 = tcpclient
endif
CONFIG_EXAMPLES_TCPBLASTER_PRIORITY1 ?= 100
CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE1 ?= 2048

PROGNAME1 = $(CONFIG_EXAMPLES_TCPBLASTER_PROGNAME1)
PRIORITY1 = $(CONFIG_EXAMPLES_TCPBLASTER_PRIORITY1)
STACKSIZE1 = $(CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE1)

# Target 2

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)

TARG2_CSRCS =
ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
TARG2_CSRCS += tcpblaster_client.c
else
TARG2_CSRCS += tcpblaster_server.c
endif
TARG2_MAINSRC = tcpblaster_target2.c

TARG2_COBJS = $(TARG2_CSRCS:.c=$(OBJEXT))
TARG2_MAINOBJ = $(TARG2_MAINSRC:.c=$(OBJEXT))

# Target 2 Application Info

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
CONFIG_EXAMPLES_TCPBLASTER_PROGNAME2 ?= tcpclient
APPNAME2 = tcpclient
else
CONFIG_EXAMPLES_TCPBLASTER_PROGNAME2 ?= tcpserver
APPNAME2 = tcpserver
endif
CONFIG_EXAMPLES_TCPBLASTER_PRIORITY2 ?= 100
CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE2 ?= 2048

PROGNAME2 = $(CONFIG_EXAMPLES_TCPBLASTER_PROGNAME2)
PRIORITY2 = $(CONFIG_EXAMPLES_TCPBLASTER_PRIORITY2)
STACKSIZE2 = $(CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE2)

endif

# All targets

TARG_SRCS = $(TARG1_CRCS) $(TARG1_MAINSRC) $(TARG2_CSRCS) $(TARG2_MAINSRC) $(TARGCMN_CSRCS)
TARG_OBJS = $(TARG1_COBJS) $(TARG2_COBJS) $(TARGCMN_COBJS)

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  TARG_OBJS += $(TARG1_MAINOBJ) $(TARG2_MAINOBJ)
endif

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
MAINNAME1 = tcpblaster1_main
MAINNAME2 = tcpblaster2_main
else
MAINNAME1 = tcpblaster_main
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

# Host

ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)

  HOSTCFLAGS += -DTCPBLASTER_HOST=1
  ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
    HOSTCFLAGS += -DCONFIG_EXAMPLES_TCPBLASTER_SERVER=1 -DCONFIG_EXAMPLES_TCPBLASTER_SERVERIP="$(CONFIG_EXAMPLES_TCPBLASTER_SERVERIP)"
  endif

  HOST_SRCS = tcpblaster_host.c
  ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
    HOST_SRCS += tcpblaster_client.c
    HOST_BIN = tcpclient$(EXEEXT)
  else
    HOST_SRCS += tcpblaster_server.c
    HOST_BIN = tcpserver$(EXEEXT)
  endif

  HOSTOBJEXT ?= hobj
  HOST_OBJS = $(HOST_SRCS:.c=.$(HOSTOBJEXT))

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

all: .built $(HOST_BIN)
.PHONY: clean depend distclean preconfig
.PRECIOUS: ../../libapps$(LIBEXT)

$(TARG1_COBJS) $(TARG1_MAINOBJ) $(TARG2_COBJS) $(TARG2_MAINOBJ) $(TARGCMN_COBJS) : %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)

$(HOST_OBJS): %.$(HOSTOBJEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

endif
endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)

$(HOST_BIN): config.h $(HOST_OBJS)
	@echo "LD:  $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@

endif
endif

.built: config.h $(TARG_OBJS)
	$(call ARCHIVE, $(TARG_BIN), $(TARG_OBJS))
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

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
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

.depend: Makefile config.h $(TARG_CSRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(TARG_CSRCS) >Make.dep
	@touch $@

depend: .depend

clean:
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)
	$(call DELFILE, *.$(HOSTOBJEXT))
	$(call DELFILE, $(HOST_BIN))
endif
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
