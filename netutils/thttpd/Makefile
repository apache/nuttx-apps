#############################################################################
# apps/netutils/thttpd/Makefile
#
#   Copyright (C) 2009, 2011-2012, 2015 Gregory Nutt. All rights reserved.
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
#############################################################################

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)$(DELIM)Make.defs

# THTTPD Library

ASRCS =
CSRCS =

ifeq ($(CONFIG_NET_TCP),y)
  CSRCS += libhttpd.c thttpd_cgi.c thttpd_alloc.c thttpd_strings.c timers.c
  CSRCS += fdwatch.c tdate_parse.c
  MAINSRC += thttpd.c
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

CONFIG_XYZ_PROGNAME ?= thttpd$(EXEEXT)
PROGNAME = $(CONFIG_XYZ_PROGNAME)

ROOTDEPPATH = --dep-path .

# CGI binaries (examples only, not used in the build)

CGIBINDIR = $(APPDIR)$(DELIM)netutils$(DELIM)thttpd$(DELIM)cgi-bin

BINFS_CSRCS =
VPATH = .

ifeq ($(CONFIG_THTTPD_NXFLAT),y)
  SUBDIR_BIN1 = phf
  SUBDIR_BIN2 = redirect
  SUBDIR_BIN3 = ssi
  SUBDIR_BIN += cgi-bin$(DELIM)$(SUBDIR_BIN1) cgi-bin$(DELIM)$(SUBDIR_BIN2)  cgi-bin$(DELIM)$(SUBDIR_BIN3)
endif

ifeq ($(CONFIG_THTTPD_BINFS),y)
  BINFS_CSRCS =  phf.c redirect.c ssi.c
  VPATH += :cgi-src
endif

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
BINFS_COBJS = $(BINFS_CSRCS:.c=$(OBJEXT))
MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(BINFS_CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS) $(BINFS_COBJS)

ifneq ($(CONFIG_BUILD_KERNEL),y)
  OBJS += $(MAINOBJ)
endif

CONFIG_THTTPD_CGI_PRIORITY ?= 100
CONFIG_THTTPD_CGI_STACKSIZE ?= 2048

PRIORITY = $(CONFIG_THTTPD_CGI_PRIORITY)
STACKSIZE = $(CONFIG_THTTPD_CGI_STACKSIZE)

all: $(SUBDIR_BIN) .built
.PHONY: context depend clean distclean

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

ifeq ($(CONFIG_THTTPD_NXFLAT),y)
cgi-bin:
	$(Q) mkdir cgi-bin

cgi-src$(DELIM)$(SUBDIR_BIN1):
	$(Q) $(MAKE) -C cgi-src $(SUBDIR_BIN1)

cgi-bin$(DELIM)$(SUBDIR_BIN1): cgi-bin cgi-src$(DELIM)$(SUBDIR_BIN1)
	$(Q) cp -a cgi-src$(DELIM)$(SUBDIR_BIN1) $@

cgi-src$(DELIM)$(SUBDIR_BIN2):
	$(Q) $(MAKE) -C cgi-src $(SUBDIR_BIN2)

cgi-bin$(DELIM)$(SUBDIR_BIN2): cgi-bin cgi-src$(DELIM)$(SUBDIR_BIN2)
	$(Q) cp -a cgi-src$(DELIM)$(SUBDIR_BIN2) $@

cgi-src$(DELIM)$(SUBDIR_BIN3):
	$(Q) $(MAKE) -C cgi-src $(SUBDIR_BIN3)

cgi-bin$(DELIM)$(SUBDIR_BIN3): cgi-bin cgi-src$(DELIM)$(SUBDIR_BIN3)
	$(Q) cp -a cgi-src$(DELIM)$(SUBDIR_BIN3) $@
endif

.built: $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	$(Q) touch .built

ifeq ($(CONFIG_BUILD_KERNEL),y)
$(BIN_DIR)$(DELIM)$(PROGNAME): $(OBJS) $(MAINOBJ)
	@echo "LD: $(PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME) $(ARCHCRT0OBJ) $(MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME)

install: $(BIN_DIR)$(DELIM)$(PROGNAME)

else
install:

endif

ifeq ($(CONFIG_THTTPD_BINFS),y)
$(BUILTIN_REGISTRY)$(DELIM)phf_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,phf,$(PRIORITY),$(STACKSIZE),phf_main)

$(BUILTIN_REGISTRY)$(DELIM)redirect_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,redirect,$(PRIORITY),$(STACKSIZE),redirect_main)

$(BUILTIN_REGISTRY)$(DELIM)ssi_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,ssi,$(PRIORITY),$(STACKSIZE),ssi_main)

context: $(BUILTIN_REGISTRY)$(DELIM)phf_main.bdat $(BUILTIN_REGISTRY)$(DELIM)redirect_main.bdat $(BUILTIN_REGISTRY)$(DELIM)ssi_main.bdat

else
context:

endif

.depend: Makefile $(SRCS)
	$(Q) $(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	$(Q) touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call CLEAN)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep
