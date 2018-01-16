############################################################################
# apps/examples/lgvldemo/Makefile
#
#   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

# LittleVGL demo built-in application info

CONFIG_EXAMPLES_LVGLDEMO_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_EXAMPLES_LVGLDEMO_STACKSIZE ?= 32768

APPNAME = lvgldemo
PRIORITY = $(CONFIG_EXAMPLES_LVGLDEMO_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_LVGLDEMO_STACKSIZE)

# LittleVGL demo Example

ASRCS =
CSRCS += fbdev.c demo.c img_bubble_pattern.c tp.c tp_cal.c
MAINSRC = lvgldemo.c

CONFIG_EXAMPLES_LVGLDEMO_PROGNAME ?= lvgldemo$(EXEEXT)
PROGNAME = $(CONFIG_EXAMPLES_LVGLDEMO_PROGNAME)

LVGLDIR=$(APPDIR)/graphics/littlevgl/lvgl
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(LVGLDIR)"}
CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(LVGLDIR)"}

include $(APPDIR)/Application.mk
