############################################################################
# apps/canutils/libcanardv1/Makefile
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

LIBCANARDV1_URL = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARDV1_URL)))
LIBCANARDV1_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBCANARDV1_VERSION)))
LIBCANARDV1_UNPACKNAME = libcanard-$(LIBCANARDV1_VERSION)
LIBCANARDV1_PACKNAME = $(LIBCANARDV1_UNPACKNAME)$(PACKEXT)
LIBCANARDV1_SRCDIR = $(LIBCANARDV1_UNPACKNAME)
LIBCANARDV1_DRVDIR = $(LIBCANARDV1_SRCDIR)$(DELIM)libcanard

O1HEAP_URL = $(patsubst "%",%,$(strip $(CONFIG_O1HEAP_URL)))
O1HEAP_VERSION = $(patsubst "%",%,$(strip $(CONFIG_O1HEAP_VERSION)))
O1HEAP_UNPACKNAME = o1heap-$(O1HEAP_VERSION)
O1HEAP_PACKNAME = $(O1HEAP_UNPACKNAME)$(PACKEXT)
O1HEAP_SRCDIR = $(O1HEAP_UNPACKNAME)
O1HEAP_DRVDIR = $(O1HEAP_SRCDIR)$(DELIM)o1heap

APPS_INCDIR = $(APPDIR)$(DELIM)include$(DELIM)canutils

CFLAGS += -std=c11 -DCANARD_ASSERT=DEBUGASSERT -DCANARD_DSDL_CONFIG_LITTLE_ENDIAN=1
CFLAGS += ${shell $(INCDIR) "$(CC)" $(APPS_INCDIR)}

CSRCS  = $(LIBCANARDV1_DRVDIR)$(DELIM)canard.c $(LIBCANARDV1_DRVDIR)$(DELIM)canard_dsdl.c
CSRCS += $(O1HEAP_DRVDIR)$(DELIM)o1heap.c


$(LIBCANARDV1_PACKNAME):
	@echo "Downloading: $@"
	$(Q) curl -o $@ -L $(LIBCANARDV1_URL)$(DELIM)$(LIBCANARDV1_VERSION)$(PACKEXT)

$(LIBCANARDV1_UNPACKNAME): $(LIBCANARDV1_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@
	
$(O1HEAP_PACKNAME):
	@echo "Downloading: $@"
	$(Q) curl -o $@ -L $(O1HEAP_URL)$(DELIM)$(O1HEAP_VERSION)$(PACKEXT)

$(O1HEAP_UNPACKNAME): $(O1HEAP_PACKNAME)
	@echo "Unpacking: $< -> $@"
	$(call DELDIR, $@)
	$(Q) $(UNPACK) $<
	$(Q) touch $@

$(LIBCANARDV1_DRVDIR)$(DELIM)canard.h: $(LIBCANARDV1_UNPACKNAME)

$(LIBCANARDV1_DRVDIR)$(DELIM)canard_dsdl.h: $(LIBCANARDV1_UNPACKNAME)

$(O1HEAP_DRVDIR)$(DELIM)o1heap.h: $(O1HEAP_UNPACKNAME)

$(APPS_INCDIR)$(DELIM)canard.h: $(LIBCANARDV1_DRVDIR)$(DELIM)canard.h
	$(Q) cp $< $@

$(APPS_INCDIR)$(DELIM)canard_dsdl.h: $(LIBCANARDV1_DRVDIR)$(DELIM)canard_dsdl.h
	$(Q) cp $< $@
	
$(APPS_INCDIR)$(DELIM)o1heap.h: $(O1HEAP_DRVDIR)$(DELIM)o1heap.h
	$(Q) cp $< $@

context:: $(APPS_INCDIR)$(DELIM)canard.h $(APPS_INCDIR)$(DELIM)canard_dsdl.h $(APPS_INCDIR)$(DELIM)o1heap.h

clean::
	$(foreach OBJ, $(OBJS), $(call DELFILE, $(OBJ)))

distclean::
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard.h)
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)canard_dsdl.h)
	$(call DELDIR, $(LIBCANARDV1_UNPACKNAME))
	$(call DELFILE, $(LIBCANARDV1_PACKNAME))
	$(call DELDIR, $(O1HEAP_UNPACKNAME))
	$(call DELFILE, $(O1HEAP_PACKNAME))
	$(call DELFILE, $(APPS_INCDIR)$(DELIM)o1heap.h)
	$(call DELDIR, $(O1HEAP_UNPACKNAME))
	$(call DELFILE, $(O1HEAP_PACKNAME))

include $(APPDIR)/Application.mk
