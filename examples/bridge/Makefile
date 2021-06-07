############################################################################
# apps/examples/bridge/Makefile
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

# Discover built-in application info

PROGNAME = bridge
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_BRIDGE)

MAINSRC = bridge_main.c

HOST_SRCS1 = host_net1.c
HOST_SRCS2 = host_net2.c

HOST_OBJS1 = $(HOST_SRCS1:.c=.o)
HOST_OBJS2 = $(HOST_SRCS2:.c=.o)
HOST_OBJS  = $(HOST_OBJS1) $(HOST_OBJS2)

HOST_BIN1 = host1
HOST_BIN2 = host2

# Common build

$(HOST_OBJS): %.o: %.c
	@echo "CC: $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

bridge_config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP: brige_config.h"
	$(Q) cp $(TOPDIR)/include/nuttx/config.h bridge_config.h

$(HOST_OBJS1): bridge_config.h

$(HOST_BIN1): $(HOST_OBJS1)
	@echo "LD: $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS1) -o $@

$(HOST_OBJS2): bridge_config.h

$(HOST_BIN2): $(HOST_OBJS2)
	@echo "LD: $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS2) -o $@

context:: $(HOST_BIN1) $(HOST_BIN2)

clean::
	$(call DELFILE, $(HOST_BIN1))
	$(call DELFILE, $(HOST_BIN2))

distclean::
	$(call DELFILE, bridge_config.h)

include $(APPDIR)/Application.mk
