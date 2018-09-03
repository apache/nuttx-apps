############################################################################
# apps/examples/bridge/Makefile
#
#   Copyright (C) 2014 Gregory Nutt. All rights reserved.
#
#   Authors: Gregory Nutt <gnutt@nuttx.org>
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

# Discover built-in application info

APPNAME = bridge
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = 2048

MAINSRC = bridge_main.c

HOST_SRCS1 = host_net1.c
HOST_SRCS2 = host_net2.c

HOST_OBJS1 = $(HOST_SRCS1:.c=.o)
HOST_OBJS2 = $(HOST_SRCS2:.c=.o)
HOST_OBJS  = $(HOST_OBJS1) $(HOST_OBJS2)

HOST_BIN1 = host1
HOST_BIN2 = host2

CONFIG_EXAMPLES_BRIDGE_PROGNAME ?= bridge$(EXEEXT)
PROGNAME = $(CONFIG_EXAMPLES_BRIDGE_PROGNAME)

# Common build

$(HOST_OBJS): %.o: %.c
	@echo "CC: $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

bridge_config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP: brigetconfig.h"
	$(Q) cp $(TOPDIR)/include/nuttx/config.h bridge_config.h

$(HOST_BIN1): bridge_config.h $(HOST_OBJS1)
	@echo "LD: $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS1) -o $@

$(HOST_BIN2): bridge_config.h $(HOST_OBJS2)
	@echo "LD: $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS2) -o $@

context:: $(HOST_BIN1) $(HOST_BIN2)

clean::
	$(call DELFILE, $(HOST_BIN1))
	$(call DELFILE, $(HOST_BIN2))

distclean::
	$(call DELFILE, bridge_config.h)

MODULE = CONFIG_EXAMPLES_BRIDGE

include $(APPDIR)/Application.mk
