############################################################################
# apps/interpreters/Wasm.mk
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

ifeq ($(WASM_BUILD),y)
  ifneq ($(CONFIG_INTERPRETERS_WAMR)$(CONFIG_INTERPRETERS_WAMR3),)

WASM_INITIAL_MEMORY ?= 65536
STACKSIZE           ?= $(CONFIG_DEFAULT_TASK_STACKSIZE)
PRIORITY            ?= SCHED_PRIORITY_DEFAULT

# Wamr mode:
# INT: Interpreter (Default)
# AOT: Ahead-of-Time
# XIP: Execution In Place
# JIT: Just In Time

WAMR_MODE       ?= INT

# WebAssembly Micro Runtime Toolchain Setting

-include $(APPDIR)$(DELIM)interpreters$(DELIM)wamr$(DELIM)Toolchain.defs

# Targets follow

.PRECIOUS: $(WBIN)

WSRCS := $(MAINSRC) $(CSRCS)
WOBJS := $(WSRCS:%.c=%.wo)

all:: $(WBIN)

$(WOBJS): %.wo : %.c
	$(Q) $(WCC) $(WCFLAGS) -c $^ -o $@

$(WBIN): $(WOBJS)
	$(shell mkdir -p $(APPDIR)/wasm)
	$(Q) $(WAR) $@ $(filter-out $(MAINSRC:%.c=%.wo),$^)
	$(foreach main,$(MAINSRC), \
	  $(eval mainindex=$(strip $(call GETINDEX,$(main),$(MAINSRC)))) \
	  $(eval dstname=$(shell echo $(main:%.c=%.wo) | sed -e 's/\//_/g')) \
	  $(shell cp -rf $(strip $(main:%.c=%.wo)) \
	    $(strip $(APPDIR)/wasm/$(word $(mainindex),$(PROGNAME))#$(WASM_INITIAL_MEMORY)#$(STACKSIZE)#$(PRIORITY)#$(WAMR_MODE)#$(dstname)) \
	   ) \
	 )

clean::
	$(call DELFILE, $(WOBJS))
	$(call DELFILE, $(WBIN))

  endif # CONFIG_INTERPRETERS_WAMR || CONFIG_INTERPRETERS_WAMR3
endif # WASM_BUILD
