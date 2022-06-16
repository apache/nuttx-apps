############################################################################
# apps/examples/elf/tests/helloxx/Makefile
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

BIN1 = hello++1
ALL_BIN = $(FSIMG_DIR)/$(BIN1)
BIN2 = hello++2
ALL_BIN += $(FSIMG_DIR)/$(BIN2)
ifeq ($(CONFIG_HAVE_CXXINITIALIZE),y)
BIN3 = hello++3
ALL_BIN += $(FSIMG_DIR)/$(BIN3)
ifeq ($(CONFIG_EXAMPLES_ELF_CXX),y)
BIN4 = hello++4
ALL_BIN += $(FSIMG_DIR)/$(BIN4)
BIN5 = hello++5
ALL_BIN += $(FSIMG_DIR)/$(BIN5)
endif
endif

SRCS1 = $(BIN1).c
OBJS1 = $(SRCS1:.c=$(OBJEXT))

SRCS2 = $(BIN2).c
OBJS2 = $(SRCS2:.c=$(OBJEXT))

ifeq ($(CONFIG_HAVE_CXXINITIALIZE),y)
SRCS3 = $(BIN3).c
OBJS3 = $(SRCS3:.c=$(OBJEXT))
ifeq ($(CONFIG_EXAMPLES_ELF_CXX),y)
SRCS4 = $(BIN4).c
OBJS4 = $(SRCS4:.c=$(OBJEXT))
SRCS5 = $(BIN5).c
OBJS5 = $(SRCS5:.c=$(OBJEXT))
endif
endif

SRCS = $(SRCS1) $(SRCS2) $(SRCS3) $(SRCS4) $(SRCS5)
OBJS = $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5)

LDLIBSTDC_STUBS_DIR	= $(TOPDIR)/libxx
LDLIBSTDC_STUBS_LIB	= $(LDLIBSTDC_STUBS_DIR)/liblibxx.a

all: $(BIN1) $(BIN2) $(BIN3) $(BIN4) $(BIN5)
.PHONY: all clean install

$(OBJS): %$(OBJEXT): %.cxx
	@echo "CC: $<"
	$(Q) $(CXX) -c $(CXXELFFLAGS) $< -o $@

# This contains libstdc++ stubs to that you can build C++ code
# without actually having libstdc++

$(LDLIBSTDC_STUBS_LIB):
	$(Q) $(MAKE) -C $(LDLIBSTDC_STUBS_DIR) TOPDIR=$(TOPDIR)

# BIN1 and BIN2 link just like C code because they contain no
# static constructors.  BIN1 is equivalent to a C hello world;
# BIN2 contains a class that implements hello world, but it is
# not statically initialized.

$(BIN1): $(OBJS1)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)

$(BIN2): $(OBJS2)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)

# BIN3 is equivalent to BIN2 except that is uses static initializers

ifeq ($(CONFIG_HAVE_CXXINITIALIZE),y)
$(BIN3): $(OBJS3)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)

# BIN4 is similar to BIN3 except that it uses the streams code from libstdc++
# Both BIN4 and BIN5 use exceptions
# NOTE:  libstdc++ is not available for NuttX as of this writing
#
ifeq ($(CONFIG_EXAMPLES_ELF_CXX),y)
ifeq ($(CONFIG_CXX_EXCEPTION),y)
$(BIN4): $(OBJS4)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)
$(BIN5): $(OBJS5)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)
endif
endif
endif

$(FSIMG_DIR)/$(BIN1): $(BIN1)
	$(Q) mkdir -p $(FSIMG_DIR)
	$(Q) install $(BIN1) $(FSIMG_DIR)/$(BIN1)
ifneq ($(CONFIG_DEBUG_SYMBOLS),y)
	$(Q) $(STRIP) $(FSIMG_DIR)/$(BIN1)
endif

$(FSIMG_DIR)/$(BIN2): $(BIN2)
	$(Q) mkdir -p $(FSIMG_DIR)
	$(Q) install $(BIN2) $(FSIMG_DIR)/$(BIN2)
ifneq ($(CONFIG_DEBUG_SYMBOLS),y)
	$(Q) $(STRIP) $(FSIMG_DIR)/$(BIN2)
endif

ifeq ($(CONFIG_HAVE_CXXINITIALIZE),y)
$(FSIMG_DIR)/$(BIN3): $(BIN3)
	$(Q) mkdir -p $(FSIMG_DIR)
	$(Q) install $(BIN3) $(FSIMG_DIR)/$(BIN3)
ifneq ($(CONFIG_DEBUG_SYMBOLS),y)
	$(Q) $(STRIP) $(FSIMG_DIR)/$(BIN3)
endif

ifeq ($(CONFIG_EXAMPLES_ELF_CXX),y)
$(FSIMG_DIR)/$(BIN4): $(BIN4)
	$(Q) mkdir -p $(FSIMG_DIR)
	$(Q) install $(BIN4) $(FSIMG_DIR)/$(BIN4)
ifneq ($(CONFIG_DEBUG_SYMBOLS),y)
	$(Q) $(STRIP) $(FSIMG_DIR)/$(BIN4)
endif

$(FSIMG_DIR)/$(BIN5): $(BIN5)
	$(Q) mkdir -p $(FSIMG_DIR)
	$(Q) install $(BIN5) $(FSIMG_DIR)/$(BIN5)
ifneq ($(CONFIG_DEBUG_SYMBOLS),y)
	$(Q) $(STRIP) $(FSIMG_DIR)/$(BIN5)
endif
endif
endif

install: $(ALL_BIN)

clean:
	$(call DELFILE, $(BIN1))
	$(call DELFILE, $(BIN2))
	$(call DELFILE, $(BIN3))
	$(call DELFILE, $(BIN4))
	$(call DELFILE, $(BIN5))
	$(call CLEAN)
