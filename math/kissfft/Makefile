############################################################################
# apps/math/kissfft/Makefile
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

CSRCS += kissfft/tools/kiss_fftr.c
CSRCS += kissfft/kiss_fft.c

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/math/kissfft/kissfft

KISSFFT_VER = 130
KISSFFT_UNPACK = kissfft

kissfft.zip:
	$(Q) curl -L https://github.com/mborgerding/kissfft/archive/refs/tags/v$(KISSFFT_VER).zip -o kissfft.zip
	$(Q) unzip -o kissfft.zip
	$(Q) mv kissfft-$(KISSFFT_VER) kissfft
	$(Q) patch -d $(KISSFFT_UNPACK) -p1 < kissfft.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard kissfft/.git),)
context:: kissfft.zip
distclean::
	$(call DELDIR, kissfft)
	$(call DELFILE, kissfft.zip)
endif

include $(APPDIR)/Application.mk
