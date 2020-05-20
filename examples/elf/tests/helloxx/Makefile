############################################################################
# examples/elf/tests/helloxx/Makefile
#
#   Copyright (C) 2012, 2014-2016 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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

$(OBJS): %$(OBJEXT): %.cpp
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
#
# NOTE:  libstdc++ is not available for NuttX as of this writing
#
ifeq ($(CONFIG_EXAMPLES_ELF_CXX),y)
$(BIN4): $(OBJS4)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)
$(BIN5): $(OBJS5)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $@ $(ARCHCRT0OBJ) $^ $(LDLIBS)
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
