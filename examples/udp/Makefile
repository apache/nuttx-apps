############################################################################
# apps/examples/udp/Makefile
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
HOST_BIN = udpclient$(HOSTEXEEXT)
else
HOST_SRCS += udp_server.c
HOST_BIN = udpserver$(HOSTEXEEXT)
endif

HOST_OBJS = $(HOST_SRCS:.c=.$(HOSTOBJSEXT))

# Common build

$(HOST_OBJS): %.$(HOSTOBJSEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)

$(HOST_OBJS): config.h

$(HOST_BIN): $(HOST_OBJS)
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@

endif

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_UDP_TARGET2),y)
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.$(HOSTOBJSEXT))
	$(call DELFILE, *.dSYM)
endif
	$(call DELFILE, config.h)

MODULE = $(CONFIG_EXAMPLES_UDP)

include $(APPDIR)/Application.mk
