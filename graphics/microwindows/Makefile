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

# Microwindows graphic library

MICROWINDOWS_DIR = .
MICROWINDOWS_DIR_NAME = microwindows

# Set up build configuration and environment

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

MICROWINDOWS_COMMIT_HASH := f9c2980fe085de87acc085980ffd672bc2d20cff

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

distclean::
	$(call DELDIR, $(MICROWINDOWS_UNPACKNAME))
	$(call DELFILE, $(MICROWINDOWS_TARBALL))
endif

CFLAGS += -Wno-undef
CFLAGS += -Wno-shadow

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/graphics/microwindows/microwindows/src/drivers

CFLAGS += -DNUTTX_FB_PATH=\"$(CONFIG_MICROWINDOWS_FB_PATH)\"
ifdef CONFIG_MICROWINDOWS_KBD_EVENT_PATH
CFLAGS += -DNUTTX_KBD_EVENT_PATH=\"$(CONFIG_MICROWINDOWS_KBD_EVENT_PATH)\"
endif
ifdef CONFIG_MICROWINDOWS_KBD_RAW_PATH
CFLAGS += -DNUTTX_KBD_RAW_PATH=\"$(CONFIG_MICROWINDOWS_KBD_RAW_PATH)\"
endif
ifdef CONFIG_MICROWINDOWS_MOUSE_PATH
CFLAGS += -DNUTTX_MOUSE_PATH=\"$(CONFIG_MICROWINDOWS_MOUSE_PATH)\"
endif
ifdef CONFIG_MICROWINDOWS_TS_PATH
CFLAGS += -DNUTTX_TOUCHSCREEN_PATH=\"$(CONFIG_MICROWINDOWS_TS_PATH)\"
endif

MW_DIR_OBJ = $(WD)/$(MICROWINDOWS_DIR_NAME)/src

# Select NuttX drivers via Microwindows Objects.rules
ARCH = NUTTX

ifeq ($(CONFIG_MICROWINDOWS_KBD_EVENT),y)
KEYBOARD = NUTTXKBD
else ifeq ($(CONFIG_MICROWINDOWS_KBD_RAW),y)
KEYBOARD = NUTTXRAWKBD
else ifeq ($(CONFIG_MICROWINDOWS_KBD_NONE),y)
KEYBOARD = NOKBD
else ifeq ($(CONFIG_MICROWINDOWS_KBD_CUSTOM),y)
KEYBOARD = CUSTOM
endif

ifeq ($(CONFIG_MICROWINDOWS_MOUSE_RELATIVE),y)
MOUSE = NUTTXMOUSE
else ifeq ($(CONFIG_MICROWINDOWS_MOUSE_TS),y)
MOUSE = NUTTXTOUCH
else ifeq ($(CONFIG_MICROWINDOWS_MOUSE_NONE),y)
MOUSE = NOMOUSE
else ifeq ($(CONFIG_MICROWINDOWS_MOUSE_CUSTOM),y)
MOUSE = CUSTOM
endif

HAVE_FNT_SUPPORT = Y

-include microwindows/src/engine/Objects.rules
-include microwindows/src/drivers/Objects.rules
-include microwindows/src/fonts/Objects.rules

CSRCS += $(MW_CORE_OBJS:%.o=%.c)

ifneq ($(CONFIG_MICROWINDOWS_MWIN),)

# Win32 API
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

ifneq ($(CONFIG_MICROWINDOWS_NANOX),)

MW_NANOX_DRAW_SRCS  = microwindows/src/nanox/nxdraw.c
MW_NANOX_DRAW_SRCS += microwindows/src/nanox/nxutil.c
MW_NANOX_DRAW_SRCS += microwindows/src/nanox/nxtransform.c
MW_NANOX_DRAW_SRCS += microwindows/src/nanox/nxpaintnc.c

MW_NANOX_WM_SRCS  = microwindows/src/nanox/wmaction.c
MW_NANOX_WM_SRCS += microwindows/src/nanox/wmclients.c
MW_NANOX_WM_SRCS += microwindows/src/nanox/wmevents.c
MW_NANOX_WM_SRCS += microwindows/src/nanox/wmutil.c

MW_NANOX_SERVER_SRCS  = microwindows/src/nanox/srvmain.c
MW_NANOX_SERVER_SRCS += microwindows/src/nanox/srvfunc.c
MW_NANOX_SERVER_SRCS += microwindows/src/nanox/srvutil.c
MW_NANOX_SERVER_SRCS += microwindows/src/nanox/srvevent.c
MW_NANOX_SERVER_SRCS += microwindows/src/nanox/srvclip.c

ifeq ($(CONFIG_MICROWINDOWS_NANOX_NONETWORK),y)

# NONETWORK mode: the client application is linked directly with the
# server and runs in the same task, without a network socket.  Only a
# single client can be active at a time.
CFLAGS += -DNONETWORK=1

ifneq ($(CONFIG_MICROWINDOWS_NANOX_NANOWM),)
CFLAGS += -DNANOWM=1
CSRCS += $(MW_NANOX_WM_SRCS)
endif

CSRCS += $(MW_NANOX_DRAW_SRCS) $(MW_NANOX_SERVER_SRCS)
CSRCS += microwindows/src/nanox/srvnonet.c

else

# Network client/server mode: each client connects to the server
# through a local stream socket.  NX_PER_CLIENT_DATA enables the
# per-task client data (POSIX per-thread key) so that several client
# tasks can run at the same time in the flat build.

MW_NANOX_CLIENT_SRCS  = $(MW_NANOX_DRAW_SRCS)
MW_NANOX_CLIENT_SRCS += microwindows/src/nanox/client.c
MW_NANOX_CLIENT_SRCS += microwindows/src/nanox/clientfb.c
MW_NANOX_CLIENT_SRCS += microwindows/src/nanox/nxproto.c
MW_NANOX_CLIENT_SRCS += microwindows/src/drivers/osdep.c

MW_NANOX_SERVER_SRCS += microwindows/src/nanox/srvnet.c

# The server main() is renamed to nanox_server_main() and invoked from
# the nanoxterm example, which starts the server as a separate task.
microwindows/src/nanox/srvmain.c_CFLAGS += -Dmain=nanox_server_main

CFLAGS += -DNX_PER_CLIENT_DATA=1
ifneq ($(CONFIG_MICROWINDOWS_NANOX_NANOWM),)
CFLAGS += -DNANOWM=1

# Rename the server-side GrXXX symbols to SVR_GrXXX, since the server
# and the client code are linked into a single image.
$(foreach src,$(MW_NANOX_WM_SRCS),$(eval $(src)_CFLAGS += -include microwindows/src/nanox/serv.h))
CSRCS += $(MW_NANOX_WM_SRCS)
endif

CSRCS += $(MW_NANOX_CLIENT_SRCS) $(MW_NANOX_SERVER_SRCS)

endif

endif

include $(APPDIR)/Application.mk
