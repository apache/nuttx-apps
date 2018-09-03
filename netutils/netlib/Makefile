############################################################################
# apps/netutils/netlib/Makefile
#
#   Copyright (C) 2011-2012, 2014-2015, 2017 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

-include $(TOPDIR)/Make.defs

# Network Library

CSRCS  = netlib_ipv4addrconv.c netlib_ethaddrconv.c netlib_parsehttpurl.c
CSRCS += netlib_setifstatus.c netlib_getifstatus.c

# IP address support

ifeq ($(CONFIG_NET_IPv4),y)
CSRCS += netlib_setipv4addr.c netlib_getipv4addr.c
CSRCS += netlib_setdripv4addr.c netlib_setipv4netmask.c
CSRCS += netlib_getdripv4addr.c netlib_getipv4netmask.c
CSRCS += netlib_ipv4adaptor.c
ifeq ($(CONFIG_NET_ARP),y)
CSRCS += netlib_getarp.c netlib_setarp.c netlib_delarp.c
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
ifeq ($(CONFIG_NET_ROUTE),y)
CSRCS += netlib_ipv6route.c netlib_ipv6router.c
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
