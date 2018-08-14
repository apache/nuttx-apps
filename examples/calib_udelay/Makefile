############################################################################
# apps/examples/calib_udelay/Makefile
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

# built-in application info

CONFIG_EXAMPLES_CALIB_UDELAY_APPNAME ?= "calib_udelay"
CONFIG_EXAMPLES_CALIB_UDELAY_STACKSIZE ?= 2048
CONFIG_EXAMPLES_CALIB_UDELAY_PRIORITY ?= 100
CONFIG_EXAMPLES_CALIB_UDELAY_PROGNAME ?= "calib_udelay"

APPNAME   = $(CONFIG_EXAMPLES_CALIB_UDELAY_APPNAME)
PRIORITY  = $(CONFIG_EXAMPLES_CALIB_UDELAY_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_CALIB_UDELAY_STACKSIZE)
PROGNAME  = $(CONFIG_EXAMPLES_CALIB_UDELAY_PROGNAME)

# Udelay calibration tool

ASRCS =
CSRCS =
MAINSRC = calib_udelay_main.c

include $(APPDIR)/Application.mk
