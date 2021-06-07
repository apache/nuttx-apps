############################################################################
# apps/netutils/cgi-src/Makefile
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

CFLAGS		+= ${shell $(INCDIR) "$(CC)" "$(APPDIR)$(DELIM)netutils$(DELIM)thttpd"}
CGIBINDIR	= $(APPDIR)/netutils/thttpd/cgi-bin
CLEANFILES	= *.o redirect ssi phf

BIN1		= phf
BIN2		= redirect
BIN3		= ssi
BIN			= $(BIN1) $(BIN2) $(BIN3)

R1SRCS1		= $(BIN1).c
R1OBJS1		= $(R1SRCS1:.c=.o)
R2SRC1		= $(BIN1)-thunk.S
R2OBJ1		= $(R2SRC1:.S=.o)

R1SRCS2		= $(BIN2).c
R1OBJS2		= $(R1SRCS2:.c=.o)
R2SRC2		= $(BIN2)-thunk.S
R2OBJ2		= $(R2SRC2:.S=.o)

R1SRCS3		= $(BIN3).c
R1OBJS3		= $(R1SRCS3:.c=.o)
R2SRC3		= $(BIN3)-thunk.S
R2OBJ3		= $(R2SRC3:.S=.o)

DERIVED		= $(R2SRC1) $(R2SRC2) $(R2SRC3)

R1COBJS		= $(R1OBJS1) $(R1OBJS2) $(R1OBJS3)
R2AOBJS		= $(R2OBJ1) $(R2OBJ2) $(R2OBJ3)

all: $(BIN1) $(BIN2) $(BIN3)

$(R1COBJS): %.o: %.c
	@echo "CC: $<"
	$(CC) -c $(CPICFLAGS) $< -o $@

$(R2AOBJS): %.o: %.S
	@echo "AS: $<"
	@$(CC) -c $(CPICFLAGS) $< -o $@

# BIN1

$(BIN1).r1: $(R1OBJS1)
	@echo "LD: $<"
	@$(LD) $(NXFLATLDFLAGS1) -o $@ $^

$(R2SRC1): $(BIN1).r1
	@echo "MK: $<"
	@$(MKNXFLAT) -o $@ $^

$(BIN1).r2: $(R2OBJ1)
	@echo "LD: $<"
	@$(LD) $(NXFLATLDFLAGS2) -o $@ $(R1OBJS1) $(R2OBJ1)

$(BIN1): $(BIN1).r2
	@echo "LD: $<"
	@$(LDNXFLAT) $(LDNXFLATFLAGS) -o $@ $^

#BIN2

$(BIN2).r1: $(R1OBJS2)
	@echo "LD: $<"
	@$(LD) $(NXFLATLDFLAGS1) -o $@ $^

$(R2SRC2): $(BIN2).r1
	@echo "MK: $<"
	@$(MKNXFLAT) -o $@ $^

$(BIN2).r2: $(R2OBJ2)
	@echo "LD: $<"
	@$(LD) $(NXFLATLDFLAGS2) -o $@ $(R1OBJS2) $(R2OBJ2)

$(BIN2): $(BIN2).r2
	@echo "LD: $<"
	@$(LDNXFLAT) $(LDNXFLATFLAGS) -o $@ $^
# BIN3

$(BIN3).r1: $(R1OBJS3)
	@echo "LD: $<"
	@$(LD) $(NXFLATLDFLAGS1) -o $@ $^

$(R2SRC3): $(BIN3).r1
	@echo "MK: $<"
	@$(MKNXFLAT) -o $@ $^

$(BIN3).r2: $(R2OBJ3)
	@echo "LD: $<"
	@$(LD) $(NXFLATLDFLAGS2) -o $@ $(R1OBJS3) $(R2OBJ3)

$(BIN3): $(BIN3).r2
	@echo "LD: $<"
	@$(LDNXFLAT) $(LDNXFLATFLAGS) -o $@ $^

clean:
	@rm -f $(BIN1) $(BIN2) $(BIN3) *.r1 *.r2 *-thunk.S *~ .*.swp
	$(call CLEAN)

distclean: clean
