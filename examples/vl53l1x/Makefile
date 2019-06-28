############################################################################
# apps/examples/vl53l1x/Makefile
#
#   Copyright (C) 2019 Acutronics Robotics. All rights reserved.
#   Author: Acutronics Robotics (Juan Flores Muñoz) <juan@erlerobotics.com>
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

CONFIG_EXAMPLES_VL53L1X_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_EXAMPLES_VL53L1X_STACKSIZE ?= 2048

APPNAME = vl53l1x

PRIORITY  = $(CONFIG_EXAMPLES_VL53L1X_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_VL53L1X_STACKSIZE)

ASRCS =
CSRCS =
MAINSRC = vl53l1x_main.c

CONFIG_EXAMPLES_VL53L1X_PROGNAME ?= vl53l1x$(EXEEXT)
PROGNAME = $(CONFIG_EXAMPLES_VL53L1X_PROGNAME)

MODULE = CONFIG_EXAMPLES_VL53L1X

include $(APPDIR)/Application.mk
