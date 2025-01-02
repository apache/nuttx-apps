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

ifeq ($(CONFIG_TESTING_NET_OTHERS),y)
MAINSRC  += others/test_others.c
PROGNAME += cmocka_net_others
CSRCS    += others/test_others_common.c others/test_others_bufpool.c
endif

endif
include $(APPDIR)/Application.mk
