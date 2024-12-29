############################################################################
# apps/videoutils/x264/Makefile
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

ifeq ($(DST_PATH),)
  include $(APPDIR)/Make.defs
  # configure in place
  DST_PATH := x264
else
  # configure out of tree
  DST_PATH := $(realpath $(DST_PATH))
  include $(DST_PATH)/Make.defs
  BIN := $(DST_PATH)/libx264.a
  # enable Application.mk PREFIX
  PREFIX := $(DST_PATH)$(DELIM)

  CFLAGS  += $(INCDIR_PREFIX)$(DST_PATH)
  ASFLAGS += $(INCDIR_PREFIX)$(DST_PATH)
endif

X264_CONFIG_SCRIPT := $(CURDIR)$(DELIM)x264$(DELIM)configure

SRCPATH:=x264

-include $(DST_PATH)/config.mak

ifneq ($(CONFIG_ARCH),sim)
  CFG_CMDS += --host=$(CONFIG_ARCH)-unknown-linux-gnu
  CFG_CMDS += --cross-prefix=$(CROSSDEV)
else
  ifeq ($(CONFIG_SIM_M32),y)
    CFG_CMDS += --host=i386-unknown-linux-gnu
  else
    CFG_CMDS += --host=x86_64-unknown-linux-gnu
  endif
endif

# suppress -Wundef -Wno-format
CFLAGS += -Wno-undef -Wno-format -fno-var-tracking-assignments \
          -Wno-unused-but-set-variable -Wno-cpp \
          -Wno-dangling-pointer -Wno-array-bounds

SRCS = common/osdep.c common/base.c common/cpu.c common/tables.c \
       encoder/api.c

SRCS_X = common/mc.c common/predict.c common/pixel.c common/macroblock.c \
         common/frame.c common/dct.c common/cabac.c \
         common/common.c common/rectangle.c \
         common/set.c common/quant.c common/deblock.c common/vlc.c \
         common/mvpred.c common/bitstream.c \
         encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
         encoder/set.c encoder/macroblock.c encoder/cabac.c \
         encoder/cavlc.c encoder/encoder.c encoder/lookahead.c

SRCCLI = autocomplete.c input/input.c input/timecode.c input/raw.c \
         input/y4m.c output/raw.c output/matroska.c output/matroska_ebml.c \
         output/flv.c output/flv_bytestream.c filters/filters.c \
         filters/video/video.c filters/video/source.c filters/video/internal.c \
         filters/video/resize.c filters/video/fix_vfr_pts.c \
         filters/video/select_every.c filters/video/crop.c

SRCCLI_X = filters/video/cache.c filters/video/depth.c

CONFIG := $(shell cat $(DST_PATH)/config.h)

# Optional module sources
ifneq ($(findstring HAVE_AVS 1, $(CONFIG)),)
  SRCCLI += input/avs.c
endif

ifneq ($(findstring HAVE_THREAD 1, $(CONFIG)),)
  SRCS_X   += common/threadpool.c
  SRCCLI_X += input/thread.c
endif

ifneq ($(findstring HAVE_WIN32THREAD 1, $(CONFIG)),)
  SRCS += common/win32thread.c
endif

ifneq ($(findstring HAVE_LAVF 1, $(CONFIG)),)
  SRCCLI += input/lavf.c
endif

ifneq ($(findstring HAVE_FFMS 1, $(CONFIG)),)
  SRCCLI += input/ffms.c
endif

ifneq ($(findstring HAVE_GPAC 1, $(CONFIG)),)
  SRCCLI += output/mp4.c
endif

ifneq ($(findstring HAVE_LSMASH 1, $(CONFIG)),)
  SRCCLI += output/mp4_lsmash.c
endif

SRCASM_X =
SRCASM_8 =
SRCASM_G =

# MMX/SSE optims
ifeq ($(SYS_ARCH),X86)
  ARCH_X86 = yes
  SRCASM_X += common/x86/dct-32.asm \
              common/x86/pixel-32.asm
endif
ifeq ($(SYS_ARCH),X86_64)
  ARCH_X86 = yes
  SRCASM_X += common/x86/dct-64.asm \
              common/x86/trellis-64.asm
endif
ifdef ARCH_X86
  SRCASM_X += common/x86/bitstream-a.asm \
              common/x86/const-a.asm \
              common/x86/cabac-a.asm \
              common/x86/dct-a.asm \
              common/x86/deblock-a.asm \
              common/x86/mc-a.asm \
              common/x86/mc-a2.asm \
              common/x86/pixel-a.asm \
              common/x86/predict-a.asm \
              common/x86/quant-a.asm
  SRCS_X   += common/x86/mc-c.c \
              common/x86/predict-c.c

  SRCASM_G += common/x86/cpu-a.asm
  ifneq ($(findstring HAVE_BITDEPTH8 1, $(CONFIG)),)
    SRCASM_8 += common/x86/sad-a.asm
  endif
endif


# AltiVec optims
ifeq ($(SYS_ARCH),PPC)
  SRCS_X += common/ppc/dct.c \
            common/ppc/deblock.c \
            common/ppc/mc.c \
            common/ppc/pixel.c \
            common/ppc/predict.c \
            common/ppc/quant.c
endif


# NEON optims
ifeq ($(SYS_ARCH),ARM)
  SRCASM_X  = common/arm/bitstream-a.S \
              common/arm/dct-a.S \
              common/arm/deblock-a.S \
              common/arm/mc-a.S \
              common/arm/pixel-a.S \
              common/arm/predict-a.S \
              common/arm/quant-a.S
  SRCS_X   += common/arm/mc-c.c \
              common/arm/predict-c.c

  SRCASM_G += common/arm/cpu-a.S
endif


# AArch64 NEON optims
ifeq ($(SYS_ARCH),AARCH64)
  SRCASM_X  = common/aarch64/bitstream-a.S \
              common/aarch64/cabac-a.S \
              common/aarch64/dct-a.S \
              common/aarch64/deblock-a.S \
              common/aarch64/mc-a.S \
              common/aarch64/pixel-a.S \
              common/aarch64/predict-a.S \
              common/aarch64/quant-a.S
  SRCS_X   += common/aarch64/asm-offsets.c \
              common/aarch64/mc-c.c \
              common/aarch64/predict-c.c
endif


# MSA optims
ifeq ($(SYS_ARCH),MIPS)
  ifneq ($(findstring HAVE_MSA 1, $(CONFIG)),)
    SRCS_X += common/mips/dct-c.c \
              common/mips/deblock-c.c \
              common/mips/mc-c.c \
              common/mips/pixel-c.c \
              common/mips/predict-c.c \
              common/mips/quant-c.c
  endif
endif


ifneq ($(HAVE_GETOPT_LONG),1)
  SRCCLI += extras/getopt.c
endif

CSRCS := $(addprefix $(SRCPATH)$(DELIM), $(SRCS) $(SRCS_X))

X264EASFLAGS := $(ASFLAGS)
X264ECFLAGS := $(CFLAGS)

CFLAGS += -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8

ifdef ARCH_X86
  ASMEXT=asm
  AS=nasm
else
  ASMEXT=S
  AS=$(CC)
endif

NASUFFIX = $(subst $(DELIM),.,$(CURDIR)$(DELIM)$(SRCPATH))

SRCASM_8 := $(addprefix $(SRCPATH)$(DELIM),$(SRCASM_8) $(SRCASM_X))
SRCASM_G := $(addprefix $(SRCPATH)$(DELIM),$(SRCASM_G))

OBJASM_8 = $(addprefix $(PREFIX),$(addsuffix $(NASUFFIX)$(OBJEXT),$(SRCASM_8)))
OBJASM_G = $(addprefix $(PREFIX),$(addsuffix $(NASUFFIX)$(OBJEXT),$(SRCASM_G)))

EXTOBJS = $(OBJASM_8) $(OBJASM_G)

$(OBJASM_8): $(PREFIX)%.$(ASMEXT)$(NASUFFIX)$(OBJEXT): %.$(ASMEXT)
	$(Q) $(AS) $(ASFLAGS) -DBIT_DEPTH=8 -Dprivate_prefix=x264_8 $< -o $@

$(OBJASM_G): $(PREFIX)%.$(ASMEXT)$(NASUFFIX)$(OBJEXT): %.$(ASMEXT)
	$(Q) $(AS) $(ASFLAGS) $< -o $@

ifneq ($(CONFIG_UTILS_X264),)
  MODULE    += CONFIG_UTILS_X264
  PROGNAME  += x264
  PRIORITY  += $(CONFIG_UTILS_X264_PRIORITY)
  STACKSIZE += $(CONFIG_UTILS_X264_STACKSIZE)
  MAINSRC   += $(SRCPATH)$(DELIM)x264.c
  CSRCS     += $(addprefix $(SRCPATH)$(DELIM), $(SRCCLI) $(SRCCLI_X))
endif

$(DST_PATH)/config.mak: x264
	$(Q)echo "X264 configure... $(CONFIG_ARCH)"
	$(Q)cd $(DST_PATH) && $(X264_CONFIG_SCRIPT) --disable-cli --enable-static \
  --disable-bashcompletion --disable-opencl \
  --disable-thread --disable-avs --disable-swscale \
  --disable-lavf --disable-ffms --disable-gpac --disable-lsmash \
  --extra-asflags="$(X264EASFLAGS)" --extra-cflags="$(X264ECFLAGS)" \
  --bit-depth=8 $(CFG_CMDS)
	$(Q)cd $(DST_PATH) && sed -n '/^\(SYS_ARCH\|CFLAGS\|ASFLAGS\|HAVE_GETOPT_LONG\)=/ p' config.mak > config.mak.mod \
    && mv -f config.mak.mod config.mak

context:: $(DST_PATH)/config.mak
ifneq ($(PREFIX),)
	@$(eval ALL_SRCS = $(ASRCS) $(CSRCS) $(MAINSRC) $(NASRCS) $(SRCASM_8) $(SRCASM_G))
	@$(foreach src,$(ALL_SRCS),$(shell mkdir -p $(PREFIX)$(dir $(src))))
endif


distclean::
	$(Q)rm -f $(DST_PATH)/x264_config.h $(DST_PATH)/config.mak \
	  $(DST_PATH)/config.log $(DST_PATH)/x264.pc $(DST_PATH)/x264.def \
	  $(DST_PATH)/config.h

# Download x264 if no x264/.git found
x264:
	$(Q)git clone https://github.com/mirror/x264.git x264
	$(Q)cd x264 && git apply ../patch/0001-x264-fix-UNUSED-redefined.patch

ifeq ($(wildcard x264/.git),)
context:: x264
distclean::
	$(call DELDIR, x264)
endif

include $(APPDIR)/Application.mk

