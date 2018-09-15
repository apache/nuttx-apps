############################################################################
# examples/nettest/Makefile
#
#   Copyright (C) 2007-2008, 2010-2012, 2017 Gregory Nutt. All rights reserved.
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

-include $(TOPDIR)/Make.defs

# Basic TCP networking test

CSRCS = nettest_cmdline.c
ifeq ($(CONFIG_EXAMPLES_NETTEST_INIT),y)
CSRCS += nettest_netinit.c
endif

# Target 1 Files

ifeq ($(CONFIG_EXAMPLES_NETTEST_LOOPBACK),y)
CSRCS += nettest_server.c nettest_client.c
else ifeq ($(CONFIG_EXAMPLES_NETTEST_SERVER),y)
CSRCS += nettest_server.c
else
CSRCS += nettest_client.c
endif
MAINSRC = nettest_target1.c

# Target 1 Application Info

ifeq ($(CONFIG_EXAMPLES_NETTEST_SERVER1),y)
CONFIG_EXAMPLES_NETTEST_PROGNAME1 ?= tcpserver
else
CONFIG_EXAMPLES_NETTEST_PROGNAME1 ?= tcpclient
endif
CONFIG_EXAMPLES_NETTEST_PRIORITY1 ?= 100
CONFIG_EXAMPLES_NETTEST_STACKSIZE1 ?= 2048

PROGNAME = $(CONFIG_EXAMPLES_NETTEST_PROGNAME1)
PRIORITY = $(CONFIG_EXAMPLES_NETTEST_PRIORITY1)
STACKSIZE = $(CONFIG_EXAMPLES_NETTEST_STACKSIZE1)

# Target 2

ifeq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)

ifeq ($(CONFIG_EXAMPLES_NETTEST_SERVER1),y)
CSRCS += nettest_client.c
else
CSRCS += nettest_server.c
endif
MAINSRC += nettest_target2.c

# Target 2 Application Info

ifeq ($(CONFIG_EXAMPLES_NETTEST_SERVER1),y)
CONFIG_EXAMPLES_NETTEST_PROGNAME2 ?= tcpclient
else
CONFIG_EXAMPLES_NETTEST_PROGNAME2 ?= tcpserver
endif
CONFIG_EXAMPLES_NETTEST_PRIORITY2 ?= 100
CONFIG_EXAMPLES_NETTEST_STACKSIZE2 ?= 2048

PROGNAME += $(CONFIG_EXAMPLES_NETTEST_PROGNAME2)
PRIORITY += $(CONFIG_EXAMPLES_NETTEST_PRIORITY2)
STACKSIZE += $(CONFIG_EXAMPLES_NETTEST_STACKSIZE2)

endif

ifeq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)
APPNAME = nettest1 nettest2
else
APPNAME = nettest
endif

# Host

ifneq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_NETTEST_LOOPBACK),y)

  HOSTCFLAGS += -DNETTEST_HOST=1
  ifeq ($(CONFIG_EXAMPLES_NETTEST_SERVER),y)
    HOSTCFLAGS += -DCONFIG_EXAMPLES_NETTEST_SERVER=1 -DCONFIG_EXAMPLES_NETTEST_SERVERIP="$(CONFIG_EXAMPLES_NETTEST_SERVERIP)"
  endif
  ifeq ($(CONFIG_EXAMPLES_NETTEST_PERFORMANCE),y)
  HOSTCFLAGS += -DCONFIG_EXAMPLES_NETTEST_PERFORMANCE=1
  endif

  HOST_SRCS = nettest_host.c
  ifeq ($(CONFIG_EXAMPLES_NETTEST_SERVER),y)
    HOST_SRCS += nettest_client.c
    HOST_BIN = tcpclient$(EXEEXT)
  else
    HOST_SRCS += nettest_server.c
    HOST_BIN = tcpserver$(EXEEXT)
  endif

  HOSTOBJEXT ?= hobj
  HOST_OBJS = $(HOST_SRCS:.c=.$(HOSTOBJEXT))

endif
endif

# Common build

ifneq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_NETTEST_LOOPBACK),y)

$(HOST_OBJS): %.$(HOSTOBJEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

endif
endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_NETTEST_LOOPBACK),y)

$(HOST_BIN): config.h $(HOST_OBJS)
	@echo "LD:  $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@

endif
endif

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_NETTEST_LOOPBACK),y)
	$(call DELFILE, *.$(HOSTOBJEXT))
	$(call DELFILE, $(HOST_BIN))
endif
endif
	$(call DELFILE, *.dSYM)
	$(call DELFILE, config.h)

MODULE = CONFIG_EXAMPLES_NETTEST

include $(APPDIR)/Application.mk
