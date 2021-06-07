############################################################################
# apps/graphics/twm4nx/Makefile
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

# Add path to cursor images to CXXFLAGS

CXXFLAGS += ${shell $(INCDIR) "$(CC)" "$(APPDIR)/graphics/nxglyphs/include"}

# Twm4Nx built-in application info

PROGNAME  = $(CONFIG_TWM4NX_PROGNAME)
PRIORITY  = $(CONFIG_TWM4NX_PRIORITY)
STACKSIZE = $(CONFIG_TWM4NX_STACKSIZE)
MODULE    = $(CONFIG_GRAPHICS_TWM4NX)


# Twm4Nx

MAINSRC   = twm4nx_main.cxx
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

CXXSRCS  += ctwm4nx.cxx

VPATH     = src:apps

include $(APPDIR)/Application.mk
