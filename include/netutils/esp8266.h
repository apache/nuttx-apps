/****************************************************************************
 * apps/include/netutils/esp8266.h
 *
 *   Copyright (C) 2015-2016 Pierre-Noel Bouteville. All rights reserved.
 *   Author: Pierre-Noel Bouteville <pnb990@gmail.com>
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

#ifndef __APPS_INCLUDE_NETUTILS_ESP8266_H
#define __APPS_INCLUDE_NETUTILS_ESP8266_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "nuttx/config.h"

#include <netinet/in.h>

#ifdef CONFIG_NETUTILS_ESP8266

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define lespSSID_SIZE 32 /* Number of character max of SSID (null char not included) */
#define lespBSSID_SIZE 6

#define lespIP(x1,x2,x3,x4) ((x1) << 24 | (x2) << 16 | (x3) << 8 | (x4) << 0)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum
{
  lesp_eMODE_AP       = 0,
  lesp_eMODE_STATION  = 1,
  lesp_eMODE_BOTH     = 2
} lesp_mode_t;

typedef enum
{
  lesp_eSECURITY_NONE = 0,
  lesp_eSECURITY_WEP,
  lesp_eSECURITY_WPA_PSK,
  lesp_eSECURITY_WPA2_PSK,
  lesp_eSECURITY_WPA_WPA2_PSK,
  lesp_eSECURITY_NBR
} lesp_security_t;

typedef struct
{
  lesp_security_t security;
  char ssid[lespSSID_SIZE+1];     /* +1 for null char */
  uint8_t bssid[lespBSSID_SIZE];
  int rssi;
  int channel;
} lesp_ap_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int lesp_initialize(void);
int lesp_soft_reset(void);

const char *lesp_security_to_str(lesp_security_t security);

int lesp_ap_connect(const char *ssid_name, const char *ap_key, int timeout_s);
int lesp_ap_get(lesp_ap_t *ap);

int lesp_ap_is_connected(void);

int lesp_set_dhcp(lesp_mode_t mode, bool enable);
int lesp_get_dhcp(bool *ap_enable, bool *sta_enable);
int lesp_set_net(lesp_mode_t mode, in_addr_t ip, in_addr_t mask,
                 in_addr_t gateway);
int lesp_get_net(lesp_mode_t mode, in_addr_t *ip, in_addr_t *mask,
                 in_addr_t *gw);

typedef void (*lesp_cb_t)(lesp_ap_t *wlan);

int lesp_list_access_points(lesp_cb_t cb);

int lesp_socket(int domain, int type, int protocol);
int lesp_closesocket(int sockfd);
int lesp_bind(int sockfd, FAR const struct sockaddr *addr, socklen_t addrlen);
int lesp_connect(int sockfd, FAR const struct sockaddr *addr, socklen_t addrlen);
int lesp_listen(int sockfd, int backlog);
int lesp_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t lesp_send(int sockfd, FAR const uint8_t *buf, size_t len, int flags);
ssize_t lesp_recv(int sockfd, FAR uint8_t *buf, size_t len, int flags);
int lesp_setsockopt(int sockfd, int level, int option,
                    FAR const void *value, socklen_t value_len);
FAR struct hostent *lesp_gethostbyname(FAR const char *hostname);

#endif /* CONFIG_NETUTILS_ESP8266 */
#endif /* __APPS_INCLUDE_NETUTILS_ESP8266_H */
