############################################################################
# apps/wireless/bluetooth/nimble/Makefile
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

PRIORITY  = 255
STACKSIZE = 16384

NIMBLE_ROOT = $(APPDIR)/wireless/bluetooth/nimble/mynewt-nimble

-include $(NIMBLE_ROOT)/porting/examples/nuttx/Make.defs

NIMBLE_TAR := $(CONFIG_NIMBLE_REF).tar.gz
NIMBLE_URL := https://github.com/v01d/mynewt-nimble/archive/$(NIMBLE_TAR)

$(NIMBLE_TAR):
	wget $(NIMBLE_URL) -O $(NIMBLE_TAR)

$(NIMBLE_ROOT): $(NIMBLE_TAR)
	tar zxf $(NIMBLE_TAR)
	mv mynewt-nimble-$(CONFIG_NIMBLE_REF) mynewt-nimble

context:: $(NIMBLE_ROOT)

distclean::
	$(call CLEAN,$(NIMBLE_TAR))
	$(call DELDIR,$(NIMBLE_ROOT))


include $(APPDIR)/Application.mk
