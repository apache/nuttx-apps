############################################################################
# examples/tcpblaster/Makefile
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
