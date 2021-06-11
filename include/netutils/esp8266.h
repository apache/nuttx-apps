/****************************************************************************
 * apps/include/netutils/esp8266.h
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
  char ssid[lespSSID_SIZE + 1];     /* +1 for null char */
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

int lesp_ap_connect(const char *ssid_name,
                    const char *ap_key, int timeout_s);
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
int lesp_bind(int sockfd,
              FAR const struct sockaddr *addr, socklen_t addrlen);
int lesp_connect(int sockfd,
                 FAR const struct sockaddr *addr, socklen_t addrlen);
int lesp_listen(int sockfd, int backlog);
int lesp_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t lesp_send(int sockfd, FAR const uint8_t *buf, size_t len, int flags);
ssize_t lesp_recv(int sockfd, FAR uint8_t *buf, size_t len, int flags);
int lesp_setsockopt(int sockfd, int level, int option,
                    FAR const void *value, socklen_t value_len);
FAR struct hostent *lesp_gethostbyname(FAR const char *hostname);

#endif /* CONFIG_NETUTILS_ESP8266 */
#endif /* __APPS_INCLUDE_NETUTILS_ESP8266_H */
