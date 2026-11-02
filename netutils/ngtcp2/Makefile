# ##############################################################################
# apps/netutils/ngtcp2/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

include $(APPDIR)/Make.defs

NGTCP2_UNPACKNAME = ngtcp2
NGTCP2_VERSION = $(patsubst "%",%,$(strip $(CONFIG_NETUTILS_QUIC_VERSION)))

# ngtcp2 library
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_pkt.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_conv.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_str.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_vec.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_buf.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_conn.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_mem.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_pq.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_map.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_rob.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_ppe.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_crypto.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_err.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_range.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_acktr.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_rtb.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_frame_chain.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_strm.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_idtr.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_gaptr.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_ringbuf.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_log.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_qlog.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_cid.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_ksl.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_cc.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_bbr.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_addr.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_path.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_pv.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_pmtud.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_version.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_rst.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_window_filter.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_opl.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_balloc.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_objalloc.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_unreachable.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_transport_params.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_settings.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_callbacks.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_dcidtr.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_pcg.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_ratelim.c
CSRCS += $(NGTCP2_UNPACKNAME)/lib/ngtcp2_conn_info.c

# ngtcp2 crypto layer
CSRCS += $(NGTCP2_UNPACKNAME)/crypto/shared.c
CSRCS += $(NGTCP2_UNPACKNAME)/crypto/wolfssl/wolfssl.c

# Compilation flags
CFLAGS = -Wno-undef

# ngtcp2 specific includes
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/netutils/ngtcp2
CFLAGS += ${INCDIR_PREFIX}$(NGTCP2_UNPACKNAME)/lib
CFLAGS += ${INCDIR_PREFIX}$(NGTCP2_UNPACKNAME)/lib/includes
CFLAGS += ${INCDIR_PREFIX}$(NGTCP2_UNPACKNAME)/crypto
CFLAGS += ${INCDIR_PREFIX}$(NGTCP2_UNPACKNAME)/crypto/includes

# NuttX specific includes
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/../nuttx/include

# wolfssl specific includes
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/crypto/wolfssl
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/crypto/wolfssl/wolfssl
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/crypto/wolfssl/openssl
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/include

CFLAGS += -DNO_INT128 -DHAVE_WOLFSSL -DBUILDING_NGTCP2 -DWOLFSSL_QUIC -DOPENSSL_EXTRA -DHAVE_AESGCM -DHAVE_SESSION_TICKET

NGTCP2_URL ?= "https://github.com/ngtcp2/ngtcp2/releases/download"
NGTCP2_TAR = ngtcp2-$(NGTCP2_VERSION).tar.bz2

UNPACK ?= tar -xf

$(NGTCP2_TAR):
	@echo "Downloading: $(NGTCP2_URL)/v$(NGTCP2_VERSION)/$(NGTCP2_TAR)"
	$(Q) curl -o $(NGTCP2_TAR) -L $(NGTCP2_URL)/v$(NGTCP2_VERSION)/$(NGTCP2_TAR)

$(NGTCP2_UNPACKNAME): $(NGTCP2_TAR)
	@echo "Unpacking: $(NGTCP2_TAR) -> $(NGTCP2_UNPACKNAME)"
	$(Q) $(UNPACK) $(NGTCP2_TAR)
	$(Q) mv ngtcp2-$(NGTCP2_VERSION) $(NGTCP2_UNPACKNAME)
	
distclean::
	$(call DELDIR, $(NGTCP2_UNPACKNAME))
	$(call DELFILE, $(NGTCP2_TAR))

context:: $(NGTCP2_UNPACKNAME)

include $(APPDIR)/Application.mk
