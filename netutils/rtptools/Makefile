############################################################################
# apps/netutils/rtptools/Makefile
#
# SPDX-License-Identifier: Apache-2.0
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
############################################################################

include $(APPDIR)/Make.defs

RTPTOOLS_VERSION := $(patsubst "%",%,$(CONFIG_NETUTILS_RTPTOOLS_VERSION))
RTPTOOLS_TARBALL  = $(RTPTOOLS_VERSION).tar.gz
RTPTOOLS_UNPACK   = rtptools
RTPTOOLS_SRCDIR   = $(RTPTOOLS_UNPACK)

DEPPATH += --dep-path $(RTPTOOLS_SRCDIR)
VPATH   += :$(RTPTOOLS_SRCDIR)

CFLAGS += -Wno-strict-prototypes -Wno-unused-function -Wno-format -Wno-shadow

# Workaround for clang.
#
# error: unknown warning option '-Wno-maybe-uninitialized'; did you mean '-Wno-uninitialized'? [-Werror,-Wunknown-warning-option]
# make[3]: *** [multimer.c.Users.runner.work.nuttx.nuttx.sources.apps.netutils.rtptools.o] Error 1
#
# It's possible to use only '-Wno-uninitialized' instead, however, it isn't
# recommended as it could mask erroneous code.
#
ifneq ($(shell $(CC) --version | grep clang),)
CFLAGS += -Wno-uninitialized
else
CFLAGS += -Wno-maybe-uninitialized
endif

CSRCS += multimer.c
CSRCS += notify.c
CSRCS += payload.c
CSRCS += rd.c
CSRCS += utils.c

ifeq ($(CONFIG_ENDIAN_BIG),y)
RTP_BIG_ENDIAN = 1
else
RTP_BIG_ENDIAN = 0
endif

ifneq ($(CONFIG_RTPTOOLS_APPS),)

MODULE = $(CONFIG_RTPTOOLS_APPS)

ifeq ($(CONFIG_NETUTILS_RTPTOOLS_RTPPLAY),y)

PROGNAME  += $(CONFIG_NETUTILS_RTPTOOLS_RTPPLAY_PROGNAME)
PRIORITY  += $(CONFIG_NETUTILS_RTPTOOLS_RTPPLAY_PRIORITY)
STACKSIZE += $(CONFIG_NETUTILS_RTPTOOLS_RTPPLAY_STACKSIZE)

MAINSRC += $(RTPTOOLS_UNPACK)/rtpplay.c
endif

ifeq ($(CONFIG_NETUTILS_RTPTOOLS_RTPSEND),y)

PROGNAME  += $(CONFIG_NETUTILS_RTPTOOLS_RTPSEND_PROGNAME)
PRIORITY  += $(CONFIG_NETUTILS_RTPTOOLS_RTPSEND_PRIORITY)
STACKSIZE += $(CONFIG_NETUTILS_RTPTOOLS_RTPSEND_STACKSIZE)

MAINSRC += $(RTPTOOLS_UNPACK)/rtpsend.c
endif

ifeq ($(CONFIG_NETUTILS_RTPTOOLS_RTPDUMP),y)

PROGNAME  += $(CONFIG_NETUTILS_RTPTOOLS_RTPDUMP_PROGNAME)
PRIORITY  += $(CONFIG_NETUTILS_RTPTOOLS_RTPDUMP_PRIORITY)
STACKSIZE += $(CONFIG_NETUTILS_RTPTOOLS_RTPDUMP_STACKSIZE)

MAINSRC += $(RTPTOOLS_UNPACK)/rtpdump.c
endif

ifeq ($(CONFIG_NETUTILS_RTPTOOLS_RTPTRANS),y)

PROGNAME  += $(CONFIG_NETUTILS_RTPTOOLS_RTPTRANS_PROGNAME)
PRIORITY  += $(CONFIG_NETUTILS_RTPTOOLS_RTPTRANS_PRIORITY)
STACKSIZE += $(CONFIG_NETUTILS_RTPTOOLS_RTPTRANS_STACKSIZE)

MAINSRC += $(RTPTOOLS_UNPACK)/rtptrans.c
endif

endif

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(RTPTOOLS_UNPACK)/.git),)
$(RTPTOOLS_UNPACK)$(DELIM)config.h: $(RTPTOOLS_UNPACK)
	$(Q) cp config.h $@
else
$(RTPTOOLS_UNPACK)$(DELIM)config.h:
	$(Q) cp config.h $@
endif

$(RTPTOOLS_TARBALL):
	@echo "Downloading: $(RTPTOOLS_TARBALL)"
	$(Q) curl -O -L https://github.com/irtlab/rtptools/archive/$(RTPTOOLS_TARBALL)

$(RTPTOOLS_UNPACK): $(RTPTOOLS_TARBALL)
	$(Q) echo "Unpacking: $(RTPTOOLS_TARBALL) -> $(RTPTOOLS_UNPACK)"
	$(Q) tar zxf $(RTPTOOLS_TARBALL)
	$(Q) mv $(RTPTOOLS_UNPACK)-$(RTPTOOLS_VERSION) $(RTPTOOLS_UNPACK)
	$(Q) touch $(RTPTOOLS_UNPACK)

context:: $(RTPTOOLS_UNPACK)$(DELIM)config.h

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(RTPTOOLS_UNPACK)/.git),)
distclean::
	$(call DELFILE, $(RTPTOOLS_TARBALL))
	$(call DELDIR, $(RTPTOOLS_UNPACK))
endif

include $(APPDIR)/Application.mk
