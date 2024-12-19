############################################################################
# apps/industry/scpi/Makefile
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

SCPI_VERSION  = 2.2
SCPI_UNPACK   = scpi-parser
SCPI_TARBALL  = v$(SCPI_VERSION).tar.gz
SCPI_URL_BASE = https://github.com/j123b567/scpi-parser/archive/refs/tags
SCPI_URL      = $(SCPI_URL_BASE)/$(SCPI_TARBALL)

CSRCS += error.c fifo.c ieee488.c
CSRCS += minimal.c parser.c units.c utils.c
CSRCS += lexer.c expression.c

VPATH += $(SCPI_UNPACK)/libscpi/src

CFLAGS += -DHAVE_STRNLEN
CFLAGS += -DHAVE_SNPRINTF
CFLAGS += -DHAVE_STRNDUP
CFLAGS += -DHAVE_STRNCASECMP

ifneq ($(CONFIG_SCPI_PARSER_DEMO),)
MAINSRC = $(SCPI_UNPACK)/examples/test-interactive/main.c
CSRCS += scpi-def.c

PROGNAME  = scpidemo
PRIORITY  = $(CONFIG_SCPI_PARSER_DEMO_PRIORITY)
STACKSIZE = $(CONFIG_SCPI_PARSER_DEMO_STACKSIZE)
MODULE    = $(CONFIG_SCPI_PARSER)

VPATH += $(SCPI_UNPACK)/examples
VPATH += $(SCPI_UNPACK)/examples/common
endif # CONFIG_SCPI_PARSER_DEMO

$(SCPI_TARBALL):
	$(Q) echo "Downloading $(SCPI_TARBALL)"
	$(Q) curl -O -L $(SCPI_URL)
	$(Q) echo "Unpacking $(SCPI_TARBALL) to $(SCPI_UNPACK)"
	$(Q) tar xzvf $(SCPI_TARBALL)
	$(Q) mv scpi-parser-$(SCPI_VERSION) $(SCPI_UNPACK)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(SCPI_UNPACK)/.git),)
context:: $(SCPI_TARBALL)

distclean::
	$(call DELDIR, $(SCPI_UNPACK))
	$(call DELFILE, $(SCPI_TARBALL))
endif

include $(APPDIR)/Application.mk
