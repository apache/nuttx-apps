/****************************************************************************
 * netutils/pppd/ppp_conf.h
 *
 *   Copyright (C) 2015 Max Nekludov. All rights reserved.
 *   Author: Max Nekludov <macscomp@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_NETUTILS_PPPD_PPP_CONF_H
#define __APPS_NETUTILS_PPPD_PPP_CONF_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IPCP_RETRY_COUNT        5
#define IPCP_TIMEOUT            5
#define IPV6CP_RETRY_COUNT      5
#define IPV6CP_TIMEOUT          5
#define LCP_RETRY_COUNT         5
#define LCP_TIMEOUT             5
#define PAP_RETRY_COUNT         5
#define PAP_TIMEOUT             5
#define LCP_ECHO_INTERVAL       20

#define PPP_IP_TIMEOUT          (6*3600)
#define PPP_MAX_CONNECT         15

#define xxdebug_printf          ninfo
#define debug_printf            ninfo

#define PPP_RX_BUFFER_SIZE      1024 //1024  //GD 2048 for 1280 IPv6 MTU

#define AHDLC_TX_OFFLINE        5

#define IPCP_GET_PEER_IP        1

#define PPP_STATISTICS          1
#define PPP_DEBUG               defined(CONFIG_DEBUG_NET_INFO)

#endif /* __APPS_NETUTILS_PPPD_PPP_CONF_H */
