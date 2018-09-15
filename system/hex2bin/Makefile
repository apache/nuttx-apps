############################################################################
# apps/system/hex2bin/Makefile
#
#   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

# Hex2bin utility

CONFIG_SYSTEM_HEX2BIN_PROGNAME ?= hex2bin$(EXEEXT)
CONFIG_SYSTEM_HEX2BIN_STACKSIZE ?= 1536
CONFIG_SYSTEM_HEX2BIN_PRIORITY ?= 100

CONFIG_SYSTEM_HEX2MEM_PROGNAME ?= hex2mem$(EXEEXT)
CONFIG_SYSTEM_HEX2MEM_STACKSIZE ?= 1536
CONFIG_SYSTEM_HEX2MEM_PRIORITY ?= 100

ifeq ($(CONFIG_SYSTEM_HEX2BIN),m)
  MAINSRC = hex2bin_main.c hex2mem_main.c
  PROGNAME = $(CONFIG_SYSTEM_HEX2BIN_PROGNAME) $(CONFIG_SYSTEM_HEX2MEM_PROGNAME)
  APPNAME = hex2bin hex2mem
  STACKSIZE = $(CONFIG_SYSTEM_HEX2BIN_STACKSIZE) $(CONFIG_SYSTEM_HEX2MEM_STACKSIZE)
  PRIORITY = $(CONFIG_SYSTEM_HEX2BIN_PRIORITY) $(CONFIG_SYSTEM_HEX2MEM_PRIORITY)
else
  ifeq ($(CONFIG_SYSTEM_HEX2BIN_BUILTIN),y)
    MAINSRC = hex2bin_main.c
    PROGNAME = $(CONFIG_SYSTEM_HEX2BIN_PROGNAME)
    APPNAME = hex2bin
    STACKSIZE = $(CONFIG_SYSTEM_HEX2BIN_STACKSIZE)
    PRIORITY = $(CONFIG_SYSTEM_HEX2BIN_PRIORITY)
  endif

  ifeq ($(CONFIG_SYSTEM_HEX2MEM_BUILTIN),y)
    MAINSRC += hex2mem_main.c
    PROGNAME += $(CONFIG_SYSTEM_HEX2MEM_PROGNAME)
    APPNAME += hex2mem
    STACKSIZE += $(CONFIG_SYSTEM_HEX2MEM_STACKSIZE)
    PRIORITY += $(CONFIG_SYSTEM_HEX2MEM_PRIORITY)
  endif
endif

MODULE = CONFIG_SYSTEM_HEX2BIN

include $(APPDIR)/Application.mk
