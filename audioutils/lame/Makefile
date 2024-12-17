############################################################################
# apps/audioutils/lame/Makefile
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

# Download lame if no lame/configure found
lame-svn:
	$(Q) echo "svn checkout lame ..."
	$(Q) svn checkout https://svn.code.sf.net/p/lame/svn/trunk/lame lame

ifeq ($(wildcard lame/configure),)
context:: lame-svn
distclean::
	$(call DELDIR, lame)
endif

ifeq ($(DST_PATH),)
  include $(APPDIR)/Make.defs
  # configure in place
  DST_PATH := lame
else
  # configure out of tree
  DST_PATH := $(realpath $(DST_PATH))
  include $(DST_PATH)/Make.defs
  BIN := $(DST_PATH)/libmp3lame.a
  # enable Application.mk PREFIX
  PREFIX := $(DST_PATH)$(DELIM)
endif

ifneq ($(CONFIG_ARCH),sim)
  CFG_CMDS += --host=$(CONFIG_ARCH)
  CFG_CMDS += --cross-prefix=$(CROSSDEV)
endif

SRC_PATH := lame

CFLAGS += -DHAVE_CONFIG_H -D'fast_log2=lame_fast_log2'
CFLAGS += $(INCDIR_PREFIX)$(DST_PATH) \
          $(INCDIR_PREFIX)$(SRC_PATH)/include \
          $(INCDIR_PREFIX)$(SRC_PATH)/libmp3lame
CFLAGS += -Wno-address -Wno-array-parameter \
          -Wno-builtin-declaration-mismatch \
          -Wno-incompatible-pointer-types \
          -Wno-implicit-function-declaration -Wno-shadow \
          -Wno-stringop-overflow -Wno-unused-variable \
          -Wno-unused-but-set-variable \
          -msse

CSRCS += $(SRC_PATH)/libmp3lame/bitstream.c \
         $(SRC_PATH)/libmp3lame/encoder.c \
         $(SRC_PATH)/libmp3lame/fft.c \
         $(SRC_PATH)/libmp3lame/gain_analysis.c \
         $(SRC_PATH)/libmp3lame/id3tag.c \
         $(SRC_PATH)/libmp3lame/lame.c \
         $(SRC_PATH)/libmp3lame/newmdct.c \
         $(SRC_PATH)/libmp3lame/psymodel.c \
         $(SRC_PATH)/libmp3lame/quantize.c \
         $(SRC_PATH)/libmp3lame/vector/xmm_quantize_sub.c \
         $(SRC_PATH)/libmp3lame/quantize_pvt.c \
         $(SRC_PATH)/libmp3lame/set_get.c \
         $(SRC_PATH)/libmp3lame/vbrquantize.c \
         $(SRC_PATH)/libmp3lame/reservoir.c \
         $(SRC_PATH)/libmp3lame/tables.c \
         $(SRC_PATH)/libmp3lame/takehiro.c \
         $(SRC_PATH)/libmp3lame/util.c \
         $(SRC_PATH)/libmp3lame/VbrTag.c \
         $(SRC_PATH)/libmp3lame/version.c \
         $(SRC_PATH)/libmp3lame/presets.c

LAME_CONFIG_SCRIPT := $(CURDIR)$(DELIM)lame$(DELIM)configure

$(DST_PATH)/config.h:
	$(Q)echo "lame configure... $(CONFIG_ARCH)"
	$(Q)cd $(DST_PATH) && $(LAME_CONFIG_SCRIPT) \
	--disable-cpml --disable-decoder --disable-efence \
	--disable-frontend --disable-mp3x --disable-gtktest \
	--disable-ipv6 --disable-rpath --enable-static \
	$(CFG_CMDS)

context:: $(DST_PATH)/config.h

ifneq ($(PREFIX),)
	@$(eval ALL_SRCS = $(CSRCS))
	@$(foreach src,$(ALL_SRCS),$(shell mkdir -p $(PREFIX)$(dir $(src))))
endif

distclean::
	$(Q)cd $(DST_PATH) && make distclean

include $(APPDIR)/Application.mk
