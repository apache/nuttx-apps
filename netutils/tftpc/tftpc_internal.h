/****************************************************************************
 * apps/netutils/tftpc/tftpc_internal.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __APPS_NETUTILS_TFTP_TFTPC_INTERNAL_H
#define __APPS_NETUTILS_TFTP_TFTPC_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/net/udp.h>
#include <nuttx/net/ip.h>
#include <nuttx/net/ethernet.h>
#include <nuttx/net/netconfig.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Verify TFTP configuration settings ***************************************/

/* The settings beginning with CONFIG_NETUTILS_TFTP_* can all be set in the
 * NuttX configuration file.  If they are are defined in the configuration
 * then default values are assigned here.
 */

/* The "well-known" server TFTP port number (usually 69).  This port number
 * is only used for the initial server contact.  The server will negotiate
 * a new transfer port number after the initial client request.
 */

#ifndef CONFIG_NETUTILS_TFTP_PORT
#  define CONFIG_NETUTILS_TFTP_PORT 69
#endif

/* recvfrom timeout in deci-seconds */

#ifndef CONFIG_NETUTILS_TFTP_TIMEOUT
#  define CONFIG_NETUTILS_TFTP_TIMEOUT 10 /* One second */
#endif

/* Dump received buffers */

#undef CONFIG_NETUTILS_TFTP_DUMPBUFFERS

/* Sizes of TFTP message headers */

#define TFTP_ACKHEADERSIZE    4
#define TFTP_ERRHEADERSIZE    4
#define TFTP_DATAHEADERSIZE   4

/* The maximum size for TFTP data is determined by the configured UDP packet
 * payload size (UDP_MSS), but cannot exceed 512 + sizeof(TFTP_DATA header).
 *
 * In the case where there are multiple network devices with different
 * link layer protocols, each network device may support a different UDP MSS
 * value.  Here, if Ethernet is enabled, we (arbitrarily) assume that the
 * Ethernet is link that will be used.  If Ethernet is not one of the
 * enabled interfaces, we (arbitrarily) select the minimum MSS.
 */

#define TFTP_DATAHEADERSIZE   4
#define TFTP_MAXPACKETSIZE    (TFTP_DATAHEADERSIZE+512)

#if defined(CONFIG_NET_ETHERNET)
#  if ETH_UDP_MSS(IPv4_HDRLEN) < TFTP_MAXPACKETSIZE
#    define TFTP_PACKETSIZE   ETH_UDP_MSS(IPv4_HDRLEN)
#    ifdef CONFIG_CPP_HAVE_WARNING
#      warning "Ethernet MSS is too small for TFTP"
#    endif
#  else
#    define TFTP_PACKETSIZE   TFTP_MAXPACKETSIZE
#  endif
#elif MIN_UDP_MSS < TFTP_MAXPACKETSIZE
#  define TFTP_PACKETSIZE     MIN_UDP_MSS
#  ifdef CONFIG_CPP_HAVE_WARNING
#    warning "Minimum MSS is too small for TFTP"
#  endif
#else
#  define TFTP_PACKETSIZE     TFTP_MAXPACKETSIZE
#endif

#define TFTP_DATASIZE         (TFTP_PACKETSIZE-TFTP_DATAHEADERSIZE)
#define TFTP_IOBUFSIZE        (TFTP_PACKETSIZE+8)

/* TFTP Opcodes *************************************************************/

#define TFTP_RRQ  1  /* Read Request          RFC 1350, RFC 2090 */
#define TFTP_WRQ  2  /* Write Request         RFC 1350 */
#define TFTP_DATA 3  /* Data chunk            RFC 1350 */
#define TFTP_ACK  4  /* Acknowledgement       RFC 1350 */
#define TFTP_ERR  5  /* Error Message         RFC 1350 */
#define TFTP_OACK 6  /* Option acknowledgment RFC 2347 */

#define TFTP_MAXRFC1350 5

/* TFTP Error Codes *********************************************************/

/* Error codes */

#define TFTP_ERR_NONE         0  /* No error */
#define TFTP_ERR_NOSUCHFILE   1  /* File not found */
#define TFTP_ERR_ACCESS       2  /* Access violation */
#define TFTP_ERR_FULL         3  /* Disk full or allocation exceeded */
#define TFTP_ERR_ILLEGALOP    4  /* Illegal TFTP operation */
#define TFTP_ERR_UNKID        5  /* Unknown transfer ID */
#define TFTP_ERR_EXISTS       6  /* File already exists */
#define TFTP_ERR_UNKUSER      7  /* No such user */
#define TFTP_ERR_NEGOTIATE    8  /* Terminate transfer due to option negotiation */

/* Error strings */

#define TFTP_ERR_STNOSUCHFILE "File not found"
#define TFTP_ERRST_ACCESS     "Access violation"
#define TFTP_ERRST_FULL       "Disk full or allocation exceeded"
#define TFTP_ERRST_ILLEGALOP  "Illegal TFTP operation"
#define TFTP_ERRST_UNKID      "Unknown transfer ID"
#define TFTP_ERRST_EXISTS     "File already exists"
#define TFTP_ERRST_UNKUSER    "No such user"
#define TFTP_ERRST_NEGOTIATE  "Terminate transfer due to option negotiation"

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Defined in tftp_packet.c *************************************************/

extern int tftp_sockinit(struct sockaddr_in *server, in_addr_t addr);
extern int tftp_mkreqpacket(uint8_t *buffer, int opcode,
                            const char *path, bool binary);
extern int tftp_mkackpacket(uint8_t *buffer, uint16_t blockno);
extern int tftp_mkerrpacket(uint8_t *buffer, uint16_t errorcode,
                            const char *errormsg);
#ifdef CONFIG_DEBUG_NET_WARN
extern int tftp_parseerrpacket(const uint8_t *packet);
#endif

extern ssize_t tftp_recvfrom(int sd, void *buf,
                             size_t len, struct sockaddr_in *from);
extern ssize_t tftp_sendto(int sd, const void *buf,
                           size_t len, struct sockaddr_in *to);

#ifdef CONFIG_NETUTILS_TFTP_DUMPBUFFERS
# define tftp_dumpbuffer(msg, buffer, nbytes) ninfodumpbuffer(msg, buffer, nbytes)
#else
# define tftp_dumpbuffer(msg, buffer, nbytes)
#endif

#endif /* __APPS_NETUTILS_TFTP_TFTPC_INTERNAL_H */
