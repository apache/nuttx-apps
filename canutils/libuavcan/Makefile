############################################################################
# apps/canutils/libuavcan/Makefile
#
#   Copyright (C) 2015-2016 Omni Hoverboards Inc. All rights reserved.
#   Author: Paul Alexander Patience <paul-a.patience@polymtl.ca>
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

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs

DELIM ?= $(strip /)
include $(APPDIR)$(DELIM)Make.defs

WGET = wget
UNPACK = unzip
PACKEXT = .zip

LIBUAVCAN_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBUAVCAN_URL)))
LIBUAVCAN_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBUAVCAN_VERSION)))
LIBUAVCAN_UNPACKNAME = libuavcan-$(LIBUAVCAN_VERSION)
LIBUAVCAN_PACKNAME = $(LIBUAVCAN_UNPACKNAME)$(PACKEXT)
LIBUAVCAN_DSDL_PATH = libuavcan$(DELIM)dsdl
LIBUAVCAN_PYUAVCAN_PATH = libuavcan$(DELIM)libuavcan$(DELIM)dsdl_compiler$(DELIM)pyuavcan
LIBUAVCAN_PATCHNAME = nuttx-$(LIBUAVCAN_VERSION).patch

DSDL_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBUAVCAN_DSDL_URL)))
DSDL_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBUAVCAN_DSDL_VERSION)))
DSDL_UNPACKNAME = dsdl-$(DSDL_VERSION)
DSDL_PACKNAME = $(DSDL_UNPACKNAME)$(PACKEXT)

PYUAVCAN_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBUAVCAN_PYUAVCAN_URL)))
PYUAVCAN_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBUAVCAN_PYUAVCAN_VERSION)))
PYUAVCAN_UNPACKNAME = pyuavcan-$(PYUAVCAN_VERSION)
PYUAVCAN_PACKNAME = $(PYUAVCAN_UNPACKNAME)$(PACKEXT)

-include libuavcan$(DELIM)libuavcan$(DELIM)include.mk
-include libuavcan$(DELIM)libuavcan_drivers$(DELIM)stm32$(DELIM)driver$(DELIM)include.mk

CXXSRCS = platform_stm32.cpp $(LIBUAVCAN_SRC) $(LIBUAVCAN_STM32_SRC)

CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(LIBUAVCAN_INC)"}
CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(LIBUAVCAN_STM32_INC)"}
CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" dsdlc_generated}
CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(TOPDIR)$(DELIM)arch$(DELIM)arm$(DELIM)src$(DELIM)common"}
CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(TOPDIR)$(DELIM)arch$(DELIM)arm$(DELIM)src$(DELIM)stm32"}

CXXFLAGS += -D__KERNEL__
CXXFLAGS += -DUAVCAN_STM32_NUTTX=1
CXXFLAGS += -DUAVCAN_STM32_NUM_IFACES=$(CONFIG_LIBUAVCAN_STM32_NUM_IFACES)

ifeq ($(CONFIG_LIBUAVCAN_STM32_TIM2),y)
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=2
else
ifeq ($(CONFIG_LIBUAVCAN_STM32_TIM3),y)
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=3
else
ifeq ($(CONFIG_LIBUAVCAN_STM32_TIM4),y)
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=4
else
ifeq ($(CONFIG_LIBUAVCAN_STM32_TIM5),y)
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=5
else
ifeq ($(CONFIG_LIBUAVCAN_STM32_TIM6),y)
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=6
else
ifeq ($(CONFIG_LIBUAVCAN_STM32_TIM7),y)
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=7
endif
endif
endif
endif
endif
endif

ifeq ($(CONFIG_LIBUAVCAN_CPP03),y)
CXXFLAGS += -std=c++03 -DUAVCAN_CPP_VERSION=UAVCAN_CPP03
else
ifeq ($(CONFIG_LIBUAVCAN_CPP11),y)
CXXFLAGS += -std=c++11 -DUAVCAN_CPP_VERSION=UAVCAN_CPP11
endif
endif

ifeq ($(CONFIG_LIBUAVCAN_DEBUG),y)
CXXFLAGS += -DUAVCAN_DEBUG=1
endif

ifeq ($(CONFIG_LIBUAVCAN_EXCEPTIONS),y)
CXXFLAGS += -DUAVCAN_EXCEPTIONS=1
else
CXXFLAGS += -DUAVCAN_EXCEPTIONS=0
endif

ifeq ($(CONFIG_LIBUAVCAN_TINY),y)
CXXFLAGS += -DUAVCAN_TINY=1
endif

ifeq ($(CONFIG_LIBUAVCAN_NO_GLOBAL_DATA_TYPE_REGISTRY),y)
CXXFLAGS += -DUAVCAN_NO_GLOBAL_DATA_TYPE_REGISTRY=1
endif

ifeq ($(CONFIG_LIBUAVCAN_TOSTRING),y)
CXXFLAGS += -DUAVCAN_TOSTRING=1
else
CXXFLAGS += -DUAVCAN_TOSTRING=0
endif

ifeq ($(CONFIG_LIBUAVCAN_IMPLEMENT_PLACEMENT_NEW),y)
CXXFLAGS += -DUAVCAN_IMPLEMENT_PLACEMENT_NEW=1
endif

ifeq ($(CONFIG_LIBUAVCAN_USE_EXTERNAL_SNPRINTF),y)
CXXFLAGS += -DUAVCAN_USE_EXTERNAL_SNPRINTF=1
endif

ifeq ($(CONFIG_LIBUAVCAN_USE_EXTERNAL_FLOAT16_CONVERSION),y)
CXXFLAGS += -DUAVCAN_USE_EXTERNAL_FLOAT16_CONVERSION=1
endif

ifeq ($(CONFIG_LIBUAVCAN_NO_ASSERTIONS),y)
CXXFLAGS += -DUAVCAN_NO_ASSERTIONS=1
endif

ifneq ($(CONFIG_LIBUAVCAN_MEM_POOL_BLOCK_SIZE),0)
CXXFLAGS += -DUAVCAN_MEM_POOL_BLOCK_SIZE=$(CONFIG_LIBUAVCAN_MEM_POOL_BLOCK_SIZE)
endif

ifneq ($(CONFIG_LIBUAVCAN_FLOAT_COMPARISON_EPSILON_MULT),0)
CXXFLAGS += -DUAVCAN_FLOAT_COMPARISON_EPSILON_MULT=$(CONFIG_LIBUAVCAN_FLOAT_COMPARISON_EPSILON_MULT)
endif

ifneq ($(CONFIG_LIBUAVCAN_MAX_CAN_ACCEPTANCE_FILTERS),0)
CXXFLAGS += -DUAVCAN_MAX_CAN_ACCEPTANCE_FILTERS=$(CONFIG_LIBUAVCAN_MAX_CAN_ACCEPTANCE_FILTERS)
endif

ifneq ($(CONFIG_LIBUAVCAN_MAX_NETWORK_SIZE_HINT),0)
CXXFLAGS += -DUAVCAN_MAX_NETWORK_SIZE_HINT=$(CONFIG_LIBUAVCAN_MAX_NETWORK_SIZE_HINT)
endif

CXXEXT = .cpp
CXXOBJS = $(CXXSRCS:$(CXXEXT)=$(OBJEXT))

ifeq ($(WINTOOL),y)
  BIN = "${shell cygpath -w $(APPDIR)$(DELIM)libapps$(LIBEXT)}"
else
  BIN = $(APPDIR)$(DELIM)libapps$(LIBEXT)
endif
DEPBIN = $(APPDIR)/libapps$(LIBEXT)

ROOTDEPPATH = --dep-path .

VPATH =

all: .built
.PHONY: clean depend distclean preconfig
.PRECIOUS: ../../libapps$(LIBEXT)

$(LIBUAVCAN_PACKNAME):
	@echo "Downloading: $@"
	$(Q) $(WGET) -O $@ $(LIBUAVCAN_URL)$(DELIM)$(LIBUAVCAN_VERSION)$(PACKEXT)

$(LIBUAVCAN_UNPACKNAME): $(LIBUAVCAN_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(DSDL_PACKNAME):
	@echo "Downloading: $@"
	$(Q) $(WGET) -O $@ $(DSDL_URL)$(DELIM)$(DSDL_VERSION)$(PACKEXT)

$(DSDL_UNPACKNAME): $(DSDL_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(PYUAVCAN_PACKNAME):
	@echo "Downloading: $@"
	$(Q) $(WGET) -O $@ $(PYUAVCAN_URL)$(DELIM)$(PYUAVCAN_VERSION)$(PACKEXT)

$(PYUAVCAN_UNPACKNAME): $(PYUAVCAN_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

libuavcan: $(LIBUAVCAN_UNPACKNAME) $(DSDL_UNPACKNAME) $(PYUAVCAN_UNPACKNAME)
	$(call DELDIR, $@)
	$(Q) cp -R $(LIBUAVCAN_UNPACKNAME) $@
	$(Q) if [ -r $(LIBUAVCAN_PATCHNAME) ]; then \
		cat $(LIBUAVCAN_PATCHNAME) | patch -p0 || { echo "ERROR: Patch failed"; exit 1; }; \
		fi
	$(call DELDIR, $(LIBUAVCAN_DSDL_PATH))
	$(Q) cp -R $(DSDL_UNPACKNAME) $(LIBUAVCAN_DSDL_PATH)
	$(call DELDIR, $(LIBUAVCAN_PYUAVCAN_PATH))
	$(Q) cp -R $(PYUAVCAN_UNPACKNAME) $(LIBUAVCAN_PYUAVCAN_PATH)
	$(Q) touch $@

dsdlc_generated: libuavcan
	$(info $(shell $(LIBUAVCAN_DSDLC) $(UAVCAN_DSDL_DIR)))

$(APPDIR)$(DELIM)include$(DELIM)uavcan: dsdlc_generated
	$(Q) mkdir -p $(APPDIR)$(DELIM)include$(DELIM)uavcan
	$(Q) cp -R libuavcan$(DELIM)libuavcan$(DELIM)include$(DELIM)uavcan$(DELIM)* $(APPDIR)$(DELIM)include$(DELIM)uavcan
	$(Q) cp -R dsdlc_generated$(DELIM)uavcan$(DELIM)* $(APPDIR)$(DELIM)include$(DELIM)uavcan

$(CXXOBJS): %$(OBJEXT): %$(CXXEXT)
	$(call COMPILEXX, $<, $@)

.built: $(CXXOBJS)
	$(call ARCHIVE, $(BIN), $(CXXOBJS))
	$(Q) touch $@

install:

context: libuavcan
	$(Q) $(MAKE) $(APPDIR)$(DELIM)include$(DELIM)uavcan TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"

.depend: Makefile $(CXXSRCS)
	$(Q) $(MKDEP) $(ROOTDEPPATH) "$(CXX)" -- $(CXXFLAGS) -- $(CXXSRCS) >Make.dep
	$(Q) touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(foreach CXXOBJ, $(CXXOBJS), $(call DELFILE, $(CXXOBJ)))
	$(call DELDIR, dsdlc_generated)
	$(call DELDIR, $(APPDIR)$(DELIM)include$(DELIM)uavcan)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)
	$(call DELDIR, libuavcan)
	$(call DELDIR, $(LIBUAVCAN_UNPACKNAME))
	$(call DELFILE, $(LIBUAVCAN_PACKNAME))
	$(call DELDIR, $(DSDL_UNPACKNAME))
	$(call DELFILE, $(DSDL_PACKNAME))
	$(call DELDIR, $(PYUAVCAN_UNPACKNAME))
	$(call DELFILE, $(PYUAVCAN_PACKNAME))

preconfig:

-include Make.dep
