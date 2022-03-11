############################################################################
# apps/examples/elf/tests/pthread/Makefile
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

ifeq ($(CONFIG_EXAMPLES_ELF_SYSCALL),y)
LDELFFLAGS += -Bstatic
LDLIBPATH += -L $(NUTTXLIB)
else
ifeq ($(CONFIG_EXAMPLES_ELF_LIBC),y)
LDELFFLAGS += -Bstatic
LDLIBPATH +=  -L $(NUTTXLIB)
endif
endif

ifeq ($(CONFIG_EXAMPLES_ELF_LIBC),y)
LDLIBS += -lc
endif

ifeq ($(CONFIG_EXAMPLES_ELF_SYSCALL),y)
LDLIBS += -lproxies
endif

BIN = pthread

SRCS = $(BIN).c
OBJS = $(SRCS:.c=$(OBJEXT))

all: $(BIN)
.PHONY: all clean install

$(OBJS): %$(OBJEXT): %.c
	@echo "CC: $<"
	$(Q) $(CC) -c $(CELFFLAGS) $< -o $@

$(BIN): $(OBJS)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)

$(FSIMG_DIR)/$(BIN): $(BIN)
	$(Q) mkdir -p $(FSIMG_DIR)
	$(Q) install $(BIN) $(FSIMG_DIR)/$(BIN)
ifneq ($(CONFIG_DEBUG_SYMBOLS),y)
	$(Q) $(STRIP) $(FSIMG_DIR)/$(BIN)
endif

install: $(FSIMG_DIR)/$(BIN)

clean:
	$(call DELFILE, $(BIN))
	$(call CLEAN)
