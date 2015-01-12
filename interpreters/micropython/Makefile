############################################################################
# apps/ interpreters/micropython/Makefile
#
#   Copyright (C) 2015 Gregory Nutt. All rights reserved.
#   Authors: Gregory Nutt <gnutt@nuttx.org>
#            Dave Marples <dave@marples.net>
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
###########################################################################

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)/Make.defs

WD := ${shell pwd | sed -e 's/ /\\ /g'}

CONFIG_INTERPRETERS_MICROPYTHON_URL ?= "https://github.com/micropython/micropython/archive"
CONFIG_INTERPRETERS_MICROPYTHON_VERSION ?= "1.3.8"
MICROPYTHON_VERSION = $(patsubst "%",%,$(strip $(CONFIG_INTERPRETERS_MICROPYTHON_VERSION)))

MICROPYTHON_TARBALL = v$(MICROPYTHON_VERSION).tar.gz
WGET ?= wget

MICROPYTHON_UNPACKNAME = micropython-$(MICROPYTHON_VERSION)
UNPACK ?= tar -zxf

MICROPYTHON_UNPACKDIR =  $(WD)/$(MICROPYTHON_UNPACKNAME)
MICROPTHYON_SRCDIR = $(MICROPYTHON_UNPACKDIR)/py
-include $(MICROPTHYON_SRCDIR)/mkenv.mk

# qstr definitions (must come before including py.mk)

QSTR_DEFS = qstrdefsport.h

# include py core make definitions

-include $(MICROPTHYON_SRCDIR)/py.mk

# c99 is needed for micropython

CFLAGS+=-std=c99 -Wno-shadow -Wno-strict-prototypes

INC =  -I. -I$(WD) -I$(MICROPYTHON_UNPACKDIR) -I$(PY_SRC) -I$(BUILD)

CFLAGS +=$(INC)

CONFIG_INTERPRETERS_MICROPYTHON_APPNAME ?= micropython
CONFIG_INTERPRETERS_MICROPYTHON_STACKSIZE ?= 2048
CONFIG_INTERPRETERS_MICROPYTHON_PRIORITY ?= SCHED_PRIORITY_DEFAULT

APPNAME = $(CONFIG_INTERPRETERS_MICROPYTHON_APPNAME)
STACKSIZE = $(CONFIG_INTERPRETERS_MICROPYTHON_STACKSIZE)
PRIORITY = $(CONFIG_INTERPRETERS_MICROPYTHON_PRIORITY)

CONFIG_INTERPRETERS_MICROPYTHON_PROGNAME ?= micropython$(EXEEXT)
PROGNAME = $(CONFIG_INTERPRETERS_MICROPYTHON_PROGNAME)

ASRCS =
CSRCS = pyexec.c py_readline.c
MAINSRC = micropython_main.c

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(ASRC:.S=$(OBJEXT)) $(CSRCS:.c=$(OBJEXT)))

ifneq ($(CONFIG_BUILD_KERNEL),y)
  OBJ += $(addprefix $(BUILD)/, $(MAINSRC:.c=$(OBJEXT)))
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

all: .built

$(MICROPYTHON_TARBALL):
	@echo "Downloading: $(MICROPYTHON_TARBALL)"
	$(Q) $(WGET) $(CONFIG_INTERPRETERS_MICROPYTHON_URL)/$(MICROPYTHON_TARBALL)

$(MICROPYTHON_UNPACKNAME): $(MICROPYTHON_TARBALL)
	@echo "Unpacking: $(MICROPYTHON_TARBALL) -> $(MICROPYTHON_UNPACKNAME)"
	$(Q) $(UNPACK) $(MICROPYTHON_TARBALL)
	@touch $(MICROPYTHON_UNPACKNAME)

.built: $(MICROPYTHON_UNPACKNAME) $(OBJ)
	$(call ARCHIVE, $(BIN), $(OBJ))
	@touch .built

ifeq ($(CONFIG_BUILD_KERNEL),y)
$(BIN_DIR)$(DELIM)$(PROGNAME): $(MICROPYTHON_UNPACKNAME) $(OBJ) $(MAINOBJ)
	@echo "LD: $(PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME) $(ARCHCRT0OBJ) $(MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME)

install: $(BIN_DIR)$(DELIM)$(PROGNAME)
else
install:
endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME),$(PRIORITY),$(STACKSIZE),micropython_main)

context: $(MICROPYTHON_UNPACKNAME) $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat
else
context:
endif

.depend: Makefile $(SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELDIR, build)
	$(call DELFILE, .built)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)
	$(call DELDIR, $(MICROPYTHON_UNPACKNAME))
	$(call DELFILE, .downloaded)
	$(call DELFILE, $(MICROPYTHON_TARBALL))

-include $(MICROPTHYON_SRCDIR)/mkrules.mk
-include Make.dep
