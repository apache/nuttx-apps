############################################################################
# apps/canutils/libcanardv0/Makefile
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

LIBCANARDV0_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARDV0_URL)))
LIBCANARDV0_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARDV0_VERSION)))
LIBCANARDV0_UNPACKNAME = libcanard-$(LIBCANARDV0_VERSION)
LIBCANARDV0_PACKNAME = $(LIBCANARDV0_UNPACKNAME)$(PACKEXT)
LIBCANARDV0_SRCDIR = $(LIBCANARDV0_UNPACKNAME)
LIBCANARDV0_DRVDIR = $(LIBCANARDV0_SRCDIR)$(DELIM)drivers$(DELIM)nuttx

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)canutils

CFLAGS += -std=c99 -DCANARD_ASSERT=DEBUGASSERT
CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPS_INCDIR)}

CSRCS = $(LIBCANARDV0_SRCDIR)$(DELIM)canard.c $(LIBCANARDV0_DRVDIR)$(DELIM)canard_nuttx.c

$(LIBCANARDV0_PACKNAME):
	@echo "Downloading: $@"
	$(Q) curl -o $@ -L $(LIBCANARDV0_URL)$(DELIM)$(LIBCANARDV0_VERSION)$(PACKEXT)

$(LIBCANARDV0_UNPACKNAME): $(LIBCANARDV0_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(LIBCANARDV0_SRCDIR)$(DELIM)canard.h: $(LIBCANARDV0_UNPACKNAME)

$(LIBCANARDV0_DRVDIR)$(DELIM)canard_nuttx.h: $(LIBCANARDV0_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)canard.h: $(LIBCANARDV0_SRCDIR)$(DELIM)canard.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)canard_nuttx.h: $(LIBCANARDV0_DRVDIR)$(DELIM)canard_nuttx.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)canard.h $(APPS_INCDIR)$(DELIM)canard_nuttx.h

clean::
	$(foreach OBJ, $(OBJS), $(call DELFILE, $(OBJ)))

distclean::
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard.h)
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard_nuttx.h)
	$(call DELDIR, $(LIBCANARDV0_UNPACKNAME))
	$(call DELFILE, $(LIBCANARDV0_PACKNAME))

include $(APPDIR)/Application.mk
