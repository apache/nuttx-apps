############################################################################
# apps/canutils/uavcan/Makefile
#
#   Copyright (C) 2015 Omni Hoverboards Inc. All rights reserved.
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
include $(APPDIR)/Make.defs

WGET = wget
UNPACK = unzip
PACKEXT = .zip

LIBUAVCAN_URL = $(patsubst "%",%,$(strip $(CONFIG_UAVCAN_LIBUAVCAN_URL)))
LIBUAVCAN_VERSION = $(patsubst "%",%,$(strip $(CONFIG_UAVCAN_LIBUAVCAN_VERSION)))
LIBUAVCAN_UNPACKNAME = libuavcan-$(LIBUAVCAN_VERSION)
LIBUAVCAN_PACKNAME = $(LIBUAVCAN_UNPACKNAME)$(PACKEXT)
LIBUAVCAN_DSDL_PATH = libuavcan$(DELIM)dsdl
LIBUAVCAN_PYUAVCAN_PATH = libuavcan$(DELIM)libuavcan$(DELIM)dsdl_compiler$(DELIM)pyuavcan

DSDL_URL = $(patsubst "%",%,$(strip $(CONFIG_UAVCAN_DSDL_URL)))
DSDL_VERSION = $(patsubst "%",%,$(strip $(CONFIG_UAVCAN_DSDL_VERSION)))
DSDL_UNPACKNAME = dsdl-$(DSDL_VERSION)
DSDL_PACKNAME = $(DSDL_UNPACKNAME)$(PACKEXT)

PYUAVCAN_URL = $(patsubst "%",%,$(strip $(CONFIG_UAVCAN_PYUAVCAN_URL)))
PYUAVCAN_VERSION = $(patsubst "%",%,$(strip $(CONFIG_UAVCAN_PYUAVCAN_VERSION)))
PYUAVCAN_UNPACKNAME = pyuavcan-$(PYUAVCAN_VERSION)
PYUAVCAN_PACKNAME = $(PYUAVCAN_UNPACKNAME)$(PACKEXT)

-include libuavcan/libuavcan/include.mk
-include libuavcan/libuavcan_drivers/stm32/driver/include.mk

CXXSRCS = $(LIBUAVCAN_SRC) $(LIBUAVCAN_STM32_SRC)

CXXFLAGS += -I$(LIBUAVCAN_INC) -I$(LIBUAVCAN_STM32_INC) -Idsdlc_generated
CXXFLAGS += -I$(TOPDIR)/arch/arm/src/common -I$(TOPDIR)/arch/arm/src/stm32

CXXFLAGS += -DUAVCAN_STM32_NUTTX=1
CXXFLAGS += -DUAVCAN_STM32_NUM_IFACES=$(CONFIG_UAVCAN_STM32_NUM_IFACES)
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=$(CONFIG_UAVCAN_STM32_TIMER_NUMBER)

ifeq ($(CONFIG_UAVCAN_CPP03),y)
CXXFLAGS += -std=c++03 -DUAVCAN_CPP_VERSION=UAVCAN_CPP03
else
ifeq ($(CONFIG_UAVCAN_CPP11),y)
CXXFLAGS += -std=c++11 -DUAVCAN_CPP_VERSION=UAVCAN_CPP11
endif
endif

ifeq ($(CONFIG_UAVCAN_TINY),y)
CXXFLAGS += -DUAVCAN_TINY=1
endif

ifeq ($(CONFIG_UAVCAN_TOSTRING),y)
CXXFLAGS += -DUAVCAN_TOSTRING=1
endif

ifeq ($(CONFIG_UAVCAN_IMPLEMENT_PLACEMENT_NEW),y)
CXXFLAGS += -DUAVCAN_IMPLEMENT_PLACEMENT_NEW=1
endif

ifeq ($(CONFIG_UAVCAN_USE_EXTERNAL_SNPRINTF),y)
CXXFLAGS += -DUAVCAN_USE_EXTERNAL_SNPRINTF=1
endif

ifeq ($(CONFIG_UAVCAN_USE_EXTERNAL_FLOAT16_CONVERSION),y)
CXXFLAGS += -DUAVCAN_USE_EXTERNAL_FLOAT16_CONVERSION=1
endif

ifeq ($(CONFIG_UAVCAN_NO_ASSERTIONS),y)
CXXFLAGS += -DUAVCAN_NO_ASSERTIONS=1
endif

ifneq ($(CONFIG_UAVCAN_MEM_POOL_BLOCK_SIZE),0)
CXXFLAGS += -DUAVCAN_MEM_POOL_BLOCK_SIZE=$(CONFIG_UAVCAN_MEM_POOL_BLOCK_SIZE)
endif

CXXEXT = .cpp
CXXOBJS = $(CXXSRCS:$(CXXEXT)=$(OBJEXT))

ifeq ($(WINTOOL),y)
  BIN = "${shell cygpath -w $(APPDIR)$(DELIM)libapps$(LIBEXT)}"
else
  BIN = $(APPDIR)$(DELIM)libapps$(LIBEXT)
endif

ROOTDEPPATH = --dep-path .

VPATH =

all: .built
.PHONY: clean depend distclean

$(LIBUAVCAN_PACKNAME):
	@echo "Downloading: $(LIBUAVCAN_PACKNAME)"
	$(Q) $(WGET) -O $(LIBUAVCAN_PACKNAME) $(LIBUAVCAN_URL)/$(LIBUAVCAN_VERSION)$(PACKEXT)

$(LIBUAVCAN_UNPACKNAME): $(LIBUAVCAN_PACKNAME)
	@echo "Unpacking: $(LIBUAVCAN_PACKNAME) -> $(LIBUAVCAN_UNPACKNAME)"
	$(Q) $(UNPACK) $(LIBUAVCAN_PACKNAME)
	$(Q) touch $(LIBUAVCAN_UNPACKNAME)

$(DSDL_PACKNAME):
	@echo "Downloading: $(DSDL_PACKNAME)"
	$(Q) $(WGET) -O $(DSDL_PACKNAME) $(DSDL_URL)/$(DSDL_VERSION)$(PACKEXT)

$(DSDL_UNPACKNAME): $(DSDL_PACKNAME)
	@echo "Unpacking: $(DSDL_PACKNAME) -> $(DSDL_UNPACKNAME)"
	$(Q) $(UNPACK) $(DSDL_PACKNAME)
	$(Q) touch $(DSDL_UNPACKNAME)

$(PYUAVCAN_PACKNAME):
	@echo "Downloading: $(PYUAVCAN_PACKNAME)"
	$(Q) $(WGET) -O $(PYUAVCAN_PACKNAME) $(PYUAVCAN_URL)/$(PYUAVCAN_VERSION)$(PACKEXT)

$(PYUAVCAN_UNPACKNAME): $(PYUAVCAN_PACKNAME)
	@echo "Unpacking: $(PYUAVCAN_PACKNAME) -> $(PYUAVCAN_UNPACKNAME)"
	$(Q) $(UNPACK) $(PYUAVCAN_PACKNAME)
	$(Q) touch $(PYUAVCAN_UNPACKNAME)

libuavcan: $(LIBUAVCAN_UNPACKNAME) $(DSDL_UNPACKNAME) $(PYUAVCAN_UNPACKNAME)
	$(Q) cp -R $(LIBUAVCAN_UNPACKNAME) libuavcan
	$(call DELDIR, $(LIBUAVCAN_DSDL_PATH))
	$(Q) cp -R $(DSDL_UNPACKNAME) $(LIBUAVCAN_DSDL_PATH)
	$(call DELDIR, $(LIBUAVCAN_PYUAVCAN_PATH))
	$(Q) cp -R $(PYUAVCAN_UNPACKNAME) $(LIBUAVCAN_PYUAVCAN_PATH)

dsdlc_generated: libuavcan
	$(info $(shell $(LIBUAVCAN_DSDLC) $(UAVCAN_DSDL_DIR)))

$(APPDIR)/include/uavcan: libuavcan/libuavcan/include/uavcan
	$(Q) cp -R libuavcan/libuavcan/include/uavcan $(APPDIR)/include

$(CXXOBJS): %$(OBJEXT): %$(CXXEXT)
	$(call COMPILEXX, $<, $@)

.built: $(CXXOBJS)
	$(call ARCHIVE, $(BIN), $(CXXOBJS))
	$(Q) touch .built

install:

context: libuavcan dsdlc_generated $(APPDIR)/include/uavcan

.depend: Makefile $(CXXSRCS)
	$(Q) $(MKDEP) $(ROOTDEPPATH) "$(CXX)" -- $(CXXFLAGS) -- $(CXXSRCS) >Make.dep
	$(Q) touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call DELDIR, libuavcan)
	$(call DELDIR, dsdlc_generated)
	$(call DELDIR, $(APPDIR)/include/uavcan)

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)
	$(call DELDIR, $(LIBUAVCAN_UNPACKNAME))
	$(call DELFILE, $(LIBUAVCAN_PACKNAME))
	$(call DELDIR, $(DSDL_UNPACKNAME))
	$(call DELFILE, $(DSDL_PACKNAME))
	$(call DELDIR, $(PYUAVCAN_UNPACKNAME))
	$(call DELFILE, $(PYUAVCAN_PACKNAME))

-include Make.dep
