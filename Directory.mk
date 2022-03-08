############################################################################
# apps/Directory.mk
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

SUBDIRS       := $(dir $(wildcard *$(DELIM)Makefile))
CONFIGSUBDIRS := $(filter-out $(dir $(wildcard *$(DELIM)Kconfig)),$(SUBDIRS))
CLEANSUBDIRS  += $(dir $(wildcard *$(DELIM).depend))
CLEANSUBDIRS  += $(dir $(wildcard *$(DELIM).kconfig))
CLEANSUBDIRS  := $(sort $(CLEANSUBDIRS))

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
	$(Q) touch .kconfig
endif

clean: $(foreach SDIR, $(CLEANSUBDIRS), $(SDIR)_clean)

distclean: $(foreach SDIR, $(CLEANSUBDIRS), $(SDIR)_distclean)
ifneq ($(MENUDESC),)
	$(call DELFILE, Kconfig)
	$(call DELFILE, .kconfig)
endif

-include Make.dep
