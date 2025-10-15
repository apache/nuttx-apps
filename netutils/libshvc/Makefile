#############################################################################
# apps/netutils/libshvc/Makefile
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

SHVDIR := shv-libs4c

# Commit hash of the supported shv-libs4c revision
SHV_LIBS4C_COMMIT_HASH := 149cf26b71c73b9d5f59b5b21d5b079b1a1150c1

SHV_LIBS4C_TARGZ := shv-libs4c-$(SHV_LIBS4C_COMMIT_HASH).tar.gz

SHV_LIBS4C_URL := https://codeload.github.com/silicon-heaven/shv-libs4c/tar.gz/$(SHV_LIBS4C_COMMIT_HASH)

.PHONY: shv-build-library

define RUN_OMK_MAKE
$(1):: $(SHVDIR)
	make -C $(SHVDIR) \
	CONFIG_SHV_LIBS4C_PLATFORM="nuttx" $(2) \
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

ifneq ($(wildcard $(SHVDIR)/_build),)
-include $(shell true; find $(SHVDIR)/_build -name 'lib*.a.omkvar') # `true' is a hack for MinGW
SHVOBJS = $(sort $(foreach lib,$(static_libs),$($(lib)_objsar)))
endif

EXTOBJS += $(SHVOBJS)

$(SHVOBJS) : shv-build-library

$(SHV_LIBS4C_TARGZ):
	@echo "Downloading SHV from $(SHV_LIBS4C_URL)"
	$(Q) curl -L $(SHV_LIBS4C_URL) -o $(SHV_LIBS4C_TARGZ)

$(SHVDIR): $(SHV_LIBS4C_TARGZ)
	@echo "Unpacking $(SHV_LIBS4C_TARGZ) -> $(SHVDIR)"
	$(Q) tar xzf $(SHV_LIBS4C_TARGZ)
	$(Q) mv shv-libs4c-$(SHV_LIBS4C_COMMIT_HASH) $(SHVDIR)
	$(Q) touch $(SHVDIR)

$(eval $(call RUN_OMK_MAKE, shv-build-library, library-pass))

$(foreach omk_o,$(SHVOBJS),$(eval $(call IMPORT_OMK_DEPEND,$(omk_o))))

ifeq ($(wildcard $(SHVDIR)/.git),)
$(eval $(call RUN_OMK_MAKE, context, default-config include-pass))

distclean::
	make -C $(SHVDIR) distclean
	$(call DELDIR, $(SHVDIR))
	$(call DELFILE, $(SHV_LIBS4C_TARGZ))
endif

include $(APPDIR)/Application.mk
