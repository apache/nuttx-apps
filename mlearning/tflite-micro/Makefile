############################################################################
# apps/mlearning/tflite-micro/Makefile
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

TFLITE_MICRO_VER = main
TFLITE_MICRO_UNPACK = tflite-micro

tflite-micro.zip:
	$(Q) curl -L https://github.com/tensorflow/tflite-micro/archive/refs/heads/$(TFLITE_MICRO_VER).zip -o tflite-micro.zip
	$(Q) unzip -o tflite-micro.zip
	$(Q) mv tflite-micro-$(TFLITE_MICRO_VER) tflite-micro
	$(Q) patch -d $(TFLITE_MICRO_UNPACK) -p1 < tflite-micro.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard tflite-micro/.git),)
context:: tflite-micro.zip
distclean::
	$(call DELDIR, tflite-micro)
	$(call DELFILE, tflite-micro.zip)
endif

CXXEXT  := .cc

COMMON_FLAGS += -DTF_LITE_STATIC_MEMORY
COMMON_FLAGS += -DTF_LITE_DISABLE_X86_NEON
COMMON_FLAGS += -Wno-undef
COMMON_FLAGS += -Wno-shadow

SRC_FILES := $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/micro/kernels/*.cc)

ifeq ($(CONFIG_MLEARNING_CMSIS_NN),y)
COMMON_FLAGS += -DCMSIS_NN
COMMON_FLAGS += ${INCDIR_PREFIX}$(APPDIR)/mlearning/cmsis-nn/cmsis-nn
NEW_SRC_FILES := $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/micro/kernels/cmsis_nn/*.cc)
SRC_FILES := $(filter-out $(notdir $(NEW_SRC_FILES)), $(notdir $(SRC_FILES)))
SRC_FILES := $(addprefix $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/micro/kernels/, $(SRC_FILES))
SRC_FILES += $(NEW_SRC_FILES)
endif

COMMON_FLAGS += ${INCDIR_PREFIX}$(APPDIR)/math/gemmlowp/gemmlowp
COMMON_FLAGS += ${INCDIR_PREFIX}$(APPDIR)/math/ruy/ruy
COMMON_FLAGS += ${INCDIR_PREFIX}$(APPDIR)/math/kissfft/kissfft
COMMON_FLAGS += ${INCDIR_PREFIX}$(APPDIR)/mlearning/tflite-micro/tflite-micro

CFLAGS   += ${COMMON_FLAGS}
CXXFLAGS += ${COMMON_FLAGS}

SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/c/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/schema/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/core/c/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/kernels/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/kernels/internal/optimized/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/kernels/internal/reference/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/kernels/internal/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/core/api/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/micro/arena_allocator/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/micro/memory_planner/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/micro/*.cc)
SRC_FILES += $(wildcard $(APPDIR)/mlearning/tflite-micro/tflite-micro/tensorflow/lite/micro/tflite_bridge/*.cc)

SRC_FILES := $(filter-out %test.cc, $(SRC_FILES))

CXXSRCS += $(SRC_FILES)

include $(APPDIR)/Application.mk
