############################################################################
# apps/system/nxdiag/Makefile
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

NXTOOLSDIR = $(realpath $(TOPDIR))$(DELIM)tools
NXDIAGDIR = $(APPDIR)$(DELIM)system$(DELIM)nxdiag

# Sysinfo application info

PROGNAME = nxdiag
PRIORITY = $(CONFIG_SYSTEM_NXDIAG_PRIORITY)
STACKSIZE = $(CONFIG_SYSTEM_NXDIAG_STACKSIZE)
MODULE = $(CONFIG_SYSTEM_NXDIAG)

# Sysinfo application

MAINSRC = nxdiag.c
NXDIAG_FLAGS = "$(realpath $(TOPDIR))"

ifeq ($(CONFIG_SYSTEM_NXDIAG_CONF),y)
NXDIAG_FLAGS += --config
endif

ifeq ($(CONFIG_SYSTEM_NXDIAG_COMP_FLAGS),y)
NXDIAG_FLAGS += --flags \""$(shell echo '$(CFLAGS)' | sed 's/"/\\\\\\"/g')"\"
NXDIAG_FLAGS += \""$(shell echo '$(CXXFLAGS)' | sed 's/"/\\\\\\"/g')"\"
NXDIAG_FLAGS += \""$(shell echo '$(LDFLAGS)' | sed 's/"/\\\\\\"/g')"\"
endif

ifeq ($(CONFIG_SYSTEM_NXDIAG_HOST_PACKAGES),y)
NXDIAG_FLAGS += --packages
endif

ifeq ($(CONFIG_SYSTEM_NXDIAG_HOST_MODULES),y)
NXDIAG_FLAGS += --modules
endif

ifeq ($(CONFIG_SYSTEM_NXDIAG_HOST_PATH),y)
NXDIAG_FLAGS += --path
endif

ifeq ($(CONFIG_SYSTEM_NXDIAG_VERBOSE),y)
NXDIAG_FLAGS += --verbose
endif

# Vendor-specific checks

# Espressif

ifeq ($(CONFIG_SYSTEM_NXDIAG_ESPRESSIF),y)
NXDIAG_FLAGS += "--target_info"
PY_FLAGS += "-B"
endif

# Common build

.PHONY: sysinfo.h

checkpython3:
	@if [ -z "$$(which python3)" ]; then \
		echo "ERROR: python3 not found in PATH"; \
		echo "       Please install python3 or fix the PATH"; \
		exit 1; \
	fi

sysinfo.h : checkpython3 $(INFO_DEPS)
	@python3 $(PY_FLAGS) $(NXTOOLSDIR)$(DELIM)host_info_dump.py $(NXDIAG_FLAGS) > sysinfo.h
	if ([ $$? -ne 0 ]); then \
		echo "ERROR: Failed to generate sysinfo.h"; \
		exit 1; \
	fi

depend:: sysinfo.h

distclean::
	$(call DELFILE, sysinfo.h)

include $(APPDIR)/Application.mk
