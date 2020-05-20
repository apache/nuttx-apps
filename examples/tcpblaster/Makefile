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
#    notice, this list of conditions and the following dsclaimer in
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

include $(APPDIR)/Make.defs

# Basic TCP networking test

CSRCS = tcpblaster_cmdline.c
ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_INIT),y)
CSRCS += tcpblaster_netinit.c
endif

# Target 1 Files

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)
CSRCS += tcpblaster_server.c tcpblaster_client.c
else ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
CSRCS += tcpblaster_server.c
else
CSRCS += tcpblaster_client.c
endif
MAINSRC = tcpblaster_target1.c

# Target 1 Application Info

PROGNAME = $(CONFIG_EXAMPLES_TCPBLASTER_PROGNAME1)
PRIORITY = $(CONFIG_EXAMPLES_TCPBLASTER_PRIORITY1)
STACKSIZE = $(CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE1)

# Target 2

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)

ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
CSRCS += tcpblaster_client.c
else
CSRCS += tcpblaster_server.c
endif
MAINSRC += tcpblaster_target2.c

# Target 2 Application Info

PROGNAME += $(CONFIG_EXAMPLES_TCPBLASTER_PROGNAME2)
PRIORITY += $(CONFIG_EXAMPLES_TCPBLASTER_PRIORITY2)
STACKSIZE += $(CONFIG_EXAMPLES_TCPBLASTER_STACKSIZE2)

endif

# Host

ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)

  HOSTCFLAGS += -DTCPBLASTER_HOST=1
  ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
    HOSTCFLAGS += -DCONFIG_EXAMPLES_TCPBLASTER_SERVER=1 -DCONFIG_EXAMPLES_TCPBLASTER_SERVERIP=$(CONFIG_EXAMPLES_TCPBLASTER_SERVERIP)
  endif

  HOST_SRCS = tcpblaster_host.c tcpblaster_cmdline.c
  ifeq ($(CONFIG_EXAMPLES_TCPBLASTER_SERVER),y)
    HOST_SRCS += tcpblaster_client.c
    HOST_BIN = tcpclient$(HOSTEXEEXT)
  else
    HOST_SRCS += tcpblaster_server.c
    HOST_BIN = tcpserver$(HOSTEXEEXT)
  endif

  HOSTOBJEXT ?= hobj
  HOST_OBJS = $(HOST_SRCS:.c=.$(HOSTOBJEXT))

# Common build

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

$(HOST_OBJS): config.h

$(HOST_BIN): $(HOST_OBJS)
	@echo "LD:  $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@

endif
endif

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK),y)
	$(call DELFILE, *.$(HOSTOBJEXT))
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.dSYM)
endif
endif
	$(call DELFILE, config.h)

MODULE = $(CONFIG_EXAMPLES_TCPBLASTER)

include $(APPDIR)/Application.mk
