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

SHVDIR = shv-libs4c

# Commit hash of the supported shv-libs4c revision (master)
SHV_LIBS4C_COMMIT_HASH := a8aa519a85e503a44b819fe3a4919dccc6fd559a

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

$(SHVDIR):
	$(call CLONE, https://github.com/silicon-heaven/shv-libs4c, $(SHVDIR), )
	( cd $(SHVDIR) && \
	  git checkout $(SHV_LIBS4C_COMMIT_HASH) ) > $(EMPTYFILE) 2>&1

$(eval $(call RUN_OMK_MAKE, shv-build-library, library-pass))

$(eval $(call RUN_OMK_MAKE, context, default-config include-pass))

$(foreach omk_o,$(SHVOBJS),$(eval $(call IMPORT_OMK_DEPEND,$(omk_o))))

distclean::
	make -C $(SHVDIR) distclean
	$(call DELDIR, $(SHVDIR))

include $(APPDIR)/Application.mk
