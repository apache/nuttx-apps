############################################################################
# apps/wireless/wapi/Makefile
#
#   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

# TODO, this makefile should run make under the app dirs, instead of
# sourcing the Make.defs!

-include $(TOPDIR)/Make.defs

# IEEE802.11 Wapi Application

CONFIG_WIRELESS_WAPI_PRIORITY ?= 100
CONFIG_WIRELESS_WAPI_STACKSIZE ?= 2048

include $(APPDIR)/wireless/wapi/src/Make.defs

ifneq ($(MAINSRC),)
APPNAME = wapi
PRIORITY = $(CONFIG_WIRELESS_WAPI_PRIORITY)
STACKSIZE = $(CONFIG_WIRELESS_WAPI_STACKSIZE)

CONFIG_WAPI_PROGNAME ?= wapi$(EXEEXT)
PROGNAME = $(CONFIG_WAPI_PROGNAME)
endif

MODULE = CONFIG_WIRELESS_WAPI

include $(APPDIR)/Application.mk
