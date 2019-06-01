############################################################################
# apps/graphics/twm4nx/Makefile
#
#   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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
-include $(APPDIR)/Make.defs

# Add path to cursor images to CXXFLAGS

CXXFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(APPDIR)/graphics/nxglyphs/include"}

# Twm4Nx built-in application info

CONFIG_TWM4NX_PRIORITY ?= SCHED_PRIORITY_DEFAULT
CONFIG_TWM4NX_STACKSIZE ?= 2048

APPNAME   = twm4nx

PRIORITY  = $(CONFIG_TWM4NX_PRIORITY)
STACKSIZE = $(CONFIG_TWM4NX_STACKSIZE)

# Twm4Nx

ASRCS     =
CSRCS     =
CXXSRCS   = twm4nx_main.cxx
CXXSRCS  += cbackground.cxx cfonts.cxx ciconmgr.cxx ciconwidget.cxx
CXXSRCS  += cmenus.cxx cmainmenu.cxx
CXXSRCS  += cwindow.cxx cwindowevent.cxx cresize.cxx cwindowfactory.cxx
CXXSRCS  += cinput.cxx

ifeq ($(CONFIG_TWM4NX_MOUSE),y)
CXXSRCS  += twm4nx_cursor.cxx
endif

ifeq ($(CONFIG_TWM4NX_CALIBRATION),y)
CXXSRCS  += ccalibration.cxx
endif

ifeq ($(CONFIG_TWM4NX_CLOCK),y)
CXXSRCS  += cclock.cxx
endif

ifeq ($(CONFIG_TWM4NX_NXTERM),y)
CXXSRCS  += cnxterm.cxx
endif

MAINSRC   = ctwm4nx.cxx

VPATH     = src:apps

CONFIG_TWM4NX_PROGNAME ?= hello$(EXEEXT)
PROGNAME = $(CONFIG_TWM4NX_PROGNAME)

MODULE = CONFIG_GRAPHICS_TWM4NX

include $(APPDIR)/Application.mk
