############################################################################
# apps/interpreters/wamr/Makefile
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

# Tarball of main branch and tagged release have little difference
# in name after unpack and URL

ifeq ($(WAMR_VERSION), main)
WAMR_VERSION_STUB  = 
else
WAMR_VERSION_STUB  = WAMR-
endif

WAMR_TARBALL  = $(WAMR_VERSION_STUB)$(WAMR_VERSION).zip
WAMR_URL_BASE = https://github.com/bytecodealliance/wasm-micro-runtime/archive/
WAMR_URL      = $(WAMR_URL_BASE)/$(WAMR_TARBALL)

-include $(WAMR_UNPACK)/product-mini/platforms/nuttx/wamr.mk

VPATH += $(WAMR_UNPACK)/product-mini/platforms/nuttx

MAINSRC = main.c

PROGNAME  = iwasm
PRIORITY  = $(CONFIG_INTERPRETERS_WAMR_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_WAMR_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_WAMR)

$(WAMR_TARBALL):
	$(Q) echo "Downloading $(WAMR_TARBALL)"
	$(Q) curl -O -L $(WAMR_URL)

$(WAMR_UNPACK): $(WAMR_TARBALL)
	$(Q) echo "Unpacking $(WAMR_TARBALL) to $(WAMR_UNPACK)"
	$(Q) unzip $(WAMR_TARBALL)
	$(Q) mv wasm-micro-runtime-$(WAMR_VERSION_STUB)$(WAMR_VERSION) $(WAMR_UNPACK)
	$(Q) touch $(WAMR_UNPACK)

context:: $(WAMR_UNPACK)

distclean::
	$(call DELDIR, $(WAMR_UNPACK))
	$(call DELFILE, $(WAMR_TARBALL))

include $(APPDIR)/Application.mk
