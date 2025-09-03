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

# Where to look for the extra object files
EXTOBJS = $(wildcard ./$(SHVDIR)/_build/user/libs4c/libshvtree/*.o)
EXTOBJS += $(wildcard ./$(SHVDIR)/_build/user/libs4c/libshvchainpack/*.o)
EXTOBJS += $(wildcard ./$(SHVDIR)/_build/user/submodule/ulut/ulut/*.o)

$(SHVDIR):
	$(call CLONE, https://github.com/silicon-heaven/shv-libs4c, $(SHVDIR), )
	
# Compile the library. Also pass all the NuttX Makefile variables.
compile-libshv: $(SHVDIR)
	$(Q) if [ ! -d "./$(SHVDIR)/_compiled" ]; then \
		( cd $(SHVDIR); git submodule update --recursive --init ) > /dev/null 2>&1; \
		make -C $(SHVDIR) CONFIG_SHV_LIBS4C_PLATFORM="nuttx" \
			CC="$(CC)" \
			CXX="$(CXX)" \
			CFLAGS="$(CFLAGS)" \
			CXXFLAGS="$(CXXFLAGS)" \
			LD="$(LD)" \
			LDFLAGS="$(LDFLAGS)" \
			CONFIG_OC_ULUT_TESTS=n; \
	fi

context:: compile-libshv

depend:: compile-libshv
	
distclean::
	make -C $(SHVDIR) distclean
	$(call DELDIR, $(SHVDIR))

include $(APPDIR)/Application.mk
