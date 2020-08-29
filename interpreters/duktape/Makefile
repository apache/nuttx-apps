############################################################################
# interpreters/duktape/Makefile
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

DUKTAPE_VERSION  = 2.5.0
DUKTAPE_UNPACK   = duktape
DUKTAPE_TARBALL  = duktape-$(DUKTAPE_VERSION).tar.xz
DUKTAPE_URL_BASE = https://github.com/svaarala/duktape/releases/download/
DUKTAPE_URL      = $(DUKTAPE_URL_BASE)v$(DUKTAPE_VERSION)/$(DUKTAPE_TARBALL)

CSRCS = duktape.c
CSRCS += duk_console.c
CSRCS += duk_print_alert.c

MAINSRC = duk_cmdline.c

VPATH += $(DUKTAPE_UNPACK)/src-noline
VPATH += $(DUKTAPE_UNPACK)/extras/console
VPATH += $(DUKTAPE_UNPACK)/extras/print-alert
VPATH += $(DUKTAPE_UNPACK)/examples/cmdline

PROGNAME  = duk
PRIORITY  = $(CONFIG_INTERPRETERS_DUKTAPE_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_DUKTAPE_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_DUKTAPE)

CFLAGS += -I$(DUKTAPE_UNPACK)/src-noline
CFLAGS += -I$(DUKTAPE_UNPACK)/extras/console
CFLAGS += -I$(DUKTAPE_UNPACK)/extras/print-alert
CFLAGS += -DDUK_CMDLINE_CONSOLE_SUPPORT
CFLAGS += -DDUK_CMDLINE_PRINTALERT_SUPPORT

$(DUKTAPE_TARBALL):
	$(Q) echo "Downloading $(DUKTAPE_TARBALL)"
	$(Q) wget $(DUKTAPE_URL)

$(DUKTAPE_UNPACK): $(DUKTAPE_TARBALL)
	$(Q) echo "Unpacking $(DUKTAPE_TARBALL) to $(DUKTAPE_UNPACK)"
	$(Q) tar xvfJ $(DUKTAPE_TARBALL)
	$(Q) mv duktape-$(DUKTAPE_VERSION) $(DUKTAPE_UNPACK)
	$(Q) echo "Patching $(DUKTAPE_UNPACK)"
	$(Q) patch -p0 < duk_cmdline.patch

$(DUKTAPE_UNPACK)/.patch: $(DUKTAPE_UNPACK)
	$(Q) touch $(DUKTAPE_UNPACK)/.patch

context:: $(DUKTAPE_UNPACK)/.patch

distclean::
	$(call DELDIR, $(DUKTAPE_UNPACK))
	$(call DELFILE, $(DUKTAPE_TARBALL))

include $(APPDIR)/Application.mk
