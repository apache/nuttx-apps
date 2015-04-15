############################################################################
# apps/system/zoneinfo/Makefile
#
#   Copyright (C) 2015, Gregory Nutt. All rights reserved.
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

ifeq ($(WINTOOL),y)
INCDIROPT = -w
endif

# Zoneinfo database

APPNAME = tzmount
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = 768

ASRCS =
CSRCS =

ifeq ($(CONFIG_SYSTEM_ZONEINFO_ROMFS),y)
CSRCS += tzromfs.c
endif

ifeq ($(CONFIG_SYSTEM_ZONEINFO_MNTCMD),y)
MAINSRC = tzmount.c
endif

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

ifneq ($(CONFIG_BUILD_KERNEL),y)
  OBJS += $(MAINOBJ)
endif

ZONEINFO_PATH = $(APPDIR)/system/zoneinfo
TZBIN_PATH = $(ZONEINFO_PATH)/tzbin
TZCODE_PATH = $(ZONEINFO_PATH)/tzcode

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

CONFIG_XYZ_PROGNAME ?= free$(EXEEXT)
PROGNAME = $(CONFIG_XYZ_PROGNAME)

ROOTDEPPATH = --dep-path .

# Common build

VPATH =

all: .built
.PHONY: romfs register context depend clean distclean

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: .tzbuilt romfs $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	$(Q) touch .built

# Kernel mode program build

ifeq ($(CONFIG_BUILD_KERNEL),y)
$(BIN_DIR)$(DELIM)$(PROGNAME): $(OBJS) $(MAINOBJ)
	@echo "LD: $(PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME) $(ARCHCRT0OBJ) $(MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME)

install: $(BIN_DIR)$(DELIM)$(PROGNAME)

else
install:

endif

# ROMFS file system containing the TZ database

ifeq ($(CONFIG_SYSTEM_ZONEINFO_MNTCMD),y)

checkgenromfs:
	@genromfs -h 1>/dev/null 2>&1 || { \
 echo "Host executable genromfs not available in PATH"; \
 echo "You may need to download in from http://romfs.sourceforge.net/"; \
 exit 1; \
	}

romfs_zoneinfo.img : checkgenromfs .tzbuilt
	@genromfs -f $@ -d $(TZBIN_PATH)/etc/zoneinfo -V "TZDatbase" || { echo "genromfs failed" ; exit 1 ; }

romfs_zoneinfo.h : romfs_zoneinfo.img
	@xxd -i $< >$@ || { echo "xxd of $< failed" ; exit 1 ; }

romfs: romfs_zoneinfo.h

else

romfs:

endif # CONFIG_SYSTEM_ZONEINFO_MNTCMD

# Build TZ database

tzcode:
	mkdir tzcode

tzbin:
	mkdir tzbin

tzcode-latest.tar.gz:
	$(Q) wget --retr-symlinks ftp://ftp.iana.org/tz/tzcode-latest.tar.gz

tzdata-latest.tar.gz:
	$(Q) wget --retr-symlinks ftp://ftp.iana.org/tz/tzdata-latest.tar.gz

.tzunpack: tzcode tzcode-latest.tar.gz tzdata-latest.tar.gz
	$(Q) tar zx -C tzcode -f tzcode-latest.tar.gz
	$(Q) tar zx -C tzcode -f tzdata-latest.tar.gz
	$(Q) touch .tzunpack

.tzbuilt: tzcode tzbin .tzunpack
	$(Q) $(MAKE) -C tzcode TOPDIR=$(TZBIN_PATH) install
	$(Q) touch .tzbuilt

# Register application

ifeq ($(CONFIG_SYSTEM_ZONEINFO_MNTCMD),y)
$(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat: $(DEPCONFIG) Makefile
	$(call REGISTER,$(APPNAME),$(PRIORITY),$(STACKSIZE),$(APPNAME)_main)

register: $(BUILTIN_REGISTRY)$(DELIM)$(APPNAME)_main.bdat
else
register:
endif

context: .tzbuilt romfs register

# Create dependencies

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
	$(call DELFILE, .tzunpack)
	$(call DELFILE, .tzbuilt)
	$(call DELFILE, romfs_zoneinfo.img)
	$(call DELFILE, romfs_zoneinfo.h)
	$(call DELFILE, tzdata-latest.tar.gz)
	$(call DELFILE, tzcode-latest.tar.gz)
	$(call DELDIR, tzbin)
	$(call DELDIR, tzcode)

-include Make.dep
