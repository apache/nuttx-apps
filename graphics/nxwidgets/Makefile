#################################################################################
# appx/graphics/nxwidgets/Makefile
#
#   Copyright (C) 2012-2014, 2018 Gregory Nutt. All rights reserved.
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

include $(APPDIR)/Make.defs

# Infrastructure

CXXSRCS  = cbitmap.cxx cbgwindow.cxx ccallback.cxx cgraphicsport.cxx
CXXSRCS += clistdata.cxx clistdataitem.cxx cnxfont.cxx
CXXSRCS += cnxserver.cxx cnxstring.cxx cnxtimer.cxx cnxwidget.cxx cnxwindow.cxx
CXXSRCS += cnxtkwindow.cxx cnxtoolbar.cxx crect.cxx crlepalettebitmap.cxx
CXXSRCS += cscaledbitmap.cxx cstringiterator.cxx ctext.cxx cwidgetcontrol.cxx
CXXSRCS += cwidgeteventhandlerlist.cxx cwindoweventhandlerlist.cxx singletons.cxx

# Widget APIs

CXXSRCS += cbutton.cxx cbuttonarray.cxx ccheckbox.cxx ccyclebutton.cxx
CXXSRCS += cglyphbutton.cxx cglyphsliderhorizontal.cxx cglyphsliderhorizontalgrip.cxx
CXXSRCS += cimage.cxx ckeypad.cxx clabel.cxx clabelgrid.cxx clatchbutton.cxx
CXXSRCS += clatchbuttonarray.cxx clistbox.cxx clistboxdataitem.cxx cmultilinetextbox.cxx
CXXSRCS += cnumericedit.cxx
CXXSRCS += cprogressbar.cxx cradiobutton.cxx cradiobuttongroup.cxx cscrollbarhorizontal.cxx
CXXSRCS += cscrollbarpanel.cxx cscrollbarvertical.cxx cscrollinglistbox.cxx
CXXSRCS += cscrollingpanel.cxx cscrollingtextbox.cxx csliderhorizontal.cxx
CXXSRCS += csliderhorizontalgrip.cxx cslidervertical.cxx csliderverticalgrip.cxx
CXXSRCS += cstickybutton.cxx cstickybuttonarray.cxx cstickyimage.cxx ctabpanel.cxx
CXXSRCS += ctextbox.cxx

VPATH = src

include $(APPDIR)/Application.mk
