#################################################################################
# apps/graphics/NxWidgets/nxwm/Makefile
#
#   Copyright (C) 2012, 2014, 2016, 2018 Gregory Nutt. All rights reserved.
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
# 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
#    me be used to endorse or promote products derived from this software
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
#################################################################################

-include $(TOPDIR)/Make.defs

ASRCS =
CSRCS =

# Window Manager

CXXSRCS  = capplicationwindow.cxx cfullscreenwindow.cxx ctaskbar.cxx cwindowmessenger.cxx

# Device support

ifeq ($(CONFIG_NXWM_TOUCHSCREEN),y)
CXXSRCS += ctouchscreen.cxx
endif

ifeq ($(CONFIG_NXWM_KEYBOARD),y)
CXXSRCS += ckeyboard.cxx
endif

# Applications

CXXSRCS += cstartwindow.cxx chexcalculator.cxx

ifeq ($(CONFIG_NXWM_TOUCHSCREEN),y)
CXXSRCS += ccalibration.cxx
endif

ifeq ($(CONFIG_NXWM_NXTERM),y)
CXXSRCS +=  cnxterm.cxx
endif

# Images

ifeq ($(CONFIG_NXWM_LARGE_ICONS),y)
CXXSRCS += glyph_calculator47x49.cxx glyph_calibration48x42.cxx glyph_cmd49x43.cxx
CXXSRCS += glyph_minimize42x42.cxx glyph_play48x48.cxx glyph_stop42x42.cxx
else
CXXSRCS += glyph_calculator24x25.cxx glyph_calibration24x21.cxx glyph_cmd25x22.cxx
CXXSRCS += glyph_minimize21x21.cxx glyph_play24x24.cxx glyph_stop21x21.cxx
endif

ifeq ($(CONFIG_NXWM_MEDIAPLAYER),y)
CXXSRCS += cmediaplayer.cxx
ifeq ($(CONFIG_NXWM_LARGE_ICONS),y)
CXXSRCS += glyph_mediaplayer44x50.cxx glyph_mplayer_controls43x41.cxx
CXXSRCS += glyph_mediagrip60x30.cxx
else
CXXSRCS += glyph_mediaplayer24x24.cxx glyph_mplayer_controls32x32.cxx
endif
endif

VPATH = src

include $(APPDIR)/Application.mk
