############################################################################
# apps/interpreters/wamr/Makefile
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

WAMR_VERSION  = $(patsubst "%",%,$(strip $(CONFIG_INTERPRETERS_WAMR_VERSION)))
WAMR_UNPACK   = wamr

WAMR_TARBALL  = $(WAMR_VERSION).zip
WAMR_URL_BASE = https://github.com/bytecodealliance/wasm-micro-runtime/archive/
WAMR_URL      = $(WAMR_URL_BASE)/$(WAMR_TARBALL)

-include $(WAMR_UNPACK)/product-mini/platforms/nuttx/wamr.mk

VPATH += $(WAMR_UNPACK)/product-mini/platforms/nuttx
DEPPATH += --dep-path $(WAMR_UNPACK)/product-mini/platforms/nuttx

ifeq ($(CONFIG_INTERPRETERS_IWASM_TASK),y)
MAINSRC = main.c

PROGNAME  = iwasm
PRIORITY  = $(CONFIG_INTERPRETERS_WAMR_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_WAMR_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_WAMR)

endif

WAMR_NEED_CUSTOM_INIT = n

# Check if we need to build custom init code

ifeq ($(CONFIG_INTERPRETERS_WAMR_EXTERNAL_MODULE_REGISTRY),y)
WAMR_NEED_CUSTOM_INIT = y
endif

# If we need custom init code, add it to the build
ifeq ($(WAMR_NEED_CUSTOM_INIT),y)
# When the loadable module configuration is enabled,we need to use CELFFLAGS
$(WAMR_UNPACK)/product-mini/platforms/nuttx/main.c_CFLAGS = -Dwasm_runtime_full_init=wamr_custom_init
$(WAMR_UNPACK)/product-mini/platforms/nuttx/main.c_CELFFLAGS = -Dwasm_runtime_full_init=wamr_custom_init
CSRCS += wamr_custom_init.c
endif

$(WAMR_TARBALL):
	$(Q) echo "Downloading $(WAMR_TARBALL)"
	$(Q) curl -O -L $(WAMR_URL)

$(WAMR_UNPACK): $(WAMR_TARBALL)
	$(Q) echo "Unpacking $(WAMR_TARBALL) to $(WAMR_UNPACK)"
	$(Q) unzip $(WAMR_TARBALL)
	$(Q) mv wasm-micro-runtime-$(WAMR_VERSION) $(WAMR_UNPACK)
	$(Q) touch $(WAMR_UNPACK)

# Register the external WAMR module

ifeq ($(CONFIG_INTERPRETERS_WAMR_EXTERNAL_MODULE_REGISTRY),y)

PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))
BDATLIST = $(strip $(call RWILDCARD, registry, *.bdat))

registry$(DELIM).updated:
	$(Q) touch registry$(DELIM).updated

wamr_external_module_list.h: registry$(DELIM).updated
ifeq ($(BDATLIST),)
	$(call DELFILE, wamr_external_module_list.h)
	$(Q) touch wamr_external_module_list.h
else
	$(call CATFILE, wamr_external_module_list.h, $(BDATLIST))
endif

wamr_external_module_proto.h: registry$(DELIM).updated
ifeq ($(PDATLIST),)
	$(call DELFILE, wamr_external_module_proto.h)
	$(Q) touch wamr_external_module_proto.h
else
	$(call CATFILE, wamr_external_module_proto.h, $(PDATLIST))
endif

depend:: $(GLUECSRCS) wamr_external_module_list.h wamr_external_module_proto.h
endif # CONFIG_INTERPRETERS_WAMR_EXTERNAL_MODULE_REGISTRY

clean::
	$(call DELFILE, wamr_external_module_list.h)
	$(call DELFILE, wamr_external_module_proto.h)

clean_context::
	$(call DELFILE, $(BDATLIST))
	$(call DELFILE, $(PDATLIST))

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(WAMR_UNPACK)/.git),)
context:: $(WAMR_UNPACK)
endif

distclean:: clean clean_context
ifeq ($(wildcard $(WAMR_UNPACK)/.git),)
	$(call DELDIR, $(WAMR_UNPACK))
	$(call DELFILE, $(WAMR_TARBALL))
endif

include $(APPDIR)/Application.mk
