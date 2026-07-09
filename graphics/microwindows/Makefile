############################################################################
# apps/graphics/microwindows/Makefile
#
# SPDX-License-Identifier: Apache-2.0
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

# LVGL graphic library

MICROWINDOWS_DIR = .
MICROWINDOWS_DIR_NAME = microwindows

#-include ./lvgl/lvgl.mk

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

MICROWINDOWS_COMMIT_HASH := 7a2ff57052d8663cbca4863709a8ee75bf9fa0c2

#CONFIG_GRAPH_MICROWINDOWS_URL ?= "https://codeload.github.com/ghaerr/microwindows/zip/refs/heads/master"
CONFIG_GRAPH_MICROWINDOWS_URL ?= https://codeload.github.com/ghaerr/microwindows/zip/$(MICROWINDOWS_COMMIT_HASH)

MICROWINDOWS_TARBALL := microwindows-$(MICROWINDOWS_COMMIT_HASH).zip

MICROWINDOWS_UNPACKNAME = microwindows
UNPACK ?= unzip -o $(if $(V),,-q)
CURL ?= curl -L $(if $(V),,-Ss)

MICROWINDOWS_UNPACKDIR =  $(WD)/$(MICROWINDOWS_UNPACKNAME)

$(MICROWINDOWS_TARBALL):
	$(ECHO_BEGIN)"Downloading: $(MICROWINDOWS_TARBALL)"
	$(Q) $(CURL) -o $(MICROWINDOWS_TARBALL) $(CONFIG_GRAPH_MICROWINDOWS_URL)
	$(ECHO_END)

$(MICROWINDOWS_UNPACKNAME): $(MICROWINDOWS_TARBALL)
	$(ECHO_BEGIN)"Unpacking: $(MICROWINDOWS_TARBALL) -> $(MICROWINDOWS_UNPACKNAME)"
	$(Q) $(UNPACK) $(MICROWINDOWS_TARBALL)
	$(Q) mv	microwindows-$(MICROWINDOWS_COMMIT_HASH) $(MICROWINDOWS_UNPACKNAME)
	$(Q) touch $(MICROWINDOWS_UNPACKNAME)
	$(ECHO_END)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(MICROWINDOWS_UNPACKNAME)/.git),)
context:: $(MICROWINDOWS_UNPACKNAME)
endif

CFLAGS += -DRTEMS=0 -DPSP=0 -D__ECOS=0 -D__MINGW32__=0
CFLAGS += -DELKS=0 -D_MINIX=0 -DMSDOS=0 -DLINUX=0
CFLAGS += -DMACOSX=0 -DTRIMEDIA=0

CFLAGS += -Wno-undef
CFLAGS += -Wno-shadow

MW_DIR_OBJ = $(WD)/$(MICROWINDOWS_DIR_NAME)/src
MW_LOCAL_DIR := mwin-local

-include microwindows/src/engine/Objects.rules
-include microwindows/src/drivers/Objects.rules
-include microwindows/src/fonts/Objects.rules

CSRCS += $(MW_CORE_OBJS:%.o=%.c)
CSRCS += $(MW_LOCAL_DIR)/scr_nuttx.c
CSRCS += $(MW_LOCAL_DIR)/mou_nuttx.c
CSRCS += $(MW_LOCAL_DIR)/kbd_nuttx.c

ifneq ($(CONFIG_EXAMPLES_MICROWINDOWS),)

# Win32 API used by mwdemo.c
MWIN_CORE_SRCS  = microwindows/src/mwin/winmain.c
MWIN_CORE_SRCS += microwindows/src/mwin/winuser.c
MWIN_CORE_SRCS += microwindows/src/mwin/wingdi.c
MWIN_CORE_SRCS += microwindows/src/mwin/winexpos.c
MWIN_CORE_SRCS += microwindows/src/mwin/winclip.c
MWIN_CORE_SRCS += microwindows/src/mwin/winevent.c
MWIN_CORE_SRCS += microwindows/src/mwin/windefw.c
MWIN_CORE_SRCS += microwindows/src/mwin/winrgn.c
MWIN_CORE_SRCS += microwindows/src/mwin/winfont.c
MWIN_CORE_SRCS += microwindows/src/mwin/winsbar.c
MWIN_CORE_SRCS += microwindows/src/mwin/winres.c

# mwin winlib
WINLIB_SRCS  = microwindows/src/mwin/winlib/draw3d.c
WINLIB_SRCS += microwindows/src/mwin/winlib/fastfill.c
WINLIB_SRCS += microwindows/src/mwin/winlib/windlg.c
WINLIB_SRCS += microwindows/src/mwin/winlib/graph3d.c
WINLIB_SRCS += microwindows/src/mwin/winlib/caret.c
WINLIB_SRCS += microwindows/src/mwin/winlib/static.c
WINLIB_SRCS += microwindows/src/mwin/winlib/button.c
WINLIB_SRCS += microwindows/src/mwin/winlib/newedit.c
WINLIB_SRCS += microwindows/src/mwin/winlib/newlistbox.c
WINLIB_SRCS += microwindows/src/mwin/winlib/combobox.c
WINLIB_SRCS += microwindows/src/mwin/winlib/progbar.c
WINLIB_SRCS += microwindows/src/mwin/winlib/scrlbar.c
WINLIB_SRCS += microwindows/src/mwin/winlib/medit.c
WINLIB_SRCS += microwindows/src/mwin/winlib/ptinsid.c
WINLIB_SRCS += microwindows/src/mwin/winlib/insetr.c

CSRCS += $(MWIN_CORE_SRCS) $(WINLIB_SRCS)
CFLAGS += -DNOGDI

endif

#$(error EXTOBJS=$(EXTOBJS))

include $(APPDIR)/Application.mk

ifeq ($(wildcard $(MICROWINDOWS_UNPACKNAME)/.git),)
distclean::
	$(call DELDIR, $(MICROWINDOWS_UNPACKNAME))
	$(call DELFILE, $(MICROWINDOWS_TARBALL))
endif
