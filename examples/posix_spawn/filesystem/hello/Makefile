############################################################################
# apps/examples/elf/tests/hello/Makefile
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

BIN = hello

SRCS = $(BIN).c
OBJS = $(SRCS:.c=.o)

all: $(BIN)
.PHONY: all clean install

$(OBJS): %.o: %.c
	@echo "CC: $<"
	$(Q) $(CC) -c $(CELFFLAGS) $< -o $@

$(BIN): $(OBJS)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) -o $@ $^

$(ROMFS_DIR)/$(BIN): $(BIN)
	$(Q) mkdir -p $(ROMFS_DIR)
	$(Q) install $(BIN) $(ROMFS_DIR)/$(BIN)

install: $(ROMFS_DIR)/$(BIN)

clean:
	$(call DELFILE, $(BIN))
	$(call DELFILE, *.dSYM)
	$(call CLEAN)
