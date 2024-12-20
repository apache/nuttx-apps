# ##############################################################################
# apps/netutils/netlib/CMakeLists.txt
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

if(CONFIG_NETUTILS_NETLIB)

  # Network Library

  set(SRCS netlib_ipv4addrconv.c netlib_ethaddrconv.c netlib_parsehttpurl.c)
  list(APPEND SRCS netlib_setifstatus.c netlib_getifstatus.c)

  # Generic URL parsing support

  if(CONFIG_NETUTILS_NETLIB_GENERICURLPARSER)
    list(APPEND SRCS netlib_parseurl.c)
  endif()

  # IP address support

  if(CONFIG_NET_IPv4)
    list(APPEND SRCS netlib_setipv4addr.c netlib_getipv4addr.c)
    list(APPEND SRCS netlib_setdripv4addr.c netlib_setipv4netmask.c)
    list(APPEND SRCS netlib_getdripv4addr.c netlib_getipv4netmask.c)
    list(APPEND SRCS netlib_ipv4adaptor.c)
    if(CONFIG_NET_ARP)
      list(APPEND SRCS netlib_getarp.c netlib_setarp.c netlib_delarp.c)
      if(CONFIG_NETLINK_ROUTE)
        list(APPEND SRCS netlib_getarptab.c)
      endif()
    endif()
    if(CONFIG_NETDB_DNSCLIENT)
      list(APPEND SRCS netlib_setipv4dnsaddr.c)
    endif()
    if(CONFIG_NET_ROUTE)
      list(APPEND SRCS netlib_ipv4route.c netlib_ipv4router.c)
    endif()
    if(CONFIG_NET_IPTABLES)
      list(APPEND SRCS netlib_iptables.c)
    endif()
    if(CONFIG_NETUTILS_DHCPC)
      list(APPEND SRCS netlib_obtainipv4addr.c)
    endif()
  endif()

  if(CONFIG_NET_IPv6)
    list(APPEND SRCS netlib_addipv6addr.c netlib_delipv6addr.c)
    list(APPEND SRCS netlib_setipv6addr.c netlib_getipv6addr.c)
    list(APPEND SRCS netlib_setdripv6addr.c netlib_setipv6netmask.c)
    list(APPEND SRCS netlib_prefix2ipv6netmask.c netlib_ipv6netmask2prefix.c)
    list(APPEND SRCS netlib_ipv6adaptor.c)
    if(CONFIG_NET_ICMPv6_AUTOCONF)
      list(APPEND SRCS netlib_autoconfig.c netlib_obtainipv6addr.c)
    endif()
    if(CONFIG_NETDB_DNSCLIENT)
      list(APPEND SRCS netlib_setipv6dnsaddr.c)
    endif()
    if(CONFIG_NET_ROUTE)
      list(APPEND SRCS netlib_ipv6route.c netlib_ipv6router.c)
    endif()
    if(CONFIG_NETLINK_ROUTE)
      list(APPEND SRCS netlib_getnbtab.c)
    endif()
    if(CONFIG_NET_IPTABLES)
      list(APPEND SRCS netlib_ip6tables.c)
    endif()
  endif()

  # Device support

  if(CONFIG_NETLINK_ROUTE)
    list(APPEND SRCS netlib_getdevs.c)
    if(CONFIG_NET_ROUTE)
      list(APPEND SRCS netlib_getroute.c)
    endif()
  endif()

  # Netfilter connection support

  if(CONFIG_NETLINK_NETFILTER)
    list(APPEND SRCS netlib_conntrack.c)
  endif()

  # These require TCP support

  if(CONFIG_NET_TCP)
    if(CONFIG_NET_IPv4) # Not yet available for IPv6
      list(APPEND SRCS netlib_server.c netlib_listenon.c)
    endif()
  endif()

  # These require wireless IOCTL support */

  if(CONFIG_NETDEV_WIRELESS_IOCTL)
    list(APPEND SRCS netlib_getessid.c netlib_setessid.c)
  endif()

  # MAC address support(Ethernet and 6LoWPAN only)

  if(CONFIG_NET_ETHERNET)
    list(APPEND SRCS netlib_setmacaddr.c netlib_getmacaddr.c)
  endif()

  list(APPEND SRCS netlib_setmtu.c)

  if(CONFIG_WIRELESS_IEEE802154)
    list(APPEND SRCS netlib_seteaddr.c netlib_getpanid.c netlib_saddrconv.c
         netlib_eaddrconv.c)
  endif()

  if(CONFIG_NETDEV_STATISTICS)
    list(APPEND SRCS netlib_getifstatistics.c)
  endif()

  if(CONFIG_WIRELESS_PKTRADIO)
    list(APPEND SRCS netlib_getproperties.c netlib_getnodeaddr.c
         netlib_setnodeaddr.c)
    list(APPEND SRCS netlib_nodeaddrconv.c)
  endif()

  # IGMP support

  if(CONFIG_NET_IGMP)
    if(CONFIG_NET_IPv4) # Not yet available for IPv6
      list(APPEND SRCS netlib_ipmsfilter.c)
    endif()
  endif()

  target_sources(apps PRIVATE ${SRCS})

endif()
