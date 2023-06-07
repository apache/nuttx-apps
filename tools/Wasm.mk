
############################################################################
# apps/tools/Wasm.mk
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

# Only build wasm if one of the following runtime is enabled

ifneq ($(CONFIG_INTERPRETERS_WAMR)$(CONFIG_INTERPRETERS_WASM)$(CONFIG_INTERPRETERS_TOYWASM),)
include $(APPDIR)$(DELIM)interpreters$(DELIM)wamr$(DELIM)Toolchain.defs

# wasi-sdk toolchain setting

WCC ?= $(WASI_SDK_PATH)/bin/clang
WAR ?= $(WASI_SDK_PATH)/bin/llvm-ar rcs

# sysroot for building wasm, default is NuttX

ifeq ($(WSYSROOT),)
	WSYSROOT := $(TOPDIR)
	
	# Force disable wasm build when WASM_SYSROOT is not defined and on specific
	# targets that do not support wasm build.
	# Since some architecture level inline assembly instructions can not be
	# recognized by wasm-clang. For example:
	# Error: /github/workspace/sources/nuttx/include/arch/chip/irq.h:220:27: error: invalid output constraint '=a' in asm
    # asm volatile("rdtscp" : "=a" (lo), "=d" (hi)::"memory");

	ifeq ($(CONFIG_ARCH_INTEL64)$(CONFIG_ARCH_SPARC_V8)$(CONFIG_ARCH_AVR)$(CONFIG_ARCH_XTENSA),y)
		WASM_BUILD = n
	endif

endif

# Only build wasm when WCC is exist

ifneq ($(wildcard $(WCC)),)

CFLAGS_STRIP = -fsanitize=kernel-address -fsanitize=address -fsanitize=undefined
CFLAGS_STRIP += $(ARCHCPUFLAGS) $(ARCHCFLAGS) $(ARCHINCLUDES) $(ARCHDEFINES) $(ARCHOPTIMIZATION) $(EXTRAFLAGS)

WCFLAGS += $(filter-out $(CFLAGS_STRIP),$(CFLAGS))
WCFLAGS += --sysroot=$(WSYSROOT) -nostdlib -D__NuttX__

# If CONFIG_LIBM not defined, then define it to 1
ifeq ($(CONFIG_LIBM),)
WCFLAGS += -DCONFIG_LIBM=1 -I$(APPDIR)$(DELIM)include$(DELIM)wasm
endif

WLDFLAGS = -z stack-size=$(STACKSIZE) -Wl,--initial-memory=$(INITIAL_MEMORY)
WLDFLAGS += -Wl,--export=main -Wl,--export=__main_argc_argv
WLDFLAGS += -Wl,--export=__heap_base -Wl,--export=__data_end
WLDFLAGS += -Wl,--no-entry -Wl,--strip-all -Wl,--allow-undefined

COMPILER_RT_LIB = $(shell $(WCC) --print-libgcc-file-name)
ifeq ($(wildcard $(COMPILER_RT_LIB)),)
  # if "--print-libgcc-file-name" unable to find the correct libgcc PATH
  # then go ahead and try "--print-file-name"
  COMPILER_RT_LIB := $(wildcard $(shell $(WCC) --print-file-name $(notdir $(COMPILER_RT_LIB))))
endif

# If called from $(APPDIR)/Make.defs, WASM_BUILD is not defined
# Provide LINK_WASM, but only execute it when file wasm/*.wo exists

ifeq ($(WASM_BUILD),)


define LINK_WASM
	$(if $(wildcard $(APPDIR)$(DELIM)wasm$(DELIM)*), \
	  $(foreach bin,$(wildcard $(APPDIR)$(DELIM)wasm$(DELIM)*.wo), \
	    $(eval INITIAL_MEMORY=$(shell echo $(notdir $(bin)) | cut -d'#' -f2)) \
	    $(eval STACKSIZE=$(shell echo $(notdir $(bin)) | cut -d'#' -f3)) \
	    $(eval PROGNAME=$(shell echo $(notdir $(bin)) | cut -d'#' -f1)) \
	    $(eval RETVAL=$(shell $(WCC) $(bin) $(WBIN) $(WCFLAGS) $(WLDFLAGS) $(COMPILER_RT_LIB) \
	        -o $(APPDIR)$(DELIM)wasm$(DELIM)$(PROGNAME).wasm || echo 1;)) \
	    $(if $(RETVAL), \
	        $(error wasm build failed for $(PROGNAME).wasm) \
	    ) \
		$(call WAMR_AOT_COMPILE) \
	   ) \
	 )
endef

endif # WASM_BUILD

# Default values for WASM_BUILD, it's a three state variable:
#   y - build wasm module only
#   n - don't build wasm module
#   both - build wasm module and native module

WASM_BUILD ?= n

ifneq ($(WASM_BUILD),n)

WASM_INITIAL_MEMORY ?= 65536
STACKSIZE           ?= $(CONFIG_DEFAULT_TASK_STACKSIZE)
PRIORITY            ?= SCHED_PRIORITY_DEFAULT

# Wamr mode:
# INT: Interpreter (Default)
# AOT: Ahead-of-Time
# XIP: Execution In Place
# JIT: Just In Time

WAMR_MODE ?= INT

# Targets follow

.PRECIOUS: $(WBIN)

WSRCS := $(MAINSRC) $(CSRCS)
WOBJS := $(WSRCS:=$(SUFFIX).wo)

# Copy math.h from $(TOPDIR)/include/nuttx/lib/math.h to $(APPDIR)/include/wasm/math.h
# Using declaration of math.h is OK for Wasm build

$(APPDIR)$(DELIM)include$(DELIM)wasm$(DELIM)math.h:
ifeq ($(CONFIG_LIBM),)
	$(call COPYFILE,$(TOPDIR)$(DELIM)include$(DELIM)nuttx$(DELIM)lib$(DELIM)math.h,$@)
endif

all:: $(WBIN)

depend:: $(APPDIR)$(DELIM)include$(DELIM)wasm$(DELIM)math.h

$(WOBJS): %.c$(SUFFIX).wo : %.c
	$(Q) $(WCC) $(WCFLAGS) -c $^ -o $@

$(WBIN): $(WOBJS)
	$(shell mkdir -p $(APPDIR)/wasm)
	$(Q) $(WAR) $@ $(filter-out $(MAINSRC:=$(SUFFIX).wo),$^)
	$(foreach main,$(MAINSRC), \
	  $(eval mainindex=$(strip $(call GETINDEX,$(main),$(MAINSRC)))) \
	$(eval dstname=$(shell echo $(main:=$(SUFFIX).wo) | sed -e 's/\//_/g')) \
	  $(shell cp -rf $(strip $(main:=$(SUFFIX).wo)) \
	    $(strip $(APPDIR)/wasm/$(word $(mainindex),$(PROGNAME))#$(WASM_INITIAL_MEMORY)#$(STACKSIZE)#$(PRIORITY)#$(WAMR_MODE)#$(dstname)) \
	   ) \
	 )

clean::
	$(call DELFILE, $(WOBJS))
	$(call DELFILE, $(WBIN))
	$(call DELFILE, $(APPDIR)$(DELIM)include$(DELIM)wasm$(DELIM)math.h)

endif # WASM_BUILD

endif # WCC
endif
