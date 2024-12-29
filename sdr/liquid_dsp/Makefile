###########################################################################
# apps/sdr/liquid_dsp/Makefile
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
############################################################################/

include $(APPDIR)/Make.defs

LIQUID_DSP_VERSION  = 1.4.0
LIQUID_DSP_UNPACK   = liquid-dsp
LIQUID_DSP_TARBALL  = v$(LIQUID_DSP_VERSION).tar.gz
LIQUID_DSP_URL_BASE = https://github.com/jgaeddert/liquid-dsp/archive
LIQUID_DSP_URL      = $(LIQUID_DSP_URL_BASE)/$(LIQUID_DSP_TARBALL)

$(LIQUID_DSP_TARBALL):
	$(Q) echo "Downloading $(LIQUID_DSP_TARBALL)"
	$(Q) curl -O -L $(LIQUID_DSP_URL)

$(LIQUID_DSP_UNPACK): $(LIQUID_DSP_TARBALL)
	$(Q) echo "Unpacking $(LIQUID_DSP_TARBALL) to $(LIQUID_DSP_UNPACK)"
	$(Q) tar xzvf $(LIQUID_DSP_TARBALL)
	$(Q) mv liquid-dsp-$(LIQUID_DSP_VERSION) $(LIQUID_DSP_UNPACK)

$(LIQUID_DSP_UNPACK)/.patch: $(LIQUID_DSP_UNPACK)
	$(Q) touch $(LIQUID_DSP_UNPACK)/.patch

CFLAGS += -std=c99
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/sdr/liquid_dsp
CFLAGS += -DM_SQRT1_2=0.7071067811865475244008443621048490
CFLAGS += -DM_SQRT2=1.4142135623730950488016887242096981
CFLAGS += -DM_PI=3.1415926535897932384626433832795029
CFLAGS += -DM_LN2=0.6931471805599453094172321214581765
CFLAGS += -DM_PI_2=1.5707963267948966192313216916397514
CFLAGS += -DM_2_PI=0.6366197723675813430755350534900574


CSRCS = $(LIQUID_DSP_UNPACK)/src/agc/src/agc_crcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/agc/src/agc_rrrf.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/audio/src/cvsd.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/buffer/src/bufferf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/buffer/src/buffercf.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/channel/src/channel_cccf.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/dotprod/src/dotprod_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/dotprod/src/dotprod_crcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/dotprod/src/dotprod_rrrf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/dotprod/src/sumsq.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/equalization/src/equalizer_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/equalization/src/equalizer_rrrf.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/crc.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_conv.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_conv_poly.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_conv_pmatrix.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_conv_punctured.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_golay2412.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_hamming74.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_hamming84.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_hamming128.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_hamming1511.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_hamming3126.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_hamming128_gentab.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_pass.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_rep3.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_rep5.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_rs.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_secded2216.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_secded3932.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/fec_secded7264.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/interleaver.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/packetizer.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fec/src/sumproduct.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/fft/src/fftf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fft/src/spgramcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fft/src/spgramf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/fft/src/fft_utilities.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/bessel.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/butter.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/cheby1.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/cheby2.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/ellip.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/filter_rrrf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/filter_crcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/filter_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/firdes.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/firdespm.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/fnyquist.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/gmsk.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/group_delay.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/hM3.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/iirdes.pll.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/iirdes.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/lpc.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/rcos.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/rkaiser.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/filter/src/rrcos.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/bpacketgen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/bpacketsync.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/bpresync_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/bsync_rrrf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/bsync_crcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/bsync_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/detector_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/dsssframegen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/dsssframesync.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/framedatastats.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/framesyncstats.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/framegen64.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/framesync64.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/flexframegen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/flexframesync.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/fskframegen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/fskframesync.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/gmskframegen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/gmskframesync.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/msourcecf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/ofdmflexframegen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/ofdmflexframesync.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/presync_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/symstreamcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/symstreamrcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/symtrack_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/qdetector_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/qpacketmodem.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/qpilotgen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/framing/src/qpilotsync.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/poly.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/polyc.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/polyf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/polycf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/math.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/math.bessel.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/math.gamma.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/math.complex.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/math.trig.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/modular_arithmetic.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/poly.findroots.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/math/src/windows.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/matrix.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/matrixf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/matrixc.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/matrixcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/smatrix.common.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/smatrixb.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/smatrixf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/matrix/src/smatrixi.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/ampmodem.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/cpfskdem.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/cpfskmod.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/fskdem.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/fskmod.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/gmskdem.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/gmskmod.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/modemcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/modem_utilities.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/modem_apsk_const.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/modem/src/modem_arb_const.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/multichannel/src/firpfbch_crcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/multichannel/src/firpfbch_cccf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/multichannel/src/ofdmframe.common.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/multichannel/src/ofdmframegen.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/multichannel/src/ofdmframesync.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/nco/src/nco_crcf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/nco/src/nco.utilities.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/nco/src/synth_crcf.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/optim/src/chromosome.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/optim/src/gasearch.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/optim/src/gradsearch.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/optim/src/optim.common.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/optim/src/qnsearch.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/optim/src/utilities.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/quantization/src/compand.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/quantization/src/quantizercf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/quantization/src/quantizerf.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/quantization/src/quantizer.inline.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/rand.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/randn.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/randexp.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/randweib.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/randgamma.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/randnakm.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/randricek.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/random/src/scramble.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/sequence/src/bsequence.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/sequence/src/msequence.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/utility/src/bshift_array.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/utility/src/byte_utilities.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/utility/src/msb_index.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/utility/src/pack_bytes.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/utility/src/shift_array.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/utility/src/utility.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorf_add.port.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorf_norm.port.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorf_mul.port.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorf_trig.port.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorcf_add.port.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorcf_norm.port.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorcf_mul.port.c
CSRCS += $(LIQUID_DSP_UNPACK)/src/vector/src/vectorcf_trig.port.c

CSRCS += $(LIQUID_DSP_UNPACK)/src/libliquid.c

ifeq ($(wildcard $(LIQUID_DSP_UNPACK)/.git),)
context:: $(LIQUID_DSP_UNPACK)/.patch

distclean::
	$(call DELDIR, $(LIQUID_DSP_UNPACK))
	$(call DELFILE, $(LIQUID_DSP_TARBALL))
endif

include $(APPDIR)/Application.mk
