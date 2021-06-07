#############################################################################
# apps/netutils/thttpd/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
#############################################################################

include $(APPDIR)/Make.defs

# THTTPD Library

ifeq ($(CONFIG_NET_TCP),y)
  CSRCS += libhttpd.c thttpd_cgi.c thttpd_alloc.c thttpd_strings.c timers.c
  CSRCS += fdwatch.c tdate_parse.c thttpd.c
endif

# CGI binaries (examples only, not used in the build)

ifeq ($(CONFIG_THTTPD_NXFLAT),y)
  SUBDIR_BIN1 = phf
  SUBDIR_BIN2 = redirect
  SUBDIR_BIN3 = ssi
  SUBDIR_BIN += cgi-bin$(DELIM)$(SUBDIR_BIN1) cgi-bin$(DELIM)$(SUBDIR_BIN2)  cgi-bin$(DELIM)$(SUBDIR_BIN3)
endif

ifeq ($(CONFIG_THTTPD_BINFS),y)
  CSRCS +=  phf.c redirect.c ssi.c
  CFLAGS += ${shell $(INCDIR) "$(CC)" "$(APPDIR)$(DELIM)netutils$(DELIM)thttpd"}
  DEPPATHS += --dep-path cgi-src
  VPATH += :cgi-src

  PROGNAME = phf redirect ssi
  PRIORITY = $(CONFIG_THTTPD_CGI_PRIORITY)
  STACKSIZE = $(CONFIG_THTTPD_CGI_STACKSIZE)
endif

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

include $(APPDIR)/Application.mk
