############################################################################
# apps/canutils/libcanard/Makefile
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

UNPACK = unzip
PACKEXT = .zip

LIBCANARD_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARD_URL)))
LIBCANARD_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARD_VERSION)))
LIBCANARD_UNPACKNAME = libcanard-$(LIBCANARD_VERSION)
LIBCANARD_PACKNAME = $(LIBCANARD_UNPACKNAME)$(PACKEXT)
LIBCANARD_SRCDIR = $(LIBCANARD_UNPACKNAME)
LIBCANARD_DRVDIR = $(LIBCANARD_SRCDIR)$(DELIM)drivers$(DELIM)nuttx

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)canutils

CFLAGS += -std=c99 -DCANARD_ASSERT=DEBUGASSERT
CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPS_INCDIR)}

CSRCS = $(LIBCANARD_SRCDIR)$(DELIM)canard.c $(LIBCANARD_DRVDIR)$(DELIM)canard_nuttx.c

$(LIBCANARD_PACKNAME):
	@echo "Downloading: $@"
	$(Q) curl -o $@ -L $(LIBCANARD_URL)$(DELIM)$(LIBCANARD_VERSION)$(PACKEXT)

$(LIBCANARD_UNPACKNAME): $(LIBCANARD_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(LIBCANARD_SRCDIR)$(DELIM)canard.h: $(LIBCANARD_UNPACKNAME)

$(LIBCANARD_DRVDIR)$(DELIM)canard_nuttx.h: $(LIBCANARD_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)canard.h: $(LIBCANARD_SRCDIR)$(DELIM)canard.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)canard_nuttx.h: $(LIBCANARD_DRVDIR)$(DELIM)canard_nuttx.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)canard.h $(APPS_INCDIR)$(DELIM)canard_nuttx.h

clean::
	$(foreach OBJ, $(OBJS), $(call DELFILE, $(OBJ)))

distclean::
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard.h)
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard_nuttx.h)
	$(call DELDIR, $(LIBCANARD_UNPACKNAME))
	$(call DELFILE, $(LIBCANARD_PACKNAME))

include $(APPDIR)/Application.mk
