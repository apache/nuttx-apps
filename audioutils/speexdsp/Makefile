############################################################################
# apps/audioutils/speexdsp/Makefile
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

SPEEXDSP_URL ?= "https://github.com/xiph/speexdsp/archive/refs/tags"
SPEEXDSP_VERSION ?= SpeexDSP-1.2.1

SPEEXDSP_DIR := $(APPDIR)/audioutils/speexdsp
SPEEXDSP_UNPACKNAME := speexdsp
SPEEXDSP_UNPACKDIR := $(SPEEXDSP_DIR)/$(SPEEXDSP_UNPACKNAME)

$(SPEEXDSP_UNPACKDIR):
	@echo "Downloading: $(SPEEXDSP_UNPACKNAME)"
	$(Q) curl -O -L $(SPEEXDSP_URL)/$(SPEEXDSP_VERSION).zip
	$(Q) unzip -o $(SPEEXDSP_VERSION).zip
	$(call DELFILE, $(SPEEXDSP_VERSION).zip)
	$(call MOVEFILE, speexdsp-$(SPEEXDSP_VERSION), $(SPEEXDSP_UNPACKDIR))

CSRCS += $(SPEEXDSP_UNPACKDIR)/libspeexdsp/resample.c

CFLAGS += -DOUTSIDE_SPEEX -DRANDOM_PREFIX=nuttx

ifeq ($(CONFIG_FIXED_POINT),y)
CFLAGS += -DFIXED_POINT
else
CFLAGS += -DFLOATING_POINT
endif

clean::
	$(call DELFILE, $(OBJS))

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(SPEEXDSP_UNPACKDIR)/.git),)
context:: $(SPEEXDSP_UNPACKDIR)

distclean::
	$(call DELDIR, $(SPEEXDSP_UNPACKDIR))
endif

include $(APPDIR)/Application.mk
