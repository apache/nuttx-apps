############################################################################
# apps/examples/flowc/Makefile
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

# FLOWC Test

# Target 1

ifeq ($(CONFIG_EXAMPLES_FLOWC_SENDER1),y)
CSRCS += flowc_sender.c
else
CSRCS += flowc_receiver.c
endif
MAINSRC += flowc_target1.c

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
HOST_BIN = receiver$(HOSTEXEEXT)
else
HOST_SRCS += flowc_sender.c
HOST_BIN = sender$(HOSTEXEEXT)
endif

HOST_OBJS = $(HOST_SRCS:.c=.$(HOSTOBJSEXT))
$(HOST_OBJS): %.$(HOSTOBJSEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)

$(HOST_OBJS): config.h

$(HOST_BIN): $(HOST_OBJS)
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@

endif

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_FLOWC_TARGET2),y)
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.$(HOSTOBJSEXT))
	$(call DELFILE, *.dSYM)
endif
	$(call DELFILE, config.h)

MODULE = $(CONFIG_EXAMPLES_FLOWC)

include $(APPDIR)/Application.mk
