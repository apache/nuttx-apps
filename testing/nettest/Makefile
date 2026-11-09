############################################################################
# apps/testing/nettest/Makefile
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

NETTEST_UTILS_DIR := $(APPDIR)/testing/nettest/utils

PRIORITY  = $(CONFIG_TESTING_NET_TEST_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_NET_TEST_STACKSIZE)
MODULE    = $(CONFIG_TESTING_NET_TEST)

ifeq ($(CONFIG_TESTING_NET_TEST),y)

CFLAGS += -I$(NETTEST_UTILS_DIR)

ifeq ($(CONFIG_TESTING_NET_TCP),y)
MAINSRC  += tcp/test_tcp.c
PROGNAME += cmocka_net_tcp
CSRCS    += tcp/test_tcp_common.c utils/nettest_tcpserver.c

ifeq ($(CONFIG_NET_IPv4), y)
CSRCS    += tcp/test_tcp_connect_ipv4.c
endif

ifeq ($(CONFIG_NET_IPv6), y)
CSRCS    += tcp/test_tcp_connect_ipv6.c
endif

endif

ifeq ($(CONFIG_TESTING_NET_UDP),y)
MAINSRC  += udp/test_udp.c
PROGNAME += cmocka_net_udp
CSRCS    += udp/test_udp_common.c udp/test_udp_v6only.c
CSRCS    += udp/test_udp_family.c udp/test_udp_bind.c
endif

ifeq ($(CONFIG_TESTING_NET_LOCAL),y)
MAINSRC  += local/test_local.c
PROGNAME += cmocka_net_local
CSRCS    += local/test_local_common.c local/test_local_scm_rights.c
ifeq ($(CONFIG_TESTING_NET_LOCAL_PEERCRED),y)
CSRCS    += local/test_local_peercred.c
endif
endif

ifeq ($(CONFIG_TESTING_NET_LOCAL_DGRAM),y)
MAINSRC  += local/test_local_dgram.c
PROGNAME += cmocka_net_local_dgram
CSRCS    += local/test_local_dgram_common.c local/test_local_dgram_connect.c \
            local/test_local_dgram_poll.c local/test_local_dgram_pollext.c \
            local/test_local_dgram_regress.c local/test_local_dgram_pair.c \
            local/test_local_dgram_wait.c
endif

ifeq ($(CONFIG_TESTING_NET_OTHERS),y)
MAINSRC  += others/test_others.c
PROGNAME += cmocka_net_others
CSRCS    += others/test_others_common.c others/test_others_bufpool.c \
            others/test_others_chksum.c
CFLAGS   += -I$(TOPDIR)/net
endif

ifeq ($(CONFIG_TESTING_NET_MCAST),y)
MAINSRC  += mcast/test_mcast.c
PROGNAME += cmocka_net_mcast
CSRCS    += mcast/test_mcast_common.c

ifeq ($(CONFIG_NET_IGMP),y)
CSRCS    += mcast/test_mcast_ipv4.c
endif

ifeq ($(CONFIG_NET_MLD),y)
CSRCS    += mcast/test_mcast_ipv6.c
endif

endif

ifeq ($(CONFIG_TESTING_NET_RADIO),y)
MAINSRC  += radio/test_radio.c
PROGNAME += cmocka_net_radio
CSRCS    += radio/test_radio_common.c

ifeq ($(CONFIG_NET_BLUETOOTH),y)
CSRCS    += radio/test_radio_bluetooth.c
endif

ifeq ($(CONFIG_NET_IEEE802154),y)
CSRCS    += radio/test_radio_ieee802154.c
endif

endif

ifeq ($(CONFIG_TESTING_NET_TIMESTAMP),y)
MAINSRC  += timestamp/test_timestamp.c
PROGNAME += cmocka_net_timestamp
CSRCS    += timestamp/test_timestamp_common.c
endif

ifeq ($(CONFIG_TESTING_NET_CAN),y)
MAINSRC  += can/test_can.c
PROGNAME += cmocka_net_can
CSRCS    += can/test_can_bind.c
endif

ifeq ($(CONFIG_TESTING_NET_PKT),y)
MAINSRC  += pkt/test_pkt.c
PROGNAME += cmocka_net_pkt
CSRCS    += pkt/test_pkt_lo.c
endif

endif
include $(APPDIR)/Application.mk
