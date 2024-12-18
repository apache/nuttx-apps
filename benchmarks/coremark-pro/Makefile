############################################################################
# apps/benchmarks/coremark-pro/Makefile
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

PRIORITY  = $(CONFIG_BENCHMARK_COREMARK_PRO_PRIORITY)
STACKSIZE = $(CONFIG_BENCHMARK_COREMARK_PRO_STACKSIZE)
MODULE    = $(CONFIG_BENCHMARK_COREMARK_PRO)

CFLAGS += -Wno-undef -Wno-strict-prototypes -Wno-unused-variable \
          -Wno-pointer-sign -Wno-unused-but-set-variable -Wno-shadow
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/benchmarks/coremark-pro/coremark-pro/mith/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/benchmarks/coremark-pro/coremark-pro/mith/al/include
CFLAGS += -DNDEBUG -DHOST_EXAMPLE_CODE=1 -DHAVE_SYS_STAT_H=1 \
          -DGCC_INLINE_MACRO=1 -DMAX_CONTEXTS=100 -DEE_SIZEOF_INT=4 \
          -DUSE_FP128=0

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_USE_SINGLE_CONTEXT),)
  CFLAGS += -DUSE_SINGLE_CONTEXT=1 -DHAVE_PTHREAD=0 -DUSE_NATIVE_PTHREAD=0
else
  CFLAGS += -DUSE_SINGLE_CONTEXT=0 -DHAVE_PTHREAD=1 -DUSE_NATIVE_PTHREAD=1
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_SMP),)
  CFLAGS += -DHAVE_PTHREAD_SETAFFINITY_NP=1 -DHAVE_PTHREAD_SELF=1
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_64BIT),)
  CFLAGS += -DEE_SIZEOF_PTR=8 -DEE_PTR_ALIGN=8 -DEE_SIZEOF_LONG=8
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64),)
  CFLAGS += -DUSE_FP64=1 -DUSE_FP32=0
  CSRCS  += coremark-pro/benchmarks/fp/linpack/ref/inputs_f64.c
else
  CFLAGS += -DUSE_FP32=1 -DUSE_FP64=0
  CSRCS  += coremark-pro/benchmarks/fp/linpack/ref/inputs_f32.c
endif

CSRCS += coremark-pro/mith/src/md5.c
CSRCS += coremark-pro/mith/src/mith_lib.c
CSRCS += coremark-pro/mith/src/mith_workload.c
CSRCS += coremark-pro/mith/src/th_bignum.c
CSRCS += coremark-pro/mith/src/th_encode.c
CSRCS += coremark-pro/mith/src/th_lib.c
CSRCS += coremark-pro/mith/src/th_math.c
CSRCS += coremark-pro/mith/src/th_rand.c

CSRCS += $(wildcard coremark-pro/mith/al/src/*.c)

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_CORE_TEST),)
  CSRCS    += $(wildcard coremark-pro/benchmarks/core/*.c)
  MAINSRC  += coremark-pro/workloads/core/core.c
  PROGNAME += core
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_CJPEG_TEST),)
  CSRCS    += $(wildcard coremark-pro/benchmarks/consumer_v2/cjpeg/*.c)
  CSRCS    += $(wildcard coremark-pro/benchmarks/consumer_v2/cjpeg/data/Rose256_bmp.c)
  MAINSRC  += coremark-pro/workloads/cjpeg-rose7-preset/cjpeg-rose7-preset.c
  PROGNAME += cjpeg_rose7_preset
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_SHA_TEST),)
  CSRCS    += $(wildcard coremark-pro/benchmarks/darkmark/sha/*.c)
  MAINSRC  += coremark-pro/workloads/sha-test/sha-test.c
  PROGNAME += sha_test
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_ZIP_TEST),)
  CFLAGS   += ${INCDIR_PREFIX}$(APPDIR)/system/zlib/zlib
  CFLAGS   += -Dgzclose_r=coremark_zip_gzclose_r -Dgzclose_w=coremark_zip_gzclose_w
  CSRCS    += coremark-pro/benchmarks/darkmark/zip/zip_darkmark.c
  MAINSRC  += coremark-pro/workloads/zip-test/zip-test.c
  PROGNAME += zip_test
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_LINEAR_ALG_MID_TEST),)
  CSRCS    += coremark-pro/benchmarks/fp/linpack/linpack.c
  MAINSRC  += coremark-pro/workloads/linear_alg-mid-100x100-sp/linear_alg-mid-100x100-sp.c
  PROGNAME += linear_alg_mid_100x100_sp
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_RADIX_TEST),)
  ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64),)
    CSRCS += $(wildcard coremark-pro/benchmarks/fp/fft_radix2/ref/*.c)
  else
    CSRCS += $(wildcard coremark-pro/benchmarks/fp/fft_radix2/ref-sp/*.c)
  endif

  CSRCS    += coremark-pro/benchmarks/fp/fft_radix2/fft_radix2.c
  MAINSRC  += coremark-pro/workloads/radix2-big-64k/radix2-big-64k.c
  PROGNAME += radix2_big_64k
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_LOOPS_ALL_MID_TEST),)
  ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64),)
    CSRCS += $(wildcard coremark-pro/benchmarks/fp/loops/ref/*.c)
  else
    CSRCS += $(wildcard coremark-pro/benchmarks/fp/loops/ref-sp/*.c)
  endif

  CSRCS    += coremark-pro/benchmarks/fp/loops/loops.c
  MAINSRC  += coremark-pro/workloads/loops-all-mid-10k-sp/loops-all-mid-10k-sp.c
  PROGNAME += loops_all_mid_10k_sp
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_NNET_TEST),)
  ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_USE_FP64),)
    CSRCS += $(wildcard coremark-pro/benchmarks/fp/nnet/ref/*.c)
  else
    CSRCS += $(wildcard coremark-pro/benchmarks/fp/nnet/ref-sp/*.c)
  endif

  CSRCS    += $(wildcard coremark-pro/benchmarks/fp/nnet/*.c)
  MAINSRC  += coremark-pro/workloads/nnet_test/nnet_test.c
  PROGNAME += nnet_test
endif

ifneq ($(CONFIG_BENCHMARK_COREMARK_PRO_PARSER_125K),)
  CSRCS    += $(wildcard coremark-pro/benchmarks/darkmark/parser/*.c)
  MAINSRC  += coremark-pro/workloads/parser-125k/parser-125k.c
  PROGNAME += parser_125k
endif

ifeq ($(wildcard coremark-pro/.git),)
VERSION ?= main
coremark-pro.zip:
	$(Q) curl -L https://github.com/eembc/coremark-pro/archive/refs/heads/main.zip -o coremark-pro.zip
	$(Q) unzip -o coremark-pro.zip
	$(Q) mv coremark-pro-$(VERSION) coremark-pro

context:: coremark-pro.zip

distclean::
	$(call DELDIR, coremark-pro)
	$(call DELFILE, coremark-pro.zip)

endif

include $(APPDIR)/Application.mk
