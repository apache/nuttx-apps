############################################################################
# examples/udpblaster/Makefile
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

CSRCS = udpblaster_text.c
MAINSRC = udpblaster_target.c

HOSTCFLAGS += -DUDPBLASTER_HOST=1

HOST_SRCS = udpblaster_host.c udpblaster_text.c

HOSTOBJEXT ?= .hobj
HOST_OBJS = $(HOST_SRCS:.c=$(HOSTOBJEXT))
HOST_BIN = host

# NET test built-in application info

PROGNAME = $(CONFIG_EXAMPLES_UDPBLASTER_PROGNAME)
PRIORITY = $(CONFIG_EXAMPLES_UDPBLASTER_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_UDPBLASTER_STACKSIZE)

# Common build

ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
$(HOST_OBJS): %$(HOSTOBJEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@
endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
$(HOST_OBJS): config.h

$(HOST_BIN): $(HOST_OBJS)
	@echo "LD:  $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@
endif

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
	$(call DELFILE, *$(HOSTOBJEXT))
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.dSYM)
endif
	$(call DELFILE, config.h)

MODULE = $(CONFIG_EXAMPLES_UDPBLASTER)

include $(APPDIR)/Application.mk
