############################################################################
# examples/sotest/lib/sotest/Makefile
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

ifeq ($(CONFIG_EXAMPLES_SOTEST_LIBGCC),y)
LIBGCC = "${shell $(CC) $(ARCHCPUFLAGS) -print-libgcc-file-name 2>/dev/null}"
ifneq ($(LIBGCC),)
  LDLIBPATH += -L "${shell dirname $(LIBGCC)}"
  LDLIBS += -lgcc
endif
endif

ifeq ($(CONFIG_EXAMPLES_SOTEST_LIBC),y)
LDMODULEFLAGS += -Bstatic
LDLIBPATH +=  -L $(NUTTXLIB)
endif

ifeq ($(CONFIG_EXAMPLES_SOTEST_LIBC),y)
LDLIBS += -lc
endif

BIN = sotest

SRCS = $(BIN).c
OBJS = $(SRCS:.c=$(OBJEXT))

all: $(BIN)
.PHONY: all clean install

$(OBJS): %$(OBJEXT): %.c
	@echo "MODULECC: $<"
	$(Q) $(MODULECC) -c $(CMODULEFLAGS) $< -o $@

$(BIN): $(OBJS)
	@echo "MODULELD: $<"
	$(Q) $(MODULELD) $(LDMODULEFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)

$(FSROOT_DIR)/$(BIN): $(BIN)
	$(Q) mkdir -p $(FSROOT_DIR)
	$(Q) install $(BIN) $(FSROOT_DIR)/$(BIN)

install: $(FSROOT_DIR)/$(BIN)

clean:
	$(call DELFILE, $(BIN))
	$(call CLEAN)
