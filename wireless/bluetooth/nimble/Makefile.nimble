############################################################################
# apps/wireless/bluetooth/nimble/Makefile.nimble
#
# SPDX-License-Identifier: Apache-2.0
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

NIMBLE_ROOT = $(APPDIR)/wireless/bluetooth/nimble/mynewt-nimble

# Configure NimBLE variables

ifneq ($(CONFIG_NIMBLE_TINYCRYPT),)
NIMBLE_CFG_TINYCRYPT = 1
endif

ifneq ($(CONFIG_NIMBLE_MESH),)
NIMBLE_CFG_MESH = 1
endif

# Skip files that don't build for this port

NIMBLE_IGNORE += $(NIMBLE_ROOT)/porting/nimble/src/hal_timer.c
NIMBLE_IGNORE += $(NIMBLE_ROOT)/porting/nimble/src/os_cputime.c
NIMBLE_IGNORE += $(NIMBLE_ROOT)/porting/nimble/src/os_cputime_pwr2.c

# include NimBLE porting defs

-include $(NIMBLE_ROOT)/porting/nimble/Makefile.defs

CSRCS += $(NIMBLE_SRC)

# Source files for NPL OSAL

CSRCS += $(wildcard $(NIMBLE_ROOT)/porting/npl/nuttx/src/*.c)
CSRCS += $(wildcard $(NIMBLE_ROOT)/nimble/transport/socket/src/*.c)
CSRCS += $(TINYCRYPT_SRC)

# Add NPL and all NimBLE directories to include paths

NIMBLE_ALL_INC =  $(APPDIR)/wireless/bluetooth/nimble/include
NIMBLE_ALL_INC += $(NIMBLE_ROOT)/porting/npl/nuttx/include
NIMBLE_ALL_INC += $(NIMBLE_ROOT)/nimble/transport/socket/include
NIMBLE_ALL_INC += $(NIMBLE_ROOT)/nimble/include
NIMBLE_ALL_INC += $(NIMBLE_INCLUDE)
NIMBLE_ALL_INC += $(TINYCRYPT_INCLUDE)

CFLAGS += $(addprefix ${INCDIR_PREFIX}, $(NIMBLE_ALL_INC))
CXXFLAGS += $(addprefix ${INCDIR_PREFIX}, $(NIMBLE_ALL_INC))

# NimBLE assumes this flag since it expects undefined macros to be zero value

CFLAGS += -Wno-pointer-to-int-cast -Wno-undef

# disable printf format checks

CFLAGS += -Wno-format -Wno-unused-but-set-variable
