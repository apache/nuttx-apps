############################################################################
# apps/benchmarks/tacle-bench/Makefile
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

############################################################################
# Targets
############################################################################


include $(APPDIR)/Make.defs

PROGNAME  = taclebench
PRIORITY  = $(CONFIG_BENCHMARK_TACLEBENCH_PRIORITY)
STACKSIZE = $(CONFIG_BENCHMARK_TACLEBENCH_STACKSIZE)
MODULE    = $(CONFIG_BENCHMARK_TACLEBENCH)

TACLEBENCH_UNPACK  = tacle-bench
TACLEBENCH_URL     = https://github.com/tacle/tacle-bench/archive
TACLEBENCH_VERSION = master
TACLEBENCH_ZIP     = $(TACLEBENCH_UNPACK)-$(TACLEBENCH_VERSION).zip
UNPACK            ?= unzip -q -o

$(TACLEBENCH_ZIP):
	@echo "Downloading: $(TACLEBENCH_URL)"
	$(Q) curl -L $(TACLEBENCH_URL)/$(TACLEBENCH_VERSION).zip -o $(TACLEBENCH_UNPACK)-$(TACLEBENCH_VERSION).zip

$(TACLEBENCH_UNPACK): $(TACLEBENCH_ZIP)
	@echo "Unpacking: $(TACLEBENCH_ZIP) -> $(TACLEBENCH_UNPACK)"
	$(Q) $(UNPACK) $(TACLEBENCH_ZIP)
	$(Q) mv	tacle-bench-$(TACLEBENCH_VERSION) $(TACLEBENCH_UNPACK)
	$(Q) touch $(TACLEBENCH_UNPACK)
	@echo "Patching: Applying patch"
	$(Q) cd tacle-bench && patch -p1 < ../0001-tacle-bench-add-makefile-and-all-in-one-main-file.patch

ifeq ($(wildcard $(TACLEBENCH_UNPACK)/.git),)
context:: $(TACLEBENCH_UNPACK)

distclean::
	$(call DELDIR, $(TACLEBENCH_UNPACK))
	$(call DELFILE, $(TACLEBENCH_ZIP))
endif

# app
CSRCS     = tacle-bench/bench/app/powerwindow/powerwindow_const_params.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_inputs.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_BackL.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_debounce.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_DRV.c
CSRCS    += tacle-bench/bench/app/powerwindow/wcclib.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_BackR.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_PW_Control_PSG_Front.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_controlexclusion.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow.c
CSRCS    += tacle-bench/bench/app/powerwindow/powerwindow_powerwindow_control.c
CSRCS    += tacle-bench/bench/app/lift/lift.c
CSRCS    += tacle-bench/bench/app/lift/liftlibio.c
CSRCS    += tacle-bench/bench/app/lift/liftlibcontrol.c

# kernel
CSRCS    += tacle-bench/bench/kernel/fac/fac.c
CSRCS    += tacle-bench/bench/kernel/bsort/bsort.c
CSRCS    += tacle-bench/bench/kernel/complex_updates/complex_updates.c
CSRCS    += tacle-bench/bench/kernel/cosf/cosf.c
CSRCS    += tacle-bench/bench/kernel/isqrt/isqrt.c
CSRCS    += tacle-bench/bench/kernel/isqrt/basicmath_libc.c
CSRCS    += tacle-bench/bench/kernel/minver/minver.c
CSRCS    += tacle-bench/bench/kernel/pm/pm_stdlib.c
CSRCS    += tacle-bench/bench/kernel/pm/pm_input.c
CSRCS    += tacle-bench/bench/kernel/pm/pm_libm.c
CSRCS    += tacle-bench/bench/kernel/pm/pm.c
CSRCS    += tacle-bench/bench/kernel/lms/lms.c
CSRCS    += tacle-bench/bench/kernel/binarysearch/binarysearch.c
CSRCS    += tacle-bench/bench/kernel/fft/fft_input.c
CSRCS    += tacle-bench/bench/kernel/fft/fft.c
CSRCS    += tacle-bench/bench/kernel/sha/memset.c
CSRCS    += tacle-bench/bench/kernel/sha/input_small.c
CSRCS    += tacle-bench/bench/kernel/sha/memhelper.c
CSRCS    += tacle-bench/bench/kernel/sha/memcpy.c
CSRCS    += tacle-bench/bench/kernel/sha/sha.c
CSRCS    += tacle-bench/bench/kernel/iir/iir.c
CSRCS    += tacle-bench/bench/kernel/matrix1/matrix1.c
CSRCS    += tacle-bench/bench/kernel/st/st.c
CSRCS    += tacle-bench/bench/kernel/prime/prime.c
CSRCS    += tacle-bench/bench/kernel/deg2rad/deg2rad.c
CSRCS    += tacle-bench/bench/kernel/filterbank/filterbank.c
CSRCS    += tacle-bench/bench/kernel/jfdctint/jfdctint.c
CSRCS    += tacle-bench/bench/kernel/fir2dim/fir2dim.c
CSRCS    += tacle-bench/bench/kernel/ludcmp/ludcmp.c
CSRCS    += tacle-bench/bench/kernel/bitcount/bitcount.c
CSRCS    += tacle-bench/bench/kernel/bitcount/bitcnt_2.c
CSRCS    += tacle-bench/bench/kernel/bitcount/bitcnt_1.c
CSRCS    += tacle-bench/bench/kernel/bitcount/bitcnt_3.c
CSRCS    += tacle-bench/bench/kernel/bitcount/bitcnt_4.c
CSRCS    += tacle-bench/bench/kernel/md5/md5.c
CSRCS    += tacle-bench/bench/kernel/recursion/recursion.c
CSRCS    += tacle-bench/bench/kernel/insertsort/insertsort.c
CSRCS    += tacle-bench/bench/kernel/cubic/cubic.c
CSRCS    += tacle-bench/bench/kernel/rad2deg/rad2deg.c
CSRCS    += tacle-bench/bench/kernel/quicksort/quicksortstdlib.c
CSRCS    += tacle-bench/bench/kernel/quicksort/input.c
CSRCS    += tacle-bench/bench/kernel/quicksort/quicksortlibm.c
CSRCS    += tacle-bench/bench/kernel/quicksort/quicksort.c
CSRCS    += tacle-bench/bench/kernel/countnegative/countnegative.c
CSRCS    += tacle-bench/bench/kernel/bitonic/bitonic.c

# sequential
CSRCS    += tacle-bench/bench/sequential/epic/epic.c
CSRCS    += tacle-bench/bench/sequential/mpeg2/mpeg2.c
CSRCS    += tacle-bench/bench/sequential/cjpeg_transupp/cjpeg_transupp.c
CSRCS    += tacle-bench/bench/sequential/adpcm_enc/adpcm_enc.c
CSRCS    += tacle-bench/bench/sequential/rijndael_dec/rijndael_dec.c
CSRCS    += tacle-bench/bench/sequential/rijndael_dec/input_small_enc.c
CSRCS    += tacle-bench/bench/sequential/rijndael_dec/aes.c
CSRCS    += tacle-bench/bench/sequential/rijndael_dec/rijndael_dec_libc.c
CSRCS    += tacle-bench/bench/sequential/huff_enc/huff_enc.c
CSRCS    += tacle-bench/bench/sequential/dijkstra/dijkstra.c
CSRCS    += tacle-bench/bench/sequential/dijkstra/input.c
CSRCS    += tacle-bench/bench/sequential/gsm_dec/gsm_dec.c
CSRCS    += tacle-bench/bench/sequential/ammunition/arithm.c
CSRCS    += tacle-bench/bench/sequential/ammunition/ammunition.c
CSRCS    += tacle-bench/bench/sequential/ammunition/bits.c
CSRCS    += tacle-bench/bench/sequential/ammunition/ammunition_libc.c
CSRCS    += tacle-bench/bench/sequential/susan/wccmalloc.c
CSRCS    += tacle-bench/bench/sequential/susan/wccfile.c
CSRCS    += tacle-bench/bench/sequential/susan/input.c
CSRCS    += tacle-bench/bench/sequential/susan/susan.c
CSRCS    += tacle-bench/bench/sequential/susan/wcclibm.c
CSRCS    += tacle-bench/bench/sequential/ndes/ndes.c
CSRCS    += tacle-bench/bench/sequential/petrinet/petrinet.c
CSRCS    += tacle-bench/bench/sequential/gsm_enc/gsm_enc.c
CSRCS    += tacle-bench/bench/sequential/h264_dec/h264_decinput.c
CSRCS    += tacle-bench/bench/sequential/h264_dec/h264_dec.c
CSRCS    += tacle-bench/bench/sequential/rijndael_enc/input_small.c
CSRCS    += tacle-bench/bench/sequential/rijndael_enc/rijndael_enc.c
CSRCS    += tacle-bench/bench/sequential/rijndael_enc/aes.c
CSRCS    += tacle-bench/bench/sequential/rijndael_enc/rijndael_enc_libc.c
CSRCS    += tacle-bench/bench/sequential/huff_dec/huff_dec.c
CSRCS    += tacle-bench/bench/sequential/anagram/anagram_stdlib.c
CSRCS    += tacle-bench/bench/sequential/anagram/anagram.c
CSRCS    += tacle-bench/bench/sequential/anagram/anagram_input.c
CSRCS    += tacle-bench/bench/sequential/g723_enc/g723_enc.c
CSRCS    += tacle-bench/bench/sequential/adpcm_dec/adpcm_dec.c
CSRCS    += tacle-bench/bench/sequential/cjpeg_wrbmp/cjpeg_wrbmp.c
CSRCS    += tacle-bench/bench/sequential/cjpeg_wrbmp/input.c
CSRCS    += tacle-bench/bench/sequential/statemate/statemate.c
CSRCS    += tacle-bench/bench/sequential/fmref/fmref.c
CSRCS    += tacle-bench/bench/sequential/fmref/wcclibm.c
CSRCS    += tacle-bench/bench/sequential/audiobeam/audiobeamlibmalloc.c
CSRCS    += tacle-bench/bench/sequential/audiobeam/audiobeam.c
CSRCS    += tacle-bench/bench/sequential/audiobeam/audiobeamlibm.c
CSRCS    += tacle-bench/bench/sequential/audiobeam/audiobeaminput.c
CSRCS    += tacle-bench/bench/test/test3/test3.c
CSRCS    += tacle-bench/bench/test/duff/duff.c
CSRCS    += tacle-bench/bench/test/cover/cover.c

# only one wcclibm is allowed
CSRCS    += tacle-bench/bench/kernel/cosf/wcclibm.c

MAINSRC   = tacle-bench/taclebench.c

CFLAGS   += -DALL_IN_ONE
CFLAGS   += -Wno-unknown-pragmas -Wno-strict-prototypes
CFLAGS   += -Wno-array-bounds -Wno-shadow
CFLAGS   += -Wno-stringop-overflow -Wno-maybe-uninitialized

include $(APPDIR)/Application.mk
