############################################################################
# apps/audioutils/rtttl-c/Makefile
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

CSRCS = $(RTTTL_C_DIR)/rtttl.c

RTTTL_C_URL = https://git.sr.ht/~qeef/rtttl-c/archive
RTTTL_C_DIR = rtttl-c
RTTTL_C_VERSION = v0.1.0
RTTTL_C_TARBALL = $(RTTTL_C_VERSION).tar.gz
RTTTL_C_RTTTL_H_SHA256 = 6331174ae34a419ad427d9755f8ba9d3b6a92c09ecc4f36bbe07ec5e4d0cc007
RTTTL_C_RTTTL_C_SHA256 = f0db64ef0b68136bac24616a3d62508bcc54ccdd68265d6447d99b264653b7b9

$(RTTTL_C_TARBALL):
	$(Q) echo "Downloading rtttl-c $(RTTTL_C_VERSION)..."
	$(Q) curl -L --connect-timeout 15 --retry 3 --retry-delay 5 \
		-o $@ $(RTTTL_C_URL)/$@

$(RTTTL_C_DIR): $(RTTTL_C_TARBALL)
	$(Q) echo "Unpacking $< -> $@"
	$(call DELDIR, $(RTTTL_C_DIR)-$(RTTTL_C_VERSION))
	$(call DELDIR, $@)
	$(Q) tar -xzf $<
	$(Q) mv $(RTTTL_C_DIR)-$(RTTTL_C_VERSION) $@
	$(Q) touch $(RTTTL_C_DIR)
	$(Q) echo "Checking rtttl-c hashes..."
	$(Q) $(APPDIR)/tools/check-hash.sh sha256 $(RTTTL_C_RTTTL_H_SHA256) $@/rtttl.h
	$(Q) $(APPDIR)/tools/check-hash.sh sha256 $(RTTTL_C_RTTTL_C_SHA256) $@/rtttl.c

create_includes: $(RTTTL_C_DIR)/rtttl.h
	$(Q) cp $< $(APPDIR)/include/audioutils

context:: $(RTTTL_C_DIR)
	$(Q) $(MAKE) create_includes

distclean::
	$(call DELFILE, $(APPDIR)/include/audioutils/rtttl.h)
	$(call DELDIR, $(RTTTL_C_DIR))
	$(call DELFILE, $(RTTTL_C_TARBALL))

include $(APPDIR)/Application.mk
