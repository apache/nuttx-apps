#############################################################################
# apps/netutils/libulut/Makefile
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
#############################################################################

include $(APPDIR)/Make.defs

ULUTDIR = ulut

# Commit hash of the supported ulut revision
ULUT_COMMIT_HASH := c931ea417a8dbfa57888984a956c49f39214d584

ULUT_TARGZ := ulut-$(ULUT_COMMIT_HASH).tar.gz

# Take from Gitlab Mirror
ULUT_URL := https://gitlab.com/pikron/sw-base/ulut/-/archive/$(ULUT_COMMIT_HASH)/$(ULUT_TARGZ)

.PHONY: ulut-build-library

define RUN_OMK_MAKE
$(1):: $(ULUTDIR)
	make -C $(ULUTDIR) \
	CONFIG_OC_ULUT_INCDIRONLY=y \
	EXPORT_LIB_OBJS_EARLY=y \
	CC="$$(CC)" \
	CXX="$$(CXX)" \
	CFLAGS="$$(CFLAGS)" \
	CXXFLAGS="$$(CXXFLAGS)" \
	LD="$$(LD)" \
	LDFLAGS="$$(LDFLAGS)" \
	CONFIG_OC_ULUT_TESTS=n
endef

define IMPORT_OMK_DEPEND

$(1) : $$(foreach omk_d,$$(wildcard $(1).d),\
         $$(shell sed -n -e 's/^ \([^\]*\)[\]*$$$$/ \1/p' $$(omk_d)))

endef

ifneq ($(wildcard $(ULUTDIR)/_build),)
-include $(shell true; find $(ULUTDIR)/_build -name 'lib*.a.omkvar') # `true' is a hack for MinGW
ULUTOBJS = $(sort $(foreach lib,$(static_libs),$($(lib)_objsar)))
endif

EXTOBJS += $(ULUTOBJS)

$(ULUTOBJS) : ulut-build-library

$(ULUT_TARGZ):
	@echo "Downloading uLut from $(ULUT_URL)"
	$(Q) curl -L $(ULUT_URL) -o $(ULUT_TARGZ)

$(ULUTDIR): $(ULUT_TARGZ)
	@echo "Unpacking $(ULUT_TARGZ) -> $(ULUTDIR)"
	$(Q) tar xzf $(ULUT_TARGZ)
	$(Q) mv ulut-$(ULUT_COMMIT_HASH) $(ULUTDIR)
	$(Q) touch $(ULUTDIR)

$(eval $(call RUN_OMK_MAKE, ulut-build-library, library-pass))

$(foreach omk_o,$(ULUTOBJS),$(eval $(call IMPORT_OMK_DEPEND,$(omk_o))))

ifeq ($(wildcard $(ULUTDIR)/.git),)
$(eval $(call RUN_OMK_MAKE, context, default-config include-pass))

distclean::
	make -C $(ULUTDIR) distclean
	$(call DELDIR, $(ULUTDIR))
	$(call DELFILE, $(ULUT_TARGZ))
endif

include $(APPDIR)/Application.mk
