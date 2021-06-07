#################################################################################
# apps/graphics/nxwm/Makefile
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
#################################################################################

include $(APPDIR)/Make.defs

PROGNAME = nxwm
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_NXWM)

# Window Manager

MAINSRC  = nxwm_main.cxx
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

ifeq ($(CONFIG_NXWM_MEDIAPLAYER),y)
CXXSRCS += cmediaplayer.cxx
endif

VPATH = src

include $(APPDIR)/Application.mk
