############################################################################
# apps/system/argtable3/Makefile
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

ARGTABLE3_VERSION := $(patsubst "%",%,$(CONFIG_SYSTEM_ARGTABLE3_VERSION))
ARGTABLE3_TARBALL  = v$(ARGTABLE3_VERSION).tar.gz
ARGTABLE3_UNPACK   = argtable3
ARGTABLE3_SRCDIR   = $(ARGTABLE3_UNPACK)$(DELIM)src

DEPPATH += --dep-path $(ARGTABLE3_SRCDIR)
VPATH   += :$(ARGTABLE3_SRCDIR)

CFLAGS += ${shell $(DEFINE) "$(CC)" ARG_REPLACE_GETOPT=0}
CSRCS := $(notdir $(wildcard $(ARGTABLE3_SRCDIR)$(DELIM)*.c))

$(ARGTABLE3_TARBALL):
	$(Q) echo "Downloading argtable3-$(ARGTABLE3_VERSION)"
	$(Q) echo "$(ARGTABLE3_SRCDIR)"
	$(Q) echo "$(ARGTABLE3_TARBALL)--$(ARGTABLE3_UNPACK)"
	$(Q) curl -O -L https://github.com/argtable/argtable3/archive/$(ARGTABLE3_TARBALL)

$(ARGTABLE3_UNPACK): $(ARGTABLE3_TARBALL)
	$(Q) tar zxf $(ARGTABLE3_TARBALL)
	$(Q) mv argtable3-$(ARGTABLE3_VERSION) $(ARGTABLE3_UNPACK)

context:: $(ARGTABLE3_UNPACK)

distclean::
	$(call DELFILE, $(ARGTABLE3_TARBALL))
	$(call DELDIR, $(ARGTABLE3_UNPACK))

include $(APPDIR)/Application.mk
