############################################################################
# apps/netutils/netlib/Makefile
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

# Network Library

CSRCS  = netlib_ipv4addrconv.c netlib_ethaddrconv.c netlib_parsehttpurl.c
CSRCS += netlib_setifstatus.c netlib_getifstatus.c

# Generic URL parsing support

ifeq ($(CONFIG_NETUTILS_NETLIB_GENERICURLPARSER),y)
CSRCS += netlib_parseurl.c
endif

# IP address support

ifeq ($(CONFIG_NET_IPv4),y)
CSRCS += netlib_setipv4addr.c netlib_getipv4addr.c
CSRCS += netlib_setdripv4addr.c netlib_setipv4netmask.c
CSRCS += netlib_getdripv4addr.c netlib_getipv4netmask.c
CSRCS += netlib_ipv4adaptor.c
ifeq ($(CONFIG_NET_ARP),y)
CSRCS += netlib_getarp.c netlib_setarp.c netlib_delarp.c
ifeq ($(CONFIG_NETLINK_ROUTE),y)
CSRCS += netlib_getarptab.c
endif
endif
ifeq ($(CONFIG_NETDB_DNSCLIENT),y)
CSRCS += netlib_setipv4dnsaddr.c
endif
ifeq ($(CONFIG_NET_ROUTE),y)
CSRCS += netlib_ipv4route.c netlib_ipv4router.c
endif
endif

ifeq ($(CONFIG_NET_IPv6),y)
CSRCS += netlib_setipv6addr.c netlib_getipv6addr.c
CSRCS += netlib_setdripv6addr.c netlib_setipv6netmask.c
CSRCS += netlib_prefix2ipv6netmask.c netlib_ipv6netmask2prefix.c
CSRCS += netlib_ipv6adaptor.c
ifeq ($(CONFIG_NET_ICMPv6_AUTOCONF),y)
CSRCS += netlib_autoconfig.c
endif
ifeq ($(CONFIG_NETDB_DNSCLIENT),y)
CSRCS += netlib_setipv6dnsaddr.c
endif
ifeq ($(CONFIG_NET_ROUTE),y)
CSRCS += netlib_ipv6route.c netlib_ipv6router.c
endif
ifeq ($(CONFIG_NETLINK_ROUTE),y)
CSRCS += netlib_getnbtab.c
endif
endif

# Device support

ifeq ($(CONFIG_NETLINK_ROUTE),y)
CSRCS += netlib_getdevs.c
ifeq ($(CONFIG_NET_ROUTE),y)
CSRCS += netlib_getroute.c
endif
endif

# These require TCP support

ifeq ($(CONFIG_NET_TCP),y)
ifeq ($(CONFIG_NET_IPv4),y) # Not yet available for IPv6
CSRCS += netlib_server.c netlib_listenon.c
endif
endif

# These require wireless IOCTL support */

ifeq ($(CONFIG_NETDEV_WIRELESS_IOCTL),y)
CSRCS += netlib_getessid.c netlib_setessid.c
endif

# MAC address support(Ethernet and 6LoWPAN only)

ifeq ($(CONFIG_NET_ETHERNET),y)
CSRCS += netlib_setmacaddr.c netlib_getmacaddr.c
endif

ifeq ($(CONFIG_WIRELESS_IEEE802154),y)
CSRCS += netlib_seteaddr.c netlib_getpanid.c netlib_saddrconv.c netlib_eaddrconv.c
endif

ifeq ($(CONFIG_WIRELESS_PKTRADIO),y)
CSRCS += netlib_getproperties.c netlib_getnodeaddr.c netlib_setnodeaddr.c
CSRCS += netlib_nodeaddrconv.c
endif

# IGMP support

ifeq ($(CONFIG_NET_IGMP),y)
ifeq ($(CONFIG_NET_IPv4),y) # Not yet available for IPv6
CSRCS += netlib_ipmsfilter.c
endif
endif

include $(APPDIR)/Application.mk
