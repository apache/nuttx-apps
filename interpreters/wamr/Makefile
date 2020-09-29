############################################################################
# interpreters/wamr/Makefile
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
WAMR_TARBALL  = WAMR-$(WAMR_VERSION).tar.gz
WAMR_URL_BASE = https://github.com/bytecodealliance/wasm-micro-runtime/archive/
WAMR_URL      = $(WAMR_URL_BASE)/$(WAMR_TARBALL)

-include $(WAMR_UNPACK)/product-mini/platforms/nuttx/wamr.mk

VPATH += $(WAMR_UNPACK)/product-mini/platforms/nuttx

MAINSRC = main.c

PROGNAME  = iwamr
PRIORITY  = $(CONFIG_INTERPRETERS_WAMR_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_WAMR_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_WAMR)

$(WAMR_TARBALL):
	$(Q) echo "Downloading $(WAMR_TARBALL)"
	$(Q) wget $(WAMR_URL)

$(WAMR_UNPACK): $(WAMR_TARBALL)
	$(Q) echo "Unpacking $(WAMR_TARBALL) to $(WAMR_UNPACK)"
	$(Q) tar xzvf $(WAMR_TARBALL)
	$(Q) mv wasm-micro-runtime-WAMR-$(WAMR_VERSION) $(WAMR_UNPACK)

context:: $(WAMR_UNPACK)

distclean::
	$(call DELDIR, $(WAMR_UNPACK))
	$(call DELFILE, $(WAMR_TARBALL))

include $(APPDIR)/Application.mk
