############################################################################
# apps/graphics/tiff/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
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
