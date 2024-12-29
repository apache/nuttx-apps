############################################################################
# apps/Directory.mk
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
#   http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# Sub-directories that have been built or configured.

SUBDIRS       := $(dir $(wildcard */Makefile))
CONFIGSUBDIRS := $(filter-out $(dir $(wildcard */Kconfig)),$(SUBDIRS))
CLEANSUBDIRS  := $(dir $(wildcard *$(DELIM).built))
CLEANSUBDIRS  += $(dir $(wildcard */.depend))
CLEANSUBDIRS  += $(dir $(wildcard */.kconfig))
CLEANSUBDIRS  := $(sort $(CLEANSUBDIRS))
ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  CONFIGSUBDIRS := $(subst /,\,$(CONFIGSUBDIRS))
  CLEANSUBDIRS  := $(subst /,\,$(CLEANSUBDIRS))
endif

all: nothing

.PHONY: nothing clean distclean

$(foreach SDIR, $(CONFIGSUBDIRS), $(eval $(call SDIR_template,$(SDIR),preconfig)))
$(foreach SDIR, $(CLEANSUBDIRS), $(eval $(call SDIR_template,$(SDIR),clean)))
$(foreach SDIR, $(CLEANSUBDIRS), $(eval $(call SDIR_template,$(SDIR),distclean)))

nothing:

install:

preconfig: $(foreach SDIR, $(CONFIGSUBDIRS), $(SDIR)_preconfig)
ifneq ($(MENUDESC),)
	$(Q) $(MKKCONFIG) -m $(MENUDESC)
endif
	$(Q) touch .kconfig

clean: $(foreach SDIR, $(CLEANSUBDIRS), $(SDIR)_clean)
	@:

distclean: $(foreach SDIR, $(CLEANSUBDIRS), $(SDIR)_distclean)
ifneq ($(MENUDESC),)
	$(call DELFILE, Kconfig)
endif
	$(call DELFILE, .kconfig)

-include Make.dep
