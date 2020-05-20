############################################################################
# apps/graphics/tiff/Makefile
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

include $(APPDIR)/Make.defs

ifeq ($(CONFIG_NXWIDGETS),y)
# Glyphs used by NxWidgets

CXXSRCS += glyph_nxlogo160x160.cxx glyph_nxlogo320x320.cxx
CXXSRCS += glyph_arrowdown.cxx glyph_checkboxon.cxx glyph_screendepthup.cxx
CXXSRCS += glyph_arrowleft.cxx glyph_control.cxx glyph_screenflipdown.cxx
CXXSRCS += glyph_arrowright.cxx glyph_cycle.cxx glyph_screenflipup.cxx
CXXSRCS += glyph_arrowup.cxx glyph_radiobuttonoff.cxx glyph_shift.cxx
CXXSRCS += glyph_backspace.cxx glyph_radiobuttonmu.cxx glyph_windowclose.cxx
CXXSRCS += glyph_capslock.cxx glyph_radiobuttonon.cxx glyph_windowdepthdown.cxx
CXXSRCS += glyph_checkboxmu.cxx glyph_return.cxx glyph_windowdepthup.cxx
CXXSRCS += glyph_checkboxoff.cxx glyph_screendepthdown.cxx

# Glyphs used by NxWM, Twm4Nx, and SLcd

ifeq ($(CONFIG_NXGLYPHS_LARGE_ICONS),y)
# Large icons

CXXSRCS += glyph_calculator47x49.cxx glyph_calibration48x42.cxx glyph_cmd49x43.cxx
CXXSRCS += glyph_minimize42x42.cxx glyph_play48x48.cxx glyph_stop42x42.cxx
CXXSRCS += glyph_menu42x42.cxx glyph_resize42x42.cxx glyph_nxicon42x42.cxx
CXXSRCS += glpyh_menu2-42x42.cxx glpyh_minimize2-42x42.cxx glpyh_resize2-42x42.cxx
CXXSRCS += glyph_stop2-42x42.cxx glyph_lcdclock102x48.cxx

else
# Small icons

CXXSRCS += glyph_calculator24x25.cxx glyph_calibration24x21.cxx glyph_cmd25x22.cxx
CXXSRCS += glyph_minimize21x21.cxx glyph_play24x24.cxx glyph_stop21x21.cxx
CXXSRCS += glyph_menu21x21.cxx glyph_resize21x21.cxx glyph_nxicon21x21.cxx
CXXSRCS += glpyh_menu2-21x21.cxx glpyh_minimize2-21x21.cxx glpyh_resize2-21x21.cxx
CXXSRCS += glpyh_stop2-21x21.cxx glyph_lcdclock51x24.cxx
endif

ifeq ($(CONFIG_NXGLYPHS_LARGE_ICONS),y)
# Large icons

CXXSRCS += glyph_mediaplayer44x50.cxx glyph_mplayer_controls43x41.cxx
CXXSRCS += glyph_mediagrip60x30.cxx

else
# Small icons

CXXSRCS += glyph_mediaplayer24x24.cxx glyph_mplayer_controls32x32.cxx
endif

endif # CONFIG_NXWIDGETS

VPATH = src

include $(APPDIR)/Application.mk
