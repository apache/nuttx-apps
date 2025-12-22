############################################################################
# apps/testing/nest/Makefile
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

# Application info

PROGNAME = $(CONFIG_TESTING_NEST_PROGNAME)
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_TESTING_NEST_STACKSIZE)
MODULE = $(CONFIG_TESTING_OSTEST)

# Source inclusion

MAINSRC = nest_main.c
CSRCS = 

ifeq ($(CONFIG_TESTING_NEST_COLLECTIONS),y)

ifeq ($(CONFIG_TESTING_NEST_COLLECTIONS_LIST),y)
CSRCS += collections/list.c
endif

ifeq ($(CONFIG_TESTING_NEST_COLLECTIONS_CBUF),y)
CSRCS += collections/cbuf.c
endif

ifeq ($(CONFIG_TESTING_NEST_COLLECTIONS_HMAP),y)
CSRCS += collections/hmap.c
endif

endif # CONFIG_TESTING_NEST_COLLECTIONS

ifeq ($(CONFIG_TESTING_NEST_DEVICES),y)

ifeq ($(CONFIG_TESTING_NEST_DEVICES_DEVNULL),y)
CSRCS += devices/devnull.c
endif

ifeq ($(CONFIG_TESTING_NEST_DEVICES_DEVZERO),y)
CSRCS += devices/devzero.c
endif

ifeq ($(CONFIG_TESTING_NEST_DEVICES_DEVASCII),y)
CSRCS += devices/devascii.c
endif

ifeq ($(CONFIG_TESTING_NEST_DEVICES_DEVCONSOLE),y)
CSRCS += devices/devconsole.c
endif

ifeq ($(CONFIG_TESTING_NEST_DEVICES_DEVURANDOM),y)
CSRCS += devices/devurandom.c
endif

endif # CONFIG_TESTING_NEST_DEVICES

include $(APPDIR)/Application.mk
