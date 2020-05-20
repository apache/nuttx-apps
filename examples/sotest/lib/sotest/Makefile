############################################################################
# examples/sotest/lib/sotest/Makefile
#
#   Copyright (C) 2015-2016 Gregory Nutt. All rights reserved.
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

LDLIBPATH =
LDLIBS =

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
