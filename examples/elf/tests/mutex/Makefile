############################################################################
# examples/elf/tests/mutex/Makefile
#
#   Copyright (C) 2012, 2014 Gregory Nutt. All rights reserved.
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

-include $(TOPDIR)/Make.defs

ifeq ($(WINTOOL),y)
NUTTXLIB = "${shell cygpath -w $(TOPDIR)$(DELIM)lib}"
else
NUTTXLIB = "$(TOPDIR)$(DELIM)lib"
endif

LIBPATH =
ifeq ($(CONFIG_EXAMPLES_ELF_SYSCALL),y)
LDELFFLAGS += -Bstatic
LIBPATH += -L $(NUTTXLIB)
else
ifeq ($(CONFIG_EXAMPLES_ELF_LIBC),y)
LDELFFLAGS += -Bstatic
LIBPATH +=  -L $(NUTTXLIB)
endif
endif

LIBS =
ifeq ($(CONFIG_EXAMPLES_ELF_LIBC),y)
ifeq ($(CONFIG_NUTTX_KERNEL),y)
LIBS += -luc
else
LIBS += -lc
endif
endif

ifeq ($(CONFIG_EXAMPLES_ELF_SYSCALL),y)
LIBS += -lproxies
endif

BIN = mutex

SRCS = $(BIN).c
OBJS = $(SRCS:.c=.o)

all: $(BIN)

$(OBJS): %.o: %.c
	@echo "CC: $<"
	$(Q) $(CC) -c $(CELFFLAGS) $< -o $@

$(BIN): $(OBJS)
	@echo "LD: $<"
	$(Q) $(LD) $(LDELFFLAGS) $(LIBPATH) -o $@ $^ $(LIBS)

clean:
	$(call DELFILE, $(BIN))
	$(call CLEAN)

install:
	$(Q) mkdir -p $(ROMFS_DIR)
	$(Q) install $(BIN) $(ROMFS_DIR)/$(BIN)
