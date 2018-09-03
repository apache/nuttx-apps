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

-include $(TOPDIR)/Make.defs

# UDP Test

CSRCS = udp_cmdline.c
ifeq ($(CONFIG_EXAMPLES_UDP_NETINIT),y)
CSRCS += udp_netinit.c
endif

# Target 1

ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
CSRCS += udp_server.c
else
CSRCS += udp_client.c
endif
MAINSRC = udp_target1.c

ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
CONFIG_EXAMPLES_UDP_PROGNAME1 ?= udpserver
else
CONFIG_EXAMPLES_UDP_PROGNAME1 ?= udpclient
endif
CONFIG_EXAMPLES_UDP_PRIORITY1 ?= 100
CONFIG_EXAMPLES_UDP_STACKSIZE1 ?= 2048

PROGNAME = $(CONFIG_EXAMPLES_UDP_PROGNAME1)
PRIORITY = $(CONFIG_EXAMPLES_UDP_PRIORITY1)
STACKSIZE = $(CONFIG_EXAMPLES_UDP_STACKSIZE1)

# Target 2

ifeq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)

ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
CSRCS += udp_client.c
else
CSRCS += udp_server.c
endif
MAINSRC += udp_target2.c

ifeq ($(CONFIG_EXAMPLES_UDP_SERVER1),y)
CONFIG_EXAMPLES_UDP_PROGNAME2 ?= udpclient
else
CONFIG_EXAMPLES_UDP_PROGNAME2 ?= udpserver
endif
CONFIG_EXAMPLES_UDP_PRIORITY2 ?= 100
CONFIG_EXAMPLES_UDP_STACKSIZE2 ?= 2048

PROGNAME += $(CONFIG_EXAMPLES_UDP_PROGNAME2)
PRIORITY += $(CONFIG_EXAMPLES_UDP_PRIORITY2)
STACKSIZE += $(CONFIG_EXAMPLES_UDP_STACKSIZE2)

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

ifeq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
APPNAME = udp1 udp2
else
APPNAME = udp
endif

# Common build

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

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.$(HOSTOBJSEXT))
endif
	$(call DELFILE, *.dSYM)
	$(call DELFILE, config.h)

MODULE = CONFIG_EXAMPLES_UDP

include $(APPDIR)/Application.mk
