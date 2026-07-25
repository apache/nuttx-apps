############################################################################
# apps/examples/nxflatxip/module/Makefile
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

# The NXFLAT module the demo downloads into xipfs.  Built exactly the way
# apps/examples/nxflat builds its test programs, so it needs the same two
# host tools from the NuttX toolchain: mknxflat and ldnxflat.

BIN       = xipmod

R1SRCS    = $(BIN).c
R1OBJS    = $(R1SRCS:.c=.o)

R2SRC     = $(BIN)-thunk.S
R2OBJ     = $(R2SRC:.S=.o)

EXAMPLDIR = $(APPDIR)$(DELIM)examples$(DELIM)nxflatxip
BINHDR    = $(EXAMPLDIR)$(DELIM)$(BIN)_bin.h
SYMTAB    = $(EXAMPLDIR)$(DELIM)symtab.c

all: $(BINHDR) $(SYMTAB)
.PHONY: all clean

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

# Embed the module in the demo.  It is not the demo's own text, it is a
# payload the demo writes into the filesystem, so a plain byte array is what
# is wanted.

$(BINHDR): $(BIN)
	@echo "GEN: $@"
	$(Q) xxd -i -n g_xipmod $(BIN) | \
		sed -e "s/^unsigned char/static const unsigned char/" \
		    -e "s/^unsigned int/static const unsigned int/" >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

# The symbols the module imports from the firmware.  mknxflat left the list
# in the thunk file it generated, so that is what is scanned.

$(SYMTAB): $(R2SRC)
	@echo "GEN: $@"
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh . g_nxflat >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

clean:
	$(call DELFILE, $(BIN))
	$(call DELFILE, $(R2SRC))
	$(call DELFILE, *.r1)
	$(call DELFILE, *.r2)
	$(call DELFILE, $(BINHDR))
	$(call DELFILE, $(SYMTAB))
	$(call CLEAN)
