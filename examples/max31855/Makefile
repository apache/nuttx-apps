############################################################################
# apps/examples/max31855/Makefile
#
#   Copyright (C) 2018 Gregory Nutt. All rights reserved.
#   Author: Tiago Almeida <tiagojbalmeida@gmail.com>
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

# max31855 built-in application info

CONFIG_EXAMPLES_MAX31855_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_EXAMPLES_MAX31855_STACKSIZE ?= 2048

APPNAME = max31855
PRIORITY = $(CONFIG_EXAMPLES_MAX31855_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_MAX31855_STACKSIZE)

# max31855 Example

ASRCS =
CSRCS =
MAINSRC = max31855_main.c

CONFIG_EXAMPLES_MAX31855_PROGNAME ?= max31855$(EXEEXT)
PROGNAME = $(CONFIG_EXAMPLES_MAX31855_PROGNAME)

MODULE = CONFIG_EXAMPLES_MAX31855

include $(APPDIR)/Application.mk
