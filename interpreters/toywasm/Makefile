############################################################################
# apps/interpreters/toywasm/Makefile
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

PROGNAME  = toywasm
PRIORITY  = $(CONFIG_INTERPRETERS_TOYWASM_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_TOYWASM_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_TOYWASM)

MAINSRC = main.c

CSRCS += bitmap.c
CSRCS += cell.c
CSRCS += context.c
CSRCS += decode.c
CSRCS += endian.c
CSRCS += exec.c
CSRCS += expr.c
CSRCS += fileio.c
CSRCS += host_instance.c
CSRCS += import_object.c
CSRCS += insn.c
CSRCS += instance.c
CSRCS += leb128.c
CSRCS += module.c
CSRCS += module_writer.c
CSRCS += repl.c
CSRCS += report.c
CSRCS += type.c
CSRCS += util.c
CSRCS += validation.c
CSRCS += vec.c
CSRCS += wasi.c
CSRCS += xlog.c

CFLAGS += -DTOYWASM_USE_SEPARATE_EXECUTE
CFLAGS += -DTOYWASM_USE_TAILCALL
CFLAGS += -DTOYWASM_USE_JUMP_BINARY_SEARCH
CFLAGS += -DTOYWASM_JUMP_CACHE2_SIZE=4
CFLAGS += -DTOYWASM_USE_SEPARATE_LOCALS
CFLAGS += -DTOYWASM_USE_SMALL_CELLS
CFLAGS += -DTOYWASM_ENABLE_WRITER

TOYWASM_VERSION  = de70389cd98ad6e4ce9098197b86f249a56d7053
TOYWASM_UNPACK   = toywasm
TOYWASM_TARBALL  = $(TOYWASM_VERSION).zip
TOYWASM_URL_BASE = https://github.com/yamt/toywasm/archive/
TOYWASM_URL      = $(TOYWASM_URL_BASE)/$(TOYWASM_TARBALL)

VPATH += $(TOYWASM_UNPACK)

$(TOYWASM_TARBALL):
	$(Q) echo "Downloading $(TOYWASM_TARBALL)"
	$(Q) curl -O -L $(TOYWASM_URL)

$(TOYWASM_UNPACK): $(TOYWASM_TARBALL)
	$(Q) echo "Unpacking $(TOYWASM_TARBALL) to $(TOYWASM_UNPACK)"
	$(Q) unzip $(TOYWASM_TARBALL)
	$(Q) mv toywasm-$(TOYWASM_VERSION) $(TOYWASM_UNPACK)
	$(Q) touch $(TOYWASM_UNPACK)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(TOYWASM_UNPACK)/.git),)
context:: $(TOYWASM_UNPACK)

distclean::
	$(call DELDIR, $(TOYWASM_UNPACK))
	$(call DELFILE, $(TOYWASM_TARBALL))
endif

include $(APPDIR)/Application.mk
