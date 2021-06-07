############################################################################
# examples/nxflat/tests/mutex/Makefile
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

BIN			= mutex

R1SRCS			= $(BIN).c
R1OBJS			= $(R1SRCS:.c=.o)

R2SRC			= $(BIN)-thunk.S
R2OBJ			= $(R2SRC:.S=.o)

all: $(BIN)

$(R1OBJS): %.o: %.c
	@echo "CC: $<"
	$(Q) $(CC) -c $(CPICFLAGS) $< -o $@

$(R2OBJ): %.o: %.S
	@echo "AS: $<"
	$(Q) $(CC) -c $(CPICFLAGS) $< -o $@

$(BIN).r1: $(R1OBJS)
	@echo "LD: $<"
	$(Q) $(LD) $(NXFLATLDFLAGS1) -o $@ $^

$(R2SRC): $(BIN).r1
	@echo "MK: $<"
	$(Q) $(MKNXFLAT) -o $@ $^

$(BIN).r2: $(R2OBJ)
	@echo "LD: $<"
	$(Q) $(LD) $(NXFLATLDFLAGS2) -o $@ $(R1OBJS) $(R2OBJ)

$(BIN): $(BIN).r2
	@echo "LD: $<"
	$(Q) $(LDNXFLAT) $(LDNXFLATFLAGS) -o $@ $^

$(ROMFS_DIR)/$(BIN): $(BIN)
	$(Q) install -D $(BIN) $(ROMFS_DIR)/$(BIN)

install: $(ROMFS_DIR)/$(BIN)

clean:
	$(call DELFILE, $(BIN))
	$(call DELFILE, $(R2SRC))
	$(call DELFILE, *.r1)
	$(call DELFILE, *.r2)
	$(call CLEAN)
