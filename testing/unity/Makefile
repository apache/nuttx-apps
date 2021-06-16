############################################################################
# apps/testing/unity/Makefile
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
###########################################################################

# Standard includes

include $(APPDIR)/Make.defs

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CONFIG_TESTING_UNITY_URL ?= "https://github.com/ThrowTheSwitch/Unity/archive"
CONFIG_TESTING_UNITY_VERSION ?= "2.5.2"
UNITY_VERSION = $(patsubst "%",%,$(strip $(CONFIG_TESTING_UNITY_VERSION)))

UNITY_TARBALL = v$(UNITY_VERSION).tar.gz

UNITY_UNPACKNAME = Unity-$(UNITY_VERSION)
UNPACK ?= tar -zxf

UNITY_UNPACKDIR =  $(WD)/$(UNITY_UNPACKNAME)
UNITY_SRCDIR = $(UNITY_UNPACKNAME)$(DELIM)src

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)testing

CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPS_INCDIR)}
CFLAGS += -DUNITY_INCLUDE_CONFIG_H

CSRCS = $(UNITY_SRCDIR)$(DELIM)unity.c

$(UNITY_TARBALL):
	@echo "Downloading: $(UNITY_TARBALL)"
	$(Q) curl -L -O $(CONFIG_TESTING_UNITY_URL)/$(UNITY_TARBALL)

$(UNITY_UNPACKNAME): $(UNITY_TARBALL)
	@echo "Unpacking: $(UNITY_TARBALL) -> $(UNITY_UNPACKNAME)"
	$(Q) $(UNPACK) $(UNITY_TARBALL)
	$(Q) touch $(UNITY_UNPACKNAME)

$(UNITY_SRCDIR)$(DELIM)unity.h: $(UNITY_UNPACKNAME)

$(UNITY_SRCDIR)$(DELIM)unity_internals.h: $(UNITY_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)unity.h: $(UNITY_SRCDIR)$(DELIM)unity.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)unity_internals.h: $(UNITY_SRCDIR)$(DELIM)unity_internals.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)unity.h $(APPS_INCDIR)$(DELIM)unity_internals.h

distclean::
	$(call DELDIR, $(UNITY_UNPACKNAME))
	$(call DELFILE, $(UNITY_TARBALL))
	$(call DELFILE, $(APPDIR)/include/testing/unity.h)
	$(call DELFILE, $(APPDIR)/include/testing/unity_internals.h)

include $(APPDIR)/Application.mk
