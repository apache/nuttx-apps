############################################################################
# apps/netutils/nng/Makefile
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

NNG_VERSION := $(patsubst "%",%,$(CONFIG_NETUTILS_NNG_VERSION))
NNG_TARBALL  = v$(NNG_VERSION).tar.gz
NNG_UNPACK   = nng
NNG_SRCDIR   = $(NNG_UNPACK)$(DELIM)src

CFLAGS += -I$(NNG_UNPACK)/src
CFLAGS += -Wno-unused-value -Wno-shadow

CFLAGS += -DNNG_PLATFORM_POSIX
CFLAGS += -DNNG_HAVE_GETRANDOM
CFLAGS += -DNNG_TRANSPORT_INPROC
CFLAGS += -DNNG_TRANSPORT_IPC
CFLAGS += -DNNG_TRANSPORT_TCP
CFLAGS += -DNNG_TRANSPORT_TLS
CFLAGS += -DNNG_TRANSPORT_WS
CFLAGS += -DNNG_TRANSPORT_WSS
CFLAGS += -DNNG_SETSTACKSIZE
CFLAGS += -DNNG_HAVE_PTHREAD_SETNAME_NP

ifneq ($(CONFIG_SMP_NCPUS),)
CFLAGS += -DNNG_MAX_TASKQ_THREADS=$(CONFIG_SMP_NCPUS)
else
CFLAGS += -DNNG_MAX_TASKQ_THREADS=2
endif

CFLAGS += -DNNG_USE_EVENTFD
ifeq ($(CONFIG_NETUTILS_NNG_HAVE_EPOLL),y)
CFLAGS += -DNNG_HAVE_EPOLL
CFLAGS += -DNNG_HAVE_EPOLL_CREATE1
endif

ifeq ($(CONFIG_NETUTILS_NNG_USE_MBEDTLS),y)
CFLAGS += -DNNG_TLS_ENGINE_INIT=nng_tls_engine_init_mbed
CFLAGS += -DNNG_TLS_ENGINE_FINI=nng_tls_engine_fini_mbed
CFLAGS += -DNNG_SUPP_TLS
endif

DEPPATH += --dep-path $(NNG_SRCDIR)
VPATH   += :$(NNG_SRCDIR)

CSRCS  = $(NNG_SRCDIR)/nng.c
CSRCS += $(NNG_SRCDIR)/nng_legacy.c
CSRCS += $(NNG_SRCDIR)/core/aio.c
CSRCS += $(NNG_SRCDIR)/core/device.c
CSRCS += $(NNG_SRCDIR)/core/dialer.c
CSRCS += $(NNG_SRCDIR)/core/file.c
CSRCS += $(NNG_SRCDIR)/core/idhash.c
CSRCS += $(NNG_SRCDIR)/core/init.c
CSRCS += $(NNG_SRCDIR)/core/list.c
CSRCS += $(NNG_SRCDIR)/core/listener.c
CSRCS += $(NNG_SRCDIR)/core/lmq.c
CSRCS += $(NNG_SRCDIR)/core/message.c
CSRCS += $(NNG_SRCDIR)/core/msgqueue.c
CSRCS += $(NNG_SRCDIR)/core/options.c
CSRCS += $(NNG_SRCDIR)/core/pollable.c
CSRCS += $(NNG_SRCDIR)/core/panic.c
CSRCS += $(NNG_SRCDIR)/core/pipe.c
CSRCS += $(NNG_SRCDIR)/core/protocol.c
CSRCS += $(NNG_SRCDIR)/core/reap.c
CSRCS += $(NNG_SRCDIR)/core/socket.c
CSRCS += $(NNG_SRCDIR)/core/stats.c
CSRCS += $(NNG_SRCDIR)/core/stream.c
CSRCS += $(NNG_SRCDIR)/core/strs.c
CSRCS += $(NNG_SRCDIR)/core/taskq.c
CSRCS += $(NNG_SRCDIR)/core/tcp.c
CSRCS += $(NNG_SRCDIR)/core/thread.c
CSRCS += $(NNG_SRCDIR)/core/timer.c
CSRCS += $(NNG_SRCDIR)/core/url.c

CSRCS += $(NNG_SRCDIR)/platform/posix/posix_alloc.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_atomic.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_clock.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_debug.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_file.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_ipcconn.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_ipcdial.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_ipclisten.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_pipe.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_resolv_gai.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_sockaddr.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_tcpconn.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_tcpdial.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_tcplisten.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_thread.c
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_udp.c
ifeq ($(CONFIG_NETUTILS_NNG_HAVE_EPOLL),y)
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_pollq_epoll.c
else
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_pollq_poll.c
endif
CSRCS += $(NNG_SRCDIR)/platform/posix/posix_rand_getrandom.c

CSRCS += $(NNG_SRCDIR)/sp/transport.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/bus0/bus.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pair0/pair.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pair1/pair.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pair1/pair1_poly.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pipeline0/pull.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pipeline0/push.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pubsub0/pub.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pubsub0/sub.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/pubsub0/xsub.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/reqrep0/rep.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/reqrep0/req.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/reqrep0/xrep.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/reqrep0/xreq.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/survey0/respond.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/survey0/survey.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/survey0/xrespond.c
CSRCS += $(NNG_SRCDIR)/sp/protocol/survey0/xsurvey.c
CSRCS += $(NNG_SRCDIR)/sp/transport/inproc/inproc.c
CSRCS += $(NNG_SRCDIR)/sp/transport/ipc/ipc.c
CSRCS += $(NNG_SRCDIR)/sp/transport/tcp/tcp.c
CSRCS += $(NNG_SRCDIR)/sp/transport/tls/tls.c
CSRCS += $(NNG_SRCDIR)/sp/transport/ws/websocket.c

CSRCS += $(NNG_SRCDIR)/supplemental/base64/base64.c
CSRCS += $(NNG_SRCDIR)/supplemental/http/http_chunk.c
CSRCS += $(NNG_SRCDIR)/supplemental/http/http_client.c
CSRCS += $(NNG_SRCDIR)/supplemental/http/http_conn.c
CSRCS += $(NNG_SRCDIR)/supplemental/http/http_msg.c
CSRCS += $(NNG_SRCDIR)/supplemental/http/http_public.c
CSRCS += $(NNG_SRCDIR)/supplemental/http/http_schemes.c
CSRCS += $(NNG_SRCDIR)/supplemental/http/http_server.c
CSRCS += $(NNG_SRCDIR)/supplemental/sha1/sha1.c
CSRCS += $(NNG_SRCDIR)/supplemental/tls/tls_common.c
ifeq ($(CONFIG_NETUTILS_NNG_USE_MBEDTLS),y)
CSRCS += $(NNG_SRCDIR)/supplemental/tls/mbedtls/tls.c
endif
CSRCS += $(NNG_SRCDIR)/supplemental/util/platform.c
CSRCS += $(NNG_SRCDIR)/supplemental/websocket/websocket.c

$(NNG_TARBALL):
	$(Q) echo "Downloading nng-$(NNG_VERSION)"
	$(Q) curl -O -L https://github.com/nanomsg/nng/archive/$(NNG_TARBALL)

$(NNG_UNPACK): $(NNG_TARBALL)
	$(Q) tar zxf $(NNG_TARBALL)
	$(Q) mv nng-$(NNG_VERSION) $(NNG_UNPACK)
	$(Q) echo "Patching $(NNG_UNPACK)"
	$(Q) patch -p0 < nng_porting_for_nuttx.patch
	$(Q) touch $(NNG_UNPACK)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(NNG_UNPACK)/.git),)
context:: $(NNG_UNPACK)

distclean::
	$(call DELFILE, $(NNG_TARBALL))
	$(call DELDIR, $(NNG_UNPACK))
endif

include $(APPDIR)/Application.mk
