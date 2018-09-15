############################################################################
# apps/examples/flowc/Makefile
#
#   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

# FLOWC Test

# Target 1

CSRCS =
MAINSRC =
ifeq ($(CONFIG_EXAMPLES_FLOWC_SENDER1),y)
CSRCS += flowc_sender.c
else
CSRCS += flowc_receiver.c
endif
MAINSRC += flowc_target1.c

ifeq ($(CONFIG_EXAMPLES_FLOWC_SENDER1),y)
CONFIG_EXAMPLES_FLOWC_PROGNAME1 ?= sender
else
CONFIG_EXAMPLES_FLOWC_PROGNAME1 ?= receiver
endif
CONFIG_EXAMPLES_FLOWC_PRIORITY1 ?= 100
CONFIG_EXAMPLES_FLOWC_STACKSIZE1 ?= 2048

PROGNAME = $(CONFIG_EXAMPLES_FLOWC_PROGNAME1)
PRIORITY = $(CONFIG_EXAMPLES_FLOWC_PRIORITY1)
STACKSIZE = $(CONFIG_EXAMPLES_FLOWC_STACKSIZE1)

# Target 2

ifeq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)

ifeq ($(CONFIG_EXAMPLES_FLOWC_SENDER1),y)
CSRCS += flowc_receiver.c
else
CSRCS += flowc_sender.c
endif
MAINSRC += flowc_target2.c

ifeq ($(CONFIG_EXAMPLES_FLOWC_SENDER1),y)
CONFIG_EXAMPLES_FLOWC_PROGNAME2 ?= receiver
else
CONFIG_EXAMPLES_FLOWC_PROGNAME2 ?= sender
endif
CONFIG_EXAMPLES_FLOWC_PRIORITY2 ?= 100
CONFIG_EXAMPLES_FLOWC_STACKSIZE2 ?= 2048

PROGNAME += $(CONFIG_EXAMPLES_FLOWC_PROGNAME2)
PRIORITY += $(CONFIG_EXAMPLES_FLOWC_PRIORITY2)
STACKSIZE += $(CONFIG_EXAMPLES_FLOWC_STACKSIZE2)

endif

# Host

ifneq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)

HOSTCFLAGS += -DEXAMPLES_FLOWC_HOST=1
HOSTOBJSEXT ?= hobj

HOST_SRCS = flowc_host.c
ifeq ($(CONFIG_EXAMPLES_FLOWC_SENDER1),y)
HOST_SRCS += flowc_receiver.c
HOST_BIN = receiver$(EXEEXT)
else
HOST_SRCS += flowc_sender.c
HOST_BIN = sender$(EXEEXT)
endif

HOST_OBJS = $(HOST_SRCS:.c=.$(HOSTOBJSEXT))
endif

ifeq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)
APPNAME = flowc1 flowc2
else
APPNAME = flowc
endif

ifneq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)
$(HOST_OBJS): %.$(HOSTOBJSEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@
endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)
$(HOST_BIN): config.h $(HOST_OBJS)
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@
endif

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.$(HOSTOBJSEXT))
endif
	$(call DELFILE, *.dSYM)
	$(call DELFILE, config.h)

MODULE = CONFIG_EXAMPLES_FLOWC

include $(APPDIR)/Application.mk
