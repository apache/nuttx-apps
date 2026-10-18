/****************************************************************************
 * apps/system/wifibridge/wifibridge_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "netutils/wifibridge.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void wifibridge_usage(FAR const char *progname)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s start [-s <ssid>] [-p <passphrase>] [-c <channel>]"
          " [-3] [-a <ipaddr>] [-d]\n", progname);
  fprintf(stderr, "  %s stop\n", progname);
  fprintf(stderr, "  %s status\n", progname);
  fprintf(stderr, "\nstart options:\n");
  fprintf(stderr, "  -s  SoftAP SSID (default %s)\n",
          CONFIG_SYSTEM_WIFIBRIDGE_SSID);
  fprintf(stderr, "  -p  SoftAP passphrase, 8..63 characters\n");
  fprintf(stderr, "  -c  2.4 GHz channel, 1..13 (default %d)\n",
          CONFIG_SYSTEM_WIFIBRIDGE_CHANNEL);
  fprintf(stderr, "  -3  WPA3-Personal instead of WPA2-PSK\n");
  fprintf(stderr, "  -a  bridge IPv4 address, netmask 255.255.255.0\n");
  fprintf(stderr, "  -d  run the DHCP server on the bridge\n");
}

static int wifibridge_start_cmd(FAR struct wifibridge_config_s *config,
                                int argc, FAR char **argv)
{
  struct in_addr addr;
  int option;
  int ret;

  optind = 1;
  while ((option = getopt(argc, argv, "s:p:c:3a:d")) != ERROR)
    {
      switch (option)
        {
          case 's':
            config->ssid = optarg;
            break;

          case 'p':
            config->passphrase = optarg;
            break;

          case 'c':
            config->channel = atoi(optarg);
            break;

          case '3':
            config->security = WIFIBRIDGE_SEC_WPA3;
            break;

          case 'a':
            if (inet_pton(AF_INET, optarg, &addr) != 1)
              {
                fprintf(stderr, "Invalid IPv4 address: %s\n", optarg);
                return EXIT_FAILURE;
              }

            config->ipaddr = addr.s_addr;
            break;

          case 'd':
#ifdef CONFIG_NETUTILS_WIFIBRIDGE_DHCPD
            config->dhcpd = true;
            break;
#else
            fprintf(stderr, "The DHCP server is not enabled\n");
            return EXIT_FAILURE;
#endif

          default:
            return EXIT_FAILURE;
        }
    }

  ret = wifibridge_start(config);
  if (ret < 0)
    {
      fprintf(stderr, "Failed to start the bridge: %d\n", ret);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

static int
wifibridge_status_cmd(FAR const struct wifibridge_config_s *config)
{
  struct wifibridge_status_s status;
  char addr[INET_ADDRSTRLEN];
  int ret;

  ret = wifibridge_status(config, &status);
  if (ret < 0)
    {
      fprintf(stderr, "Failed to read the status: %d\n", ret);
      return EXIT_FAILURE;
    }

  printf("%-5s %s", config->bridge, status.exists ? "present" : "absent");
  if (status.exists &&
      inet_ntop(AF_INET, &status.ipaddr, addr, sizeof(addr)) != NULL)
    {
      printf(" %s", addr);
    }

  printf("\n%-5s %s\n", config->ethif, status.eth_up ? "up" : "down");
  printf("%-5s %s ssid \"%s\"\n", config->wlanif,
         status.wlan_up ? "up" : "down", status.ssid);
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct wifibridge_config_s config;
  int ret;

  memset(&config, 0, sizeof(config));
  config.bridge     = CONFIG_SYSTEM_WIFIBRIDGE_BRIDGE;
  config.ethif      = CONFIG_SYSTEM_WIFIBRIDGE_ETHIF;
  config.wlanif     = CONFIG_SYSTEM_WIFIBRIDGE_WLANIF;
  config.ssid       = CONFIG_SYSTEM_WIFIBRIDGE_SSID;
  config.passphrase = CONFIG_SYSTEM_WIFIBRIDGE_PASSPHRASE;
  config.channel    = CONFIG_SYSTEM_WIFIBRIDGE_CHANNEL;
  config.security   = WIFIBRIDGE_SEC_WPA2;
  config.ipaddr     = htonl(CONFIG_SYSTEM_WIFIBRIDGE_IPADDR);
  config.netmask    = htonl(0xffffff00);

  if (argc < 2)
    {
      wifibridge_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (strcmp(argv[1], "start") == 0)
    {
      return wifibridge_start_cmd(&config, argc - 1, argv + 1);
    }
  else if (strcmp(argv[1], "stop") == 0)
    {
      ret = wifibridge_stop(&config);
      if (ret < 0)
        {
          fprintf(stderr, "Failed to stop the bridge: %d\n", ret);
          return EXIT_FAILURE;
        }

      return EXIT_SUCCESS;
    }
  else if (strcmp(argv[1], "status") == 0)
    {
      return wifibridge_status_cmd(&config);
    }

  wifibridge_usage(argv[0]);
  return EXIT_FAILURE;
}
