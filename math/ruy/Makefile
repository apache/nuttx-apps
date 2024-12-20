############################################################################
# apps/math/ruy/Makefile
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

RUY_VER = d37128311b445e758136b8602d1bbd2a755e115d

ruy.zip:
	$(Q) curl -L https://github.com/google/ruy/archive/$(RUY_VER).zip -o ruy.zip
	$(Q) unzip -o ruy.zip
	$(Q) mv ruy-$(RUY_VER) ruy

# Download and unpack tarball if no git repo found
ifeq ($(wildcard ruy/.git),)
context:: ruy.zip

distclean::
	$(call DELDIR, ruy)
	$(call DELFILE, ruy.zip)
endif

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/math/ruy/ruy
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/math/ruy/ruy

include $(APPDIR)/Application.mk
