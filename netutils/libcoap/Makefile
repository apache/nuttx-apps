############################################################################
# apps/netutils/libcoap/Makefile
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

COAP_URL ?= "https://codeload.github.com/obgm/libcoap/zip/refs/tags"

LIBCOAP_VERSION = $(patsubst "%",%,$(strip $(CONFIG_LIBCOAP_VERSION)))
COAP_ZIP = libcoap-$(LIBCOAP_VERSION).zip

COAP_UNPACKNAME = libcoap
UNPACK ?= unzip -q -o

$(COAP_ZIP):
	@echo "Downloading: $(COAP_URL)/v$(LIBCOAP_VERSION)"
	$(Q) curl -o $(COAP_ZIP) -L $(COAP_URL)/v$(LIBCOAP_VERSION)

$(COAP_UNPACKNAME): $(COAP_ZIP)
	@echo "Unpacking: $(COAP_ZIP) -> $(COAP_UNPACKNAME)"
	$(Q) $(UNPACK) $(COAP_ZIP)
	@echo "Unpacking: $(COAP_ZIP) -> $(COAP_UNPACKNAME)"
	$(Q) mv libcoap-$(LIBCOAP_VERSION) $(COAP_UNPACKNAME)
	$(Q) touch $(COAP_UNPACKNAME)

context:: $(COAP_UNPACKNAME)/include/coap3/coap.h
# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(COAP_UNPACKNAME)/.git),)
distclean::
	$(call DELDIR, $(COAP_UNPACKNAME))
	$(call DELFILE, $(COAP_ZIP))

$(COAP_UNPACKNAME)/include/coap3/coap.h:: $(COAP_UNPACKNAME)
else
$(COAP_UNPACKNAME)/include/coap3/coap.h::
endif
	cd $(COAP_UNPACKNAME) && ./autogen.sh && \
	./configure --disable-doxygen --disable-manpages

CSRCS  += libcoap/src/coap_address.c
CSRCS  += libcoap/src/coap_asn1.c
CSRCS  += libcoap/src/coap_async.c
CSRCS  += libcoap/src/coap_block.c
CSRCS  += libcoap/src/coap_cache.c
CSRCS  += libcoap/src/coap_debug.c
CSRCS  += libcoap/src/coap_dtls.c
CSRCS  += libcoap/src/coap_encode.c
CSRCS  += libcoap/src/coap_event.c
CSRCS  += libcoap/src/coap_hashkey.c
CSRCS  += libcoap/src/coap_gnutls.c
CSRCS  += libcoap/src/coap_io.c
CSRCS  += libcoap/src/coap_layers.c
CSRCS  += libcoap/src/coap_mbedtls.c
CSRCS  += libcoap/src/coap_mem.c
CSRCS  += libcoap/src/coap_net.c
CSRCS  += libcoap/src/coap_netif.c
CSRCS  += libcoap/src/coap_notls.c
CSRCS  += libcoap/src/coap_openssl.c
CSRCS  += libcoap/src/coap_option.c
CSRCS  += libcoap/src/coap_oscore.c
CSRCS  += libcoap/src/coap_pdu.c
CSRCS  += libcoap/src/coap_prng.c
CSRCS  += libcoap/src/coap_resource.c
CSRCS  += libcoap/src/coap_session.c
CSRCS  += libcoap/src/coap_str.c
CSRCS  += libcoap/src/coap_subscribe.c
CSRCS  += libcoap/src/coap_tcp.c
CSRCS  += libcoap/src/coap_time.c
CSRCS  += libcoap/src/coap_tinydtls.c
CSRCS  += libcoap/src/coap_uri.c
CSRCS  += libcoap/src/coap_ws.c
CSRCS  += libcoap/src/oscore/oscore.c
CSRCS  += libcoap/src/oscore/oscore_cbor.c
CSRCS  += libcoap/src/oscore/oscore_context.c
CSRCS  += libcoap/src/oscore/oscore_cose.c
CSRCS  += libcoap/src/oscore/oscore_crypto.c

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)$(DELIM)netutils$(DELIM)libcoap
CFLAGS += -Wno-undef

ifneq ($(CONFIG_NETUTILS_LIBCOAP_EXAMPLE),)
PROGNAME += coap_server coap_client
PRIORITY  = $(CONFIG_NETUTILS_LIBCOAP_EXAMPLE_PRIORITY)
STACKSIZE = $(CONFIG_NETUTILS_LIBCOAP_EXAMPLE_STACKSIZE)
MODULE    = $(CONFIG_NETUTILS_LIBCOAP_EXAMPLE)
MAINSRC   = libcoap/examples/coap-server.c libcoap/examples/coap-client.c
endif

include $(APPDIR)/Application.mk
