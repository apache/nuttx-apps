/****************************************************************************
 * apps/netutils/libcoap/coap_config.h
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * SPDX-FileCopyrightText: 2020 Carlos Gomes Martinho
 *                    <carlos.gomes_martinho@siemens.com>
 * SPDX-FileCopyrightText: 2021-2023 Jon Shallow
 *                     <supjps-libcoap@jpshallow.com>
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 ****************************************************************************/

#ifndef COAP_CONFIG_H_
#define COAP_CONFIG_H_

#if ! defined(_WIN32)
#define _GNU_SOURCE
#endif

/* Define to 1 if you have <ws2tcpip.h> header file. */

/* #undef HAVE_WS2TCPIP_H */

/* Define to 1 if the system has small stack size. */

/* #undef COAP_CONSTRAINED_STACK */

/* Define to 1 if you have <winsock2.h> header file. */

/* #undef HAVE_WINSOCK2_H */

#ifdef CONFIG_LIBCOAP_EXAMPLE_CLIENT
/* Define to 1 if the library has client support. */
#define COAP_CLIENT_SUPPORT 1

/* Define to 1 if the library has server support. */
#define COAP_SERVER_SUPPORT 1
#endif

/* Define to 1 if the library is to have observe persistence. */
#define COAP_WITH_OBSERVE_PERSIST 1

/* Define to 1 if the system has epoll support. */

/* #undef COAP_EPOLL_SUPPORT */

/* Define to 1 if the library has OSCORE support. */
#define COAP_OSCORE_SUPPORT 1

/* Define to 1 if the library has WebSockets support. */

/* #undef COAP_WS_SUPPORT */

/* Define to 1 if the library has async separate response support. */
#define COAP_ASYNC_SUPPORT 1

/* Define to 0-8 for maximum logging level. */

/* #undef COAP_MAX_LOGGING_LEVEL */

/* Define to 1 to build without TCP support. */
#define COAP_DISABLE_TCP 0

#ifdef CONFIG_NET_IPv4
/* Define to 1 to build with IPv4 support. */
#define COAP_IPV4_SUPPORT 1
#endif

#ifdef CONFIG_NET_IPv6
/* Define to 1 to build with IPv6 support. */
#define COAP_IPV6_SUPPORT 1
#endif

/* Define to 1 to build with Unix socket support. */
#define COAP_AF_UNIX_SUPPORT 1

/* Define to 1 to build with Q-Block (RFC 9177) support. */
#define COAP_Q_BLOCK_SUPPORT 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */

/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <erno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if the system has openssl */

/* #undef COAP_WITH_LIBOPENSSL */

/* Define to 1 if the system has libgnutls28 */

/* #undef COAP_WITH_LIBGNUTLS */

/* Define to 1 if the system has libtinydtls */

/* #undef COAP_WITH_LIBTINYDTLS */

/* Define to 1 if the system has libmbedtls */
#define COAP_WITH_LIBMBEDTLS 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `malloc' function. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `if_nametoindex' function. */
#define HAVE_IF_NAMETOINDEX 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <net/if.h> header file. */
#define HAVE_NET_IF_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `pthread_mutex_lock' function. */
#define HAVE_PTHREAD_MUTEX_LOCK 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strnlen' function. */
#define HAVE_STRNLEN 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `getrandom' function. */
#define HAVE_GETRANDOM 1

/* Define to 1 if you have the `randon' function. */
#define HAVE_RANDOM 1

/* Define to 1 if the system has the type `struct cmsghdr'. */
#define HAVE_STRUCT_CMSGHDR 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/unistd.h> header file. */
#define HAVE_SYS_UNISTD_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "libcoap-developers@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libcoap"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libcoap 4.3.4"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libcoap"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://libcoap.net/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "4.3.4"

#if defined(_MSC_VER) && (_MSC_VER < 1900) && !defined(snprintf)
#define snprintf _snprintf
#endif

#endif /* COAP_CONFIG_H_ */
