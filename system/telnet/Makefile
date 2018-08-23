############################################################################
# apps/system/telnet/Makefile
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

# Telnet Programs

ifeq ($(CONFIG_SYSTEM_TELNET_CHATD),y)

# Chatd files

CHATD_CSRCS =
CHATD_MAINSRC = telnet_chatd.c

CHATD_COBJS = $(CHATD_CSRCS:.c=$(OBJEXT))
CHATD_MAINOBJ = $(CHATD_MAINSRC:.c=$(OBJEXT))

# Chatd Application Info

CONFIG_SYSTEM_TELNET_CHATD_PROGNAME ?= chatd
CONFIG_SYSTEM_TELNET_CHATD_PRIORITY ?= 100
CONFIG_SYSTEM_TELNET_CHATD_STACKSIZE ?= 2048

CHATD_MAINNAME = chatd_main
CHATD_APPNAME = chatd
CHATD_PROGNAME = $(CONFIG_SYSTEM_TELNET_CHATD_PROGNAME)
CHATD_PRIORITY = $(CONFIG_SYSTEM_TELNET_CHATD_PRIORITY)
CHATD_STACKSIZE = $(CONFIG_SYSTEM_TELNET_CHATD_STACKSIZE)

endif

ifeq ($(CONFIG_SYSTEM_TELNET_CLIENT),y)

# Telnet client files

CLIENT_CSRCS =
CLIENT_MAINSRC = telnet_client.c

CLIENT_COBJS = $(CLIENT_CSRCS:.c=$(OBJEXT))
CLIENT_MAINOBJ = $(CLIENT_MAINSRC:.c=$(OBJEXT))

# Telnet client Application Info

CONFIG_SYSTEM_TELNET_CLIENT_PROGNAME ?= tcpclient
CONFIG_SYSTEM_TELNET_CLIENT_PRIORITY ?= 100
CONFIG_SYSTEM_TELNET_CLIENT_STACKSIZE ?= 2048

CLIENT_MAINNAME = telnet_main
CLIENT_APPNAME = telnet
CLIENT_PROGNAME = $(CONFIG_SYSTEM_TELNET_CLIENT_PROGNAME)
CLIENT_PRIORITY = $(CONFIG_SYSTEM_TELNET_CLIENT_PRIORITY)
CLIENT_STACKSIZE = $(CONFIG_SYSTEM_TELNET_CLIENT_STACKSIZE)

endif

# All programns

TARG_SRCS = $(CHATD_CRCS) $(CHATD_MAINSRC) $(CLIENT_CSRCS) $(CLIENT_MAINSRC)
TARG_OBJS = $(CHATD_COBJS) $(CLIENT_COBJS)

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  TARG_OBJS += $(CHATD_MAINOBJ) $(CLIENT_MAINOBJ)
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

$(CHATD_COBJS) $(CHATD_MAINOBJ) $(CLIENT_COBJS) $(CLIENT_MAINOBJ) : %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(TARG_OBJS)
	$(call ARCHIVE, $(TARG_BIN), $(TARG_OBJS))
	$(Q) touch .built

ifeq ($(CONFIG_BUILD_LOADABLE),y)
ifeq ($(CONFIG_SYSTEM_TELNET_CHATD),y)

$(BIN_DIR)$(DELIM)$(CHATD_PROGNAME): $(OBJS) $(CHATD_MAINOBJ)
	@echo "LD: $(CHATD_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(CHATD_PROGNAME) $(ARCHCRT0OBJ) $(CHATD_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(CHATD_PROGNAME)

chatd_install: $(BIN_DIR)$(DELIM)$(CHATD_PROGNAME)
else
chatd_install:
endif

ifeq ($(CONFIG_SYSTEM_TELNET_CLIENT),y)

$(BIN_DIR)$(DELIM)$(CLIENT_PROGNAME): $(OBJS) $(CLIENT_MAINOBJ)
	@echo "LD: $(CLIENT_PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(CLIENT_PROGNAME) $(ARCHCRT0OBJ) $(CLIENT_MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(CLIENT_PROGNAME)

client_install: $(BIN_DIR)$(DELIM)$(CLIENT_PROGNAME)
else
client_install:
endif

install: chatd_install client_install

else
install:

endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
ifeq ($(CONFIG_SYSTEM_TELNET_CHATD),y)

$(BUILTIN_REGISTRY)$(DELIM)$(CHATD_APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(CHATD_APPNAME),$(CHATD_PRIORITY),$(CHATD_STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,$(CHATD_MAINNAME)))

chatd_register: $(BUILTIN_REGISTRY)$(DELIM)$(CHATD_APPNAME)_main.bdat
else
chatd_register:
endif

ifeq ($(CONFIG_SYSTEM_TELNET_CLIENT),y)
$(BUILTIN_REGISTRY)$(DELIM)$(CLIENT_APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(CLIENT_APPNAME),$(CLIENT_PRIORITY),$(CLIENT_STACKSIZE),$(if $(CONFIG_BUILD_LOADABLE),,$(CLIENT_MAINNAME)))

client_register: $(BUILTIN_REGISTRY)$(DELIM)$(CLIENT_APPNAME)_main.bdat
else
client_register:
endif

context: chatd_register client_register
else
context:
endif

.depend: Makefile $(TARG_CSRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(TARG_CSRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call DELFILE, *.dSYM)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

preconfig:

-include Make.dep
