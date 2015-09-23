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

CXXEXT = .cpp

include libuavcan/libuavcan/include.mk
include libuavcan/libuavcan_drivers/stm32/driver/include.mk

$(info $(shell $(LIBUAVCAN_DSDLC) $(UAVCAN_DSDL_DIR)))

CXXSRCS += $(LIBUAVCAN_SRC) $(LIBUAVCAN_STM32_SRC)

include $(APPDIR)/Application.mk

CXXFLAGS += -I$(LIBUAVCAN_INC) -I$(LIBUAVCAN_STM32_INC) -Idsdlc_generated
CXXFLAGS += -I$(TOPDIR)/arch/arm/src/stm32

CXXFLAGS += -DUAVCAN_STM32_NUTTX=1
CXXFLAGS += -DUAVCAN_STM32_TIMER_NUMBER=$(CONFIG_UAVCAN_STM32_TIMER_NUMBER)

ifeq ($(CONFIG_STM32_CAN2),y)
CXXFLAGS += -DUAVCAN_STM32_NUM_IFACES=2
else
CXXFLAGS += -DUAVCAN_STM32_NUM_IFACES=1
endif

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
