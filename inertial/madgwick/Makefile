############################################################################
# apps/inertial/madgwick/Makefile
#
# SPDX-License-Identifier: BSD-3-Clause
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

SRC = fusion/Fusion

CSRCS += $(SRC)/FusionAhrs.c
CSRCS += $(SRC)/FusionCompass.c
CSRCS += $(SRC)/FusionOffset.c

CFLAGS += -Wno-shadow -Wno-strict-prototypes -Wno-unknown-pragmas

MODULE = $(CONFIG_LIB_MADGWICK)

libmadgwick.tar.gz:
	$(Q) curl -L https://github.com/xioTechnologies/Fusion/archive/refs/tags/v$(CONFIG_LIB_MADGWICK_VER).tar.gz -o libmadgwick.tar.gz
	$(Q) tar xf libmadgwick.tar.gz
	$(Q) mv Fusion-$(CONFIG_LIB_MADGWICK_VER) fusion

# Download and unpack tarball if no git repo found
ifeq ($(wildcard madgwick/fusion),)
context:: libmadgwick.tar.gz

distclean::
	$(call DELDIR, fusion)
	$(call DELFILE, libmadgwick.tar.gz)
endif

include $(APPDIR)/Application.mk
