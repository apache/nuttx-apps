############################################################################
# apps/system/flatbuffers/Makefile
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

FLATBUFFERS_VER = 23.5.26
FLATBUFFERS_UNPACK = flatbuffers

flatbuffers.tar.gz:
	$(Q) curl -L https://github.com/google/flatbuffers/archive/v$(FLATBUFFERS_VER).tar.gz -o flatbuffers.tar.gz
	$(Q) tar -xzf flatbuffers.tar.gz
	$(Q) mv flatbuffers-$(FLATBUFFERS_VER) flatbuffers
	$(Q) patch -d $(FLATBUFFERS_UNPACK) -p1 < flatbuffers.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard flatbuffers/.git),)
context:: flatbuffers.tar.gz
distclean::
	$(call DELDIR, flatbuffers)
	$(call DELFILE, flatbuffers.tar.gz)
endif

include $(APPDIR)/Application.mk
