############################################################################
# examples/nettest/Makefile
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

PROGNAME = $(CONFIG_EXAMPLES_NETTEST_PROGNAME1)
PRIORITY = $(CONFIG_EXAMPLES_NETTEST_PRIORITY1)
STACKSIZE = $(CONFIG_EXAMPLES_NETTEST_STACKSIZE1)

# Target 2

ifeq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)

ifeq ($(CONFIG_EXAMPLES_NETTEST_SERVER),y)
CSRCS += nettest_client.c
else
CSRCS += nettest_server.c
endif
MAINSRC += nettest_target2.c

# Target 2 Application Info

PROGNAME += $(CONFIG_EXAMPLES_NETTEST_PROGNAME2)
PRIORITY += $(CONFIG_EXAMPLES_NETTEST_PRIORITY2)
STACKSIZE += $(CONFIG_EXAMPLES_NETTEST_STACKSIZE2)

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
    HOST_BIN = tcpclient$(HOSTEXEEXT)
  else
    HOST_SRCS += nettest_server.c
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

ifneq ($(CONFIG_EXAMPLES_NETTEST_TARGET2),y)
ifneq ($(CONFIG_EXAMPLES_NETTEST_LOOPBACK),y)

$(HOST_OBJS): config.h

$(HOST_BIN): $(HOST_OBJS)
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
	$(call DELFILE, *.dSYM)
endif
endif
	$(call DELFILE, config.h)

MODULE = $(CONFIG_EXAMPLES_NETTEST)

include $(APPDIR)/Application.mk
