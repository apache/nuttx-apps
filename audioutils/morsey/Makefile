############################################################################
# apps/audioutils/morsey/Makefile
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

CSRCS = $(MORSEY_DIR)/morsey.c

MORSEY_DIR = morsey
MORSEY_VERSION = v1.0.0
MORSEY_MORSEY_H_SHA256 = 55f45e7b9a4d810f928ffe3ee9bea4cc318e28b1f49abf9512aee2638bd1dbfa
MORSEY_MORSEY_C_SHA256 = f946c9845a90a4c1a459490906ae071d0ac5b41326b7a5ce971efb97792ef56a

$(MORSEY_DIR):
	$(Q) echo "Cloning Morsey repo..."
	$(Q) git clone --depth=1 --branch=$(MORSEY_VERSION) https://github.com/linguini1/morsey.git
	$(Q) touch $(MORSEY_DIR)
	$(Q) echo "Checking Morsey hashes..."
	$(Q) $(APPDIR)/tools/check-hash.sh sha256 $(MORSEY_MORSEY_H_SHA256) $@/morsey.h
	$(Q) $(APPDIR)/tools/check-hash.sh sha256 $(MORSEY_MORSEY_C_SHA256) $@/morsey.c

create_includes: $(MORSEY_DIR)/morsey.h
	$(Q) cp $< $(APPDIR)/include/audioutils

context:: $(MORSEY_DIR)
	$(Q) $(MAKE) create_includes

distclean::
	$(call DELFILE, $(APPDIR)/include/audioutils/morsey.h)
	$(call DELDIR, morsey)

include $(APPDIR)/Application.mk
