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
          " [-3] [-a <ipaddr>] [-d|-D]\n", progname);
  fprintf(stderr, "  %s stop\n", progname);
  fprintf(stderr, "  %s status\n", progname);
#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST
  fprintf(stderr, "  %s save [-s <ssid>] [-p <passphrase>] [-c <channel>]"
          " [-3] [-a <ipaddr>] [-d|-D]\n", progname);
  fprintf(stderr, "  %s clear\n", progname);
#endif
  fprintf(stderr, "\nstart and save options:\n");
  fprintf(stderr, "  -s  SoftAP SSID (default %s)\n",
          CONFIG_SYSTEM_WIFIBRIDGE_SSID);
  fprintf(stderr, "  -p  SoftAP passphrase, 8..63 characters\n");
  fprintf(stderr, "  -c  2.4 GHz channel, 1..13 (default %d)\n",
          CONFIG_SYSTEM_WIFIBRIDGE_CHANNEL);
  fprintf(stderr, "  -3  WPA3-Personal instead of WPA2-PSK\n");
  fprintf(stderr, "  -a  bridge IPv4 address, netmask 255.255.255.0\n");
  fprintf(stderr, "  -d  run the DHCP server on the bridge\n");
  fprintf(stderr, "  -D  do not run the DHCP server\n");
#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST
  fprintf(stderr, "\nThe options are applied over the saved configuration,"
          " if there is one,\nand over the defaults otherwise.  save keeps"
          " them, start only uses them.\n");
#endif
}

static int wifibridge_parse(FAR struct wifibridge_config_s *config,
                            int argc, FAR char **argv)
{
  struct in_addr addr;
  int option;

  optind = 1;
  while ((option = getopt(argc, argv, "s:p:c:3a:dD")) != ERROR)
    {
      switch (option)
        {
          case 's':
            if (strlen(optarg) > WIFIBRIDGE_SSID_MAXLEN)
              {
                fprintf(stderr, "The SSID has more than %d characters\n",
                        WIFIBRIDGE_SSID_MAXLEN);
                return ERROR;
              }

            strlcpy(config->ssid, optarg, sizeof(config->ssid));
            break;

          case 'p':
            if (strlen(optarg) > WIFIBRIDGE_PASSPHRASE_MAXLEN)
              {
                fprintf(stderr, "The passphrase has more than %d "
                        "characters\n", WIFIBRIDGE_PASSPHRASE_MAXLEN);
                return ERROR;
              }

            strlcpy(config->passphrase, optarg, sizeof(config->passphrase));
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
                return ERROR;
              }

            config->ipaddr = addr.s_addr;
            break;

          case 'd':
#ifdef CONFIG_NETUTILS_WIFIBRIDGE_DHCPD
            config->dhcpd = true;
            break;
#else
            fprintf(stderr, "The DHCP server is not enabled\n");
            return ERROR;
#endif

          case 'D':
            config->dhcpd = false;
            break;

          default:
            return ERROR;
        }
    }

  return OK;
}

static int wifibridge_start_cmd(FAR struct wifibridge_config_s *config,
                                int argc, FAR char **argv)
{
  int ret;

  if (wifibridge_parse(config, argc, argv) < 0)
    {
      return EXIT_FAILURE;
    }

  ret = wifibridge_start(config);
  if (ret < 0)
    {
      fprintf(stderr, "Failed to start the bridge: %d\n", ret);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST
static int wifibridge_save_cmd(FAR struct wifibridge_config_s *config,
                               int argc, FAR char **argv)
{
  int ret;

  if (wifibridge_parse(config, argc, argv) < 0)
    {
      return EXIT_FAILURE;
    }

  ret = wifibridge_save(config);
  if (ret < 0)
    {
      fprintf(stderr, "Failed to save the configuration: %d\n", ret);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

static int wifibridge_clear_cmd(void)
{
  int ret;

  ret = wifibridge_clear();
  if (ret < 0 && ret != -ENOENT)
    {
      fprintf(stderr, "Failed to clear the configuration: %d\n", ret);
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
#endif

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
  strlcpy(config.ssid, CONFIG_SYSTEM_WIFIBRIDGE_SSID, sizeof(config.ssid));
  strlcpy(config.passphrase, CONFIG_SYSTEM_WIFIBRIDGE_PASSPHRASE,
          sizeof(config.passphrase));
  config.channel    = CONFIG_SYSTEM_WIFIBRIDGE_CHANNEL;
  config.security   = WIFIBRIDGE_SEC_WPA2;
  config.ipaddr     = htonl(CONFIG_SYSTEM_WIFIBRIDGE_IPADDR);
  config.netmask    = htonl(0xffffff00);

  if (argc < 2)
    {
      wifibridge_usage(argv[0]);
      return EXIT_FAILURE;
    }

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST
  /* The saved configuration, if there is one, replaces the defaults */

  if (strcmp(argv[1], "start") == 0 || strcmp(argv[1], "save") == 0)
    {
      ret = wifibridge_load(&config);
      if (ret < 0 && ret != -ENOENT)
        {
          fprintf(stderr, "Ignoring the saved configuration: %d\n", ret);
        }
    }
#endif

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

#ifdef CONFIG_NETUTILS_WIFIBRIDGE_PERSIST
  else if (strcmp(argv[1], "save") == 0)
    {
      return wifibridge_save_cmd(&config, argc - 1, argv + 1);
    }
  else if (strcmp(argv[1], "clear") == 0)
    {
      return wifibridge_clear_cmd();
    }
#endif

  wifibridge_usage(argv[0]);
  return EXIT_FAILURE;
}
