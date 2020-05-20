############################################################################
# examples/udpblaster/Makefile
#
#   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

# Basic TCP networking test

CSRCS = udpblaster_text.c
MAINSRC = udpblaster_target.c

HOSTCFLAGS += -DUDPBLASTER_HOST=1

HOST_SRCS = udpblaster_host.c udpblaster_text.c

HOSTOBJEXT ?= .hobj
HOST_OBJS = $(HOST_SRCS:.c=$(HOSTOBJEXT))
HOST_BIN = host

# NET test built-in application info

PROGNAME = $(CONFIG_EXAMPLES_UDPBLASTER_PROGNAME)
PRIORITY = $(CONFIG_EXAMPLES_UDPBLASTER_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_UDPBLASTER_STACKSIZE)

# Common build

ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
$(HOST_OBJS): %$(HOSTOBJEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@
endif

config.h: $(TOPDIR)/include/nuttx/config.h
	@echo "CP:  $<"
	$(Q) cp $< $@

ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
$(HOST_OBJS): config.h

$(HOST_BIN): $(HOST_OBJS)
	@echo "LD:  $@"
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(HOST_OBJS) -o $@
endif

context:: config.h $(HOST_BIN)

clean::
ifneq ($(CONFIG_EXAMPLES_UDPBLASTER_LOOPBACK),y)
	$(call DELFILE, *$(HOSTOBJEXT))
	$(call DELFILE, $(HOST_BIN))
	$(call DELFILE, *.dSYM)
endif
	$(call DELFILE, config.h)

MODULE = $(CONFIG_EXAMPLES_UDPBLASTER)

include $(APPDIR)/Application.mk
