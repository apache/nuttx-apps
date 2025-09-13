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

# Commit hash of the supported ulut revision (master)
ULUT_COMMIT_HASH := 7aa18c41fc89b470cff9b5cb294687739402cf87

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

$(ULUTDIR):
	$(call CLONE, https://git.code.sf.net/p/ulan/ulut.git, $(ULUTDIR), )
	( cd $(ULUTDIR) && \
	  git checkout $(ULUT_LIBS4C_COMMIT_HASH) && \
	  git submodule update --recursive --init ) > $(EMPTYFILE) 2>&1

$(eval $(call RUN_OMK_MAKE, ulut-build-library, library-pass))

$(eval $(call RUN_OMK_MAKE, context, default-config include-pass))

$(foreach omk_o,$(ULUTOBJS),$(eval $(call IMPORT_OMK_DEPEND,$(omk_o))))

distclean::
	make -C $(ULUTDIR) distclean
	$(call DELDIR, $(ULUTDIR))

include $(APPDIR)/Application.mk
