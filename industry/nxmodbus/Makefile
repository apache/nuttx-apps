############################################################################
# apps/industry/nxmodbus/Makefile
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

include $(APPDIR)/Make.defs

# NxModbus Library

ifeq ($(CONFIG_INDUSTRY_NXMODBUS),y)

# Core sources
CSRCS := core/nxmb_context.c
CSRCS += core/nxmb_utils.c

ifeq ($(CONFIG_NXMODBUS_SERVER),y)
CSRCS += core/nxmb_server.c
CSRCS += core/nxmb_func.c
CSRCS += core/nxmb_func_coils.c
CSRCS += core/nxmb_func_discrete.c
CSRCS += core/nxmb_func_holding.c
CSRCS += core/nxmb_func_input.c
CSRCS += core/nxmb_func_diag.c
CSRCS += core/nxmb_func_other.c
endif

ifeq ($(CONFIG_NXMODBUS_CLIENT),y)
CSRCS += core/nxmb_client.c
endif

# Transport layer sources
ifneq (,$(filter y,$(CONFIG_NXMODBUS_RTU) $(CONFIG_NXMODBUS_ASCII)))
CSRCS += transport/nxmb_serial_common.c
endif

ifeq ($(CONFIG_NXMODBUS_RTU),y)
CSRCS += transport/nxmb_crc.c
CSRCS += transport/nxmb_serial.c
endif

ifeq ($(CONFIG_NXMODBUS_ASCII),y)
CSRCS += transport/nxmb_lrc.c
CSRCS += transport/nxmb_ascii.c
endif

ifeq ($(CONFIG_NXMODBUS_RAW_ADU),y)
CSRCS += transport/nxmb_raw.c
endif

ifeq ($(CONFIG_NXMODBUS_TCP),y)
CSRCS += transport/nxmb_tcp.c
endif

# Include private headers
CFLAGS += ${INCDIR_PREFIX}$(CURDIR)

endif

include $(APPDIR)/Application.mk
