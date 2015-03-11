#ifndef __APPS_NETUTILS_PPPD_PPP_CONF_H
#define __APPS_NETUTILS_PPPD_PPP_CONF_H

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

#define PAP_USERNAME_SIZE       16
#define PAP_PASSWORD_SIZE       16
#define PAP_USERNAME  "user"
#define PAP_PASSWORD  "pass"

#define xxdebug_printf          printf
#define debug_printf            printf

#define PPP_RX_BUFFER_SIZE      1024 //1024  //GD 2048 for 1280 IPv6 MTU
#define PPP_TX_BUFFER_SIZE      64

#define AHDLC_TX_OFFLINE        5
//#define AHDLC_COUNTERS          1 //defined for AHDLC stats support, Guillaume Descamps, September 19th, 2011

#define IPCP_GET_PEER_IP        1

#define PPP_STATISTICS          1
#define PPP_DEBUG               1

#endif /* __APPS_NETUTILS_PPPD_PPP_CONF_H */