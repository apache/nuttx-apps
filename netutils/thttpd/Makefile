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

-include $(TOPDIR)/Make.defs

# THTTPD Library
CSRCS =

ifeq ($(CONFIG_NET_TCP),y)
  CSRCS += libhttpd.c thttpd_cgi.c thttpd_alloc.c thttpd_strings.c timers.c
  CSRCS += fdwatch.c tdate_parse.c
  MAINSRC += thttpd.c
endif

CONFIG_XYZ_PROGNAME ?= thttpd$(EXEEXT)
PROGNAME = $(CONFIG_XYZ_PROGNAME)

# CGI binaries (examples only, not used in the build)

ifeq ($(CONFIG_THTTPD_NXFLAT),y)
  SUBDIR_BIN1 = phf
  SUBDIR_BIN2 = redirect
  SUBDIR_BIN3 = ssi
  SUBDIR_BIN += cgi-bin$(DELIM)$(SUBDIR_BIN1) cgi-bin$(DELIM)$(SUBDIR_BIN2)  cgi-bin$(DELIM)$(SUBDIR_BIN3)
endif

ifeq ($(CONFIG_THTTPD_BINFS),y)
  CSRCS +=  phf.c redirect.c ssi.c
  DEPPATHS += --dep-path cgi-src
  VPATH += :cgi-src
endif

ifneq ($(CONFIG_BUILD_LOADABLE),y)
  OBJS += $(MAINOBJ)
endif

CONFIG_THTTPD_CGI_PRIORITY ?= 100
CONFIG_THTTPD_CGI_STACKSIZE ?= 2048

APPNAME = phf redirect ssi
PRIORITY = $(CONFIG_THTTPD_CGI_PRIORITY)
STACKSIZE = $(CONFIG_THTTPD_CGI_STACKSIZE)

context:: $(SUBDIR_BIN)

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

MODULE = CONFIG_NETUTILS_THTTPD

include $(APPDIR)/Application.mk
