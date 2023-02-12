############################################################################
# apps/wireless/esp-hosted/Makefile
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
	CSRCS = {CODE_BASE}/common/include
	CSRCS = {CODE_BASE}/common/protobuf-c
	CSRCS = {CODE_BASE}/host/control_lib/include
	CSRCS = {CODE_BASE}/host/control_lib/src/include
	CSRCS = {CODE_BASE}/host/virtual_serial_if/include
	CSRCS = {CODE_BASE}/host/stm32/port/include
	CSRCS = {CODE_BASE}/host/stm32/common
	CSRCS = {CODE_BASE}/host/stm32/driver/transport/spi
	CSRCS = {CODE_BASE}/host/stm32/driver/serial/
	CSRCS = {CODE_BASE}/host/stm32/driver/network
	CSRCS = {CODE_BASE}/host/stm32/driver/netif
	CSRCS = {CODE_BASE}/host/stm32/app/control
	CSRCS = {CODE_BASE}/host/stm32/app/data
	CSRCS = {CODE_BASE}/host/stm32/app

CSRCS   = app.c
CSRCS = nx_events.c nx_kbdin.c
MAINSRC = nx_main.c
CFLAGS += -std=c99
PROGNAME = esphosted
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_NX)

include $(APPDIR)/Application.mk


