#############################################################################
# apps/netutils/libwebsockets/Makefile
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
#############################################################################

include $(APPDIR)/Make.defs

LIBWEBSOCKETS_URL     ?= "https://github.com/warmcat/libwebsockets/archive"

LIBWEBSOCKETS_VERSION := $(patsubst "%",%,$(CONFIG_NETUTILS_LIBWEBSOCKETS_VERSION))
LIBWEBSOCKETS_TARBALL  = v$(LIBWEBSOCKETS_VERSION).tar.gz
LIBWEBSOCKETS_UNPACK   = libwebsockets

word-dot = $(word $2,$(subst ., ,$1))

CFLAGS += \
	-DLWS_LIBRARY_VERSION_MAJOR=$(call word-dot,$(LIBWEBSOCKETS_VERSION),1) \
	-DLWS_LIBRARY_VERSION_MINOR=$(call word-dot,$(LIBWEBSOCKETS_VERSION),2) \
	-DLWS_LIBRARY_VERSION_PATCH=$(call word-dot,$(LIBWEBSOCKETS_VERSION),3) \
	-DLWS_LIBRARY_VERSION_PATCH_ELABORATED=$(call word-dot,$(LIBWEBSOCKETS_VERSION),3)-unknown \
	-DLWS_LIBRARY_VERSION=\"$(LIBWEBSOCKETS_VERSION)-unknown\"

CFLAGS += \
	-I. \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/core \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/event-libs \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/system/smd \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/system/metrics \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/core-net \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/roles \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/roles/http \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/roles/h1 \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/roles/h2 \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/roles/ws \
	-I$(LIBWEBSOCKETS_UNPACK)/lib/tls

CSRCS += \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-caps.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-misc.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-init.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-file.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-pipe.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-service.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-sockets.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/plat/unix/unix-fds.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/alloc.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/buflist.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/context.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/lws_dll2.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/lws_map.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/libwebsockets.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/logs.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core/vfs.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/base64-decode.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/cache-ttl/lws-cache-ttl.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/cache-ttl/heap.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/cache-ttl/file.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/dir.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/prng.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/lws-ring.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/lwsac/lwsac.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/lwsac/cached-file.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/lejp.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/misc/sha-1.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/system/system.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/system/smd/smd.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/dummy-callback.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/output.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/close.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/network.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/vhost.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/pollfd.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/service.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/sorted-usec-list.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/wsi.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/wsi-timeout.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/adopt.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/state.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/client/client.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/client/connect.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/client/connect2.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/client/connect3.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/client/connect4.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/client/sort-dns.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/core-net/client/conmon.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/pipe/ops-pipe.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/http/header.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/http/date.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/http/parsers.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/http/cookie.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/h1/ops-h1.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/h2/http2.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/h2/hpack.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/h2/ops-h2.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/ws/ops-ws.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/ws/client-ws.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/ws/client-parser-ws.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/raw-skt/ops-raw-skt.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/raw-file/ops-raw-file.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/http/client/client-http.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/event-libs/poll/poll.c

CSRCS += \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/tls.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/tls-network.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/tls-sessions.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/tls-client.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/mbedtls/mbedtls-tls.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/mbedtls/mbedtls-extensions.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/mbedtls/mbedtls-x509.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/mbedtls/mbedtls-ssl.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/mbedtls/mbedtls-session.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/tls/mbedtls/mbedtls-client.c \

ifneq ($(CONFIG_NETUTILS_MQTTC),)
	CFLAGS += -I$(LIBWEBSOCKETS_UNPACK)/lib/roles/mqtt

	CSRCS += \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/mqtt/mqtt.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/mqtt/ops-mqtt.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/mqtt/primitives.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/mqtt/client/client-mqtt.c \
	$(LIBWEBSOCKETS_UNPACK)/lib/roles/mqtt/client/client-mqtt-handshake.c
endif

ifeq ($(wildcard $(LIBWEBSOCKETS_UNPACK)/.git),)
$(LIBWEBSOCKETS_TARBALL):
	$(Q) echo "Downloading libwebsockets-$(LIBWEBSOCKETS_VERSION)"
	$(Q) curl -O -L $(LIBWEBSOCKETS_URL)/$(LIBWEBSOCKETS_TARBALL)

$(LIBWEBSOCKETS_UNPACK): $(LIBWEBSOCKETS_TARBALL)
	$(Q) echo "Unpacking: $(LIBWEBSOCKETS_TARBALL) -> $(LIBWEBSOCKETS_UNPACK)"
	$(Q) tar zxf $(LIBWEBSOCKETS_TARBALL)
	$(Q) mv libwebsockets-$(LIBWEBSOCKETS_VERSION) $(LIBWEBSOCKETS_UNPACK)
	$(Q) echo "Patching $(LIBWEBSOCKETS_UNPACK)"
	$(Q) patch -p0 < libwebsockets.patch
	$(Q) touch $(LIBWEBSOCKETS_UNPACK)

context:: $(LIBWEBSOCKETS_UNPACK)

distclean::
	$(call DELFILE, $(LIBWEBSOCKETS_TARBALL))
	$(call DELDIR, $(LIBWEBSOCKETS_UNPACK))
endif

include $(APPDIR)/Application.mk
